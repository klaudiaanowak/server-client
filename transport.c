// Klaudia Nowak
// 297936

#include "sliding_window.h"

FILE *fp;
int WINDOW_SIZE = 450;
int all_packets;

int not_all_received(window_object_t *window)
{
    for (int i = 0; i < WINDOW_SIZE; i++)
    {
        if (window[i].status != SAVED)
        {
            return 1;
        }
    }
    return 0;
}
int print_to_file(window_object_t *window, char filename[], int saved_packets)
{
    int status0 = window[0].status;
    if (status0 != RECEIVED && status0 != SAVED)
    {
        return saved_packets;
    }
    int index = 0;
    fp = fopen(filename, "ab");

    while (index < WINDOW_SIZE)
    {
        window_object_t el_to_write = window[index];
        if (el_to_write.status == SAVED)
        {
            index++;
            continue;
        }
        if (el_to_write.status != RECEIVED)
        {
            break;
        }
        else
        {
            char *saveme;
            size_t elements_written = fwrite(el_to_write.data, sizeof *el_to_write.data, el_to_write.length, fp);
            if (elements_written != el_to_write.length){
                ERROR("Writing to file error: %s\n");
            }
            window[index].status = SAVED;
            saved_packets++;
            double percent = (saved_packets / (double)all_packets) * 100;
            printf("%0.3f%% done\n", percent);
        }
        index++;
    }
    fclose(fp);
    return saved_packets;
}
int main(int argc, char *argv[])
{
    char ip_address[20];
    int port;
    char file_name[100];
    int bytes;
    int saved_packets = 0;

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
    all_packets = packets_left;
    if (packets_left < WINDOW_SIZE)
    {
        WINDOW_SIZE = packets_left;
    }
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
        ERROR("Incorrect IP address\n");
    }

    window_object_t current_window[WINDOW_SIZE];

    // initialize window
    initialize_window(current_window, bytes, WINDOW_SIZE);
    packets_left -= WINDOW_SIZE;

    // Send requests, receive packages and save to file untill all packages are not received
    while (packets_left > 0 || not_all_received(current_window))
    {
        // Send packets from current window
        send_packets(current_window, 0, sockfd, &server_address, server_len, WINDOW_SIZE);

        // Proceed received packets
        proceed_packets(sockfd, &server_address, current_window, WINDOW_SIZE,all_packets-saved_packets);

        // Print to file first packages from current window
        saved_packets = print_to_file(current_window, file_name, saved_packets);

        // Slide window
        packets_left = slide_window(current_window, packets_left, bytes, WINDOW_SIZE);
    }
    // Close socket
    if (close(sockfd) < 0)
        ERROR("close error");
}