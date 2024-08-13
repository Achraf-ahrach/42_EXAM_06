#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct client {
    int     id;
    int     len;
    char    msg[1024];
} t_cient;

fd_set  readfds, writefds, active;
int     fd_max = 0, next_id = 0;
char    r_buff[1024], w_buff[1024];
t_cient clients[1024];

void err(void) {
    write(2, "Fatal error\n", 12);
    exit(1);
}

void send_all(int c) {
    for (int i=0;i <= fd_max; i++)
        if (FD_ISSET(i, &writefds) & i != c)
            send(i, w_buff, strlen(w_buff), 0);
}

int main(int ac, char **av) {
	int sockfd;
	struct sockaddr_in servaddr; 

    if (ac != 2) {
        write(2, "Wrong number of arguments\n", 26);
        exit(1);
    }

	bzero(&clients, sizeof(clients));
    FD_ZERO(&active);

	// socket create and verification
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1)
        err();

    fd_max = sockfd;
    FD_SET(sockfd, &active);

	// assign IP, PORT
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(av[1]));
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
        err();

	if (listen(sockfd, 1024) != 0)
        err();

    while (1) {
        readfds = writefds = active;
        if (select(fd_max + 1, &readfds, &writefds, NULL, NULL) < 0)
            continue;

        for (int fd=0;fd <= fd_max; fd++) {
            if (FD_ISSET(fd, &readfds)) {
                if (sockfd == fd) {
                    int conn = accept(fd, NULL, NULL);
                    if (conn < 0)
                        continue;

                    if (fd_max < conn)
                        fd_max = conn;
                    clients[conn].len = 0;
                    clients[conn].id = next_id++;
                    FD_SET(conn, &active);
                    sprintf(w_buff, "server: client %d just arrived\n", clients[conn].id);
                    send_all(conn);
                    break;
                }
                else {
                    int len = recv(fd, r_buff, 1024, 0);
                    if (len <= 0) {
                        sprintf(w_buff, "server: client %d just left\n", clients[fd].id);
                        send_all(fd);
                        FD_CLR(fd, &active);
                        close(fd);
                        break;
                    }
                    else {
                        for (int i=0;i < len; i++) {
                            clients[fd].msg[clients[fd].len] = r_buff[i];
                            clients[fd].len++;
                            if (r_buff[i] == '\n') {
                                clients[fd].msg[clients[fd].len] = '\0';
                                clients[fd].len = 0;
                                sprintf(w_buff, "client %d: %s", clients[fd].id, clients[fd].msg);
                                send_all(fd);
                            }
                        }
                        break;
                    }
                }
            }
        }
    }
}