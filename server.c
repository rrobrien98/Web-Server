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
#include <sys/sendfile.h>



char* doc_root;
int num_connections;
pthread_mutex_t lock;

void *process_request(void *arg){
	
	pthread_mutex_lock(&lock);
	int file_num;
	off_t offset;
	char droot_copy[50];
	strcpy(droot_copy, doc_root);	
	int c_sock = *((int *) arg);
	int rec_size;
	
	long fsize = 0;
	
	int proto;
	
	
	
	
	int keep_open = 1;
	while(keep_open){
		FILE *fp;
		char request[1024] = {0};
		
		
		int keep_requesting = 1;
		
		rec_size = 0;
		while(keep_requesting){
				
			
			if ((rec_size += recv(c_sock, request + rec_size,1,0)) < 0){
		       		printf("receiving error");
			}
			
			
			
			
			
			char * end_test;
	 		if((end_test = strstr(request, "\r\n\r\n")) != NULL){
			         printf("checked for returns\n");
			         keep_requesting = 0;
			}
			if(!strcmp(request,"")){
				close(c_sock);
				num_connections--;
				pthread_mutex_unlock(&lock);
				printf("socket closed\n");
				return NULL;
			}
		}

		printf("request: \n%s", request);

		char* point_name = strstr(request, "GET /") + 5;
		char* point_type = strstr(request, ".") + 1;
		char* point_proto = strstr(request, " HTTP/1.") + 8;
    char* point_connect = strstr(request, "Connection: ") + 12;
    char* point_host = strstr(request, "Host: ") + 6;
    char file[50] = {0}; 
		
    
    
		char type[50] = {0};
    int valid_host = 1;
    if(point_host != NULL){
      if(strncmp(point_host, "139.140.235.145", 15) && strncmp(point_host, "turing.bowdoin.edu", 18)){
        valid_host = 0;
      }
    }
    		
		char* response_code;
		if (point_name != NULL && point_proto != NULL && valid_host){
			if(point_connect == NULL){
        proto = *point_proto - '0';
      }
      else{
        if(!strncmp(point_connect, "keep_alive", 10)){
          proto = 1;
        }
        else{
          proto = 0;
        }
      }
        
			keep_open = proto;
			if (point_type == NULL || (point_type >= point_proto) ){
				strcpy(file , "index\0");
				strcpy(type , "html\0");
			}
			
			
			
			else{
					
				
				
				
				strncpy(file, point_name, (point_type-1) - point_name);
				
				
				
				 	
				
				
				
				 	
				
				
				strncpy(type, point_type, (point_proto - 8) -point_type);
				
				
				
				

				
	
				
				
			}
			printf("file, type, proto: %s, %s, %s\n", file, type, point_proto);
				
			strcat(droot_copy, "/");
			strcat(droot_copy, file);
			strcat(droot_copy, ".");
			strcat(droot_copy, type);
			printf("address: %s\n", droot_copy);
		
			
			
			if((fp = fopen(droot_copy, "rb")) == NULL){
				
				printf("error opening file\n");
				if(errno == 2){
					response_code = " 404 Not Found\0";
				}
				else if(errno == 13){
					response_code = " 403 Forbidden\0";
				}
				
				
				
			}
			else{
				response_code = " 200 OK";
				fseek(fp, 0 , SEEK_END);
				fsize = ftell(fp);
				fseek(fp, 0, SEEK_SET);
				
				file_num = fileno(fp);
				offset = 0;
				 
				
									
				
				
						
				
				
				
				
					
				
			}
		}
		else{
			proto = 0;
			 
			response_code = " 400 Bad Request\n";
			keep_open=0;
		}			
		
		
		time_t date = time(0);
		char response_time[1000];
		struct tm tm = *gmtime(&date);
		strftime(response_time, sizeof(response_time), "%a, %d %b %Y %H:%M:%S %Z", &tm);
		
	
		
		char size[10];
		char protocol[10];

		sprintf(size, "%li", fsize);
		sprintf(protocol, "%d", proto);

		
		 
		char response[1000]= "HTTP/1.";
		
		
		strcat(response,  protocol);
		
		strcat(response, response_code);
		strcat(response, "\n");
		strcat(response, "Date: ");
		strcat(response, response_time);
		strcat(response, "\n");
		strcat(response, "Content-Length: ");
	
		strcat(response, size);
		
		strcat(response, "\n");
		strcat(response, "Content-Type: ");
		strcat(response, type);
		strcat(response, "\n\n");
		
		strcat(response, "\0");

    printf("response %s,%d\n", response, (int) strlen(response));	
		size_t to_send = strlen(response);
		int bytes_sent = 0;
		while(to_send){
			if((bytes_sent += send(c_sock,response+bytes_sent, to_send,0)) < 0){
        printf("sending error\n");
      }
			to_send = to_send - bytes_sent;
		}
		
		if(file_num){
			
			bytes_sent = 0;
			to_send = fsize;
			while(to_send){	
				if ((bytes_sent += sendfile(c_sock, file_num, &offset, to_send)) < 0){
          printf("sending error\n");
        }
				offset = bytes_sent;
				to_send = to_send - bytes_sent;
			}
			fclose(fp);
		}	
		

		

		
		strcpy(droot_copy, "");
		strcpy(droot_copy,doc_root);
		printf("docroot %s\n", droot_copy);
		point_type = NULL;
		point_proto = NULL;
		point_name = NULL;
		
		if (keep_open){
			struct timeval tv;
			if (num_connections < 7){

				tv.tv_sec = 7-num_connections;
			}
			else{
				tv.tv_sec = 1;
			}
			tv.tv_usec = 0;
			int timeout_stat = setsockopt(c_sock, SOL_SOCKET, SO_RCVTIMEO,(char *)&tv, sizeof(struct timeval));//test w/ telnet
			printf("timeout worked:%d, %d\n", timeout_stat, errno);
			
		}
		
	}
	close(c_sock);
  num_connections--;
	pthread_mutex_unlock(&lock);
  pthread_exit(NULL);
 
	
	return NULL;
}
int main(int argc, char** argv){
	int p_num = 8888;

	

	int c;
 	if (pthread_mutex_init(&lock, NULL) != 0){
        	printf("\n mutex init failed\n");
        	return -1;
    	}
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
	
	
	return 0;
}

