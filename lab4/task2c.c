#include "lab4_util.h"

#define O_RDONLY	    0
#define SYS_READ        0
#define SYS_WRITE       1
#define SYS_OPEN        2
#define SYS_CLOSE       3
#define SYS_EXIT        60
#define SYS_GETDENTS    78
#define STDIN           0
#define STDOUT          1
#define STDERR          2
#define DT_DIR          4
#define SEARCH_ALL      0
#define SEARCH_FILE     1
#define SEARCH_COMMAND  2
#define BUF_SIZE        2048

extern int system_call();
int search (char *directory, char *filename, char *command, int flag);

struct linux_dirent {
    unsigned long  d_ino;     /* Inode number */
    unsigned long  d_off;     /* Offset to next linux_dirent */
    unsigned short d_reclen;  /* Length of this linux_dirent */
    char           d_name[];  /* Filename (null-terminated) */
                              /* length is actually (d_reclen - 2 -
                              offsetof(struct linux_dirent, d_name) */
};

int main (int argc , char* argv[], char* envp[])
{
    if (argc == 1) {
        search(".", ".", ".", SEARCH_ALL);
    } else if (argc == 3 && !simple_strcmp(argv[1],"-n")) {
        search(".", argv[2], ".", SEARCH_FILE);
    } else if (argc == 4 && !simple_strcmp(argv[1],"-e")) {
        if (!search(".", argv[2], argv[3], SEARCH_COMMAND)) {
            system_call(SYS_WRITE, STDOUT, "The file '", sizeof("The file '"));
            system_call(SYS_WRITE, STDOUT, argv[2], simple_strlen(argv[2]));
            system_call(SYS_WRITE, STDOUT, "' does not exist.\n", sizeof("' does not exist.\n"));
        }
    } else {
        system_call(SYS_WRITE, STDERR,
                    "Invalid arguments.\n",
                    sizeof("Invalid arguments.\n"));
        system_call(SYS_EXIT, 0x55);
    }
    return 0;
}

int search (char *directory, char *filename, char *command, int flag)
{
    int fd, nread, file_exists;
    char buf[BUF_SIZE];
    struct linux_dirent *d;
    int buf_pos = 0;
    char d_type;
    char *fn;

    /* open directory */
    if ((fd = system_call(SYS_OPEN, directory, O_RDONLY, 0444)) < 0) {
        system_call(SYS_WRITE, STDERR, "Directory does not exist.\n", sizeof("Directory does not exist.\n"));
        system_call(SYS_EXIT, 0x55);
    }

    /* indicate if the file on which the command is to be executed exists */
    file_exists = flag == SEARCH_COMMAND ? 0 : 1;

    while (1) {
        nread = system_call(SYS_GETDENTS, fd, buf, BUF_SIZE);

        if (nread < 0) {
            system_call(
                SYS_WRITE, STDERR,
                "Error reading files from the directory.\n",
                sizeof("Error reading files from the directory.\n"));
            system_call(SYS_EXIT, 0x55);
        }
        
        /* end of directory */
        if (nread == 0) {
            break;
        }

        /* iterate over all the files in the current directory */
        while (buf_pos < nread) {
            d = (struct linux_dirent *) (buf + buf_pos);
            fn = (char *) d->d_name;
            if (simple_strncmp(fn, ".", 1)) {
                if (d->d_ino != 0) {

                    if ((flag == SEARCH_ALL) || 
                        (flag == SEARCH_FILE && !simple_strcmp(filename, fn))) {

                        /* print file path */
                        system_call(SYS_WRITE, STDOUT, directory, simple_strlen(directory));
                        system_call(SYS_WRITE, STDOUT, "/", sizeof("/"));
                        system_call(SYS_WRITE, STDOUT, fn, simple_strlen(fn));
                        system_call(SYS_WRITE, STDOUT, "\n", sizeof("\n"));
                        
                    } else if (flag == SEARCH_COMMAND && !simple_strcmp(filename, fn)) {

                        /* command is to be executed on current file */
                        char full_command[simple_strlen(command)+simple_strlen(directory)+simple_strlen(fn)+2];
                        int com_pos = 0;

                        while (*command) {
                            full_command[com_pos++] = *command++;
                        }
                        command -= com_pos;
                        full_command[com_pos++] = ' ';

                        while (*directory) {
                            full_command[com_pos++] = *directory++;
                        }
                        directory -= com_pos + simple_strlen(command)+1;
                        full_command[com_pos++] = '/';

                        while (*fn) {
                            full_command[com_pos++] = *fn++;
                        }
                        full_command[com_pos] = '\0';

                        /* execute command */
                        simple_system(full_command);
                        file_exists = 1;
                    }
                }
                d_type = *(buf + buf_pos + d->d_reclen - 1);

                /* current file is a sub-directory */
                if (d_type == DT_DIR) {

                    char sub_dir[simple_strlen(directory)+simple_strlen(fn)+1];
                    int sub_pos = 0;

                    while (*directory) {
                        sub_dir[sub_pos++] = *directory++;
                    }
                    directory -= sub_pos;
                    sub_dir[sub_pos++] = '/';

                    while (*fn) {
                        sub_dir[sub_pos++] = *fn++;
                    }
                    sub_dir[sub_pos] = '\0';

                    /* search recursively in sub-directory
                     * return 1 if file exists in sub-directory */
                    file_exists = search(sub_dir, filename, command, flag) || file_exists;
                }
            }
            buf_pos += d->d_reclen;
        }
    }

    system_call(SYS_CLOSE, fd);
    return file_exists;
}