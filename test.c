#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

#define FIB_DEV "/dev/fibonacci"

void *child(void *data)
{
    char buf[1];
    int offset = 10; /* TODO: try test something bigger than the limit */

    int fd = open(FIB_DEV, O_RDWR);
    for (int i = 0; i <= offset; i++) {
        long long sz;
        lseek(fd, i, SEEK_SET);
        sz = read(fd, buf, 1);
        printf("Reading from child" FIB_DEV
               " at offset %d, returned the sequence "
               "%lld.\n",
               i, sz);
    }
    pthread_exit(NULL);
}

int main()
{
    char buf[1];
    int offset = 10; /* TODO: try test something bigger than the limit */

    pthread_t t;

    int fd = open(FIB_DEV, O_RDWR);
    if (fd < 0) {
        perror("Failed to open character device");
        exit(1);
    }


    pthread_create(&t, NULL, child, "Child");
    for (int i = 0; i <= offset; i++) {
        long long sz;
        lseek(fd, i, SEEK_SET);
        sz = read(fd, buf, 1);
        printf("Reading from parent " FIB_DEV
               " at offset %d, returned the sequence "
               "%lld.\n",
               i, sz);
    }

    pthread_join(t, NULL);
    close(fd);
    return 0;
}
