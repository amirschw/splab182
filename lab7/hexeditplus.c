#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define MAX_BUF     100

char buf[MAX_BUF];
unsigned int bytes = 1;

void set_file_name();
void set_unit_size();
void file_display();
void file_modify();
void copy_from_file();
void quit();
void prompt_user(char *str);
void display(unsigned char *units, int length, int base);

void (*actions[])() = {set_file_name, set_unit_size, file_display, file_modify, copy_from_file, quit};

int main(int argc, char **argv)
{
    unsigned int a, i;
    int scanned;
    const char *menu[] = 
    {"Choose action:",
     "1-Set File Name",
     "2-Set Unit Size",
     "3-File Display",
     "4-File Modify",
     "5-Copy From File",
     "6-Quit",
     NULL};

    for (;;) {
        
        i = 0;
        while (menu[i]) {
            printf("%s\n", menu[i++]);
        }
        
        scanned = scanf("%d", &a);
        while (getchar() != '\n');

        if (scanned != 1 || (a != 1 && a != 2 && a != 3 && a != 4 && a != 5 && a != 6)) {
            printf("Invalid action...\n");
        } else {
            actions[a - 1]();
        }
    }

    return 0;
}

void set_file_name()
{
    prompt_user("filename");
    fgets(buf, MAX_BUF, stdin);
    buf[strlen(buf) - 1] = '\0';
}

void set_unit_size()
{
    int s, scanned;

    prompt_user("size");
    scanned = scanf("%d", &s);
    while (getchar() != '\n');

    if (scanned != 1 || (s != 1 && s != 2 && s != 4)) {
        printf("Invalid input... Size should be 1, 2, or 4.\n");
    } else {
        bytes = s;
    }
}

void file_display()
{
    int fd, location, length, scanned;
    char str[MAX_BUF];
    unsigned char *units;

    if (NULL == buf) {
        printf("No filename given...\n");
    } else if (-1 == (fd = open(buf, O_RDONLY))) {
        perror("open failed");
    } else {
        prompt_user("<location> <length>");

        fgets(str, MAX_BUF, stdin);
        scanned = sscanf(str, "%x %d", &location, &length);

        if (scanned != 2) {
            printf("Invalid input...\n");
        } else {
            units = (unsigned char*)calloc(length, bytes);

            lseek(fd, location, SEEK_SET);
            read(fd, units, length * bytes);

            close(fd);

            display(units, length * bytes, 16);
            display(units, length * bytes, 10);

            free(units);
        }
    }
}

void file_modify()
{
    int fd, location, val, offset, scanned;
    char str[MAX_BUF];

    if (NULL == buf) {
        printf("No filename given...\n");
    } else if (-1 == (fd = open(buf, O_WRONLY))) {
        perror("open failed");
    } else {
        prompt_user("<location> <val>");

        fgets(str, MAX_BUF, stdin);
        scanned = sscanf(str, "%x %x", &location, &val);

        if (scanned != 2) {
            printf("Invalid input...\n");
        } else {
            if (-1 == (offset = lseek(fd, location, SEEK_SET))) {
                perror("lseek failed");
            } else if (offset + bytes > lseek(fd, 0L, SEEK_END)) {
                printf("Invalid location...\n");
            } else {
                lseek(fd, location, SEEK_SET);
                write(fd, (char*)(&val), bytes);
            }
        }

        close(fd);
    }
}

void copy_from_file()
{
    int src_fd, dst_fd, src_offset, dst_offset, length, scanned;
    char str[MAX_BUF], src_file[MAX_BUF];
    unsigned char *to_copy;

    if (NULL == buf) {
        printf("no dst_file given...\n");
    } else {
        prompt_user("<src_file> <src_offset> <dst_offset> <length>");

        fgets(str, MAX_BUF, stdin);
        scanned = sscanf(str, "%s %x %x %d", src_file, &src_offset, &dst_offset, &length);

        if (scanned != 4) {
            printf("Invalid input...\n");
        } else if (-1 == (src_fd = open(src_file, O_RDONLY))) {
            perror("open failed");
        } else if (-1 == (dst_fd = open(buf, O_RDWR))) {
            close(src_fd);
            perror("open failed");
        } else {
            to_copy = (unsigned char*)calloc(length, sizeof(unsigned char));

            lseek(src_fd, src_offset, SEEK_SET);
            read(src_fd, to_copy, length);
            close(src_fd);

            lseek(dst_fd, dst_offset, SEEK_SET);
            write(dst_fd, to_copy, length);
            close(dst_fd);

            printf("Loaded %d bytes into %p FROM %s TO %s\n", length, to_copy, src_file, buf);

            free(to_copy);
        }
    }
}

void quit()
{
    exit(0);
}

void prompt_user(char *str)
{
    printf("Please enter %s\n", str);
}

void display(unsigned char *units, int length, int base)
{
    unsigned int i, j, value;

    printf("%s Representation:\n", 16 == base ? "Hexadecimal" : "Decimal");
    for (i = 0; i < length; i += bytes) {
        
        /* convert to big endian */
        for (j = value = 0; j < bytes; ++j) {
            value |= (unsigned int)units[i + j] << 8 * j;
        }

        if (16 == base) {
            printf("%0*x ", bytes * 2, value);
        } else {
            printf("%d ", value);
        }
    }
    printf("\n");
}
