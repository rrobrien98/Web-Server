#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <errno.h>

//struct sockaddr_in{
//	short sin_family;
//	u_short sin_port;
//	struct in_addr sin_addr;
//}

//struct in_addr{
//	unsigned long s_addr;
//}
char* doc_root;

void *process_request(void *arg){
	char* file_data;
	char droot_copy[50];
	strcpy(droot_copy, doc_root);	
	int c_sock = *((int *) arg);
	int rec_size = 0;
	char request[1024];
	long fsize = 0;
	pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
	int proto;
	char* point_name;
	char* point_type;
	char* point_proto;
	char* file;
	int keep_open = 1;
	while(keep_open){
		int keep_requesting = 1;
		//printf("waiting for message\n");
		while(keep_requesting){
			printf("waiting for message\n");	
			rec_size += recv(c_sock, request+rec_size,1024,0);
		       		//printf("receiving error");
			
			if (!strcmp((request + rec_size -4), "\r\n\r\n\0")){
				keep_requesting = 0;
			}
		}

		printf("request: \n%s", request);

		point_name = strstr(request, "GET /") + 5;
		point_type = strstr(request, ".") + 1;
		point_proto = strstr(request, " HTTP/1.") + 8;
		file = (char*) malloc(100 * sizeof(char));

		char* type = (char*) malloc(100 * sizeof(char));
	
		char* response_code;
		if (point_name != NULL && point_proto != NULL){
			proto = *point_proto - '0';
			keep_open = proto;
			if (point_type == NULL || (point_type >= point_proto) ){
				file = "index";
				type = "html";
			}
			else{
				char* file_copy = file;
				for ( ; point_name != (point_type -1); point_name++){
				
	
					*file_copy = *point_name;
					file_copy++;
				}
				char* type_copy = type;
				char* j = point_type;
				for ( j; j != (point_proto -8); j++){

					*type_copy = *j;
	
					type_copy++;
				}
			}
			//printf("%s, %s, %s\n", file, type, point_proto);
				
			strcat(droot_copy, "/");
			strcat(droot_copy, file);
			strcat(droot_copy, ".");
			strcat(droot_copy, type);
			printf("address: %s\n", droot_copy);
		
			FILE *fp;
			if((fp = fopen(droot_copy, "r")) == NULL){
				printf("error opening file\n");
				if(errno == 2){
					response_code = " 404 Not Found\n";
				}
				else if(errno == 13){
					response_code = " 403 Forbidden\n";
				}
				file_data = "";
			}
			else{
				response_code = " 200 OK\n";
				fseek(fp, 0 , SEEK_END);
				fsize = ftell(fp);
				fseek(fp, 0, SEEK_SET);
				file_data = (char*) malloc(fsize + 1);
	
				fread(file_data, fsize, 1, fp);
				fclose(fp);
	
				file_data[fsize] = 0;
			
			}
		}
		else{

			response_code = " 400 Bad Request\n";
			keep_open=0;
		}			
		time_t date = time(0);
		char response_time[1000];
		struct tm tm = *gmtime(&date);
		strftime(response_time, sizeof(response_time), "%a, %d %b %Y %H:%M:%S %Z", &tm);
		
	
			//send(c_sock, response, 1024, 0);



	
		char* response = (char*) malloc( sizeof(char) * 1000);
		char size[10];
		char protocol[10];

		sprintf(size, "%li", fsize);
		sprintf(protocol, "%d", proto);
	
		strcat(response, "HTTP/1.");
	
		strcat(response,  protocol);
	
		strcat(response, response_code);
		strcat(response, "Date : ");
		strcat(response, response_time);
		strcat(response, "\n");
		strcat(response, "Content-Length: ");
	
		strcat(response, size);
		
		strcat(response, "\n");
		strcat(response, "Content-Type: ");
		strcat(response, type);
		strcat(response, "\n\n");
		strcat(response, file_data);
		//printf("response:\n %s", response);
		send(c_sock, response, 1000, 0);
	
		free(file_data);
		free(response);
		free(file);
		free(type);
		strcpy(request, "");
		strcpy(droot_copy, "");
		strcpy(droot_copy,doc_root);
		printf("docroot %s\n", droot_copy);
		strcpy(point_type, "");
		strcpy(point_proto, "");
		strcpy(point_name, "");
		if (keep_open){
			struct timeval tv;
			tv.tv_sec = 1;
			tv.tv_usec = 0;
			int timeout_stat = setsockopt(c_sock, SOL_SOCKET, SO_RCVTIMEO,(char *)&tv, sizeof(struct timeval));
			printf("timeout worked:%d, %d\n", timeout_stat, errno);
			int test = recv(c_sock, request, 1024,0);
			printf("shouldnt get here, %d\n", test);
		}
		
	}
	close(c_sock);
	pthread_exit(NULL);
 

	return NULL;
}
int main(int argc, char** argv){
	int p_num = 8888;

	

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

