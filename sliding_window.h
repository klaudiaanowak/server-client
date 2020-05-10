// Klaudia Nowak
// 297936 

#include <netinet/ip.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define ERROR(str) { fprintf(stderr, "%s: %s\n", str, strerror(errno)); exit(EXIT_FAILURE); }

#define BUFFER_SIZE 4096
#define DEFAULT_LENGTH 1000
#define RECEIVED 1
#define SENT 0
#define NOT_ACTIVE -1
#define SAVED 2

typedef struct
{
    int start;
    int length;
    int status;
    char *data;
} window_object_t;

void initialize_window(window_object_t *window, int bytes_to_receive, int window_size);
void send_packets(window_object_t *window, int first_index, int sockfd, const struct sockaddr_in *server_address, int server_len, int window_size);
void proceed_packets(int sockfd, const struct sockaddr_in *server_address, window_object_t *window , int window_size);
int slide_window(window_object_t *window, int packets_left, int bytes_to_receive, int window_size);
