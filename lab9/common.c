#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>
#include "common.h"

#define TRUE	1
#define FALSE	0

#ifndef NULL
    #define NULL 0
#endif

#ifndef FREE
	#define FREE(X) if(X) free((void*)X)
#endif

static char *clone_first_word(char *str)
{
    char *start = NULL;
    char *end = NULL;
    char *word;

    while (!end) {
        switch (*str) {
            case '>':
            case '<':
            case 0:
                end = str - 1;
                break;
            case ' ':
                if (start)
                    end = str - 1;
                break;
            default:
                if (!start)
                    start = str;
                break;
        }
        str++;
    }

    if (start == NULL) {
        return NULL;
	}

    word = (char*) malloc(end-start+2);
    strncpy(word, start, ((int)(end-start)+1)) ;
    word[ (int)((end-start)+1)] = 0;

    return word;
}

static void extract_redirections(char *str_line, cmd_line *line)
{
    char *s = str_line;

    while ( (s = strpbrk(s,"<>")) ) {
        if (*s == '<') {
            FREE(line->input_redirect);
            line->input_redirect = clone_first_word(s+1);
        }
        else {
            FREE(line->output_redirect);
            line->output_redirect = clone_first_word(s+1);
        }

        *s++ = 0;
    }
}

static char *str_clone(const char *source)
{
    char* clone = (char*)malloc(strlen(source) + 1);
    strcpy(clone, source);
    return clone;
}

static int is_empty(const char *str)
{
  if (!str)
    return 1;
  
  while (*str) {
    if (!isspace(*(str++))) {
      return 0;
	}
  }  
  return 1;
}

static cmd_line *parse_single_cmd_line(const char *str_line)
{
    char *delimiter = " ";
    char *line_cpy, *result;
    
    if (is_empty(str_line)){
      return NULL;
	}
    
    cmd_line* line = (cmd_line*)malloc( sizeof(cmd_line) ) ;
    memset(line, 0, sizeof(cmd_line));
    
    line_cpy = str_clone(str_line);
         
    extract_redirections(line_cpy, line);
    
    result = strtok(line_cpy, delimiter);    
    while( result && line->arg_count < MAX_ARGUMENTS-1) {
        ((char**)line->arguments)[line->arg_count++] = str_clone(result);
        result = strtok (NULL, delimiter);
    }

    FREE(line_cpy);
    return line;
}

static cmd_line* _parse_cmd_lines(char *line_str)
{
	char *next_str_cmd;
	cmd_line *line;
	char pipe_delimiter = '|';
	
	if (is_empty(line_str)) {
	  return NULL;
	}
	
	next_str_cmd = strchr(line_str , pipe_delimiter);
	if (next_str_cmd) {
	  *next_str_cmd = 0;
	}
	
	line = parse_single_cmd_line(line_str);
	if (!line) {
	  return NULL;
	}
	
	if (next_str_cmd)
	  line->next = _parse_cmd_lines(next_str_cmd+1);

	return line;
}

cmd_line *parse_cmd_lines(const char *str_line)
{
	char* line, *ampersand;
	cmd_line *head, *last;
	int idx = 0;
	
	if (is_empty(str_line)) {
	  return NULL;
	}
	
	line = str_clone(str_line);
	if (line[strlen(line)-1] == '\n') {
	  line[strlen(line)-1] = 0;
	}
	
	ampersand = strchr( line,  '&');
	if (ampersand) {
	  *(ampersand) = 0;
	}
		
	if ( (last = head = _parse_cmd_lines(line)) )
	{	
	  while (last->next) {
	    last = last->next;
	  }
	  last->blocking = ampersand? 0:1;
	}
	
	for (last = head; last; last = last->next) {
		last->idx = idx++;
	}
			
	FREE(line);
	return head;
}


void free_cmd_lines(cmd_line *line)
{
  int i;
  if (!line)
    return;

  FREE(line->input_redirect);
  FREE(line->output_redirect);
  for (i=0; i<line->arg_count; ++i)
      FREE(line->arguments[i]);

  if (line->next)
	  free_cmd_lines(line->next);

  FREE(line);
}

int replace_cmd_arg(cmd_line *line, int num, const char *new_string)
{
  if (num >= line->arg_count)
    return 0;
  
  FREE(line->arguments[num]);
  ((char**)line->arguments)[num] = str_clone(new_string);
  return 1;
}

static char* _list_dir(DIR * dir, int len) {
	struct dirent * ent = readdir(dir);
	struct stat info;

	if (ent == NULL) {
		char * listing = malloc((len + 1) * sizeof(char));
		listing[len] = '\0';
		return listing;
	}
	if (stat(ent->d_name, &info) < 0) {
		perror("stat");
		return NULL;
	}
	
	int ent_len = 0;
	if(S_ISREG(info.st_mode)){
		ent_len = strlen(ent->d_name);
		ent_len++; /* Account for the \n */
	}
	
	char * listing = _list_dir(dir, len + ent_len);	
	
	if(S_ISREG(info.st_mode)){	
		strcpy(&listing[len], ent->d_name);
		listing[len + ent_len - 1] = '\n';
	}
	return listing;
}

long file_size(char * filename){
	long filesize = -1;
	FILE * file;
	file = fopen (filename, "r");
	if (file == NULL){
		perror("fopen");
		goto err;
	}
	
	if(fseek (file, 0, SEEK_END) != 0){
		perror("fseek");
		goto err;
	}
	if((filesize = ftell(file)) < 0){
		perror("ftell");
		goto err;
	}		
	
	fclose(file);
	
	return filesize;
	
	err:
	if (file) {
		fclose(file);
	}
	return -1;
}

//@return must be freed by the caller
char* list_dir(){
	DIR *dir = opendir("."); 
	if (dir == NULL){
		perror("opendir");
		return NULL;
	}
	
	char * listing = _list_dir(dir, 0);
	closedir(dir);
	return listing;
}

void set_debug_mode(int argc, char **argv, int *debug)
{
    if (1 == argc) {
        *debug = FALSE;
    } else if (2 == argc && !strcmp(argv[1], "-d")) {
        *debug = TRUE;
    } else {
       fprintf(stderr, "usage: %s [-d]\n", argv[0]);
	   exit(-1);
	}
}

void print_message(char *message, const char *srv_addr, int debug)
{
    if (TRUE == debug) {
        printf("%s|log: %s\n", srv_addr, message);
    }
}

void init_client_state(client_state *cs, char *srv_addr)
{
    cs->client_id = NULL;
    cs->conn_state = IDLE;
    cs->sock_fd = -1;
    cs->server_addr = (char *)malloc(strlen(srv_addr) + 1);
    strcpy(cs->server_addr, srv_addr);
}

void free_client_state(client_state *cs)
{
  	if (!cs) {
		return;
	}
	
	FREE(cs->client_id);
	FREE(cs->server_addr);
	FREE(cs);
}
