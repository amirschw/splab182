#define MAX_ARGUMENTS	256
#define DIR_MAX_SIZE	2048
#define LS_RESP_SIZE	2048
#define FILE_BUFF_SIZE	1024
#define PORT       		"2018"

typedef struct cmd_line
{
    char * const arguments[MAX_ARGUMENTS]; /* command line arguments (arg 0 is the command)*/
    int arg_count;		/* number of arguments */
    char const *input_redirect;	/* input redirection path. NULL if no input redirection */
    char const *output_redirect;	/* output redirection path. NULL if no output redirection */
    char blocking;	/* boolean indicating blocking/non-blocking */
    int idx;				/* index of current command in the chain of cmd_lines (0 for the first) */
    struct cmd_line *next;	/* next cmd_line in chain */
} cmd_line;

typedef enum {
	IDLE,
	CONNECTING,
	CONNECTED,
	DOWNLOADING,
	SIZE
} c_state;
	
typedef struct {
	char* server_addr;	// Address of the server as given in the [connect] command. "nil" if not connected to any server
	c_state conn_state;	// Current state of the client. Initially set to IDLE
	char* client_id;	// Client identification given by the server. NULL if not connected to a server.
	int sock_fd;
} client_state;

long file_size(char * filename);
char* list_dir();
void set_debug_mode(int argc, char **argv, int *debug);
void print_message(char *message, const char *srv_addr, int debug);
void init_client_state(client_state *cs, char *srv_addr);
void free_client_state(client_state *cs);
cmd_line *parse_cmd_lines(const char *str_line);	/* Parse string line */
void free_cmd_lines(cmd_line *line);		/* Free parsed line */
int replace_cmd_arg(cmd_line *line, int num, const char *new_string);
