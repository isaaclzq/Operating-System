#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unistd.h>

#include "libhttp.h"
#define BUFF_SIZE 1024
/*
 * Global configuration variables.
 * You need to use these in your implementation of handle_files_request and
 * handle_proxy_request. Their values are set up in main() using the
 * command line arguments (already implemented for you).
 */
int server_port;
char *server_files_directory;
char *server_proxy_hostname;
int server_proxy_port;

void requestSuccess(int fd, char *path);
char* combine(char* addr1, char* addr2);
/*
 * Reads an HTTP request from stream (fd), and writes an HTTP response
 * containing:
 *
 *   1) If user requested an existing file, respond with the file
 *   2) If user requested a directory and index.html exists in the directory,
 *      send the index.html file.
 *   3) If user requested a directory and index.html doesn't exist, send a list
 *      of files in the directory with links to each.
 *   4) Send a 404 Not Found response.
 */

// struct http_request *http_request_parse(int fd);
// void http_start_response(int fd, int status_code);
// void http_send_header(int fd, char *key, char *value);
// void http_end_headers(int fd);
// void http_send_string(int fd, char *data);
// void http_send_data(int fd, char *data, size_t size);
// char *http_get_mime_type(char *file_name);



void requestSuccess(int fd, char *path){
  char content_length[10];
  FILE *f;
  long fsize;

  f = fopen(path, "r");
  if (!f) perror("error"), exit(1);
  fseek(f, 0L, SEEK_END);
  fsize = ftell(f);
  rewind(f);
  char *string = malloc(fsize+1);
  fread(string, fsize, 1, f);
  fclose(f);
  http_start_response(fd, 200);
  http_send_header(fd, "Content-type", http_get_mime_type(path));
  snprintf(content_length, 10, "%ld", fsize);
  http_send_header(fd, "Content-Length", content_length);
  http_end_headers(fd);
  http_send_data(fd, string, fsize);
  free(string);
}


char* combine(char* addr1, char* addr2){
  unsigned int length = strlen(addr1) + strlen(addr2) + 1;

  return NULL;
}



void handle_files_request(int fd) {

  /* YOUR CODE HERE (Feel free to delete/modify the existing code below) */

  struct http_request *request = http_request_parse(fd);
  char* delim = "/";
  char* defalutFile = "index.html";
  unsigned int length = strlen(request->path) + strlen(server_files_directory) + 1;
  char path[length];
  struct stat st;
  
  memset(path, 0, sizeof(path));
  strncpy(path, server_files_directory, strlen(server_files_directory));
  strncat(path, delim, strlen(delim));
  strncat(path, request->path, strlen(request->path));
  
  //printf("path : %s\n", path);
  stat(path, &st);
  if (access(path, F_OK) == F_OK && S_ISREG(st.st_mode)){
    requestSuccess(fd, path);
  } else if (access(path, F_OK) == F_OK && S_ISDIR(st.st_mode)){
    length = strlen(server_files_directory) + strlen(defalutFile) + 2*strlen(delim) + strlen(request->path);
    char absPath[length];
    memset(absPath, 0, sizeof(absPath));
    strncpy(absPath, server_files_directory, strlen(server_files_directory));
    strncat(absPath, delim, strlen(delim));
    strncat(absPath , request->path, strlen(request->path));
    strncat(absPath, delim, strlen(delim));
    strncat(absPath, defalutFile, strlen(defalutFile));
    //printf("here: %s\n", absPath);
    if (access(absPath, F_OK) == F_OK){
      requestSuccess(fd, absPath);
    }
    else {
      DIR *output = opendir(path);
      char buffer[2048*4];
      struct dirent *dir;
      char *parent = "<a href='../'>Parent directory</a>";
      char *type = "text/html";
      char *starter = "<!DOCTYPE html><html><body>";
      char *end = "</body></html>";
      char *parag = "<p>";
      char *end_parag = "</p>";
      char *start_link = "<a href='";
      char *mid_link = "'>";
      char *end_link = "</a>";
      char *current = ".";
      char *previous = "..";

      if (output) {
        memset(&buffer, 0, sizeof(buffer));
        strncpy(buffer, starter, strlen(buffer));
        strncat(buffer, parent, strlen(parent));
        while ((dir = readdir(output)) != NULL){
          if (strcmp(dir->d_name, current) == 0 || strcmp(dir->d_name, previous) == 0){
            continue;
          }
          strncat(buffer, parag, strlen(parag));
          strncat(buffer, start_link, strlen(start_link));
          strncat(buffer, dir->d_name, strlen(dir->d_name));
          strncat(buffer, mid_link, strlen(mid_link));
          strncat(buffer, dir->d_name, strlen(dir->d_name));
          strncat(buffer, end_link, strlen(end_link));
          strncat(buffer, end_parag, strlen(end_parag));
        }
      }
      char content_length[10];
      strncat(buffer, end, strlen(end));
      http_start_response(fd, 200);
      http_send_header(fd, "Content-type", type);
      snprintf(content_length, 10, "%ld", strlen(buffer));
      http_send_header(fd, "Content-Length", content_length);
      http_end_headers(fd);
      http_send_data(fd, buffer, strlen(buffer));
    }
  } else {
    http_start_response(fd, 404);
  }

  // http_start_response(fd, 200);
  // http_send_header(fd, "Content-type", "text/html");
  // http_end_headers(fd);
  // http_send_string(fd,
  //     "<center>"
  //     "<h1>Welcome to httpserver!</h1>"
  //     "<hr>"
  //     "<p>Nothing's here yet.</p>"
  //     "</center>");

}

/*
 * Opens a connection to the proxy target (hostname=server_proxy_hostname and
 * port=server_proxy_port) and relays traffic to/from the stream fd and the
 * proxy target. HTTP requests from the client (fd) should be sent to the
 * proxy target, and HTTP responses from the proxy target should be sent to
 * the client (fd).
 *
 *   +--------+     +------------+     +--------------+
 *   | client | <-> | httpserver | <-> | proxy target |
 *   +--------+     +------------+     +--------------+
 */

void handle_proxy_request(int fd) {

  /* YOUR CODE HERE */
  char *hostname = server_proxy_hostname;
  int select_check, numByte, flag;
  int socket_option = 1;
  char buffer[BUFF_SIZE];
  int port = server_proxy_port;
  struct hostent *lookup = gethostbyname(hostname);
  char* ip_addr = inet_ntoa(*(struct in_addr*)lookup->h_addr);
  fd_set fd_set, active_fd_set;

  int r_sock = socket(AF_INET, SOCK_STREAM, 0);
  //setsockopt(r_sock, SOL_SOCKET, SO_REUSEADDR, &socket_option, sizeof(socket_option));
  struct sockaddr_in dest;
  memset(&dest, 0, sizeof(dest));
  dest.sin_family = AF_INET;
  dest.sin_addr.s_addr = inet_addr(ip_addr);
  dest.sin_port = htons(port);
  flag = connect(r_sock, (struct sockaddr *)&dest, sizeof(struct sockaddr_in));
  if (flag < 0){
    close(r_sock);
    printf("fuck up\n");
  }  

  FD_ZERO(&active_fd_set);
  FD_ZERO(&fd_set);
  FD_SET(r_sock, &active_fd_set);
  FD_SET(fd, &active_fd_set);
  while(1){
    fd_set = active_fd_set;
    select_check = select(FD_SETSIZE, &fd_set, NULL, NULL, NULL);
    if(select_check == -1){
      exit(EXIT_FAILURE);
    } else if (FD_ISSET(fd, &fd_set)){
      if ((numByte = read(fd, buffer, sizeof(buffer))) < 0) exit(EXIT_FAILURE);
      if ((numByte = write(r_sock, buffer, numByte)) < 0) exit(EXIT_FAILURE);
    } else if (FD_ISSET(r_sock, &fd_set)){
      if ((numByte = read(r_sock, buffer, sizeof(buffer))) < 0) exit(EXIT_FAILURE);
      if ((numByte = write(fd, buffer, numByte)) < 0) exit(EXIT_FAILURE);
    } else {
      exit(EXIT_FAILURE);
    }
  }
  close(r_sock);
  close(fd);
}

/*
 * Opens a TCP stream socket on all interfaces with port number PORTNO. Saves
 * the fd number of the server socket in *socket_number. For each accepted
 * connection, calls request_handler with the accepted fd number.
 */
void serve_forever(int *socket_number, void (*request_handler)(int)) {

  struct sockaddr_in server_address, client_address;
  size_t client_address_length = sizeof(client_address);
  int client_socket_number;
  pid_t pid;

  *socket_number = socket(PF_INET, SOCK_STREAM, 0);
  if (*socket_number == -1) {
    perror("Failed to create a new socket");
    exit(errno);
  }

  int socket_option = 1;
  if (setsockopt(*socket_number, SOL_SOCKET, SO_REUSEADDR, &socket_option,
        sizeof(socket_option)) == -1) {
    perror("Failed to set socket options");
    exit(errno);
  }

  memset(&server_address, 0, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(server_port);

  if (bind(*socket_number, (struct sockaddr *) &server_address,
        sizeof(server_address)) == -1) {
    perror("Failed to bind on socket");
    exit(errno);
  }

  if (listen(*socket_number, 1024) == -1) {
    perror("Failed to listen on socket");
    exit(errno);
  }

  printf("Listening on port %d...\n", server_port);

  while (1) {

    client_socket_number = accept(*socket_number,
        (struct sockaddr *) &client_address,
        (socklen_t *) &client_address_length);
    if (client_socket_number < 0) {
      perror("Error accepting socket");
      continue;
    }

    printf("Accepted connection from %s on port %d\n",
        inet_ntoa(client_address.sin_addr),
        client_address.sin_port);

    pid = fork();
    if (pid > 0) {
      close(client_socket_number);
    } else if (pid == 0) {
      // Un-register signal handler (only parent should have it)
      signal(SIGINT, SIG_DFL);
      close(*socket_number);
      request_handler(client_socket_number);
      close(client_socket_number);
      exit(EXIT_SUCCESS);
    } else {
      perror("Failed to fork child");
      exit(errno);
    }
  }

  close(*socket_number);

}

int server_fd;
void signal_callback_handler(int signum) {
  printf("Caught signal %d: %s\n", signum, strsignal(signum));
  printf("Closing socket %d\n", server_fd);
  if (close(server_fd) < 0) perror("Failed to close server_fd (ignoring)\n");
  exit(0);
}

char *USAGE =
  "Usage: ./httpserver --files www_directory/ --port 8000\n"
  "       ./httpserver --proxy inst.eecs.berkeley.edu:80 --port 8000\n";

void exit_with_usage() {
  fprintf(stderr, "%s", USAGE);
  exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
  signal(SIGINT, signal_callback_handler);

  /* Default settings */
  server_port = 8000;
  void (*request_handler)(int) = NULL;

  int i;
  for (i = 1; i < argc; i++) {
    if (strcmp("--files", argv[i]) == 0) {
      request_handler = handle_files_request;
      free(server_files_directory);
      server_files_directory = argv[++i];
      if (!server_files_directory) {
        fprintf(stderr, "Expected argument after --files\n");
        exit_with_usage();
      }
    } else if (strcmp("--proxy", argv[i]) == 0) {
      request_handler = handle_proxy_request;

      char *proxy_target = argv[++i];
      if (!proxy_target) {
        fprintf(stderr, "Expected argument after --proxy\n");
        exit_with_usage();
      }

      char *colon_pointer = strchr(proxy_target, ':');
      if (colon_pointer != NULL) {
        *colon_pointer = '\0';
        server_proxy_hostname = proxy_target;
        server_proxy_port = atoi(colon_pointer + 1);
      } else {
        server_proxy_hostname = proxy_target;
        server_proxy_port = 80;
      }
    } else if (strcmp("--port", argv[i]) == 0) {
      char *server_port_string = argv[++i];
      if (!server_port_string) {
        fprintf(stderr, "Expected argument after --port\n");
        exit_with_usage();
      }
      server_port = atoi(server_port_string);
    } else if (strcmp("--help", argv[i]) == 0) {
      exit_with_usage();
    } else {
      fprintf(stderr, "Unrecognized option: %s\n", argv[i]);
      exit_with_usage();
    }
  }

  if (server_files_directory == NULL && server_proxy_hostname == NULL) {
    fprintf(stderr, "Please specify either \"--files [DIRECTORY]\" or \n"
                    "                      \"--proxy [HOSTNAME:PORT]\"\n");
    exit_with_usage();
  }

  serve_forever(&server_fd, request_handler);

  return EXIT_SUCCESS;
}
