#ifndef EPOLL_SERVER_H
#define EPOLL_SERVER_H

#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <assert.h>
#include <time.h>

#define MAX_EVENTS 2048
#define MAX_CONNS 4096
#define EPOLL_TIMEOUT 10
#define THREAD_MIN 4
#define THREAD_MAX 64
#define TASK_QUEUE_MAX 8192

typedef struct task {
    void (*func)(void *arg);
    void *arg;
    struct task *next;
} task_t;

typedef struct threadpool {
    pthread_mutex_t lock;
    pthread_cond_t notify;
    pthread_t *threads;
    task_t *queue_head;
    task_t *queue_tail;
    int thread_count;
    int queue_size;
    int shutdown;
    int started;
} threadpool_t;

typedef struct conn_info {
    int fd;
    struct sockaddr_in addr;
    char recv_buf[65536];
    int recv_len;
    char send_buf[65536];
    int send_len;
    time_t last_active;
    int uid;
    char access_token[2048];
    char refresh_token[2048];
} conn_info_t;

typedef void (*msg_handler_t)(int fd, const char *data, int len, conn_info_t *conn);

threadpool_t *threadpool_create(int thread_count, int queue_size);
int threadpool_add(threadpool_t *pool, void (*func)(void *), void *arg);
void threadpool_destroy(threadpool_t *pool);

int setnonblocking(int fd);
void addfd(int epollfd, int fd, int oneshot);
void reset_oneshot(int epollfd, int fd);
void removefd(int epollfd, int fd);

void register_msg_handler(int cmd, msg_handler_t handler);
void dispatch_message(int fd, const char *data, int len, conn_info_t *conn);

conn_info_t *get_conn(int fd);
void free_conn(int fd);

void server_run(const char *ip, int port);

#endif
