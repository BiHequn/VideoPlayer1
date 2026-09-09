#include "epoll_server.h"
#include "protocol.h"
#include <json-c/json.h>
#include <sqlite3.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <dirent.h>
#include <sys/stat.h>
#include <math.h>

static int m_epollfd = -1;
static int m_listenfd = -1;
static conn_info_t m_conns[MAX_CONNS];
static msg_handler_t m_handlers[256];
static threadpool_t *m_pool = NULL;
static sqlite3 *m_db = NULL;

static pthread_mutex_t g_conn_lock = PTHREAD_MUTEX_INITIALIZER;

static const char *JWT_SECRET = "VideoPlayer_SecretKey_2026";
static const char *JWT_REFRESH_SECRET = "VideoPlayer_RefreshKey_2026";
static const char *NET_XOR_KEY = "VideoPlayer2026!";

#define RECOMMEND_LIMIT 50
#define CF_LIMIT 20
#define MIN_CF_ACTIONS 2

static char *base64_encode(const unsigned char *input, int length) {
    BIO *bmem, *b64;
    BUF_MEM *bptr;
    b64 = BIO_new(BIO_f_base64());
    bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64, input, length);
    BIO_flush(b64);
    BIO_get_mem_ptr(b64, &bptr);
    char *buff = (char *)malloc(bptr->length + 1);
    memcpy(buff, bptr->data, bptr->length);
    buff[bptr->length] = 0;
    BIO_free_all(b64);
    return buff;
}

static int base64_decode(const char *input, unsigned char **output, int *out_len) {
    BIO *b64, *bmem;
    int len = strlen(input);
    *output = (unsigned char *)malloc(len + 1);
    b64 = BIO_new(BIO_f_base64());
    bmem = BIO_new_mem_buf(input, len);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    b64 = BIO_push(b64, bmem);
    *out_len = BIO_read(b64, *output, len);
    if (*out_len < 0) *out_len = 0;
    (*output)[*out_len] = 0;
    BIO_free_all(b64);
    return *out_len;
}

static char *xor_base64_encode(const char *input) {
    int input_len = strlen(input);
    int key_len = strlen(NET_XOR_KEY);
    unsigned char *tmp = (unsigned char *)malloc(input_len);
    for (int i = 0; i < input_len; ++i) {
        tmp[i] = ((unsigned char)input[i]) ^ ((unsigned char)NET_XOR_KEY[i % key_len]);
    }
    char *encoded = base64_encode(tmp, input_len);
    free(tmp);
    return encoded;
}

static char *xor_base64_decode(const char *input, int *out_len) {
    unsigned char *decoded = NULL;
    int decoded_len = 0;
    base64_decode(input, &decoded, &decoded_len);
    if (decoded_len <= 0) {
        free(decoded);
        *out_len = 0;
        return NULL;
    }
    int key_len = strlen(NET_XOR_KEY);
    char *plain = (char *)malloc(decoded_len + 1);
    for (int i = 0; i < decoded_len; ++i) {
        plain[i] = ((char)decoded[i]) ^ NET_XOR_KEY[i % key_len];
    }
    plain[decoded_len] = 0;
    free(decoded);
    *out_len = decoded_len;
    return plain;
}

static void sha256_hash(const char *input, unsigned char *output) {
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, input, strlen(input));
    SHA256_Final(output, &ctx);
}

static char *generate_token(int uid, const char *secret, int expire_seconds) {
    time_t now = time(NULL);
    time_t exp = now + expire_seconds;
    char header[] = "{\"alg\":\"HS256\",\"typ\":\"JWT\"}";
    char payload[512];
    snprintf(payload, sizeof(payload), "{\"uid\":%d,\"iat\":%ld,\"exp\":%ld}", uid, (long)now, (long)exp);

    char *h_b64 = base64_encode((unsigned char *)header, strlen(header));
    char *p_b64 = base64_encode((unsigned char *)payload, strlen(payload));

    char sign_input[1024];
    snprintf(sign_input, sizeof(sign_input), "%s.%s", h_b64, p_b64);

    unsigned char hmac_result[32];
    unsigned int hmac_len;
    HMAC(EVP_sha256(), secret, strlen(secret),
         (unsigned char *)sign_input, strlen(sign_input),
         hmac_result, &hmac_len);

    char *s_b64 = base64_encode(hmac_result, hmac_len);

    char *token = (char *)malloc(strlen(h_b64) + strlen(p_b64) + strlen(s_b64) + 3);
    sprintf(token, "%s.%s.%s", h_b64, p_b64, s_b64);

    free(h_b64);
    free(p_b64);
    free(s_b64);
    return token;
}

static int verify_token_details(const char *token, const char *secret, int *out_uid,
                                char *out_username, size_t username_size) {
    char *copy = strdup(token);
    char *saveptr = NULL;
    char *h_b64 = strtok_r(copy, ".", &saveptr);
    char *p_b64 = strtok_r(NULL, ".", &saveptr);
    char *s_b64 = strtok_r(NULL, ".", &saveptr);

    if (!h_b64 || !p_b64 || !s_b64) {
        free(copy);
        return -1;
    }

    char sign_input[1024];
    snprintf(sign_input, sizeof(sign_input), "%s.%s", h_b64, p_b64);

    unsigned char hmac_result[32];
    unsigned int hmac_len;
    HMAC(EVP_sha256(), secret, strlen(secret),
         (unsigned char *)sign_input, strlen(sign_input),
         hmac_result, &hmac_len);

    char *expected_sig = base64_encode(hmac_result, hmac_len);
    if (strcmp(expected_sig, s_b64) != 0) {
        free(copy);
        free(expected_sig);
        return -1;
    }
    free(expected_sig);

    unsigned char *payload_dec;
    int payload_len;
    base64_decode(p_b64, &payload_dec, &payload_len);
    payload_dec[payload_len] = 0;

    json_object *jobj = json_tokener_parse((char *)payload_dec);
    free(payload_dec);

    if (!jobj) {
        free(copy);
        return -1;
    }

    json_object *j_exp, *j_uid, *j_username = NULL;
    if (!json_object_object_get_ex(jobj, "exp", &j_exp) ||
        !json_object_object_get_ex(jobj, "uid", &j_uid)) {
        json_object_put(jobj);
        free(copy);
        return -1;
    }
    json_object_object_get_ex(jobj, "username", &j_username);

    time_t exp = (time_t)json_object_get_int64(j_exp);
    *out_uid = json_object_get_int(j_uid);
    if (out_username && username_size > 0) {
        const char *username = j_username ? json_object_get_string(j_username) : "";
        snprintf(out_username, username_size, "%s", username);
    }
    json_object_put(jobj);
    free(copy);

    if (time(NULL) > exp) return -2;
    return 0;
}

static int verify_token(const char *token, const char *secret, int *out_uid) {
    return verify_token_details(token, secret, out_uid, NULL, 0);
}

static int init_database() {
    mkdir("data", 0755);
    mkdir("data/uploads", 0755);
    mkdir("data/uploads/files", 0755);
    mkdir("data/uploads/gif", 0755);
    mkdir("data/uploads/chunks", 0755);

    int rc = sqlite3_open("data/videoplayer.db", &m_db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(m_db));
        return -1;
    }

    const char *sql_users = "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "username TEXT UNIQUE NOT NULL,"
        "email TEXT UNIQUE NOT NULL,"
        "phone TEXT UNIQUE,"
        "password TEXT NOT NULL,"
        "salt TEXT NOT NULL,"
        "created_at TEXT NOT NULL,"
        "last_login TEXT,"
        "like_count INTEGER DEFAULT 0,"
        "play_count INTEGER DEFAULT 0)";

    const char *sql_videos = "CREATE TABLE IF NOT EXISTS videos ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "user_id INTEGER NOT NULL,"
        "filename TEXT NOT NULL,"
        "filepath TEXT NOT NULL,"
        "filesize INTEGER NOT NULL,"
        "file_md5 TEXT NOT NULL,"
        "duration REAL DEFAULT 0,"
        "like_count INTEGER DEFAULT 0,"
        "play_count INTEGER DEFAULT 0,"
        "created_at TEXT NOT NULL,"
        "FOREIGN KEY(user_id) REFERENCES users(id))";

    const char *sql_uploads = "CREATE TABLE IF NOT EXISTS uploads ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "user_id INTEGER NOT NULL,"
        "file_md5 TEXT NOT NULL,"
        "filename TEXT NOT NULL,"
        "filesize INTEGER NOT NULL,"
        "uploaded_size INTEGER DEFAULT 0,"
        "chunk_size INTEGER DEFAULT 4096,"
        "status INTEGER DEFAULT 0,"
        "created_at TEXT NOT NULL,"
        "updated_at TEXT)";

    const char *sql_likes = "CREATE TABLE IF NOT EXISTS user_likes ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "user_id INTEGER NOT NULL,"
        "video_id INTEGER NOT NULL,"
        "created_at TEXT NOT NULL,"
        "UNIQUE(user_id, video_id),"
        "FOREIGN KEY(user_id) REFERENCES users(id),"
        "FOREIGN KEY(video_id) REFERENCES videos(id))";

    const char *sql_actions = "CREATE TABLE IF NOT EXISTS user_actions ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "user_id INTEGER NOT NULL,"
        "video_id INTEGER NOT NULL,"
        "action_type TEXT NOT NULL,"
        "weight REAL DEFAULT 1,"
        "created_at TEXT NOT NULL,"
        "UNIQUE(user_id, video_id, action_type),"
        "FOREIGN KEY(user_id) REFERENCES users(id),"
        "FOREIGN KEY(video_id) REFERENCES videos(id))";

    const char *sql_streams = "CREATE TABLE IF NOT EXISTS streams ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "user_id INTEGER NOT NULL,"
        "title TEXT NOT NULL,"
        "rtmp_url TEXT NOT NULL,"
        "hls_url TEXT,"
        "status INTEGER DEFAULT 1,"
        "viewer_count INTEGER DEFAULT 0,"
        "created_at TEXT NOT NULL)";

    const char *sql_api_identities = "CREATE TABLE IF NOT EXISTS api_identities ("
        "api_user_id INTEGER PRIMARY KEY,"
        "media_user_id INTEGER NOT NULL UNIQUE,"
        "created_at TEXT NOT NULL,"
        "FOREIGN KEY(media_user_id) REFERENCES users(id))";

    sqlite3_exec(m_db, sql_users, NULL, NULL, NULL);
    sqlite3_exec(m_db, "ALTER TABLE users ADD COLUMN phone TEXT", NULL, NULL, NULL);
    sqlite3_exec(m_db, sql_videos, NULL, NULL, NULL);
    sqlite3_exec(m_db, sql_uploads, NULL, NULL, NULL);
    sqlite3_exec(m_db, sql_likes, NULL, NULL, NULL);
    sqlite3_exec(m_db, sql_actions, NULL, NULL, NULL);
    sqlite3_exec(m_db, "CREATE INDEX IF NOT EXISTS idx_user_actions_user ON user_actions(user_id)", NULL, NULL, NULL);
    sqlite3_exec(m_db, "CREATE INDEX IF NOT EXISTS idx_user_actions_video ON user_actions(video_id)", NULL, NULL, NULL);
    sqlite3_exec(m_db, "CREATE INDEX IF NOT EXISTS idx_user_actions_user_video ON user_actions(user_id, video_id)", NULL, NULL, NULL);
    sqlite3_exec(m_db,
        "INSERT OR IGNORE INTO user_actions (user_id, video_id, action_type, weight, created_at) "
        "SELECT user_id, video_id, 'like', 10, created_at FROM user_likes",
        NULL, NULL, NULL);
    sqlite3_exec(m_db, sql_streams, NULL, NULL, NULL);
    sqlite3_exec(m_db, sql_api_identities, NULL, NULL, NULL);

    return 0;
}

static void send_json_response(int fd, int cmd, json_object *resp) {
    json_object_object_add(resp, "cmd", json_object_new_int(cmd));
    const char *json_str = json_object_to_json_string(resp);
    char *payload = xor_base64_encode(json_str);
    int len = strlen(payload);

    char sendbuf[len + MSG_HEAD_LEN + 1];
    int net_len = htonl(len);
    memcpy(sendbuf, &net_len, MSG_HEAD_LEN);
    memcpy(sendbuf + MSG_HEAD_LEN, payload, len);

    pthread_mutex_lock(&g_conn_lock);
    send(fd, sendbuf, len + MSG_HEAD_LEN, MSG_NOSIGNAL);
    pthread_mutex_unlock(&g_conn_lock);
    free(payload);
}

static void handle_register(int fd, const char *data, int len, conn_info_t *conn) {
    json_object *jobj = json_tokener_parse(data);
    if (!jobj) return;

    json_object *j_user, *j_email, *j_pwd, *j_phone = NULL;
    json_object_object_get_ex(jobj, "username", &j_user);
    json_object_object_get_ex(jobj, "email", &j_email);
    json_object_object_get_ex(jobj, "password", &j_pwd);
    json_object_object_get_ex(jobj, "phone", &j_phone);

    const char *username = json_object_get_string(j_user);
    const char *email = json_object_get_string(j_email);
    const char *password = json_object_get_string(j_pwd);
    const char *phone = j_phone ? json_object_get_string(j_phone) : email;

    json_object *resp = json_object_new_object();

    unsigned char salt_bytes[16];
    RAND_bytes(salt_bytes, sizeof(salt_bytes));
    char *salt_b64 = base64_encode(salt_bytes, 16);

    char salted_pwd[1024];
    snprintf(salted_pwd, sizeof(salted_pwd), "%s%s", password, salt_b64);

    unsigned char hash[32];
    sha256_hash(salted_pwd, hash);
    char *hash_b64 = base64_encode(hash, 32);

    char timebuf[64];
    time_t now = time(NULL);
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", localtime(&now));

    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO users (username, email, phone, password, salt, created_at) VALUES (?, ?, ?, ?, ?, ?)";
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, email, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, phone, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, hash_b64, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 5, salt_b64, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 6, timebuf, -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) == SQLITE_DONE) {
            json_object_object_add(resp, "result", json_object_new_string("ok"));
            json_object_object_add(resp, "msg", json_object_new_string("注册成功"));
        } else {
            json_object_object_add(resp, "result", json_object_new_string("fail"));
            json_object_object_add(resp, "msg", json_object_new_string("用户名、手机号或邮箱已存在"));
        }
        sqlite3_finalize(stmt);
    } else {
        json_object_object_add(resp, "result", json_object_new_string("fail"));
        json_object_object_add(resp, "msg", json_object_new_string("服务器错误"));
    }

    free(salt_b64);
    free(hash_b64);
    json_object_put(jobj);
    send_json_response(fd, CMD_REGISTER_RESP, resp);
    json_object_put(resp);
}

static void handle_login(int fd, const char *data, int len, conn_info_t *conn) {
    json_object *jobj = json_tokener_parse(data);
    if (!jobj) return;

    json_object *j_user, *j_pwd, *j_account = NULL;
    json_object_object_get_ex(jobj, "username", &j_user);
    json_object_object_get_ex(jobj, "password", &j_pwd);
    json_object_object_get_ex(jobj, "account", &j_account);

    const char *username = json_object_get_string(j_user);
    const char *password = json_object_get_string(j_pwd);
    const char *account = j_account ? json_object_get_string(j_account) : "";

    json_object *resp = json_object_new_object();

    sqlite3_stmt *stmt;
    const char *sql = "SELECT id, username, password, salt FROM users "
                      "WHERE username = ? OR email = ? OR phone = ?";
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, account, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, account, -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int uid = sqlite3_column_int(stmt, 0);
            const char *db_username = (const char *)sqlite3_column_text(stmt, 1);
            const char *stored_hash = (const char *)sqlite3_column_text(stmt, 2);
            const char *salt = (const char *)sqlite3_column_text(stmt, 3);

            char salted_pwd[1024];
            snprintf(salted_pwd, sizeof(salted_pwd), "%s%s", password, salt);

            unsigned char hash[32];
            sha256_hash(salted_pwd, hash);
            char *hash_b64 = base64_encode(hash, 32);

            if (strcmp(hash_b64, stored_hash) == 0) {
                char *access_token = generate_token(uid, JWT_SECRET, 7200);
                char *refresh_token = generate_token(uid, JWT_REFRESH_SECRET, 86400 * 7);

                conn->uid = uid;
                strncpy(conn->access_token, access_token, sizeof(conn->access_token) - 1);
                strncpy(conn->refresh_token, refresh_token, sizeof(conn->refresh_token) - 1);

                json_object_object_add(resp, "result", json_object_new_string("ok"));
                json_object_object_add(resp, "uid", json_object_new_int(uid));
                json_object_object_add(resp, "username", json_object_new_string(db_username));
                json_object_object_add(resp, "access_token", json_object_new_string(access_token));
                json_object_object_add(resp, "refresh_token", json_object_new_string(refresh_token));

                char timebuf[64];
                time_t now = time(NULL);
                strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", localtime(&now));
                sqlite3_stmt *upd;
                const char *upd_sql = "UPDATE users SET last_login = ? WHERE id = ?";
                if (sqlite3_prepare_v2(m_db, upd_sql, -1, &upd, NULL) == SQLITE_OK) {
                    sqlite3_bind_text(upd, 1, timebuf, -1, SQLITE_STATIC);
                    sqlite3_bind_int(upd, 2, uid);
                    sqlite3_step(upd);
                    sqlite3_finalize(upd);
                }

                free(access_token);
                free(refresh_token);
            } else {
                json_object_object_add(resp, "result", json_object_new_string("fail"));
                json_object_object_add(resp, "msg", json_object_new_string("密码错误"));
            }
            free(hash_b64);
        } else {
            json_object_object_add(resp, "result", json_object_new_string("fail"));
            json_object_object_add(resp, "msg", json_object_new_string("用户不存在"));
        }
        sqlite3_finalize(stmt);
    }

    json_object_put(jobj);
    send_json_response(fd, CMD_LOGIN_RESP, resp);
    json_object_put(resp);
}

static void handle_heartbeat(int fd, const char *data, int len, conn_info_t *conn) {
    (void)data;
    (void)len;

    conn->last_active = time(NULL);

    json_object *resp = json_object_new_object();
    json_object_object_add(resp, "result", json_object_new_string("ok"));
    json_object_object_add(resp, "server_time", json_object_new_int64((long long)time(NULL)));
    send_json_response(fd, CMD_HEARTBEAT_RESP, resp);
    json_object_put(resp);
}

static void handle_api_token_auth(int fd, const char *data, int len, conn_info_t *conn) {
    (void)len;
    json_object *jobj = json_tokener_parse(data);
    if (!jobj) return;

    json_object *j_token = NULL;
    json_object_object_get_ex(jobj, "access_token", &j_token);
    const char *access_token = j_token ? json_object_get_string(j_token) : "";

    json_object *resp = json_object_new_object();
    int api_uid = 0;
    char username[128] = {0};
    int rc = verify_token_details(access_token, JWT_SECRET, &api_uid,
                                  username, sizeof(username));

    if (rc == 0 && api_uid > 0 && username[0] != '\0') {
        sqlite3_stmt *stmt = NULL;
        int media_uid = 0;
        if (sqlite3_prepare_v2(m_db,
                               "SELECT media_user_id FROM api_identities WHERE api_user_id = ?",
                               -1,
                               &stmt, NULL) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, api_uid);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                media_uid = sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        }

        if (media_uid <= 0) {
            char media_username[192];
            char email[256];
            char timebuf[64];
            snprintf(media_username, sizeof(media_username), "%s", username);
            snprintf(email, sizeof(email), "api-%d@api.local", api_uid);
            time_t now = time(NULL);
            strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", localtime(&now));

            if (sqlite3_prepare_v2(m_db, "SELECT id FROM users WHERE username = ?", -1,
                                   &stmt, NULL) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, media_username, -1, SQLITE_STATIC);
                if (sqlite3_step(stmt) == SQLITE_ROW) {
                    snprintf(media_username, sizeof(media_username), "%s_api_%d",
                             username, api_uid);
                }
                sqlite3_finalize(stmt);
            }

            if (sqlite3_prepare_v2(
                    m_db,
                    "INSERT INTO users (username, email, password, salt, created_at) "
                    "VALUES (?, ?, '', '', ?)",
                    -1, &stmt, NULL) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, media_username, -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 2, email, -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 3, timebuf, -1, SQLITE_STATIC);
                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    media_uid = (int)sqlite3_last_insert_rowid(m_db);
                }
                sqlite3_finalize(stmt);
            }

            if (media_uid > 0 && sqlite3_prepare_v2(
                    m_db,
                    "INSERT INTO api_identities (api_user_id, media_user_id, created_at) "
                    "VALUES (?, ?, ?)",
                    -1, &stmt, NULL) == SQLITE_OK) {
                sqlite3_bind_int(stmt, 1, api_uid);
                sqlite3_bind_int(stmt, 2, media_uid);
                sqlite3_bind_text(stmt, 3, timebuf, -1, SQLITE_STATIC);
                if (sqlite3_step(stmt) != SQLITE_DONE) {
                    media_uid = 0;
                }
                sqlite3_finalize(stmt);
            }
        }

        if (media_uid > 0) {
            conn->uid = media_uid;
            snprintf(conn->access_token, sizeof(conn->access_token), "%s", access_token);
            json_object_object_add(resp, "result", json_object_new_string("ok"));
            json_object_object_add(resp, "uid", json_object_new_int(media_uid));
            json_object_object_add(resp, "username", json_object_new_string(username));
        } else {
            json_object_object_add(resp, "result", json_object_new_string("fail"));
            json_object_object_add(resp, "msg", json_object_new_string("无法创建媒体用户"));
        }
    } else {
        json_object_object_add(resp, "result", json_object_new_string("fail"));
        json_object_object_add(resp, "msg", json_object_new_string(
            rc == -2 ? "登录令牌已过期" : "无效的登录令牌"));
    }

    json_object_put(jobj);
    send_json_response(fd, CMD_API_TOKEN_AUTH_RESP, resp);
    json_object_put(resp);
}

static int check_auth(conn_info_t *conn, json_object *resp) {
    if (conn->uid <= 0) {
        json_object_object_add(resp, "result", json_object_new_string("fail"));
        json_object_object_add(resp, "msg", json_object_new_string("未登录或Token已过期"));
        return -1;
    }
    return 0;
}

static void handle_upload_init(int fd, const char *data, int len, conn_info_t *conn) {
    json_object *jobj = json_tokener_parse(data);
    if (!jobj) return;

    json_object *resp = json_object_new_object();
    if (check_auth(conn, resp) != 0) {
        json_object_put(jobj);
        send_json_response(fd, CMD_UPLOAD_INIT_RESP, resp);
        json_object_put(resp);
        return;
    }

    json_object *j_fname, *j_fsize, *j_fmd5;
    json_object_object_get_ex(jobj, "filename", &j_fname);
    json_object_object_get_ex(jobj, "filesize", &j_fsize);
    json_object_object_get_ex(jobj, "file_md5", &j_fmd5);

    const char *filename = json_object_get_string(j_fname);
    long long filesize = json_object_get_int64(j_fsize);
    const char *file_md5 = json_object_get_string(j_fmd5);

    sqlite3_stmt *stmt;
    const char *check_sql = "SELECT id, uploaded_size, status FROM uploads WHERE file_md5 = ? AND user_id = ?";
    long long uploaded_size = 0;
    int upload_id = -1;
    int status = 0;

    if (sqlite3_prepare_v2(m_db, check_sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, file_md5, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, conn->uid);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            upload_id = sqlite3_column_int(stmt, 0);
            uploaded_size = sqlite3_column_int64(stmt, 1);
            status = sqlite3_column_int(stmt, 2);
        }
        sqlite3_finalize(stmt);
    }

    if (upload_id > 0 && status == 0 && uploaded_size < filesize) {
        json_object_object_add(resp, "result", json_object_new_string("resume"));
        json_object_object_add(resp, "upload_id", json_object_new_int(upload_id));
        json_object_object_add(resp, "uploaded_size", json_object_new_int64(uploaded_size));
        json_object_object_add(resp, "chunk_size", json_object_new_int(CHUNK_SIZE));
    } else {
        char timebuf[64];
        time_t now = time(NULL);
        strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", localtime(&now));

        const char *insert_sql = "INSERT INTO uploads (user_id, file_md5, filename, filesize, chunk_size, status, created_at) VALUES (?, ?, ?, ?, ?, 0, ?)";
        if (sqlite3_prepare_v2(m_db, insert_sql, -1, &stmt, NULL) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, conn->uid);
            sqlite3_bind_text(stmt, 2, file_md5, -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 3, filename, -1, SQLITE_STATIC);
            sqlite3_bind_int64(stmt, 4, filesize);
            sqlite3_bind_int(stmt, 5, CHUNK_SIZE);
            sqlite3_bind_text(stmt, 6, timebuf, -1, SQLITE_STATIC);

            if (sqlite3_step(stmt) == SQLITE_DONE) {
                upload_id = (int)sqlite3_last_insert_rowid(m_db);
                json_object_object_add(resp, "result", json_object_new_string("ok"));
                json_object_object_add(resp, "upload_id", json_object_new_int(upload_id));
                json_object_object_add(resp, "uploaded_size", json_object_new_int64(0));
                json_object_object_add(resp, "chunk_size", json_object_new_int(CHUNK_SIZE));
            } else {
                json_object_object_add(resp, "result", json_object_new_string("fail"));
                json_object_object_add(resp, "msg", json_object_new_string("创建上传任务失败"));
            }
            sqlite3_finalize(stmt);
        }

        char chunk_dir[256];
        snprintf(chunk_dir, sizeof(chunk_dir), "data/uploads/chunks/%s", file_md5);
        mkdir(chunk_dir, 0755);
    }

    json_object_put(jobj);
    send_json_response(fd, CMD_UPLOAD_INIT_RESP, resp);
    json_object_put(resp);
}

static void handle_upload_chunk(int fd, const char *data, int len, conn_info_t *conn) {
    json_object *jobj = json_tokener_parse(data);
    if (!jobj) return;

    json_object *resp = json_object_new_object();
    if (check_auth(conn, resp) != 0) {
        json_object_put(jobj);
        send_json_response(fd, CMD_UPLOAD_CHUNK_RESP, resp);
        json_object_put(resp);
        return;
    }

    json_object *j_uid, *j_md5, *j_offset, *j_chunk_b64;
    json_object_object_get_ex(jobj, "upload_id", &j_uid);
    json_object_object_get_ex(jobj, "file_md5", &j_md5);
    json_object_object_get_ex(jobj, "offset", &j_offset);
    json_object_object_get_ex(jobj, "chunk_data", &j_chunk_b64);

    int upload_id = json_object_get_int(j_uid);
    const char *file_md5 = json_object_get_string(j_md5);
    long long offset = json_object_get_int64(j_offset);
    const char *chunk_b64 = json_object_get_string(j_chunk_b64);

    unsigned char *chunk_data;
    int chunk_len;
    base64_decode(chunk_b64, &chunk_data, &chunk_len);

    char chunk_path[512];
    snprintf(chunk_path, sizeof(chunk_path), "data/uploads/chunks/%s/chunk_%lld", file_md5, offset / CHUNK_SIZE);
    FILE *fp = fopen(chunk_path, "wb");
    if (fp) {
        fwrite(chunk_data, 1, chunk_len, fp);
        fclose(fp);
    }
    free(chunk_data);

    long long new_size = offset + chunk_len;

    sqlite3_stmt *stmt;
    const char *upd_sql = "UPDATE uploads SET uploaded_size = ?, updated_at = ? WHERE id = ?";
    char timebuf[64];
    time_t now = time(NULL);
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", localtime(&now));

    if (sqlite3_prepare_v2(m_db, upd_sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, new_size);
        sqlite3_bind_text(stmt, 2, timebuf, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 3, upload_id);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    json_object_object_add(resp, "result", json_object_new_string("ok"));
    json_object_object_add(resp, "upload_id", json_object_new_int(upload_id));
    json_object_object_add(resp, "uploaded_size", json_object_new_int64(new_size));

    json_object_put(jobj);
    send_json_response(fd, CMD_UPLOAD_CHUNK_RESP, resp);
    json_object_put(resp);
}

static void handle_upload_finish(int fd, const char *data, int len, conn_info_t *conn) {
    json_object *jobj = json_tokener_parse(data);
    if (!jobj) return;

    json_object *resp = json_object_new_object();
    if (check_auth(conn, resp) != 0) {
        json_object_put(jobj);
        send_json_response(fd, CMD_UPLOAD_FINISH_RESP, resp);
        json_object_put(resp);
        return;
    }

    json_object *j_uid, *j_md5, *j_fname, *j_fsize;
    json_object_object_get_ex(jobj, "upload_id", &j_uid);
    json_object_object_get_ex(jobj, "file_md5", &j_md5);
    json_object_object_get_ex(jobj, "filename", &j_fname);
    json_object_object_get_ex(jobj, "filesize", &j_fsize);

    int upload_id = json_object_get_int(j_uid);
    const char *file_md5 = json_object_get_string(j_md5);
    const char *filename = json_object_get_string(j_fname);
    long long filesize = json_object_get_int64(j_fsize);

    char final_path[512];
    const char *ext = strrchr(filename, '.');
    int is_gif = (ext && strcasecmp(ext, ".gif") == 0);

    if (is_gif) {
        snprintf(final_path, sizeof(final_path), "data/uploads/gif/%s", filename);
    } else {
        snprintf(final_path, sizeof(final_path), "data/uploads/files/%s", filename);
    }

    FILE *out_fp = fopen(final_path, "wb");
    if (!out_fp) {
        json_object_object_add(resp, "result", json_object_new_string("fail"));
        json_object_object_add(resp, "msg", json_object_new_string("无法创建目标文件"));
        json_object_put(jobj);
        send_json_response(fd, CMD_UPLOAD_FINISH_RESP, resp);
        json_object_put(resp);
        return;
    }

    long long total_written = 0;
    int chunk_idx = 0;
    while (total_written < filesize) {
        char chunk_path[512];
        snprintf(chunk_path, sizeof(chunk_path), "data/uploads/chunks/%s/chunk_%d", file_md5, chunk_idx);
        FILE *chunk_fp = fopen(chunk_path, "rb");
        if (!chunk_fp) break;

        char buf[CHUNK_SIZE];
        size_t nread;
        while ((nread = fread(buf, 1, sizeof(buf), chunk_fp)) > 0) {
            fwrite(buf, 1, nread, out_fp);
            total_written += nread;
        }
        fclose(chunk_fp);
        remove(chunk_path);
        chunk_idx++;
    }
    fclose(out_fp);

    char chunk_dir[256];
    snprintf(chunk_dir, sizeof(chunk_dir), "data/uploads/chunks/%s", file_md5);
    rmdir(chunk_dir);

    char timebuf[64];
    time_t now = time(NULL);
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", localtime(&now));

    if (!is_gif) {
        sqlite3_stmt *stmt;
        const char *sql = "INSERT INTO videos (user_id, filename, filepath, filesize, file_md5, created_at) VALUES (?, ?, ?, ?, ?, ?)";
        if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, conn->uid);
            sqlite3_bind_text(stmt, 2, filename, -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 3, final_path, -1, SQLITE_STATIC);
            sqlite3_bind_int64(stmt, 4, filesize);
            sqlite3_bind_text(stmt, 5, file_md5, -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 6, timebuf, -1, SQLITE_STATIC);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }

    sqlite3_stmt *stmt;
    const char *upd_sql = "UPDATE uploads SET status = 1, updated_at = ? WHERE id = ?";
    if (sqlite3_prepare_v2(m_db, upd_sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, timebuf, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, upload_id);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    json_object_object_add(resp, "result", json_object_new_string("ok"));
    json_object_object_add(resp, "filepath", json_object_new_string(final_path));

    json_object_put(jobj);
    send_json_response(fd, CMD_UPLOAD_FINISH_RESP, resp);
    json_object_put(resp);
}

static void handle_video_list(int fd, const char *data, int len, conn_info_t *conn) {
    json_object *resp = json_object_new_object();
    json_object *arr = json_object_new_array();

    sqlite3_stmt *stmt;
    const char *sql = "SELECT v.id, v.filename, v.filepath, v.filesize, v.like_count, v.play_count, v.created_at, u.username FROM videos v JOIN users u ON v.user_id = u.id ORDER BY v.created_at DESC LIMIT 100";
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            json_object *item = json_object_new_object();
            json_object_object_add(item, "id", json_object_new_int(sqlite3_column_int(stmt, 0)));
            json_object_object_add(item, "filename", json_object_new_string((const char *)sqlite3_column_text(stmt, 1)));
            json_object_object_add(item, "filepath", json_object_new_string((const char *)sqlite3_column_text(stmt, 2)));
            json_object_object_add(item, "filesize", json_object_new_int64(sqlite3_column_int64(stmt, 3)));
            json_object_object_add(item, "like_count", json_object_new_int(sqlite3_column_int(stmt, 4)));
            json_object_object_add(item, "play_count", json_object_new_int(sqlite3_column_int(stmt, 5)));
            json_object_object_add(item, "created_at", json_object_new_string((const char *)sqlite3_column_text(stmt, 6)));
            json_object_object_add(item, "username", json_object_new_string((const char *)sqlite3_column_text(stmt, 7)));
            json_object_array_add(arr, item);
        }
        sqlite3_finalize(stmt);
    }

    json_object_object_add(resp, "result", json_object_new_string("ok"));
    json_object_object_add(resp, "videos", arr);
    send_json_response(fd, CMD_VIDEO_LIST_RESP, resp);
    json_object_put(resp);
}

static void get_current_time_str(char *buf, size_t size) {
    time_t now = time(NULL);
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", localtime(&now));
}

static void record_user_action(int user_id, int video_id, const char *action_type, double weight) {
    if (user_id <= 0 || video_id <= 0 || !action_type) {
        return;
    }

    char timebuf[64];
    get_current_time_str(timebuf, sizeof(timebuf));

    sqlite3_stmt *stmt;
    const char *insert_sql =
        "INSERT OR IGNORE INTO user_actions (user_id, video_id, action_type, weight, created_at) "
        "VALUES (?, ?, ?, ?, ?)";
    if (sqlite3_prepare_v2(m_db, insert_sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, user_id);
        sqlite3_bind_int(stmt, 2, video_id);
        sqlite3_bind_text(stmt, 3, action_type, -1, SQLITE_STATIC);
        sqlite3_bind_double(stmt, 4, weight);
        sqlite3_bind_text(stmt, 5, timebuf, -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    const char *update_sql =
        "UPDATE user_actions SET weight = ?, created_at = ? "
        "WHERE user_id = ? AND video_id = ? AND action_type = ?";
    if (sqlite3_prepare_v2(m_db, update_sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_double(stmt, 1, weight);
        sqlite3_bind_text(stmt, 2, timebuf, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 3, user_id);
        sqlite3_bind_int(stmt, 4, video_id);
        sqlite3_bind_text(stmt, 5, action_type, -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
}

static int get_user_action_count(int user_id) {
    if (user_id <= 0) {
        return 0;
    }

    int count = 0;
    sqlite3_stmt *stmt;
    const char *sql = "SELECT COUNT(DISTINCT video_id) FROM user_actions WHERE user_id = ?";
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, user_id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    return count;
}

static double get_user_vector_norm(int user_id) {
    if (user_id <= 0) {
        return 0.0;
    }

    double norm = 0.0;
    sqlite3_stmt *stmt;
    const char *sql =
        "SELECT SUM(score * score) FROM ("
        "SELECT video_id, SUM(weight) AS score FROM user_actions WHERE user_id = ? GROUP BY video_id"
        ")";
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, user_id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            norm = sqlite3_column_double(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    return norm;
}

static int find_most_similar_user(int user_id, double *out_similarity) {
    if (out_similarity) {
        *out_similarity = 0.0;
    }

    double current_norm = get_user_vector_norm(user_id);
    if (current_norm <= 0.0) {
        return 0;
    }

    int best_user_id = 0;
    double best_similarity = 0.0;
    sqlite3_stmt *stmt;
    const char *sql =
        "SELECT common.user_id, common.dot_product, norms.other_norm "
        "FROM ("
        "  SELECT other.user_id, SUM(cur.score * other.score) AS dot_product "
        "  FROM ("
        "    SELECT video_id, SUM(weight) AS score FROM user_actions WHERE user_id = ? GROUP BY video_id"
        "  ) cur "
        "  JOIN ("
        "    SELECT user_id, video_id, SUM(weight) AS score FROM user_actions WHERE user_id != ? GROUP BY user_id, video_id"
        "  ) other ON cur.video_id = other.video_id "
        "  GROUP BY other.user_id"
        ") common "
        "JOIN ("
        "  SELECT user_id, SUM(score * score) AS other_norm "
        "  FROM ("
        "    SELECT user_id, video_id, SUM(weight) AS score FROM user_actions WHERE user_id != ? GROUP BY user_id, video_id"
        "  ) "
        "  GROUP BY user_id"
        ") norms ON common.user_id = norms.user_id";

    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, user_id);
        sqlite3_bind_int(stmt, 2, user_id);
        sqlite3_bind_int(stmt, 3, user_id);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int other_user_id = sqlite3_column_int(stmt, 0);
            double dot_product = sqlite3_column_double(stmt, 1);
            double other_norm = sqlite3_column_double(stmt, 2);
            if (other_norm <= 0.0) {
                continue;
            }

            double similarity = dot_product / sqrt(current_norm * other_norm);
            if (similarity > best_similarity) {
                best_similarity = similarity;
                best_user_id = other_user_id;
            }
        }
        sqlite3_finalize(stmt);
    }

    if (out_similarity) {
        *out_similarity = best_similarity;
    }
    return best_user_id;
}

static int recommend_contains(int *ids, int count, int video_id) {
    for (int i = 0; i < count; ++i) {
        if (ids[i] == video_id) {
            return 1;
        }
    }
    return 0;
}

static void append_recommend_item(json_object *arr, int *ids, int *count, sqlite3_stmt *stmt,
                                  const char *source, int similar_user_id, double similarity,
                                  int has_cf_score) {
    if (*count >= RECOMMEND_LIMIT) {
        return;
    }

    int video_id = sqlite3_column_int(stmt, 0);
    if (recommend_contains(ids, *count, video_id)) {
        return;
    }

    const unsigned char *filename = sqlite3_column_text(stmt, 1);
    const unsigned char *filepath = sqlite3_column_text(stmt, 2);

    json_object *item = json_object_new_object();
    json_object_object_add(item, "id", json_object_new_int(video_id));
    json_object_object_add(item, "filename", json_object_new_string(filename ? (const char *)filename : ""));
    json_object_object_add(item, "filepath", json_object_new_string(filepath ? (const char *)filepath : ""));
    json_object_object_add(item, "like_count", json_object_new_int(sqlite3_column_int(stmt, 3)));
    json_object_object_add(item, "play_count", json_object_new_int(sqlite3_column_int(stmt, 4)));
    json_object_object_add(item, "hot_score", json_object_new_int(sqlite3_column_int(stmt, has_cf_score ? 6 : 5)));
    json_object_object_add(item, "source", json_object_new_string(source));
    if (has_cf_score) {
        json_object_object_add(item, "cf_score", json_object_new_double(sqlite3_column_double(stmt, 5)));
        json_object_object_add(item, "similar_user_id", json_object_new_int(similar_user_id));
        json_object_object_add(item, "similarity", json_object_new_double(similarity));
    }
    json_object_array_add(arr, item);
    ids[*count] = video_id;
    (*count)++;
}

static void append_hot_recommendations(json_object *arr, int *ids, int *count, int user_id) {
    sqlite3_stmt *stmt;
    const char *sql =
        "SELECT v.id, v.filename, v.filepath, v.like_count, v.play_count, "
        "(v.like_count * 3 + v.play_count) AS hot_score "
        "FROM videos v "
        "WHERE (? <= 0 OR v.id NOT IN (SELECT video_id FROM user_actions WHERE user_id = ?)) "
        "ORDER BY hot_score DESC, v.created_at DESC LIMIT 100";

    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, user_id);
        sqlite3_bind_int(stmt, 2, user_id);
        while (*count < RECOMMEND_LIMIT && sqlite3_step(stmt) == SQLITE_ROW) {
            append_recommend_item(arr, ids, count, stmt, "hot", 0, 0.0, 0);
        }
        sqlite3_finalize(stmt);
    }
}

static void handle_video_recommend(int fd, const char *data, int len, conn_info_t *conn) {
    json_object *resp = json_object_new_object();
    json_object *arr = json_object_new_array();

    int ids[RECOMMEND_LIMIT];
    int count = 0;
    int current_actions = get_user_action_count(conn->uid);
    int similar_user_id = 0;
    double similarity = 0.0;
    const char *strategy = "hot";

    if (conn->uid > 0 && current_actions >= MIN_CF_ACTIONS) {
        similar_user_id = find_most_similar_user(conn->uid, &similarity);
        if (similar_user_id > 0 && similarity > 0.0) {
            sqlite3_stmt *cf_stmt;
            const char *cf_sql =
                "SELECT v.id, v.filename, v.filepath, v.like_count, v.play_count, "
                "SUM(ua.weight) AS cf_score, "
                "(v.like_count * 3 + v.play_count) AS hot_score "
                "FROM user_actions ua "
                "JOIN videos v ON v.id = ua.video_id "
                "WHERE ua.user_id = ? "
                "AND v.id NOT IN (SELECT video_id FROM user_actions WHERE user_id = ?) "
                "GROUP BY v.id "
                "ORDER BY cf_score DESC, hot_score DESC LIMIT ?";

            if (sqlite3_prepare_v2(m_db, cf_sql, -1, &cf_stmt, NULL) == SQLITE_OK) {
                sqlite3_bind_int(cf_stmt, 1, similar_user_id);
                sqlite3_bind_int(cf_stmt, 2, conn->uid);
                sqlite3_bind_int(cf_stmt, 3, CF_LIMIT);
                while (count < RECOMMEND_LIMIT && sqlite3_step(cf_stmt) == SQLITE_ROW) {
                    append_recommend_item(arr, ids, &count, cf_stmt, "collaborative",
                                          similar_user_id, similarity, 1);
                }
                sqlite3_finalize(cf_stmt);
            }

            if (count > 0) {
                strategy = "collaborative";
            }
        }
    }

    append_hot_recommendations(arr, ids, &count, conn->uid);
    if (count == 0 && conn->uid > 0) {
        append_hot_recommendations(arr, ids, &count, 0);
    }

    json_object_object_add(resp, "result", json_object_new_string("ok"));
    json_object_object_add(resp, "strategy", json_object_new_string(strategy));
    json_object_object_add(resp, "current_action_count", json_object_new_int(current_actions));
    json_object_object_add(resp, "similar_user_id", json_object_new_int(similar_user_id));
    json_object_object_add(resp, "similarity", json_object_new_double(similarity));
    json_object_object_add(resp, "videos", arr);
    send_json_response(fd, CMD_VIDEO_RECOMMEND_RESP, resp);
    json_object_put(resp);
}

static void handle_video_play(int fd, const char *data, int len, conn_info_t *conn) {
    json_object *jobj = json_tokener_parse(data);
    if (!jobj) return;

    json_object *resp = json_object_new_object();
    json_object *j_vid = NULL;
    json_object_object_get_ex(jobj, "video_id", &j_vid);
    int video_id = j_vid ? json_object_get_int(j_vid) : 0;

    if (video_id > 0) {
        sqlite3_stmt *stmt;
        if (sqlite3_prepare_v2(m_db, "UPDATE videos SET play_count = play_count + 1 WHERE id = ?", -1, &stmt, NULL) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, video_id);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
        if (conn->uid > 0 &&
            sqlite3_prepare_v2(m_db, "UPDATE users SET play_count = play_count + 1 WHERE id = ?", -1, &stmt, NULL) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, conn->uid);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
        if (conn->uid > 0) {
            record_user_action(conn->uid, video_id, "play", 1.0);
        }
    }

    json_object_object_add(resp, "result", json_object_new_string(video_id > 0 ? "ok" : "fail"));
    send_json_response(fd, CMD_VIDEO_PLAY_RESP, resp);
    json_object_put(resp);
    json_object_put(jobj);
}

static void handle_video_like(int fd, const char *data, int len, conn_info_t *conn) {
    json_object *jobj = json_tokener_parse(data);
    if (!jobj) return;

    json_object *resp = json_object_new_object();
    if (check_auth(conn, resp) != 0) {
        json_object_put(jobj);
        send_json_response(fd, CMD_VIDEO_LIKE_RESP, resp);
        json_object_put(resp);
        return;
    }

    json_object *j_vid = NULL;
    json_object_object_get_ex(jobj, "video_id", &j_vid);
    int video_id = j_vid ? json_object_get_int(j_vid) : 0;
    int inserted = 0;

    if (video_id > 0) {
        char timebuf[64];
        time_t now = time(NULL);
        strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", localtime(&now));

        sqlite3_stmt *stmt;
        const char *sql = "INSERT OR IGNORE INTO user_likes (user_id, video_id, created_at) VALUES (?, ?, ?)";
        if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, conn->uid);
            sqlite3_bind_int(stmt, 2, video_id);
            sqlite3_bind_text(stmt, 3, timebuf, -1, SQLITE_STATIC);
            sqlite3_step(stmt);
            inserted = sqlite3_changes(m_db) > 0;
            sqlite3_finalize(stmt);
        }

        if (inserted) {
            record_user_action(conn->uid, video_id, "like", 10.0);
            if (sqlite3_prepare_v2(m_db, "UPDATE videos SET like_count = like_count + 1 WHERE id = ?", -1, &stmt, NULL) == SQLITE_OK) {
                sqlite3_bind_int(stmt, 1, video_id);
                sqlite3_step(stmt);
                sqlite3_finalize(stmt);
            }
            if (sqlite3_prepare_v2(m_db, "UPDATE users SET like_count = like_count + 1 WHERE id = ?", -1, &stmt, NULL) == SQLITE_OK) {
                sqlite3_bind_int(stmt, 1, conn->uid);
                sqlite3_step(stmt);
                sqlite3_finalize(stmt);
            }
        }
    }

    json_object_object_add(resp, "result", json_object_new_string(video_id > 0 ? "ok" : "fail"));
    json_object_object_add(resp, "liked", json_object_new_boolean(inserted));
    send_json_response(fd, CMD_VIDEO_LIKE_RESP, resp);
    json_object_put(resp);
    json_object_put(jobj);
}

static void handle_stream_list(int fd, const char *data, int len, conn_info_t *conn) {
    json_object *resp = json_object_new_object();
    json_object *arr = json_object_new_array();

    sqlite3_stmt *stmt;
    const char *sql = "SELECT s.id, s.title, s.rtmp_url, s.hls_url, s.viewer_count, u.username FROM streams s JOIN users u ON s.user_id = u.id WHERE s.status = 1 ORDER BY s.viewer_count DESC";
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            json_object *item = json_object_new_object();
            json_object_object_add(item, "id", json_object_new_int(sqlite3_column_int(stmt, 0)));
            json_object_object_add(item, "title", json_object_new_string((const char *)sqlite3_column_text(stmt, 1)));
            json_object_object_add(item, "rtmp_url", json_object_new_string((const char *)sqlite3_column_text(stmt, 2)));
            const char *hls = (const char *)sqlite3_column_text(stmt, 3);
            json_object_object_add(item, "hls_url", json_object_new_string(hls ? hls : ""));
            json_object_object_add(item, "viewer_count", json_object_new_int(sqlite3_column_int(stmt, 4)));
            json_object_object_add(item, "username", json_object_new_string((const char *)sqlite3_column_text(stmt, 5)));
            json_object_array_add(arr, item);
        }
        sqlite3_finalize(stmt);
    }

    json_object_object_add(resp, "result", json_object_new_string("ok"));
    json_object_object_add(resp, "streams", arr);
    send_json_response(fd, CMD_STREAM_LIST_RESP, resp);
    json_object_put(resp);
}

static void handle_token_refresh(int fd, const char *data, int len, conn_info_t *conn) {
    json_object *jobj = json_tokener_parse(data);
    if (!jobj) return;

    json_object *j_rt;
    json_object_object_get_ex(jobj, "refresh_token", &j_rt);
    const char *refresh_token = json_object_get_string(j_rt);

    json_object *resp = json_object_new_object();
    int uid = 0;
    int rc = verify_token(refresh_token, JWT_REFRESH_SECRET, &uid);

    if (rc == 0) {
        char *new_access = generate_token(uid, JWT_SECRET, 7200);
        char *new_refresh = generate_token(uid, JWT_REFRESH_SECRET, 86400 * 7);

        conn->uid = uid;
        strncpy(conn->access_token, new_access, sizeof(conn->access_token) - 1);
        strncpy(conn->refresh_token, new_refresh, sizeof(conn->refresh_token) - 1);

        json_object_object_add(resp, "result", json_object_new_string("ok"));
        json_object_object_add(resp, "access_token", json_object_new_string(new_access));
        json_object_object_add(resp, "refresh_token", json_object_new_string(new_refresh));

        free(new_access);
        free(new_refresh);
    } else {
        json_object_object_add(resp, "result", json_object_new_string("fail"));
        json_object_object_add(resp, "msg", json_object_new_string(rc == -2 ? "Refresh Token已过期" : "无效Token"));
    }

    json_object_put(jobj);
    send_json_response(fd, CMD_TOKEN_REFRESH_RESP, resp);
    json_object_put(resp);
}

static void handle_gif_list(int fd, const char *data, int len, conn_info_t *conn) {
    json_object *resp = json_object_new_object();
    json_object *arr = json_object_new_array();

    DIR *dir = opendir("data/uploads/gif");
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_name[0] == '.') continue;
            char filepath[512];
            snprintf(filepath, sizeof(filepath), "data/uploads/gif/%s", ent->d_name);
            struct stat st;
            if (stat(filepath, &st) == 0 && S_ISREG(st.st_mode)) {
                json_object *item = json_object_new_object();
                json_object_object_add(item, "filename", json_object_new_string(ent->d_name));
                json_object_object_add(item, "filepath", json_object_new_string(filepath));
                json_object_object_add(item, "filesize", json_object_new_int64(st.st_size));
                json_object_array_add(arr, item);
            }
        }
        closedir(dir);
    }

    json_object_object_add(resp, "result", json_object_new_string("ok"));
    json_object_object_add(resp, "gifs", arr);
    send_json_response(fd, CMD_GIF_LIST_RESP, resp);
    json_object_put(resp);
}

static void *thread_worker(void *arg) {
    threadpool_t *pool = (threadpool_t *)arg;
    while (1) {
        pthread_mutex_lock(&(pool->lock));
        while (pool->queue_size == 0 && !pool->shutdown) {
            pthread_cond_wait(&(pool->notify), &(pool->lock));
        }
        if (pool->shutdown) {
            pthread_mutex_unlock(&(pool->lock));
            pthread_exit(NULL);
        }
        task_t *task = pool->queue_head;
        if (task) {
            pool->queue_head = task->next;
            if (!pool->queue_head) pool->queue_tail = NULL;
            pool->queue_size--;
        }
        pthread_mutex_unlock(&(pool->lock));
        if (task) {
            (*(task->func))(task->arg);
            free(task);
        }
    }
    return NULL;
}

threadpool_t *threadpool_create(int thread_count, int queue_size) {
    threadpool_t *pool = (threadpool_t *)malloc(sizeof(threadpool_t));
    memset(pool, 0, sizeof(threadpool_t));
    pool->thread_count = thread_count;
    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_count);
    pthread_mutex_init(&(pool->lock), NULL);
    pthread_cond_init(&(pool->notify), NULL);
    pool->queue_head = pool->queue_tail = NULL;
    pool->queue_size = 0;
    pool->shutdown = 0;
    pool->started = 0;
    for (int i = 0; i < thread_count; i++) {
        pthread_create(&(pool->threads[i]), NULL, thread_worker, (void *)pool);
        pool->started++;
    }
    return pool;
}

int threadpool_add(threadpool_t *pool, void (*func)(void *), void *arg) {
    task_t *task = (task_t *)malloc(sizeof(task_t));
    task->func = func;
    task->arg = arg;
    task->next = NULL;
    pthread_mutex_lock(&(pool->lock));
    if (pool->queue_tail) {
        pool->queue_tail->next = task;
    } else {
        pool->queue_head = task;
    }
    pool->queue_tail = task;
    pool->queue_size++;
    pthread_cond_signal(&(pool->notify));
    pthread_mutex_unlock(&(pool->lock));
    return 0;
}

void threadpool_destroy(threadpool_t *pool) {
    if (!pool) return;
    pool->shutdown = 1;
    pthread_cond_broadcast(&(pool->notify));
    for (int i = 0; i < pool->thread_count; i++) {
        pthread_join(pool->threads[i], NULL);
    }
    free(pool->threads);
    while (pool->queue_head) {
        task_t *t = pool->queue_head;
        pool->queue_head = t->next;
        free(t);
    }
    pthread_mutex_destroy(&(pool->lock));
    pthread_cond_destroy(&(pool->notify));
    free(pool);
}

int setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, old_option | O_NONBLOCK);
    return old_option;
}

void addfd(int epollfd, int fd, int oneshot) {
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    if (oneshot) event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

void reset_oneshot(int epollfd, int fd) {
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

void removefd(int epollfd, int fd) {
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);
}

void register_msg_handler(int cmd, msg_handler_t handler) {
    if (cmd >= 0 && cmd < 256) {
        m_handlers[cmd] = handler;
    }
}

void dispatch_message(int fd, const char *data, int len, conn_info_t *conn) {
    if (len < 4) return;
    char enc[len + 1];
    memcpy(enc, data, len);
    enc[len] = 0;

    int plain_len = 0;
    char *plain = xor_base64_decode(enc, &plain_len);
    char *copy = plain;
    if (!copy || plain_len <= 0 || copy[0] != '{') {
        free(plain);
        copy = (char *)malloc(len + 1);
        memcpy(copy, data, len);
        copy[len] = 0;
        plain_len = len;
    }

    int cmd = 0;
    json_object *jobj = json_tokener_parse(copy);
    if (jobj) {
        json_object *j_cmd;
        if (json_object_object_get_ex(jobj, "cmd", &j_cmd)) {
            cmd = json_object_get_int(j_cmd);
        }
        json_object_put(jobj);
    }

    if (cmd > 0 && cmd < 256 && m_handlers[cmd]) {
        fprintf(stderr, "[recv] fd=%d cmd=%d len=%d\n", fd, cmd, plain_len);
        m_handlers[cmd](fd, copy, plain_len, conn);
    } else {
        fprintf(stderr, "[warn] fd=%d unhandled cmd=%d len=%d\n", fd, cmd, plain_len);
    }
    free(copy);
}

conn_info_t *get_conn(int fd) {
    if (fd >= 0 && fd < MAX_CONNS) return &m_conns[fd];
    return NULL;
}

void free_conn(int fd) {
    if (fd >= 0 && fd < MAX_CONNS) {
        memset(&m_conns[fd], 0, sizeof(conn_info_t));
        m_conns[fd].fd = -1;
    }
}

typedef struct {
    int fd;
} task_arg_t;

static void process_conn(void *arg) {
    task_arg_t *targ = (task_arg_t *)arg;
    int fd = targ->fd;
    free(targ);

    conn_info_t *conn = get_conn(fd);
    if (!conn) return;

    while (1) {
        int left = (int)sizeof(conn->recv_buf) - conn->recv_len;
        if (left <= 0) {
            fprintf(stderr, "[warn] fd=%d recv buffer full\n", fd);
            removefd(m_epollfd, fd);
            free_conn(fd);
            return;
        }

        int ret = recv(fd, conn->recv_buf + conn->recv_len, left, 0);
        if (ret < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            perror("recv");
            removefd(m_epollfd, fd);
            free_conn(fd);
            return;
        }
        if (ret == 0) {
            fprintf(stderr, "[conn] fd=%d closed by peer\n", fd);
            removefd(m_epollfd, fd);
            free_conn(fd);
            return;
        }
        conn->recv_len += ret;
    }

    conn->last_active = time(NULL);

    while (conn->recv_len >= MSG_HEAD_LEN) {
        int msg_len = 0;
        memcpy(&msg_len, conn->recv_buf, MSG_HEAD_LEN);
        msg_len = ntohl(msg_len);

        if (msg_len <= 0 || msg_len > MAX_MSG_LEN) {
            removefd(m_epollfd, fd);
            free_conn(fd);
            return;
        }

        if (conn->recv_len < msg_len + MSG_HEAD_LEN) break;

        dispatch_message(fd, conn->recv_buf + MSG_HEAD_LEN, msg_len, conn);

        int remain = conn->recv_len - msg_len - MSG_HEAD_LEN;
        if (remain > 0) {
            memmove(conn->recv_buf, conn->recv_buf + msg_len + MSG_HEAD_LEN, remain);
        }
        conn->recv_len = remain;
    }

    reset_oneshot(m_epollfd, fd);
}

void server_run(const char *ip, int port) {
    signal(SIGPIPE, SIG_IGN);

    if (init_database() != 0) {
        fprintf(stderr, "Database init failed, server stopped.\n");
        return;
    }
    memset(m_conns, 0, sizeof(m_conns));
    for (int i = 0; i < MAX_CONNS; i++) m_conns[i].fd = -1;

    register_msg_handler(CMD_REGISTER, handle_register);
    register_msg_handler(CMD_LOGIN, handle_login);
    register_msg_handler(CMD_HEARTBEAT, handle_heartbeat);
    register_msg_handler(CMD_API_TOKEN_AUTH, handle_api_token_auth);
    register_msg_handler(CMD_TOKEN_REFRESH, handle_token_refresh);
    register_msg_handler(CMD_UPLOAD_INIT, handle_upload_init);
    register_msg_handler(CMD_UPLOAD_CHUNK, handle_upload_chunk);
    register_msg_handler(CMD_UPLOAD_FINISH, handle_upload_finish);
    register_msg_handler(CMD_VIDEO_LIST, handle_video_list);
    register_msg_handler(CMD_VIDEO_LIKE, handle_video_like);
    register_msg_handler(CMD_VIDEO_PLAY, handle_video_play);
    register_msg_handler(CMD_VIDEO_RECOMMEND, handle_video_recommend);
    register_msg_handler(CMD_STREAM_LIST, handle_stream_list);
    register_msg_handler(CMD_GIF_LIST, handle_gif_list);

    m_pool = threadpool_create(THREAD_MIN, TASK_QUEUE_MAX);

    m_listenfd = socket(PF_INET, SOCK_STREAM, 0);
    if (m_listenfd < 0) {
        perror("socket");
        return;
    }

    int opt = 1;
    setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    if (bind(m_listenfd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        perror("bind");
        fprintf(stderr, "Bind failed on %s:%d. Check whether the port is already in use.\n", ip, port);
        close(m_listenfd);
        return;
    }
    if (listen(m_listenfd, 512) != 0) {
        perror("listen");
        close(m_listenfd);
        return;
    }

    m_epollfd = epoll_create(MAX_EVENTS);
    if (m_epollfd < 0) {
        perror("epoll_create");
        close(m_listenfd);
        return;
    }

    addfd(m_epollfd, m_listenfd, 0);

    printf("Server started on %s:%d, epoll+threadpool ready\n", ip, port);

    struct epoll_event events[MAX_EVENTS];
    while (1) {
        int nfds = epoll_wait(m_epollfd, events, MAX_EVENTS, EPOLL_TIMEOUT);
        for (int i = 0; i < nfds; i++) {
            int sockfd = events[i].data.fd;
            if (sockfd == m_listenfd) {
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int connfd = accept(m_listenfd, (struct sockaddr *)&client_addr, &client_len);
                if (connfd >= 0) {
                    if (connfd < MAX_CONNS) {
                        memset(&m_conns[connfd], 0, sizeof(conn_info_t));
                        m_conns[connfd].fd = connfd;
                        m_conns[connfd].addr = client_addr;
                        m_conns[connfd].last_active = time(NULL);
                        fprintf(stderr, "[conn] new client fd=%d\n", connfd);
                        addfd(m_epollfd, connfd, 1);
                    } else {
                        close(connfd);
                    }
                }
            } else if (events[i].events & EPOLLIN) {
                task_arg_t *arg = (task_arg_t *)malloc(sizeof(task_arg_t));
                arg->fd = sockfd;
                threadpool_add(m_pool, process_conn, arg);
            }
        }

        time_t now = time(NULL);
        for (int i = 0; i < MAX_CONNS; i++) {
            if (m_conns[i].fd > 0 && m_conns[i].uid > 0 && (now - m_conns[i].last_active) > 300) {
                removefd(m_epollfd, m_conns[i].fd);
                free_conn(i);
            }
        }
    }
}
