#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "common.h"

#define NIL         "nil"
#define LINE_MAX    100

client_state *cs;
int debug;

typedef int (*func)(cmd_line *p_cmd_line);

int exec(cmd_line *p_cmd_line);
int quit(cmd_line *p_cmd_line);
int conn(cmd_line *p_cmd_line);
int bye(cmd_line *p_cmd_line);
int ls(cmd_line *p_cmd_line);
int get(cmd_line *p_cmd_line);
int unknown(cmd_line *p_cmd_line);

typedef struct {
    char *name;
    func callback;
} str_to_func;

int main(int argc, char **argv)
{
    char line[LINE_MAX];
    cmd_line* p_cmd_line;

   set_debug_mode(argc, argv, &debug);

    cs = (client_state *)malloc(sizeof(client_state));
    init_client_state(cs, NIL);
    
    for (;;) {
        printf("server:%s>", cs->server_addr);
        if (fgets(line, LINE_MAX, stdin)) {
            if ((p_cmd_line = parse_cmd_lines(line))) {
                exec(p_cmd_line);
                free_cmd_lines(p_cmd_line); 
            }
        } else if (errno) {
            perror("fgets failed");
            free_client_state(cs);
            exit(-1);
        } else {
            printf("\n");
            free_client_state(cs);
            exit(0);
        }
    }
}

int check_state(c_state state, char *state_name, char *cmd)
{
    if (cs->conn_state != state) {
        fprintf(stderr, "%s may only run in %s state\n", cmd, state_name);
        return -2;
    }

    return 0;
}

void print_nok(char *error_name)
{
    fprintf(stderr, "server error: %s\n", error_name);
}

int exec(cmd_line *p_cmd_line)
{
    str_to_func menu[] = { { "quit" ,quit },
                           { "conn", conn },
                           { "bye", bye },
                           { "ls", ls },
                           { "get", get },
                           { NULL, NULL } };
    unsigned int i = 0;
    const char *cmd = p_cmd_line->arguments[0];

    do {
        if (!strcmp(cmd, menu[i].name)) {
            return menu[i].callback(p_cmd_line);
        }
    } while (menu[++i].name);

    return unknown(p_cmd_line);
}

int quit(cmd_line *p_cmd_line)
{
    free_cmd_lines(p_cmd_line);

    if (CONNECTED == cs->conn_state) {
        if (-1 == close(cs->sock_fd)) {
            perror("close failed");
        }
    }
    free_client_state(cs);
    exit(0);
}

int conn(cmd_line *p_cmd_line)
{
    int status, numbytes, sockfd;
    struct addrinfo hints, *res, *p;
    char response[LS_RESP_SIZE];
    const char *srv_addr = p_cmd_line->arguments[1];
    struct timeval timeout;      
    
    if (check_state(IDLE, "IDLE", "connect") < 0) {
        return -2;
    }

    if (p_cmd_line->arg_count != 2) {
        fprintf(stderr, "usage: conn <srv_addr>\n");
        return -1;
    }

    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(srv_addr, PORT, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return -1;
    }

    for (p = res; p != NULL; p = p->ai_next) {

        if (-1 == (sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))) {
            perror("socket failed");
            continue;
        }

        if (-1 == setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof timeout)) {
            perror("setsockopt failed");
            continue;
        }

        if (-1 == connect(sockfd, p->ai_addr, p->ai_addrlen)) {
            perror("connect failed");
            continue;
        }

        break;
    }

    if (NULL == p) {
        fprintf(stderr, "failed to connect\n");
        goto err;
    }

    if (-1 == send(sockfd, "hello", 5, 0)) {
        perror("send failed");
        goto err;
    }

    cs->conn_state = CONNECTING;

    if (-1 == (numbytes = recv(sockfd, response, LS_RESP_SIZE - 1, 0))) {
        perror("recv failed");
        cs->conn_state = IDLE;
        goto err;
    }

    response[numbytes] = '\0';
    print_message(response, srv_addr, debug);

    if (!strncmp(response, "hello ", 6)) {
        cs->client_id = (char *)realloc(cs->client_id, strlen(response) - 5);
        strcpy(cs->client_id, response + 6);
        cs->conn_state = CONNECTED;
        cs->sock_fd = sockfd;
        cs->server_addr = (char *)realloc(cs->server_addr, strlen(srv_addr) + 1);
        strcpy(cs->server_addr, srv_addr);
    } else {
        print_nok(response + 4);
        cs->conn_state = IDLE;
    }

    freeaddrinfo(res);
    return 0;

    err:
    freeaddrinfo(res);
    return -1;
}

int disconnect()
{
    if (-1 == close(cs->sock_fd)) {
        perror("close failed");
        return -1;
    }

    free(cs->client_id);
    cs->client_id = NULL;
    cs->conn_state = IDLE;
    cs->sock_fd = -1;
    cs->server_addr = (char *)realloc(cs->server_addr, strlen(NIL) + 1);
    strcpy(cs->server_addr, NIL);
    
    return 0;
}

int try_send(char *msg, cmd_line *p_cmd_line)
{
    if (check_state(CONNECTED, "CONNECTED", msg) < 0) {
        return -2;
    }
    
    if (p_cmd_line->arg_count != 1) {
        fprintf(stderr, "usage: %s\n", msg);
        return -1;
    }

    if (-1 == send(cs->sock_fd, msg, strlen(msg), 0)) {
        perror("send failed");
        return -1;
    }

    return 0;
}

int bye(cmd_line *p_cmd_line)
{
    int send_res;

    if ((send_res = try_send("bye", p_cmd_line)) < 0) {
        return send_res;
    }

    return disconnect();
}

int ls(cmd_line *p_cmd_line)
{
    char oknok[4];
    char response[LS_RESP_SIZE], full_response[LS_RESP_SIZE];
    int send_res, oknok_numbytes, resp_numbytes;

    if ((send_res = try_send("ls", p_cmd_line)) < 0) {
        return send_res;
    }

    if (-1 == (oknok_numbytes = recv(cs->sock_fd, oknok, 3, 0)) ||
        -1 == (resp_numbytes = recv(cs->sock_fd, response, LS_RESP_SIZE - 1, 0))) {
        perror("recv failed");
        return -1;
    }

    oknok[oknok_numbytes] = '\0';
    response[resp_numbytes] = '\0';

    strcpy(full_response, oknok);
    strcat(full_response, response);
    print_message(full_response, cs->server_addr, debug);

    if (!strcmp(oknok, "nok")) {
        print_nok(response);
        disconnect();
        return -1;
    }

    printf("%s", response);
    return 0;
}

int get(cmd_line *p_cmd_line)
{
    char msg[MAX_ARGUMENTS], tmp_filename[MAX_ARGUMENTS];
    char oknok1[LS_RESP_SIZE], oknok2[LS_RESP_SIZE];
    char buf[FILE_BUFF_SIZE];
    const char *filename;
    FILE *file;
    int oknok1_numbytes, oknok2_numbytes, read, total_read, file_size, fd;

    if (check_state(CONNECTED, "CONNECTED", "get") < 0) {
        return -2;
    }

    if (p_cmd_line->arg_count != 2) {
        fprintf(stderr, "usage: get <file_name>\n");
        return -1;
    }

    filename = p_cmd_line->arguments[1];
    sprintf(msg, "get %s", filename);

    if (-1 == send(cs->sock_fd, msg, strlen(msg), 0)) {
        perror("send failed");
        return -1;
    }

    if (-1 == (oknok1_numbytes = recv(cs->sock_fd, oknok1, LS_RESP_SIZE, 0))) {
        perror("recv failed");
        return -1;
    }

    oknok1[oknok1_numbytes] = '\0';
    print_message(oknok1, cs->server_addr, debug);
    
    if (!strncmp(oknok1, "nok", 3)) {
        print_nok(oknok1 + 4);
        disconnect();
        return -1;
    }

    cs->conn_state = DOWNLOADING;

    total_read = 0;
    file_size = atoi(oknok1 + 3);

    sprintf(tmp_filename, "%s.tmp", filename);
    file = fopen(tmp_filename, "w+");
    fd = fileno(file);

    do {
        if (-1 == (read = recv(cs->sock_fd, buf, FILE_BUFF_SIZE, 0))) {
            perror("recv failed");
            return -1;
        }

        write(fd, buf, read);
        total_read += read;
    } while (total_read < file_size && read > 0);

    fclose(file);

    if (total_read < file_size) {
        goto nofile;
    }

    if (-1 == send(cs->sock_fd, "done", 4, 0)) {
        perror("send failed");
        return -1;
    }

    if (-1 == (oknok2_numbytes = recv(cs->sock_fd, oknok2, LS_RESP_SIZE, 0))) {
        perror("recv failed");
        return -1;
    }

    oknok2[oknok2_numbytes] = '\0';
    print_message(oknok2, cs->server_addr, debug);

    if (!strncmp(oknok2, "nok", 3)) {
        print_nok(oknok2 + 4);
        goto nofile;
    }

    rename(tmp_filename, filename);
    cs->conn_state = CONNECTED;
    return 0;

    nofile:
    remove(tmp_filename);
    printf("error while downloading file %s\n", filename);
    disconnect();
    return -1;
}

int unknown(cmd_line *p_cmd_line)
{
    int numbytes;
    char response[LS_RESP_SIZE];
    const char *cmd = p_cmd_line->arguments[0];

    if (cs->conn_state != CONNECTED) {
        printf("invalid command: %s\n", cmd);
    } else if (-1 == send(cs->sock_fd, cmd, strlen(cmd), 0)) {
        perror("send failed");
    } else if (-1 == (numbytes = recv(cs->sock_fd, response, LS_RESP_SIZE - 1, 0))) {
        perror("recv failed");
    } else {
        response[numbytes] = '\0';
        print_message(response, cs->server_addr, debug);
        print_nok(response + 4);
        disconnect();
    }

    return -1;
}
