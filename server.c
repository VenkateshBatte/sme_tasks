#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

void error(const char *msg){
	perror(msg);
	exit(1);
}



int main(int argc,char *argv[]){
	if(argc<2){
		fprintf(stderr,"Port no. not provided. Program terminated\n");
		exit(1);
	}

	int sockfd,newsockfd,portno,n;
	char buff[255];

	struct sockaddr_in serv_addr, cli_addr;
	socklen_t clilen;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd<0){
		error("Error opening Socket.");
	}

	bzero((char*)&serv_addr, sizeof(serv_addr));
	portno = atoi(argv[1]);

	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=INADDR_ANY;
	serv_addr.sin_port=htons(portno);

	if(bind(sockfd,(struct sockaddr *) &serv_addr, sizeof(serv_addr))<0){
		error("Binding Failed.");
	}

	listen(sockfd, 5);
	clilen=sizeof(cli_addr);

	newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
	if(newsockfd < 0)
		error("Error on Accept");
	
	while(1){
		bzero(buff,255);
		n=read(newsockfd, buff,255);
		printf("CLient : %s\n",buff);

		bzero(buff,255);
		int fd1=open("/sys/devices/platform/soc/fe804000.i2c/i2c-1/1-0038/dht2x_temp",O_RDONLY);
		read(fd1,buff,sizeof(buff));
		close(fd1);

		n=write(newsockfd,buff,strlen(buff));

		memset(buff,0,255);
		read(newsockfd,buff,255);
		printf("Client : %s\n",buff);

		fd1=open("/sys/devices/platform/soc/fe804000.i2c/i2c-1/1-0038/dht2x_ack",O_RDWR);
		write(fd1,buff,strlen(buff));
		close(fd1);

		int i = strncmp("Bye",buff,3);
		if(i==0)
			break;
		sleep(2);
	} 

	close(newsockfd);
	close(sockfd);
	return 0;
}

