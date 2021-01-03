#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <assert.h>
#include <netdb.h>
#include <ctype.h>

void set_nonblock(int socket) {
    int flags;
    flags = fcntl(socket,F_GETFL,0);
    assert(flags != -1);
    fcntl(socket, F_SETFL, flags | O_NONBLOCK);
}

int main(int argc, char** argv) {

    char listen_ip[39] = "0.0.0.0";
    char port[5] = "4950";

    for (int i = 0; i < argc; i++){
        char* arg = argv[i];

        if(arg[0] == '-'){
            switch (tolower(arg[1]) ) {
                case 'l':
                    if(i+1 < argc){
                        char arg_listen_addr[21];
                        char delimiter[] = ":";
                        strncpy(arg_listen_addr, argv[i+1], sizeof arg_listen_addr);

                        strncpy(listen_ip, strtok(arg_listen_addr, delimiter), sizeof listen_ip);

                        char* opt_port = strtok(NULL, delimiter);
                        if(opt_port != NULL)
                            strncpy(port, opt_port, sizeof port);
                    }else {
                        perror("Missing listen_address");
                    }
                    break;
            }
        }
    }

    int status, sock, new_sd;


    struct addrinfo hints;
    struct addrinfo *server_info;  //will point to the results

    //store the connecting address and size
    struct sockaddr_storage their_addr;
    socklen_t their_addr_size;

    fd_set read_flags,write_flags; // the flag sets to be used
    struct timeval waitd = {10, 0};// the max wait time for an event
    int sel;                      // holds return value for select();

    //socket infoS
    memset(&hints, 0, sizeof hints); //make sure the struct is empty
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM; // tcp
    hints.ai_flags = AI_PASSIVE;     // Socket address is intended for `bind'

    //get server info, put into server_info
    if ((status = getaddrinfo(listen_ip, port, &hints, &server_info)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    //make socket
    sock = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
    if (sock < 0) {
        printf("\nserver socket failure %m", errno);
        exit(1);
    }

    //allow reuse of port
    int reuse_socket_addr=1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse_socket_addr, sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }

    //unlink and bind
    unlink(listen_ip);
    if(bind (sock, server_info->ai_addr, server_info->ai_addrlen) < 0) {
        printf("\nBind error %m", errno);
        exit(1);
    }

    //listen
    if(listen(sock, 5) < 0) {
        printf("\nListen error %m", errno);
        exit(1);
    }
    their_addr_size = sizeof(their_addr);

    char hostname[39] = "";
    char service[5] = "";
    if((status = getnameinfo(server_info->ai_addr, server_info->ai_addrlen, hostname, sizeof hostname, service, sizeof service, NI_NUMERICHOST | NI_NUMERICSERV)) != 0){
        fprintf(stderr, "getnameinfo error: %s\n", gai_strerror(status));
    }else{
        printf("Listening on IP: %s Port: %s \n", hostname, service);
    }

    freeaddrinfo(server_info);


    while (1){
        printf("Waiting for Client...\n");

        //accept
        new_sd = accept(sock, (struct sockaddr*)&their_addr, &their_addr_size);
        if( new_sd < 0) {
            printf("\nAccept error %m", errno);
            exit(1);
        }

        set_nonblock(new_sd);
        printf("\nSuccessful Connection!\n");

        char buf_out[255];
        memset(&buf_out, 0, sizeof(buf_out));
        int numBytesSent;

        while(1) {
            FD_ZERO(&read_flags);
            FD_ZERO(&write_flags);
            FD_SET(new_sd, &read_flags);
            FD_SET(new_sd, &write_flags);

            sel = select(new_sd+1, &read_flags, &write_flags, (fd_set*)0, &waitd);

            //if an error with select
            if(sel < 0)
                continue;

            //socket ready for writing
            if(FD_ISSET(new_sd, &write_flags)) {
                FD_CLR(new_sd, &write_flags);
                numBytesSent = send(new_sd, buf_out, strnlen(buf_out, sizeof(buf_out)), 0);
                if(numBytesSent < 0){
                    printf("\nClosing socket\n");
                    close(new_sd);
                    break;
                }
                if(numBytesSent > 0)
                    printf("\nSent %d bytes", numBytesSent);

                memset(&buf_out, 0, sizeof(buf_out));

                if(numBytesSent == 0){
                    usleep(100);
                    strncpy(buf_out ,"Hello World!\n", sizeof(buf_out));
                }
            }
        }
    }
}