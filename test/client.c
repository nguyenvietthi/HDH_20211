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
	int sk;
	struct ifreq ifr;
	struct sockaddr_in serv_addr;
	char client_buf[1024];
	char server_buf[1024];
	char check[]="exit";
	

	sk = socket(AF_INET, SOCK_STREAM, 0); //AF_INET - IPV4, SOCK_STREAM - TCP
	if (sk < 0) {
		perror("ERROR opening socket");
		exit(1);
	}
// ĐĂNG KÝ SOCKET CHO SN1
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, "sn1", 5);
	if (setsockopt(sk, SOL_SOCKET, SO_BINDTODEVICE, (void*)(&ifr), sizeof(ifr)) < 0) {
		perror("setsocket filed\n");
		exit(1);
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;//IPV4
	serv_addr.sin_port = htons(5001);//PORT
	if (inet_pton(AF_INET, "172.16.100.1", &serv_addr.sin_addr) <= 0) { //ĐỊA CHỈ SERVER
		perror("inet_pton() failed\n");
		exit(1);
	}

	if (connect(sk, (struct sockaddr*)(&serv_addr), sizeof(serv_addr)) < 0) {  // KẾT NỐI CLIENT ĐẾN SERVER
		perror("Connect failed\n");
		exit(1);
	}
	else {
		while(1){
			memset(client_buf, 0, 1024); // lấp đầy 1024byte đầu tiên băng 0
			printf("client: ");
			fgets(client_buf,1024,stdin);// ghi mess từ bàn phi,s
			if(strncmp(client_buf,check,4)==0){
				close(sk);
				break;
			}
			send(sk,client_buf,1024,0);
	
			memset(server_buf, 0, 1024);
			recv(sk, server_buf, 1024, 0); // nhận tin nhắn từ server gửi xuống
			if(server_buf[0]==0){
				close(sk);
				break;
			}
			printf("server: %s", server_buf);
			
		}
	}

	return 0;
}
