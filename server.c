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
#define MAX_WAIT 12
/**
 * Project 1: Web Server
 *
 * <Russell O'Brien, Vincent Dong>
 *
 * server.c
 *
 * This is a program to run a simple web server. The port number for the server and the document root
 * out of which files will be served are specified by the user. This server supports HTTP 1.0 and 1.1
 * requests, and supports the file types txt, html, png, and gif. To connect to this server, run 
 * turing.bowdoin.edu:(chosen port number)/(desired file) on a web browser while server is running.
 */

char* doc_root;
int num_connections;
pthread_mutex_t lock;


/**
 * process_request
 *
 * This function is called by every new thread that is created to handle an incoming request from
 * a client. This function handles the request by parsing the HTTP formatted command and its headers, 
 * and then opening and transmitting the appropriate file(if it exists) through the socket, accompanied
 * by the appropriate HTTP response headers
 *
 * Parameters:
 * arg: the socket number of the client
 *
 * Return:
 * NULL
 *
 */
 
void *process_request(void *arg){	
	pthread_mutex_lock(&lock);
	int file_num;
	off_t offset;
	char droot_copy[50];
	
	if(doc_root == NULL){
		doc_root = "/home/rrobrien/Distributed/server";
	}	
	strcpy(droot_copy, doc_root);	
	
	int c_sock = *((int *) arg);
	int rec_size;
	long fsize = 0;
	int proto;
	int keep_open = 1;
	
	//this loop processes requests repeatedly until the socket is closed
	while(keep_open){
		FILE *fp;
		char request[1024] = {0};
		int keep_requesting = 1;
		rec_size = 0;
		char* end_test;
		
		//read one byte at a time until full request received
		while(keep_requesting){
			if ((rec_size += recv(c_sock, request + rec_size,1,0)) < 0){
		       		printf("receiving error\n");
			}
			
			end_test = strstr(request, "\r\n\r\n");
			
			if(end_test != NULL){         
			         keep_requesting = 0;
			}
			
			//close socket if recv timed out
			if(!strcmp(request,"")){
				close(c_sock);
				num_connections--;
				pthread_mutex_unlock(&lock);
				printf("Socket closed\n");
				return NULL;
			}
		}

		
		//extract key data from request
		char* point_name = strstr(request, "GET /");
		char* point_type = strstr(request, ".");
		char* point_proto = strstr(request, " HTTP/1.");
    		char* point_connect = strstr(request, "Connection: ");
    		char* point_host = strstr(request, "Host: ");
    		 

   		if (point_host != NULL){
		       point_host += 6;
		}	       
   		if (point_connect != NULL){
		       point_connect += 12;
		}	       
   		if (point_proto != NULL){
		       point_proto += 8;
		}	       
   		if (point_type != NULL){
		       point_type += 1;
		}	       
   		if (point_name != NULL){
		       point_name += 5;
		}	       
		
		char type[50] = {0};
    		char file[50] = {0}; 
    		int valid_host = 1;
    		
		//check if user specified host matches our server, if not, bad request
		if(point_host != NULL){
			if(strncmp(point_host, "139.140.235.145", 15) 
					&& strncmp(point_host, "turing.bowdoin.edu", 18)){
        			valid_host = 0;
      			}
    		}
    		
		char* response_code;
		
		//if response is valid, extract the file information
		if (point_name != NULL && point_proto != NULL && valid_host){
			
			//check if type of connection specified, if not, default to protocol
			if(point_connect == NULL){
        			proto = *point_proto - '0';
				keep_open = proto;
      			}
			else{
        			if(!strncmp(point_connect, "keep-alive", 10)){
          				keep_open = 1;
      				}
        			else{
          				keep_open = 0;
        			}
      			}
        
			
			if (point_type == NULL || (point_type >= point_proto) ){
				strcpy(file , "index\0");
				strcpy(type , "html\0");
			}
			
			
			
			else{		
				strncpy(file, point_name, (point_type-1) - point_name);
				strncpy(type, point_type, (point_proto - 8) -point_type);	
			}
			
			//make filename into absolute path	
			strcat(droot_copy, "/");
			strcat(droot_copy, file);
			strcat(droot_copy, ".");
			strcat(droot_copy, type);
				
		
			//try to open file, set response code accordingly
			if((fp = fopen(droot_copy, "rb")) == NULL){
				printf("File could not be opened\n");
				if(errno == 2){
					response_code = " 404 Not Found";
				}
				else if(errno == 13){
					response_code = " 403 Forbidden";
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
			response_code = " 400 Bad Request";
			keep_open=0;
		}			
		
		//get the time/date the response will be sent
		time_t date = time(0);
		char response_time[1000];
		struct tm tm = *gmtime(&date);
		strftime(response_time, sizeof(response_time), 
				"%a, %d %b %Y %H:%M:%S %Z", &tm);
		
		char size[10];
		char protocol[10];
		sprintf(size, "%li", fsize);
		sprintf(protocol, "%d", proto);
 
		char response[1000]= "HTTP/1.";
		
		//construct the headers for the request into one string		
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

    			
		size_t to_send = strlen(response);
		int bytes_sent = 0;
		
		//send the entire contents of the response header string
		while(to_send){
			if((bytes_sent += send(c_sock,response+bytes_sent, to_send,0)) < 0){
        			printf("sending error\n");
      			}
			to_send = to_send - bytes_sent;
		}
		
		//if file given was opened, send the file data
		if(file_num){
			bytes_sent = 0;
			to_send = fsize;
			
			while(to_send){	
				if ((bytes_sent += sendfile(c_sock, 
								file_num, &offset, to_send)) < 0){
          				printf("sending error\n");
        			}
				offset = bytes_sent;
				to_send = to_send - bytes_sent;
			}
			fclose(fp);
		}	
		

		

		//clear strings and reset doc_root
		strcpy(droot_copy, "");
		strcpy(droot_copy,doc_root);
		
		point_type = NULL;
		point_proto = NULL;
		point_name = NULL;
		point_connect = NULL;
		point_host = NULL;
		
		//set timeout for recv if request dictates connection will stay open
		if (keep_open){
			struct timeval tv;
			
			//sockets stay open for some constant time minus # of open connections
			if (num_connections < MAX_WAIT){
				tv.tv_sec = MAX_WAIT - num_connections;
			
			}
			else{
				tv.tv_sec = 1;
			}
			
			tv.tv_usec = 0;
			
			int timeout_stat = setsockopt(c_sock, SOL_SOCKET, 
					SO_RCVTIMEO,(char *)&tv, sizeof(struct timeval));
			
			if (timeout_stat < 0){
				printf("Timeout error\n");
			}		
			
		}
		
	}

	close(c_sock);
  	printf("Socket closed\n");
	num_connections--;
	pthread_mutex_unlock(&lock);
  	pthread_exit(NULL);
	return NULL;
}
/**
 * main
 *
 * reads command line arguments and establishes socket connection accordingly
 * creates a new thread for each connection
 *
 */
int main(int argc, char** argv){
	int p_num = 8888;	
	int c;
 	
	//parse command line args
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

	//infinite loop waits for clients to connect to server
	while(1){
		addrlen = sizeof(myaddr);
		printf("Listening for connection\n");

		if ((client_sock = accept(sock_fd, 
						(struct sockaddr *)&myaddr, &addrlen)) < 0){
		       printf("error in accepting\n");
		       return -1;
		} 
		
		printf("Made connection\n");	
		pthread_t response;
		num_connections++;
		
		if(pthread_create(&response, NULL, process_request, &client_sock ) <0 ){
			printf("thread error\n");
		}		
		
	}
	
	
	return 0;
}

