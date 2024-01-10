#include<stdio.h>
#include<stdbool.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>

#define MAX_FRAMES 100

typedef struct{
    bool sended;
    int sequence_number;
    bool acknowledged ;
    char data[100];
}Frame;

void error(const char *msg){
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]){
    if(argc < 2){
        fprintf(stderr,"Port N not provided. Program terminated\n");
        exit(1);
    }

    Frame frame;

    strcat(frame.data,"the sending is correct");
    frame.sequence_number = 3;
    frame.acknowledged = false;


    int sockfd, newsockfd, portno, n;
    char buffer[255];

    struct sockaddr_in serv_addr,cli_addr;
    socklen_t clilen;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        error("Error opening Socket.");
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if(bind(sockfd,(struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){       
        error("Binding failed.");
    }

    listen(sockfd,1);
    clilen = sizeof(cli_addr);

    newsockfd = accept(sockfd,(struct sockaddr *) &cli_addr, &clilen);

    if(newsockfd < 0){
        error("Accepting failed.");
    }


    

    //selective repeat sender
    int num_frames = 27, window_size = 4, base =0;

    Frame frames[MAX_FRAMES];
    
    //defaulting the frame
    for(int i=0;i<num_frames;i++){
        frames[i].sended = false;
        frames[i].acknowledged = false;
        frames[i].sequence_number = i;
    }

    while(base<num_frames && window_size<num_frames){
        //sending frames in the window
        for(int i=base;i<base+window_size&&i<num_frames;i++){
            //frame will be sent if it is not sended and not acknowledged
            if(!frames[i].acknowledged && !frames[i].sended){
                frames[i].sended = true;
                printf("sending frame %d\n",frames[i].sequence_number);
                n = write(newsockfd,(struct Frame*) &frames[i],sizeof(frames[i]));
                if(n < 0){
                    error("Sending failed");
                }
            }
        }

        //reading the receiver request
        n = read(newsockfd,(struct Frame*) &frames[base],sizeof(frames[base]));
        if(n < 0){
            error("Reading failed.");
        }

        //informing that we received the frame and moving the window
        if(frames[base].acknowledged){
            printf("received ACK for frame %d\n",frames[base].sequence_number);
            base++;
        }

        //informing that the frame is requisted again
        else{
            printf("ACK for frame %d lost\n",frames[base].sequence_number);
            frames[base].sended=false;
        }
    }

    close(newsockfd);
    close(sockfd);

    return 0;
}