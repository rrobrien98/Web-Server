#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <pthread.h>

//struct sockaddr_in{
//	short sin_family;
//	u_short sin_port;
//	struct in_addr sin_addr;
//}

//struct in_addr{
//	unsigned long s_addr;
//}

void *process_request(void *arg){//testing git commit
	printf("in thread\n");
	int c_sock = *((int *) arg);
	ssize_t rec_size;
	char request[1024];
	char * response = "Hello Client";//make outside function
	pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

	if((rec_size = recv(c_sock, request,1024,0))<0){
	       printf("receiving error");
	}	       
	printf("%s\n", request);
	send(c_sock, response, 1024, 0);
	close(c_sock);
	pthread_exit(NULL);


}
int main(int argc, char** argv){
	int p_num = 8888;

	char* doc_root;

	int c;
 		
	while ((c = getopt(argc,argv, "p:r:")) != -1){

		switch (c) {

			case 'p':
				p_num = atoi(optarg);
				break;

			case 'r':
				doc_root = optarg;
				break;


			default:
				//unrecognized argumend
				printf("unrecognized argument type");
				break;
		}
	
	}
	int sock_fd;
	if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		printf("error opening socket\n");
		return -1;
	}





	
	struct sockaddr_in myaddr;
	myaddr.sin_family = AF_INET;
	myaddr.sin_port = htons(p_num);
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	//int addrlen = sizeof(myaddr);
	socklen_t addrlen;
	int client_sock;
	if(bind(sock_fd, (struct sockaddr*) &myaddr, sizeof(myaddr)) < 0){
		printf("error binding socket\n");
		return -1;
	}

	if ((listen(sock_fd, 10)) < 0){
		printf("listening error\n");
		return -1;
	}
	printf("listening for connection\n");

	while(1){
		addrlen = sizeof(myaddr);
		printf("listening for connection\n");

		if ((client_sock = accept(sock_fd, (struct sockaddr *)&myaddr, &addrlen)) < 0){
		       printf("error in accepting\n");


		       
		       return -1;
		} 
		printf("made connection\n");	
		pthread_t response;

		if(pthread_create(&response, NULL, process_request, &client_sock ) <0 ){
			printf("thread error\n");
		}		
		
	}
	//pthread_join(response, NULL);
	return 0;
}

