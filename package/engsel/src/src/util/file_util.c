#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "../include/util/file_util.h"

char *file_read_all(const char *path, size_t *out_size) {
    if (!path) return NULL;
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return NULL; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return NULL; }

    char *buf = malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return NULL; }

    size_t got = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    if (got != (size_t)sz) { free(buf); return NULL; }
    buf[sz] = '\0';
    if (out_size) *out_size = (size_t)sz;
    return buf;
}

int file_write_atomic(const char *path, const char *content) {
    if (!path) return -1;
    const char *data = content ? content : "";
    size_t len = strlen(data);

    size_t plen = strlen(path);
    char *tmp = malloc(plen + 16);
    if (!tmp) return -1;
    snprintf(tmp, plen + 16, "%s.tmp.%d", path, (int)getpid());

    int fd = open(tmp, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) { free(tmp); return -1; }

    size_t off = 0;
    while (off < len) {
        ssize_t w = write(fd, data + off, len - off);
        if (w < 0) {
            if (errno == EINTR) continue;
            close(fd); unlink(tmp); free(tmp); return -1;
        }
        off += (size_t)w;
    }

    /* Opsional: fsync supaya data benar-benar di flash sebelum rename. */
#if defined(__linux__)
    fdatasync(fd);
#endif
    close(fd);

    if (rename(tmp, path) != 0) {
        unlink(tmp); free(tmp); return -1;
    }
    free(tmp);
    return 0;
}
