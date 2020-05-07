#include "sliding_window.h"

FILE *fp;

int all_received(window_object_t *window){
    for (int i =0 ; i < WINDOW_SIZE; i ++){
        if (window[i].status != RECEIVED){
            printf("false\n");
            return 0;
        }
    }
    printf("true\n");
    return 1;
}

int main(int argc, char *argv[])
{
    char ip_address[20];
    int port;
    char file_name[100];
    int bytes;

    // Check input data
    if (argc != 5)
    {
        ERROR("Usage: ./transport ip port output_file bytes\n");
    }
    // Assign input data
    strcpy(ip_address, argv[1]);
    port = atoi(argv[2]);
    strcpy(file_name, argv[3]);
    bytes = atoi(argv[4]);
    int packets_left = bytes % DEFAULT_LENGTH == 0 ? (int)(bytes / DEFAULT_LENGTH) : (int)(bytes / DEFAULT_LENGTH) + 1;
    printf("Packet left: %d\n", packets_left);
    // Remove file if exists - appending data on the end of file could be incorrect if file exists before and contains same data
    remove(file_name);
    // Create socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        ERROR("socket error: %s\n");
    }
    // Set server address
    struct sockaddr_in server_address;
    bzero(&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    int server_len = sizeof(server_address);
    if (inet_pton(AF_INET, ip_address, &server_address.sin_addr) != 1)
    {
        printf("Incorrect IP address\n");
        return EXIT_FAILURE;
    }

    window_object_t current_window[WINDOW_SIZE];
    // initialize window
    printf("\nInitialize\n");

    initialize_window(current_window, bytes);
    packets_left -= WINDOW_SIZE;
    printf("Packet left: %d\n", packets_left);

    for (int i = 0; i < WINDOW_SIZE; i++)
    {
        printf("Window: %d: start = %d, len= %d, status = %d\n", i, current_window[i].start, current_window[i].length, current_window[i].status);
    }
   while (packets_left>0 || all_received(current_window))
    {
        // Send packets from window
        printf("\nSend\n");

        send_packets(current_window, 0, sockfd, &server_address, server_len);
        for (int i = 0; i < WINDOW_SIZE; i++)
        {
            printf("Window: %d: start = %d, len= %d, status = %d\n", i, current_window[i].start, current_window[i].length, current_window[i].status);
        }
        // Proceed received packets
        printf("\nProceed\n");

        proceed_packets(sockfd, &server_address, current_window);

        for (int i = 0; i < WINDOW_SIZE; i++)
        {
            printf("Window: %d: start = %d, len= %d, status = %d\n", i, current_window[i].start, current_window[i].length, current_window[i].status);
        }

        // Slide window
        printf("\nSlide\n");

        packets_left = slide_window(current_window, packets_left, bytes);
        for (int i = 0; i < WINDOW_SIZE; i++)
        {
            printf("Window: %d: start = %d, len= %d, status = %d\n", i, current_window[i].start, current_window[i].length, current_window[i].status);
        }
        printf("Packet left: %d\n", packets_left);
    }
    // Close socket
    if (close(sockfd) < 0)
        ERROR("close error");
}