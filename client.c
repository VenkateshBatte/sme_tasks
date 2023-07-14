#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/msg.h>

void error(const char *msg){
	perror(msg);
	exit(0);
}


int main(int argc,char *argv[]){
	int sockfd, portno, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	char buff[255];
	if(argc<3){
		fprintf(stderr,"usage: %s <hostname> <port_no>\n",argv[0]);
		exit(1);
	}

	portno = atoi(argv[2]);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd<0)
		error("ERROR opening socket");

	server = gethostbyname(argv[1]);
	if(server == NULL){
		fprintf(stderr,"Error , no such host");
	}

	bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *) server->h_addr, (char*) &serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);

	if(connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr))<0){
		error("Connection Failed");
	}
	
	while(1){
		bzero(buff,255);
		//fgets(buff,255,stdin);
		n=write(sockfd,"Temparature?",strlen("Temparature")+1);
		bzero(buff,255);
		n=read(sockfd,buff,255);
		printf("Server : %s\n",buff);

		write(sockfd,"Received",strlen("Received")+1);

		int i=strncmp("Bye",buff,3);
		if(i==0)
			break;
		sleep(2);
	}
	close(sockfd);
	return 0;
}

