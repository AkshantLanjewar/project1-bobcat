#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BOB_BUF_SIZE (64 * 1024)

static int copy_stream(int infd, const char *label, char *buffer, size_t bufsize) {
    while(true) {
        ssize_t num_read = read(infd, buffer, bufsize);
        if (num_read == 0) {
            return 0;  
        }

        if (num_read < 0) {
            if (errno == EINTR) {
                continue;
            }

            warn("%s", label);
            return -1;
        }

        size_t off = 0;
        while (off < (size_t)num_read) {
            ssize_t nw = write(STDOUT_FILENO, buffer + off, (size_t)num_read - off);

            if (nw < 0) {
                if (errno == EINTR) {
                    continue;
                }
                warn("write");
                return -1;
            }

            off += (size_t)nw;
        }
    }
}

int main(int argc, char *argv[]) {
    char *buffer = malloc(BOB_BUF_SIZE);
    if (buffer == NULL) {
        warn("malloc");
        return EXIT_FAILURE;
    }

    bool had_error = false;

    if (argc <= 1) {
        if (copy_stream(STDIN_FILENO, "-", buffer, BOB_BUF_SIZE) != 0) {
            had_error = true;
        }
    } else {
        for (int i = 1; i < argc; i++) {
            const char *operand = argv[i];
            if (strcmp(operand, "-") == 0) {
                if (copy_stream(STDIN_FILENO, "-", buffer, BOB_BUF_SIZE) != 0) {
                    had_error = true;
                }

                continue;
            }

            int fd = open(operand, O_RDONLY);
            if (fd < 0) {
                warn("%s", operand);
                had_error = true;
                continue;
            }

            if (copy_stream(fd, operand, buffer, BOB_BUF_SIZE) != 0) {
                had_error = true;
            }

            if (close(fd) < 0) {
                warn("%s", operand);
                had_error = true;
            }
        }
    }

    free(buffer);
    return had_error ? EXIT_FAILURE : EXIT_SUCCESS;
}
