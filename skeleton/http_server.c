#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>

#include "util.h"

#include "thread_pool.h"
#include "seats.h"

#define BUFSIZE 1024
#define FILENAMESIZE 100

void shutdown_server(int);

int listenfd;

// TODO: Declare your threadpool!
pool_t* threadpool;

int main(int argc,char *argv[])
{
    int flag, num_seats = 20;
    int connfd = 0;
    struct sockaddr_in serv_addr;
   int num_threads = 4;
    char send_buffer[BUFSIZE];
    
    listenfd = 0; 

    int server_port = 8080;

    if (argc > 1)
    {
        num_seats = atoi(argv[1]);
//	num_threads = atoi(argv[2]);
    }
    if (argc > 2)
    {
	num_threads = atoi(argv[2]);
    } 

    if (server_port < 1500)
    {
        fprintf(stderr,"INVALID PORT NUMBER: %d; can't be < 1500\n",server_port);
        exit(-1);
    }
    
    if (signal(SIGINT, shutdown_server) == SIG_ERR) 
        printf("Issue registering SIGINT handler");

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if ( listenfd < 0 ){
        perror("Socket");
        exit(errno);
    }
    printf("Established Socket: %d\n", listenfd);
    flag = 1;
    setsockopt( listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag) );

    // Load the seats;
    load_seats(num_seats);

    // set server address 
    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(send_buffer, '0', sizeof(send_buffer));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(server_port);

    // bind to socket
    if ( bind(listenfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) != 0)
    {
        perror("socket--bind");
        exit(errno);
    }

    // listen for incoming requests
    listen(listenfd, 10);

    // TODO: Initialize your threadpool!
    threadpool = pool_create(1000,num_threads);
    initialize_standby();
    // This while loop "forever", handling incoming connections
    while(1)
    {
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);

        /*********************************************************************
            You should not need to modify any of the code above this comment.
            However, you will need to add lines to declare and initialize your 
            threadpool!

            The lines below will need to be modified! Some may need to be moved
            to other locations when you make your server multithreaded.
        *********************************************************************/
      //  printf("\t\t Got new request! \n");
        //printf("%d",connfd);
        
        //add a request to parse to the threadpool
        int error;
        struct request req={0,0,0,NULL};
        //received a request, so add to the number of requests
        float arrival_time;
        arrival_time = clock();
        
        error = pool_add_task(threadpool,1,NULL,NULL,connfd,req,arrival_time);
       // printf("\t\t waiting for new request\n");
        if (error==-1)
        {
            //cannot handle request
            printf("\t\t Cannot add to queue \n");
            close(connfd);
        }
        
        // struct request req;
        // // parse_request fills in the req struct object
        // parse_request(connfd, &req);
        // // process_request reads the req struct and processes the command
        // process_request(connfd, &req);
        // close(connfd);
    }
}

void shutdown_server(int signo){
    printf("Shutting down the server...\n");
    
    // TODO: Teardown your threadpool
    pool_destroy(threadpool);
    // TODO: Print stats about your ability to handle requests.
    unload_seats();
    kill_standby();
    close(listenfd);
    exit(0);
}
