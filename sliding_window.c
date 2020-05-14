// Klaudia Nowak
// 297936

#include "sliding_window.h"

int update_window_received_packets(window_object_t *window, char *rest, int window_size)
{

    char delim[] = " ";
    char *ptr = strtok_r(rest, delim, &rest);

    if (strcmp(ptr, "DATA"))
    {
        return 0;
    }
    int start = atoi(strtok_r(rest, delim, &rest));
    int bytes = atoi(strtok_r(rest, "\n", &rest));
    char *to_write = rest;

    for (int i = 0; i < window_size; i++)
    {
        window_object_t el_to_write = window[i];
        if (el_to_write.start == start && el_to_write.length == bytes && el_to_write.status != SAVED && el_to_write.status != RECEIVED)
        {
            window[i].status = RECEIVED;
            memcpy(window[i].data, to_write, sizeof(char) * window[i].length);
        }
    }
    return 0;
}

void initialize_window(window_object_t window[], int bytes_to_receive, int window_size)
{

    int i = 0;
    int start = 0;
    while (i < window_size && start < bytes_to_receive)
    {
        int request = 0;
        if (bytes_to_receive - start > DEFAULT_LENGTH)
        {
            request = DEFAULT_LENGTH;
        }
        else
        {
            request = bytes_to_receive - start;
        }
        window[i].start = start;
        window[i].length = request;
        window[i].status = NOT_ACTIVE;
        window[i].data = (char *)malloc(request * sizeof(char));
        i++;
        start += request;
    }
}
void send_packets(window_object_t *window, int first_index, int sockfd, const struct sockaddr_in *server_address, int server_len, int window_size)
{
    for (int i = first_index; i < window_size; i++)
    {
        int status = window[i].status;
        if (status == SENT || status == NOT_ACTIVE)
        {
            char message[20];
            sprintf(message, "GET %d %d\n", window[i].start, window[i].length);
            ssize_t message_len = strlen(message);
            if (sendto(sockfd, message, message_len, 0, (struct sockaddr *)server_address, server_len) != message_len)
            {
                ERROR("sendto error");
            }
            window[i].status = SENT;
        }
    }
}

void proceed_packets(int sockfd, const struct sockaddr_in *server_address, window_object_t *window, int window_size, int packets_left)
{
    int iteration = packets_left < 60 ? packets_left+1 : 60;
    struct timeval tv;
    int tms = 5;
    struct sockaddr_in receiver_address;
    socklen_t len = sizeof(receiver_address);
    u_int8_t recv_buffer[BUFFER_SIZE + 1];
    for (int i = 0; i < iteration; i++)
    {
        // wait some time to receive
        tv.tv_sec = tms / DEFAULT_LENGTH;
        tv.tv_usec = (tms % DEFAULT_LENGTH) * DEFAULT_LENGTH;
        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,
                       sizeof tv) < 0)
            ERROR("setsockopt failed\n");
        // Receive packet
        tms = 1;
        ssize_t bytes_read = recvfrom(sockfd, recv_buffer, BUFFER_SIZE, 0, (struct sockaddr *)&receiver_address, &len);
        if (bytes_read < 0)
            continue;
        if (receiver_address.sin_addr.s_addr != (*server_address).sin_addr.s_addr || receiver_address.sin_port != (*server_address).sin_port)
        {
            continue;
        }
        recv_buffer[bytes_read] = 0;
        char *rest = recv_buffer;
        // Update statuses and data of current window if packet received
        update_window_received_packets(window, rest, window_size);
    }
}

int slide_window(window_object_t *window, int packets_left, int bytes_to_receive, int window_size)
{
    int first_index = -1;
    int index = 0;
    int slide = 0;
    // Check how many packets can be moved
    while (index < window_size)
    {
        if (window[index].status != SAVED)
        {
            break;
        }
        else
        {
            first_index = index;
            index++;
        }
    }
    if (first_index < 0)
    {
        return packets_left;
    }
    if (first_index + 1 > packets_left)
    {
        // move by packets_left
        slide = packets_left;
        packets_left = 0;
    }
    else
    {
        // move by first_index
        slide = first_index + 1;
        packets_left -= slide;
    }
    int i = 0;
    int start_out_of_window = window[window_size - 1].start;
    // move packets
    while (i < window_size)
    {
        // move packets inside the window
        if (i + slide < window_size)
        {
            window[i].start = window[i + slide].start;
            window[i].length = window[i + slide].length;
            window[i].status = window[i + slide].status;
            memcpy(window[i].data, window[i + slide].data, sizeof(char) * window[i].length);
        }
        // move packets and set new at the end of the window
        else
        {
            window[i].start = start_out_of_window + DEFAULT_LENGTH;
            if (bytes_to_receive - window[i].start > DEFAULT_LENGTH)
            {
                window[i].length = DEFAULT_LENGTH;
            }
            else
            {
                window[i].length = bytes_to_receive - window[i].start;
            }
            window[i].status = NOT_ACTIVE;
            start_out_of_window += window[i].length;
        }
        i++;
    }
    return packets_left;
}
