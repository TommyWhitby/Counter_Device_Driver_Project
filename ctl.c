// This is a user sample calling into the device.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

#define uint8_t unsigned char
#define DEV_IOC_MAGIC 0x5b /* 0x91 ? */
#define DEV_IOC_RST _IO(DEV_IOC_MAGIC, 0)
#define DEV_IOC_GET _IOR(DEV_IOC_MAGIC, 1, uint8_t*)
#define DEV_IOC_STP _IOW(DEV_IOC_MAGIC, 2, uint8_t*)
#define DEV_IOC_ERR _IO(DEV_IOC_MAGIC, 3)

char *prog;

int usage(int fd, int8_t oparg) {
    fprintf(stderr, "%s: <dev> { rst | get | stp <val> | err }\n", prog);
    return 1;
}

int rst(int fd, int8_t oparg) {
    return ioctl(fd, DEV_IOC_RST, NULL);
}

int get(int fd, int8_t oparg) {
    int result = ioctl(fd, DEV_IOC_GET, &oparg);
    printf("%d\n", oparg);
    return result;
}

int stp(int fd, int8_t oparg) {
    return ioctl(fd, DEV_IOC_STP, &oparg);
}

int err(int fd, int8_t oparg) {
    return ioctl(fd, DEV_IOC_ERR, NULL);
}

int main(int argc, char **argv) {
    int (*op)(int, int8_t);
    int8_t oparg = 0x00;
    int conv;
    char *inv;
    int fd = -1;
    int result;

    errno = 0;
    prog = argv[0];
    if(argc == 3 || argc == 4) {
        if(strcmp(argv[2], "rst") == 0) {
            op = &rst;
        } else if(strcmp(argv[2], "get") == 0) {
            op = &get;
        } else if(strcmp(argv[2], "stp") == 0) {
            if(argc == 4) {
                conv = strtol(argv[3], &inv, 0);
                if(errno != 0 || *inv != '\0' || conv > 127 || conv < -128) {
                    op = &usage;
                } else {
                    op = &stp;
                    oparg = (int8_t)conv; // transmute signed char to unsigned
                }
            } else {
                op = &usage;
            }
        } else if(strcmp(argv[2], "err") == 0) {
            op = &err;
        } else {
            op = &usage;
        }
        fd = open(argv[1], O_RDONLY);
        if(fd<0) {
            fprintf(stderr, "Error: couldn't open device file...\n");
            return 1;
        }
    } else {
        op = &usage;
    }
    result = ((*op)(fd, oparg));
    close(fd);
    return result;
}

/* vi: set ts=4 sw=4 et ic nows: */
