#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include "common.h"

#define HOST_NAME_SIZE  128
#define BACKLOG         5
#define MAX_ID_SIZE     15
#define SUCCESS         1
#define FAIL            -1
#define DISCONNECT      -2

unsigned int client_index = 0;

typedef int (*func)(client_state *cs, int client_fd);

int hello(client_state *cs, int client_fd);
int bye(client_state *cs, int client_fd);
int ls(client_state *cs, int client_fd);
int get(client_state *cs, int client_fd, char *filename);
int send_nok(char *msg, int client_fd);
void fail(client_state *cs);

typedef struct {
    char *name;
    func callback;
} str_to_func;

int main(int argc, char **argv)
{
    int debug, status, sockfd, client_fd, numbytes, cb_res, i;
    client_state *cs;
    char srv_addr[HOST_NAME_SIZE];
    char message[LS_RESP_SIZE];
    struct addrinfo hints, *res, *p;
    struct sockaddr_storage client_addr;
    socklen_t addr_size;
    int yes = 1;
    str_to_func menu[] = { { "hello" ,hello },
                           { "bye", bye },
                           { "ls", ls },
                           { NULL, NULL } };

    set_debug_mode(argc, argv, &debug);
    gethostname(srv_addr, sizeof srv_addr);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(NULL, PORT, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return -1;
    }

    for (p = res; p != NULL; p = p->ai_next) {

        if (-1 == (sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))) {
            perror("socket failed");
            continue;
        }

        if (-1 == setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes)) {
            perror("setsockopt failed");
            freeaddrinfo(res);
            return -1;
        }

        if (-1 == bind(sockfd, p->ai_addr, p->ai_addrlen)) {
            close(sockfd);
            perror("bind failed");
            continue;
        }

        break;
    }

    freeaddrinfo(res);

    if (NULL == p)  {
        fprintf(stderr, "server: failed to bind\n");
        return -1;
    }

    if (-1 == listen(sockfd, BACKLOG)) {
        perror("listen");
        return -1;
    }

    for (;;) {
        addr_size = sizeof client_addr;
        if (-1 == (client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &addr_size))) {
            perror("accept failed");
            continue;
        }

        cs = (client_state *)malloc(sizeof(client_state));
        init_client_state(cs, srv_addr);

        for (cb_res = i = 0; cb_res != DISCONNECT; i = 0) {
            if (-1 == (numbytes = recv(client_fd, message, LS_RESP_SIZE - 1, 0))) {
                perror("recv failed");
                fail(cs);
            }

            message[numbytes] = '\0';
            print_message(message, srv_addr, debug);
            cb_res = 0;

            do {
                if (!strcmp(message, menu[i].name)) {
                    cb_res = menu[i].callback(cs, client_fd);
                }
            } while (0 == cb_res && menu[++i].name);

            if (!strncmp(message, "get", 3)) {
                cb_res = get(cs, client_fd, message + 4);
            }

            if (FAIL == cb_res) {
                fail(cs);
            } else if (SUCCESS != cb_res) {
                if (DISCONNECT != cb_res) {
                    send_nok(message, client_fd);
                    fprintf(stderr, "%s|ERROR: unknown message %s\n", cs->client_id, message);
                    cb_res = DISCONNECT;
                }
                printf("client %s disconnected\n", cs->client_id);
                close(client_fd);
                free_client_state(cs);
            }
        }
        client_index++;
    }
}

void set_client_id(client_state *cs)
{
    char tmp_buf[MAX_ID_SIZE];
    sprintf(tmp_buf, "client_%u", client_index);
    cs->client_id = (char *)realloc(cs->client_id, strlen(tmp_buf) + 1);
    strcpy(cs->client_id, tmp_buf);
}

int send_nok(char *msg, int client_fd)
{
    char nok_msg[LS_RESP_SIZE];
    sprintf(nok_msg, "nok %s", msg);
    if (-1 == send(client_fd, nok_msg, strlen(nok_msg), 0)) {
        perror("send failed");
        return FAIL;
    }

    return DISCONNECT;
}

int check_state(c_state state, client_state *cs, int client_fd)
{
    if (cs->conn_state != state) {
        return send_nok("state", client_fd);
    }

    return 0;
}

int hello(client_state *cs, int client_fd)
{
    char response[LS_RESP_SIZE];
    int check_state_res;

    if ((check_state_res = check_state(IDLE, cs, client_fd)) < 0) {
        return check_state_res;
    }

    set_client_id(cs);
    cs->conn_state = CONNECTED;
    cs->sock_fd = client_fd;

    sprintf(response, "hello %s", cs->client_id);

    if (-1 == send(client_fd, response, strlen(response), 0)) {
        perror("send failed");
        return FAIL;
    }

    printf("client %s connected\n", cs->client_id);

    return SUCCESS;
}

int bye(client_state *cs, int client_fd)
{
    int check_state_res;

    if ((check_state_res = check_state(CONNECTED, cs, client_fd)) < 0) {
        return check_state_res;
    }

    if (-1 == send(client_fd, "bye", 3, 0)) {
        perror("send failed");
        return FAIL;
    }

    return DISCONNECT;
}

int ls(client_state *cs, int client_fd)
{
    int check_state_res;
    char *list_of_files;
    
    if ((check_state_res = check_state(CONNECTED, cs, client_fd)) < 0) {
        return check_state_res;
    }

    if (NULL == (list_of_files = list_dir())) {
        send_nok("filesystem", client_fd);
        return DISCONNECT;
    }

    if (-1 == send(client_fd, "ok ", 3, 0) ||
        -1 == send(client_fd, list_of_files, strlen(list_of_files), 0)) {
        perror("send failed");
        free(list_of_files);
        return FAIL;
    }

    free(list_of_files);
    return SUCCESS;
}

int get(client_state *cs, int client_fd, char *filename)
{
    int check_state_res, fd, done_numbytes;
    long size;
    char ok[LS_RESP_SIZE], done[LS_RESP_SIZE];
    
    if ((check_state_res = check_state(CONNECTED, cs, client_fd)) < 0) {
        return check_state_res;
    }

    if (-1 == (size = file_size(filename))) {
        send_nok("file", client_fd);
        return DISCONNECT;
    }

    sprintf(ok, "ok %ld", size);

    if (-1 == send(client_fd, ok, strlen(ok), 0)) {
        perror("send failed");
        return FAIL;
    }

    cs->conn_state = DOWNLOADING;

	if (-1 == (fd = open(filename, O_RDONLY))) {
		perror("open failed");
        send_nok("unable to open file", client_fd);
        return DISCONNECT;
    }

    if (-1 == (sendfile(cs->sock_fd, fd, NULL, size))) {
        perror("sendfile failed");
        send_nok("unable to send file", client_fd);
        return DISCONNECT;
    }
    close(fd);

    if (-1 == (done_numbytes = recv(cs->sock_fd, done, LS_RESP_SIZE, 0))) {
        perror("recv failed");
        return FAIL;
    }

    done[done_numbytes] = '\0';

    if (strcmp(done, "done")) {
        send_nok("done", client_fd);
        return DISCONNECT;
    }

    cs->conn_state = CONNECTED;

    if (-1 == send(client_fd, "ok", 2, 0)) {
        perror("send failed");
        return FAIL;
    }

    printf("sent file %s\n", filename);
    return SUCCESS;
}

void fail(client_state *cs)
{
    free_client_state(cs);
    exit(-1);
}
