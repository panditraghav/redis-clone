#include <arpa/inet.h>
#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <errno.h>
#include <netinet/ip.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

const size_t k_max_msg = 4096;

static void die(const char *msg) {
  int err = errno;
  fprintf(stderr, "[%d] %s\n", err, msg);
  abort();
}

static void msg(const char *msg) {
  int err = errno;
  fprintf(stderr, "[%d] %s\n", err, msg);
}

static int32_t write_all(int fd, const char *buf, size_t n) {
  while (n > 0) {
    ssize_t rv = write(fd, buf, n);
    if (rv <= 0) {
      msg("EOF error");
      return -1;
    }
    assert((size_t)rv <= n);
    n -= (size_t)rv;
    buf += rv;
  }
  return 0;
}

static int32_t read_full(int connfd, char *buf, size_t n) {
  while (n > 0) {
    ssize_t rv = read(connfd, buf, n);
    if (rv <= 0) {
      if (rv == -1 && errno == EINTR) {
        /*
         * read can also be interrupted by a signal because it must wait if the
         * buffer is empty. In this case, 0 bytes are read, but the return value
         * is -1 and errno is EINTR. This is not an error.
         */
      } else {
        msg("EOF error");
        return -1;
      }
    }
    assert((size_t)rv <= n);
    n -= (size_t)rv;
    buf += rv;
  }
  return 0;
}

static int32_t query(int fd, char *text) {
  uint32_t len = (uint32_t)strlen(text);

  if (len > k_max_msg) {
    return -1;
  }
  char wbuf[4 + k_max_msg];
  memcpy(wbuf, &len, 4);
  memcpy(&wbuf[4], text, len);

  int32_t error = write_all(fd, wbuf, len + 4);
  if (error) {
    return error;
  }

  // Reading incoming message
  char rbuf[4 + k_max_msg + 1];

  errno = 0;
  int32_t err = read_full(fd, rbuf, 4);
  if (err) {
    if (errno == 0) {
      msg("EOF");
    } else {
      msg("read() error");
    }
  }
  memcpy(&len, rbuf, 4);
  printf("Length of message by server: %u\n", len);

  if (len > k_max_msg) {
    msg("msg too long");
    return -1;
  }

  err = read_full(fd, &rbuf[4], len);
  if (err) {
    msg("read() error");
  }
  rbuf[4 + len] = '\0';
  printf("Server says: %s\n", &rbuf[4]);

  return 0;
}

int main() {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    die("socket()");
  }

  struct sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_port = ntohs(1234);
  addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK); // 127.0.0.1
  int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
  if (rv) {
    die("connect");
  }

  int32_t err = query(fd, (char *)"Hello 1");
  if (err) {
    goto L_DONE;
  }

  err = query(fd, (char *)"Hello 2");
  if (err) {
    goto L_DONE;
  }

  err = query(fd, (char *)"Hello 3");
  if (err) {
    goto L_DONE;
  }

L_DONE:
  close(fd);
  return 0;
}
