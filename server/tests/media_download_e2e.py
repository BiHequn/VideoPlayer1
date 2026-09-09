"""End-to-end test for the encrypted TCP upload and download protocol.

Run this against a disposable media-service database:

    python3 tests/media_download_e2e.py HOST PORT ACCESS_TOKEN
"""

from __future__ import print_function

import base64
import hashlib
import json
import socket
import struct
import sys


NETWORK_KEY = b"VideoPlayer2026!"
TEST_FILENAME = "download-protocol-e2e.bin"
TEST_CONTENT = (b"VideoPlayer1 download protocol test\n" * 2500) + b"EOF"


def encode_message(message):
    raw = json.dumps(
        message,
        separators=(",", ":"),
        ensure_ascii=False,
    ).encode("utf-8")
    encrypted = bytes(
        value ^ NETWORK_KEY[index % len(NETWORK_KEY)]
        for index, value in enumerate(bytearray(raw))
    )
    return base64.b64encode(encrypted)


def send_message(connection, message):
    payload = encode_message(message)
    connection.sendall(struct.pack("!I", len(payload)) + payload)


def read_exactly(connection, size):
    result = bytearray()
    while len(result) < size:
        chunk = connection.recv(size - len(result))
        if not chunk:
            raise RuntimeError("connection closed before the response completed")
        result.extend(chunk)
    return bytes(result)


def receive_message(connection):
    payload_size = struct.unpack("!I", read_exactly(connection, 4))[0]
    payload = base64.b64decode(read_exactly(connection, payload_size))
    decrypted = bytes(
        value ^ NETWORK_KEY[index % len(NETWORK_KEY)]
        for index, value in enumerate(bytearray(payload))
    )
    return json.loads(decrypted.decode("utf-8"))


def expect_result(response, expected, label):
    if response.get("result") != expected:
        raise AssertionError("{}: {}".format(label, response))


def authenticate(connection, access_token):
    send_message(connection, {"cmd": 6, "access_token": access_token})
    response = receive_message(connection)
    expect_result(response, "ok", "access-token authentication")


def upload_fixture(connection, access_token):
    file_md5 = hashlib.md5(TEST_CONTENT).hexdigest()
    send_message(
        connection,
        {
            "cmd": 10,
            "filename": TEST_FILENAME,
            "filesize": len(TEST_CONTENT),
            "file_md5": file_md5,
            "access_token": access_token,
        },
    )
    init_response = receive_message(connection)
    if init_response.get("result") not in ("ok", "resume"):
        raise AssertionError("upload initialization: {}".format(init_response))

    upload_id = init_response["upload_id"]
    offset = int(init_response.get("uploaded_size", 0))
    chunk_size = int(init_response.get("chunk_size", 4096))
    while offset < len(TEST_CONTENT):
        chunk = TEST_CONTENT[offset : offset + chunk_size]
        send_message(
            connection,
            {
                "cmd": 11,
                "upload_id": upload_id,
                "file_md5": file_md5,
                "offset": str(offset),
                "chunk_data": base64.b64encode(chunk).decode("ascii"),
                "access_token": access_token,
            },
        )
        chunk_response = receive_message(connection)
        expect_result(chunk_response, "ok", "upload chunk")
        offset = int(chunk_response["uploaded_size"])

    send_message(
        connection,
        {
            "cmd": 12,
            "upload_id": upload_id,
            "file_md5": file_md5,
            "filename": TEST_FILENAME,
            "filesize": str(len(TEST_CONTENT)),
            "access_token": access_token,
        },
    )
    expect_result(receive_message(connection), "ok", "upload finish")

    send_message(connection, {"cmd": 30, "access_token": access_token})
    listing = receive_message(connection)
    expect_result(listing, "ok", "video list")
    matches = [
        video
        for video in listing.get("videos", [])
        if video.get("filename") == TEST_FILENAME
    ]
    if len(matches) != 1:
        raise AssertionError("video fixture missing from list: {}".format(listing))
    video = matches[0]
    if video.get("file_md5") != file_md5 or int(video.get("filesize", -1)) != len(TEST_CONTENT):
        raise AssertionError("video metadata mismatch: {}".format(video))
    return int(video["id"]), file_md5


def download_fixture(connection, access_token, video_id, expected_md5):
    send_message(
        connection,
        {"cmd": 23, "video_id": video_id, "access_token": access_token},
    )
    init_response = receive_message(connection)
    expect_result(init_response, "ok", "download initialization")
    if init_response.get("file_md5") != expected_md5:
        raise AssertionError("download MD5 metadata mismatch: {}".format(init_response))

    chunk_size = int(init_response["chunk_size"])
    expected_size = int(init_response["filesize"])
    downloaded = bytearray()
    while len(downloaded) < expected_size:
        send_message(
            connection,
            {
                "cmd": 24,
                "video_id": video_id,
                "offset": str(len(downloaded)),
                "size": chunk_size,
                "access_token": access_token,
            },
        )
        response = receive_message(connection)
        expect_result(response, "ok", "download chunk")
        if int(response["offset"]) != len(downloaded):
            raise AssertionError("unexpected download offset: {}".format(response))
        downloaded.extend(base64.b64decode(response["data"]))
        if response.get("eof"):
            break

    if bytes(downloaded) != TEST_CONTENT:
        raise AssertionError("downloaded bytes do not match the uploaded fixture")
    if hashlib.md5(downloaded).hexdigest() != expected_md5:
        raise AssertionError("downloaded MD5 does not match")


def validate_rejections(host, port, access_token, video_id):
    with socket.create_connection((host, port), timeout=5) as connection:
        send_message(connection, {"cmd": 23, "video_id": video_id})
        expect_result(receive_message(connection), "fail", "unauthenticated download")

    with socket.create_connection((host, port), timeout=5) as connection:
        authenticate(connection, access_token)
        send_message(
            connection,
            {"cmd": 23, "video_id": 2147483647, "access_token": access_token},
        )
        expect_result(receive_message(connection), "fail", "missing video")
        send_message(
            connection,
            {
                "cmd": 24,
                "video_id": video_id,
                "offset": str(len(TEST_CONTENT)),
                "size": 1,
                "access_token": access_token,
            },
        )
        expect_result(receive_message(connection), "fail", "out-of-range download")


def main():
    if len(sys.argv) != 4:
        raise SystemExit("usage: media_download_e2e.py HOST PORT ACCESS_TOKEN")
    host = sys.argv[1]
    port = int(sys.argv[2])
    access_token = sys.argv[3]

    with socket.create_connection((host, port), timeout=5) as connection:
        authenticate(connection, access_token)
        video_id, file_md5 = upload_fixture(connection, access_token)
        download_fixture(connection, access_token, video_id, file_md5)

    validate_rejections(host, port, access_token, video_id)
    print("MEDIA_DOWNLOAD_E2E_OK video_id={}".format(video_id))


if __name__ == "__main__":
    main()
