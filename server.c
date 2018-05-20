
#include "string_util.h"

#include <assert.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h> 
#include <unistd.h>
#include <pthread.h>
#include <zlib.h>

//Only allow 5 concurrent threads
#define MAXTHREAD (5)
#define windowBits 15
#define GZIP_ENCODING 16

const char* GET = "GET";
const int MAX_CWD = 100;


typedef enum {
        HTML,
        CSS,
		JPG,
		PDF,
		PPTX,
		ERROR
}   filetype_t;



//Error printing helper function
void error(char *message) {
	perror(message);
	exit(1);
}


//First function to read socket message - returns a char buffer
char *readFromSocket(int sockfd) {
	const int bSize = 256;
	char *buffer = malloc(bSize);
	char *result = malloc(1);
	result[0] = '\0';
	int n;
	while (1) {
		int n = read(sockfd,buffer,bSize-1);
		if (n < 0) {
			error("Socket Error: Can't read");
		}
		buffer[n] = '\0';
		char *last_result = result;
		result = strappend(last_result, buffer);
		free(last_result);
		if (n < bSize - 1) {
			break;
		}
	}
	free(buffer);
	buffer = 0;
	return result;
}


//Write a char buffer to Socket
void writeToSocket(int sockfd, const char *message) {
	write(sockfd, message, strlen(message));
	write(sockfd, "\r\n", 2);
}

void writeNumberToSocket(int sockfd, int number) {
	char* message = malloc(3);
	sprintf(message, "%x\r\n", (int)number);
	write(sockfd, message, strlen(message));
}


void writeContentToSocket(int sockfd, const char *content, filetype_t fileType) {
	char lengthStr[100];
	sprintf(lengthStr, "%d", (int)strlen(content));
	char *contentLengthBuffer = concat("Content-Length: ", lengthStr);
	writeToSocket(sockfd, "Server: Linux Mint KDE");
	if(fileType == HTML)
		writeToSocket(sockfd, "Content-Type: text/html");
	else if(fileType == PDF)
		writeToSocket(sockfd, "Content-Type: application/pdf");
	else if(fileType == JPG)
		writeToSocket(sockfd, "Content-Type: image/jpg");
	writeToSocket(sockfd, contentLengthBuffer);
	writeToSocket(sockfd, "");
	writeToSocket(sockfd, content);
	free(contentLengthBuffer);
	contentLengthBuffer = 0;
}


void writeFileToSocket(int client, FILE* file, filetype_t fileType) {

  char buffer[1024];
  long file_size;
  FILE *requested_file = file;
  size_t read_bytes, total_read_bytes;

  char html_response[] = "HTTP/1.1 200 OK:\r\n" "Content-Type: text/html; charset=UTF-8\r\n\r\n";
  char text_response[] = "HTTP/1.1 200 OK:\r\n" "Content-Type: text/plain; charset=UTF-8\r\n\r\n";
  char css_response[] = "HTTP/1.1 200 OK:\r\n" "Content-Type: text/css; charset=UTF-8\r\n\r\n";
  char jpg_response[] = "HTTP/1.1 200 OK:\r\n" "Content-Type: image/jpeg; charset=UTF-8\r\n\r\n";
  char pdf_response[] = "HTTP/1.1 200 OK:\r\n" "Content-Type: application/pdf; charset=UTF-8\r\n\r\n";


  fseek(requested_file, 0, SEEK_END);
  file_size = ftell(requested_file);
  fseek(requested_file, 0, SEEK_SET);


  if (fileType == HTML)
    send(client, html_response, sizeof(html_response)-1, 0);
  
  if ( fileType == JPG)
    send(client, jpg_response, sizeof(jpg_response)-1, 0);
  
  if ( fileType == CSS)
    send(client, css_response, sizeof(css_response)-1, 0);

  if ( fileType == PDF)
    send(client, pdf_response, sizeof(pdf_response)-1, 0);
  
  total_read_bytes = 0;
  while (!feof(requested_file)) {
    read_bytes = fread(buffer, 1, 1024, requested_file);
    total_read_bytes += read_bytes;
    send(client, buffer, read_bytes, 0);
  }
  fclose(requested_file);

}

void writeFileToSocketChunked(int client, FILE* file, filetype_t fileType) {

  char buffer[1024];
  long file_size;
  FILE *requested_file = file;
  size_t read_bytes, total_read_bytes;

  char html_response[] = "HTTP/1.1 200 OK:\r\n" "Transfer-Encoding: chunked\r\n" "Content-Type: text/html; charset=UTF-8\r\n\r\n";
  char text_response[] = "HTTP/1.1 200 OK:\r\n" "Transfer-Encoding: chunked\r\n" "Content-Type: text/plain; charset=UTF-8\r\n\r\n";
  char css_response[] = "HTTP/1.1 200 OK:\r\n" "Transfer-Encoding: chunked\r\n" "Content-Type: text/css; charset=UTF-8\r\n\r\n";
  char jpg_response[] = "HTTP/1.1 200 OK:\r\n" "Transfer-Encoding: chunked\r\n" "Content-Type: image/jpeg; charset=UTF-8\r\n\r\n";
  char pdf_response[] = "HTTP/1.1 200 OK:\r\n" "Transfer-Encoding: chunked\r\n" "Content-Type: application/pdf; charset=UTF-8\r\n\r\n";
  char pptx_response[] = "HTTP/1.1 200 OK:\r\n" "Transfer-Encoding: chunked\r\n" "Content-Type: application/vnd.openxmlformats-officedocument.presentationml.presentation; charset=UTF-8\r\n\r\n";


  fseek(requested_file, 0, SEEK_END);
  file_size = ftell(requested_file);
  fseek(requested_file, 0, SEEK_SET);


  if (fileType == HTML)
    send(client, html_response, sizeof(html_response)-1, 0);
  
  if ( fileType == JPG)
    send(client, jpg_response, sizeof(jpg_response)-1, 0);
  
  if ( fileType == CSS)
    send(client, css_response, sizeof(css_response)-1, 0);

  if ( fileType == PDF)
    send(client, pdf_response, sizeof(pdf_response)-1, 0);

  if ( fileType == PPTX)
    send(client, pptx_response, sizeof(pptx_response)-1, 0);
  
  total_read_bytes = 0;
  while (!feof(requested_file)) {
    read_bytes = fread(buffer, 1, 1024, requested_file);
    total_read_bytes += read_bytes;
	writeNumberToSocket(client, read_bytes);
    write(client, buffer, read_bytes);
	write(client, "\r\n", 2);
  }
  write(client, "0\r\n\r\n", 5);
  fclose(requested_file);
}


void writeFileToSocketWithCompression(int client, FILE* file, filetype_t fileType) {

  char buffer[1024];
  long file_size;
  FILE *requested_file = file;
  size_t read_bytes, total_read_bytes;

  char html_response[] = "HTTP/1.1 200 OK:\r\n" "Content-Encoding: gzip\r\n" "Content-Type: text/html; charset=UTF-8\r\n\r\n";
  char css_response[] = "HTTP/1.1 200 OK:\r\n" "Content-Encoding: gzip\r\n" "Content-Type: text/css; charset=UTF-8\r\n\r\n";


  fseek(requested_file, 0, SEEK_END);
  file_size = ftell(requested_file);
  fseek(requested_file, 0, SEEK_SET);


  if (fileType == HTML)
    send(client, html_response, sizeof(html_response)-1, 0);
  
  if ( fileType == CSS)
    send(client, css_response, sizeof(css_response)-1, 0);

  
  char* encryptedBuffer;
  total_read_bytes = 0;
  z_stream defstream;
  defstream.zalloc = Z_NULL;
  defstream.zfree = Z_NULL;
  defstream.opaque = Z_NULL;
  
  
  
  while (!feof(requested_file)) {
    read_bytes = fread(buffer, 1, 1024, requested_file);
    total_read_bytes += read_bytes;
    defstream.next_in = (Bytef *)buffer; // input char array
	defstream.avail_in = read_bytes; // size of input, string + terminator
	defstream.avail_out = read_bytes; // size of output
	encryptedBuffer = malloc(read_bytes);
	defstream.next_out = (Bytef *)encryptedBuffer; // output char array
	deflateInit2(&defstream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
              windowBits | GZIP_ENCODING,
              8,
              Z_DEFAULT_STRATEGY);
	deflate(&defstream, Z_FINISH);
	deflateEnd(&defstream);
    send(client, encryptedBuffer, (char*)defstream.next_out - encryptedBuffer, 0);
	free(encryptedBuffer);
  }
  fclose(requested_file);
}



void sendHTTP404Error(int sockfd) {
	writeToSocket(sockfd, "HTTP/1.1 404 Not Found");
	static const char *content = "<html><body><h1>404 Error: Requested content not found</h1></body></html>\r\n";
	writeContentToSocket(sockfd, content, HTML);
}

void sendHTTPGetReply(int sockfd, FILE* file, filetype_t fileType) {
	writeToSocket(sockfd, "HTTP/1.1 200 OK");
	writeFileToSocketChunked(sockfd, file, fileType);
}

void sendHTTPGetEncryptedReply(int sockfd, FILE* file, filetype_t fileType) {
	writeToSocket(sockfd, "HTTP/1.1 200 OK");
	writeFileToSocketWithCompression(sockfd, file, fileType);
}

int checkIfGet(char *buffer) {
	return starts_with(buffer, GET);
}

int checkIfGzipDeflate(char *buffer){
	return contains(buffer, "Accept-Encoding: gzip, deflate");
}

char *extractPath(char *buffer) {
	int startPos = strlen(GET) + 1;
	char *end_of_path = strchr(buffer + startPos, ' ');
	int end_pos = end_of_path - buffer;
	int pathlen = end_pos - startPos;
	char *path = malloc(pathlen + 1);
	substr(buffer, startPos, pathlen, path);
	path[pathlen] = '\0';
	return path;
}

filetype_t extractFileType(char *path) {
	char* tempBuffer;
	int typeStartPos;
	for(int i=0; i<strlen(path)-1; i++) {
		if(path[strlen(path)-i] == '.') {
			tempBuffer = malloc(i);
			memcpy(tempBuffer, &path[strlen(path)-i+1], i-1);
			tempBuffer[i-1] = '\0';
			break; 
		}
		else if(i == strlen(path) - 2){
			return ERROR;
		}
	}
	if(!strcmp(tempBuffer, "html"))
		return HTML;
	else if (!strcmp(tempBuffer, "pdf"))
		return PDF;
	else if (!strcmp(tempBuffer, "jpg"))
		return JPG;
	else if (!strcmp(tempBuffer, "pptx"))
		return PPTX;
	else if (!strcmp(tempBuffer, "css"))
		return CSS;
	else
		return ERROR;
	free(tempBuffer);
}


void outputStaticFile(int sockfd, const char *curdir, const char *path, int gzipflag) {
	char* fullpath = malloc(strlen(curdir) + strlen(path) + 1);
	strcpy(fullpath, curdir);
	strcat(fullpath, path);
	printf("Opening static file: [%s]\n", fullpath);
	filetype_t fileType = extractFileType(fullpath);
	if (fileType == ERROR) {
		perror("Filetype not supported");
		sendHTTP404Error(sockfd);
	}
	FILE *f = fopen(fullpath, "r");
	if (!f) {
		perror("Problem with fopen");
		sendHTTP404Error(sockfd);
	} else {
		if(gzipflag && fileType == HTML) {
			printf("Encrypted HTML serving\n");
			sendHTTPGetEncryptedReply(sockfd, f, fileType);
		}
		else
			sendHTTPGetReply(sockfd, f, fileType);
	}
}

void *handleSocket(void* sockfd_arg) {
	int sockfd = *((int *)sockfd_arg);
	int gzipflag;
	printf("Now socket: %d\n", sockfd);
	char *text = readFromSocket(sockfd);
	printf("Text from socket: %s\n\n", text);
	if (checkIfGet(text)) {
		char curdir[MAX_CWD];
		if (!getcwd(curdir, MAX_CWD)) {
			error("Couldn't read curdir");
		}
		gzipflag = checkIfGzipDeflate(text);
		char *path = extractPath(text);
		outputStaticFile(sockfd, curdir, path, gzipflag);
		free(path);
	} else {
		sendHTTP404Error(sockfd);
	}
	free(text);
	close(sockfd);
	free(sockfd_arg);
	return NULL;
}

int createNewSocket() {
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		error("ERROR opening socket");
	}
	int setopt = 1;
	if (-1 == setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&setopt, sizeof(setopt))) {
		error("ERROR setting socket options");
	}
	struct sockaddr_in serv_addr;
	uint16_t port = 8000;
	while (1) {
		bzero(&serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(port);
		serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
			port++;
		} else {
			break;
		}
	}
	if (listen(sockfd, SOMAXCONN) < 0) error("Couldn't listen");
	printf("Running on port: %d\n", port);
	return sockfd;
}

int main() {
	int sockfd = createNewSocket();
	struct sockaddr_in client_addr;
	int cli_len = sizeof(client_addr);
	int threads_count = 0;
	pthread_t threads[MAXTHREAD];
	while (1) {
		int newsockfd = accept(sockfd, (struct sockaddr *) &client_addr, (socklen_t *) &cli_len);
		if (newsockfd < 0) error("Error on accept");
		printf("New socket: %d\n", newsockfd);
		int *arg = malloc(sizeof(int));
		*arg = newsockfd;
		if (pthread_create(&threads[threads_count], NULL, handleSocket, arg)) {
			printf("Error when creating thread %d\n", threads_count );
			return 0;
		}
	if (++threads_count >= MAXTHREAD) {
			break;
		}
	}
	printf("Max thread number reached, wait for all threads to finish and exit...\n");
	for (int i = 0; i < MAXTHREAD; ++i) {
		pthread_join(threads[i], NULL);
	}
	close(sockfd);
	return 0;
}
