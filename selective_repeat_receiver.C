#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define RAND_MAX
#define MAX_FRAMES 100

typedef struct
{
    bool sended;
    int sequence_number;
    bool acknowledged;
    char data[100];
} Frame;

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{

    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    Frame frame;

    char buffer[255];
    if (argc < 3)
    {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        exit(1);
    }

    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("Error opening socket.");

    server = gethostbyname(argv[1]);
    if (server == NULL)
    {
        fprintf(stderr, "Error, no such host");
        exit(1);
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("Connection failed");

    // selective repeat receiver
    int num_frames = 27, window_size = 4, base = 0, index = 0, j = 0;

    Frame frame_buffer;
    Frame frames[MAX_FRAMES];
    Frame Buffer[MAX_FRAMES];

    while (base < num_frames)
    {
        for (int i = base; i < base + window_size && i < num_frames; i++)
        {
            // receiving the frame
            n = read(sockfd, (struct Frame *)&frame_buffer, sizeof(frame_buffer));
            if (n < 0)
                error("Receiving failed");
            printf("receiving frame %d\n", frame_buffer.sequence_number);

            // puting randomly the problem in receiving
            if (rand() % 4)
            {
                // without problem, informing and sending the ACK
                frame_buffer.acknowledged = true;
                printf("Sending ACK for frame %d\n", frame_buffer.sequence_number);
                n = write(sockfd, (struct Frame *)&frame_buffer, sizeof(frame_buffer));
                if (n < 0)
                    error("Receiving failed");
                base++;
                //if order is correct, add to data base
                if (index == frame_buffer.sequence_number)
                {
                    frames[i] = frame_buffer;
                    index++;
                }
                //if not, adding the new frame to the buffer
                //until we receive the lost frame
                else
                {
                    printf("BUFFERING: buffering frame %d\n", frame_buffer.sequence_number);
                    Buffer[j] = frame_buffer;
                    j++;
                }
            }
            else
            {
                // with problem, informing and sending the NACK
                frame_buffer.acknowledged = false;
                printf("Sending NACK for frame %d\n", frame_buffer.sequence_number);
                n = write(sockfd, (struct Frame *)&frame_buffer, sizeof(frame_buffer));
                if (n < 0)
                    error("Receiving failed");
            }

            //adding the frames that were buffered to the data base
            //7 
            if (index == frame_buffer.sequence_number + 1 && Buffer[0].acknowledged)
            {
                int k = 0;
                while (k < j)
                {
                    frames[index] = Buffer[k];
                    printf("adding frame %d to frames\n", frames[index].sequence_number);
                    index++;
                    k++;
                }
                bzero(Buffer, sizeof(Buffer));
                j = 0;
            }
        }
    }

    // close the connection
    close(sockfd);

    return 0;
}