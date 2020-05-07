#include "sliding_window.h"

int update_window_received_packets(window_object_t *window, char *rest)
{

    char delim[] = " ";
    // char *rest = recv_buffer;
    char *ptr = strtok_r(rest, delim, &rest);
    printf("PTR1 %s\n", ptr);

    if (strcmp(ptr, "DATA"))
    {
        return 0;
    }
    int start = atoi(strtok_r(rest, delim, &rest));

    int bytes = atoi(strtok_r(rest, "\n", &rest));

    printf("Success!\n");

    char *to_write = rest;

    for (int i = 0; i < WINDOW_SIZE; i++)
    {
        if (window[i].start == start && window[i].length == bytes)
        {
            printf("FOUND %d %d\n", start, bytes);
            window[i].status = RECEIVED;
            window[i].data = to_write;
        }
    }
    return 0;
}

void initialize_window(window_object_t window[], int bytes_to_receive)
{
    int i = 0;
    int start = 0;
    while (i < WINDOW_SIZE && start < bytes_to_receive)
    {
        printf("I=%d, diff=%d\n", i, bytes_to_receive - start);
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
        i++;
        start += request;
    }
}
void send_packets(window_object_t *window, int first_index, int sockfd, const struct sockaddr_in *server_address, int server_len)
{
    for (int i = first_index; i < WINDOW_SIZE; i++)
    {
        int status = window[i].status;
        if (status == SENT || status == NOT_ACTIVE)
        {
            char message[20];
            sprintf(message, "GET %d %d\n", window[i].start, window[i].length);
            puts(message);
            ssize_t message_len = strlen(message);
            if (sendto(sockfd, message, message_len, 0, (struct sockaddr *)server_address, server_len) != message_len)
            {
                ERROR("sendto error");
            }

            window[i].status = SENT;
        }
    }
}

void proceed_packets(int sockfd, const struct sockaddr_in *server_address, window_object_t *window)
{
    for (int i = 0; i < WINDOW_SIZE; i++)
    {
        // wait some time to receive
        int tms = 5000;
        struct timeval tv;
        tv.tv_sec = tms / DEFAULT_LENGTH;
        tv.tv_usec = (tms % DEFAULT_LENGTH) * DEFAULT_LENGTH;
        // select(0, NULL, NULL, NULL, &tv);
        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,
                       sizeof tv) < 0)
            ERROR("setsockopt failed\n");
        // Receive packet
        struct sockaddr_in receiver_address;
        socklen_t len = sizeof(receiver_address);
        u_int8_t recv_buffer[BUFFER_SIZE + 1];
        ssize_t bytes_read = recvfrom(sockfd, recv_buffer, BUFFER_SIZE, 0, (struct sockaddr *)&receiver_address, &len);
        if (bytes_read < 0)
            // ERROR("recv error");
            continue;
        if (receiver_address.sin_addr.s_addr != (*server_address).sin_addr.s_addr || receiver_address.sin_port != (*server_address).sin_port)
        {
            continue;
        }
        recv_buffer[bytes_read] = 0;
        char *rest = recv_buffer;

        update_window_received_packets(window, rest);
    }
}

int slide_window(window_object_t *window, int packets_left, int bytes_to_receive)
{
    int first_index = -1;
    int index = 0;
    int slide = 0;
    while (index < WINDOW_SIZE)
    {
        if (window[index].status != RECEIVED)
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
        packets_left -=slide;
    }
    printf("to slide: %d\n",slide);
    int i = 0;
    int start_out_of_window = window[WINDOW_SIZE-1].start;
    while (i < WINDOW_SIZE)
    {
        //zapis do pliku
        if (i + slide < WINDOW_SIZE)
        {
            window[i] = window[i + slide];
        }
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
            start_out_of_window+=window[i].length;
        }
        i++;
    }
    return packets_left;
}
