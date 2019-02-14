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


//	unsigned long s_addr;
//}
char* doc_root;

void *process_request(void *arg){
	//char* file_data;
	char droot_copy[50];
	strcpy(droot_copy, doc_root);	
	int c_sock = *((int *) arg);
	int rec_size;
	//char request[1024];
	long fsize = 0;
	//pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
	int proto;
	//char* point_name;
	//char* point_type;
	//char* point_proto;
	//char* file;
	int keep_open = 1;
	while(keep_open){
		char request[1024] = {0};
		char file_data[5000] = {0};
		//char* file_data;
		int keep_requesting = 1;
		//printf("waiting for message\n");
		rec_size = 0;
		while(keep_requesting){
			printf("waiting for message\n");	
			//printf("request buffer %s\n", request);
			if ((rec_size += recv(c_sock, request + rec_size,1024,0)) < 0){
		       		//printf("receiving error");
			}
			//if(!rec_size){
			//	keep_requesting = 0;
			//}
			keep_requesting = 0;
			printf("%s\n",request);
			if (!strcmp((request + rec_size -4), "\r\n\r\n\0")){
				printf("checked for returns\n");
				keep_requesting = 0;
			}
			if(!strcmp(request,"")){
				close(c_sock);

				printf("socket closed\n");
				return NULL;
			}
		}

		printf("request: \n%s", request);

		char* point_name = strstr(request, "GET /") + 5;
		char* point_type = strstr(request, ".") + 1;
		char* point_proto = strstr(request, " HTTP/1.") + 8;
		char file[50] = {0}; //= (char*) malloc(100 * sizeof(char));
		//char* file = "";
		//char* type = "";
		char type[50] = {0};// = (char*) malloc(100 * sizeof(char));
				
		char* response_code;
		if (point_name != NULL && point_proto != NULL){
			proto = *point_proto - '0';
			keep_open = proto;
			if (point_type == NULL || (point_type >= point_proto) ){
				strcpy(file , "index\0");
				strcpy(type , "html\0");
			}
			//printf("name: \n",point_name);
			
			
			else{
				printf("got here\n");	
				//char* file_copy = file;
				//char* i = point_name;
				//file = "";
				strncpy(file, point_name, (point_type-1) - point_name);
				printf("copiend file\n");
				//strcat(file, "\0");
				//for ( ; i != (point_type -1); i++){
				//
					//printf("letter copied %s\n",file); 	
				//	*file_copy = *i;
				//	file_copy++;
				
				//	printf("letter copied %c %s\n",*point_name,file); 	
				
				//}
				//type = "";
				strncpy(type, point_type, (point_proto - 8) -point_type);
				//strcat(type, "\0");
				//char* type_copy = type;
				//char* j = point_type;
				//for ( ; j != (point_proto -8); j++){

				//	*type_copy = *j;
	
				//	type_copy++;
				
			}
			printf("file, type, proto: %s, %s, %s\n", file, type, point_proto);
				
			strcat(droot_copy, "/");
			strcat(droot_copy, file);
			strcat(droot_copy, ".");
			strcat(droot_copy, type);
			printf("address: %s\n", droot_copy);
		
			FILE *fp;
			//char* file_data;
			if((fp = fopen(droot_copy, "rb")) == NULL){
				//printf("errno %d\n", errno);
				printf("error opening file\n");
				if(errno == 2){
					response_code = " 404 Not Found\0";
				}
				else if(errno == 13){
					response_code = " 403 Forbidden\0";
				}
				//file_data = "";
			}
			else{
				response_code = " 200 OK";
				fseek(fp, 0 , SEEK_END);
				fsize = ftell(fp);
				fseek(fp, 0, SEEK_SET);
				//char file_contents[fsize+1];// = (char*) malloc(fsize + 1);
				//rewind(fp);
				fread(file_data, fsize, 1, fp);
				fclose(fp);
				
				printf("read data\n");
				//file_data[fsize] = '\0';
						
				//file_data = file_contents;
				file_data[fsize] = '\0';
				//strcat(file_data, "\0");
				//strcpy(file_data, file_contents);
				printf("file data %s\n", file_data);	
				printf("file length %li\n", strlen(file_data));
			}
		}
		else{
			proto = 0;
			//file_data = ""; 
			response_code = " 400 Bad Request\n";
			keep_open=0;
		}			
		printf("file data outside %s\n", file_data);
		printf("size of file %lu\n", strlen(file_data));
		time_t date = time(0);
		char response_time[1000];
		struct tm tm = *gmtime(&date);
		strftime(response_time, sizeof(response_time), "%a, %d %b %Y %H:%M:%S %Z", &tm);
		strcat(response_time, "\0");
	
			//send(c_sock, response, 1024, 0);
		char size[10];
		char protocol[10];

		sprintf(size, "%li", fsize);
		sprintf(protocol, "%d", proto);

		int response_len = 7 + 1 + strlen(response_code) +1 + 7 + strlen(response_time) + 1 + 16 + strlen(size) + 1 + 14 + strlen(type)+ 2 + fsize + 1;
		 
		char response[response_len];
		
		printf("size of file %lu\n", strlen(file_data));
		

		strcpy(response, "");	
	
		strcat(response, "HTTP/1.");
	
		strcat(response,  protocol);
		
		strcat(response, response_code);
		strcat(response, "\n");
		strcat(response, "Date : ");
		strcat(response, response_time);
		strcat(response, "\n");
		strcat(response, "Content-Length: ");
	
		strcat(response, size);
		
		strcat(response, "\n");//send file
		strcat(response, "Content-Type: ");
		strcat(response, type);
		strcat(response, "\n\n");
		strcat(response, file_data);
		strcat(response, "\0");
		printf("response : %s\n",response);
		//printf("response end:\n %d", *(response +strlen(response) -2) - '0');
		int keep_sending = 1;
		while(keep_sending){
			int sent = 0;
			printf("sending\n");
			if((sent += send(c_sock, response + sent, strlen(response), 0)) < 0){
				printf("error sending\n");
			}
			if(sent == strlen(response)){
			//if(!strcmp(response + sent, "")){
				keep_sending = 0;
			//	printf("response so far%s\n", response);
				printf("stopped sending\n");
			}
		}
		memset(file_data,0, 5000);
		///memset(file_data,"");
		//free(file_data);
		memset(response, 0, response_len); 
		//strcpy(response, "");
		//free(response);
		memset(file,0,50);
		//strcpy(file, "");
		//free(file);
		memset(type,0,50);
		//strcpy(type, "");
		//free(type);
		memset(request, 0 , 1024);
		//strcpy(request, "");
		strcpy(droot_copy, "");
		strcpy(droot_copy,doc_root);
		printf("docroot %s\n", droot_copy);
		point_type = NULL;
		point_proto = NULL;
		point_name = NULL;
		
		if (keep_open){
			struct timeval tv;
			tv.tv_sec = 7;
			tv.tv_usec = 0;
			int timeout_stat = setsockopt(c_sock, SOL_SOCKET, SO_RCVTIMEO,(char *)&tv, sizeof(struct timeval));//test w/ telnet
			printf("timeout worked:%d, %d\n", timeout_stat, errno);
			//int test = recv(c_sock, request, 1024,0);
			//printf("shouldnt get here, %d\n", test);
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

