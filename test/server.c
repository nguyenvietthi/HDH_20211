#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <net/if.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[])
{
	int rv, sk, confd;
	char client_buf[1024];
	char server_buf[1024];
	char check[]="exit";
	struct ifreq ifr;
	struct sockaddr_in serv_addr;
	int addrlen = sizeof(serv_addr); 

	sk = socket(AF_INET, SOCK_STREAM, 0);
	if (sk < 0) {
		perror("ERROR opening socket");
		exit(1);
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, "sn0", 5);
	if (setsockopt(sk, SOL_SOCKET, SO_BINDTODEVICE, (void*)(&ifr), sizeof(ifr)) < 0) {
		perror("setsocket filed\n");
		exit(1);
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(5001);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	rv = bind(sk, (struct sockaddr*)(&serv_addr), sizeof(serv_addr));
	if (rv) {
		perror("bind failed\n");
		exit(1);
	}

	rv = listen(sk, 10);
	if (rv) {
		perror("listen failed\n");
		exit(1);
	}
	confd = accept(sk, (struct sockaddr*)&serv_addr, (socklen_t*)&addrlen);	
	printf("accept ==== \n");
	char str_cli_ip[INET_ADDRSTRLEN];
	struct sockaddr_in* ip_client = (struct sockaddr_in*)&serv_addr;
	inet_ntop(AF_INET, &ip_client->sin_addr, str_cli_ip, INET_ADDRSTRLEN);
	printf("ipclient: %s\n", str_cli_ip );
	char str_cli_port[INET_ADDRSTRLEN];
	printf("port: %d\n", ntohs(ip_client->sin_port));
	while (1) {
		memset(client_buf, 0, 1024);
		recv(confd, client_buf, 1024, 0);
		if(client_buf[0]==0){
			close(sk);
			break;
		}
		printf("client: %s", client_buf);
		memset(server_buf, 0, 1024);
		printf("server: ");
		fgets(server_buf,1024,stdin);
		if(strncmp(server_buf,check,4)==0){
			break;
			close(sk);
		}
		send(confd,server_buf,1024,0);
	}



	return 0;
}
