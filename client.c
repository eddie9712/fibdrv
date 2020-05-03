#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#define FIB_DEV "/dev/fibonacci"

int main()
{
    struct timespec start, finish;
    long long k = 1;
    char buf[50];
    char write_buf[] = "testing writing";
    int offset = 110; /* TODO: try test something bigger than the limit */
    FILE *fout;
    fout = fopen("correct.txt", "w+t");
    if (fout == NULL) {
        printf("error");
    }
    int fd = open(FIB_DEV, O_RDWR);
    if (fd < 0) {
        perror("Failed to open character device");
        exit(1);
    }
    for (int i = 0; i <= 100; i++) {
        // sz = write(fd, write_buf, strlen(write_buf));
        printf("Writing to " FIB_DEV ", returned the sequence %lld\n", k);
    }
    for (int i = 0; i <= offset; i++) {
        long ker;
        lseek(fd, i, SEEK_SET);
        clock_gettime(CLOCK_REALTIME, &start);
        read(fd, buf, 50);
        clock_gettime(CLOCK_REALTIME, &finish);
        printf("Reading from " FIB_DEV
               " at offset %d, returned the sequence "
               "%s.\n",
               i, buf);
        ker = write(fd, write_buf, strlen(write_buf));
        fprintf(fout, "%d %ld %ld %ld\n", i, finish.tv_nsec - start.tv_nsec,
                ker, finish.tv_nsec - start.tv_nsec - ker);
    }
    for (int i = offset; i >= 0; i--) {
        lseek(fd, i, SEEK_SET);
        read(fd, buf, 50);
        printf("Reading from " FIB_DEV
               " at offset %d, returned the sequence "
               "%s.\n",
               i, buf);
    }

    close(fd);
    fclose(fout);
    return 0;
}
