/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the 
 *     GET method to serve static and dynamic content.
 */

#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>

#include "interface.h"
#include "sio.h"

#define MAX_LINE    8192        /* 8KB line buffer */
#define MAX_BUF     1048576     /* 1MB buffer size */

typedef struct sockaddr SA;

extern char **environ; /* Defined by libc */

void
serve_client(int fd);

void
read_requestheaders(Sio *sio);

int 
parse_uri(char *uri, char *filename, char *cgiargs);

void 
serve_static(int fd, char *filename, int filesize);

void 
get_file_type(char *filename, char *filetype);

void 
serve_dynamic(int fd, char *filename, char *cgiargs);

void 
client_error(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg);

int
main(int argc, char **argv) 
{
    int listenfd, connfd;
    char hostname[MAX_LINE], port[MAX_LINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    /* Check command line args */
    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(1);
    }

    listenfd = open_listenfd(argv[1]);
    while (1) {
	clientlen = sizeof(clientaddr);
	connfd = accept(listenfd, (SA *)&clientaddr, &clientlen); 
        getnameinfo((SA *) &clientaddr, clientlen, hostname, MAX_LINE, 
                    port, MAX_LINE, 0);
        printf("accepted connection from (%s, %s)\n", hostname, port);
	serve_client(connfd);                                             
	close(connfd);                                            
    }
}


/*
 * serve_client - handle one HTTP request/response transaction
 */
void
serve_client(int fd) 
{
    int is_static;
    struct stat sbuf;
    char buf[MAX_LINE], method[MAX_LINE], uri[MAX_LINE], version[MAX_LINE];
    char filename[MAX_LINE], cgiargs[MAX_LINE];
    Sio sio;

    /* Read request line and headers */
    sio_initbuf(&sio, fd);
    if (!sio_read_line(&sio, buf, MAX_LINE))  
        return;
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);       
    if (strcasecmp(method, "GET")) {                    
        client_error(fd, method, "501", "Not Implemented",
                    "Tiny does not implement this method");
        return;
    }                                                  
    read_requestheaders(&sio);                        

    /* Parse URI from GET request */
    is_static = parse_uri(uri, filename, cgiargs);   
    if (stat(filename, &sbuf) < 0) {                
	client_error(fd, filename, "404", "Not found",
		    "Tiny couldn't find this file");
	return;
    }                                              

    if (is_static) { /* Serve static content */          
	if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
	    client_error(fd, filename, "403", "Forbidden",
			"Tiny couldn't read the file");
	    return;
	}
	serve_static(fd, filename, sbuf.st_size);        
    }
    else { /* Serve dynamic content */
	if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) { 
	    client_error(fd, filename, "403", "Forbidden",
			"Tiny couldn't run the CGI program");
	    return;
	}
	serve_dynamic(fd, filename, cgiargs);            
    }
}

/*
 * read_requestheaders - read HTTP request headers
 */
void
read_requestheaders(Sio *sio) 
{
    char buf[MAX_LINE];

    sio_read_line(sio, buf, MAX_LINE);
    printf("%s", buf);
    while(strcmp(buf, "\r\n")) {          
	sio_read_line(sio, buf, MAX_LINE);
	printf("%s", buf);
    }
    return;
}

/*
 * parse_uri - parse URI into filename and CGI args
 *             return 0 if dynamic content, 1 if static
 */
int
parse_uri(char *uri, char *filename, char *cgiargs) 
{
    char *ptr;

    if (!strstr(uri, "cgi-bin")) {  /* Static content */ 
	strcpy(cgiargs, "");                             
	strcpy(filename, ".");                           
	strcat(filename, uri);                           
	if (uri[strlen(uri)-1] == '/')                   
	    strcat(filename, "home.html");               
	return 1;
    }
    else {  /* Dynamic content */                       
	ptr = index(uri, '?');                           
	if (ptr) {
	    strcpy(cgiargs, ptr+1);
	    *ptr = '\0';
	}
	else 
	    strcpy(cgiargs, "");                         
	strcpy(filename, ".");                           
	strcat(filename, uri);                           
	return 0;
    }
}

/*
 * serve_static - copy a file back to the client 
 */
void
serve_static(int fd, char *file_name, int file_size) 
{
    int srcfd;
    char *srcp, file_type[MAX_LINE], response_hdrs[MAX_BUF], buf[MAX_BUF];

    /* Send response headers to client */
    response_hdrs[0] = '\0';
    get_file_type(file_name, file_type);    
    strcat(response_hdrs, "HTTP/1.0 200 OK\r\n");
    strcat(response_hdrs, "Server: Tiny Web Server\r\n");
    strcat(response_hdrs, "Connection: close\r\n");

    sprintf(buf, "Content-length: %d\r\n", file_size);
    strcat(response_hdrs, buf);
    sprintf(buf, "Content-type: %s\r\n\r\n", file_type);
    strcat(response_hdrs, buf);
    
    sio_writen(fd, response_hdrs, strlen(response_hdrs));      

    printf("Response headers:\n");
    printf("%s", buf);

    /* Send response body to client */
    srcfd = open(file_name, O_RDONLY, 0); 
    srcp = mmap(0, file_size, PROT_READ, MAP_PRIVATE, srcfd, 0);
    close(srcfd);                        
    sio_writen(fd, srcp, file_size);         
    munmap(srcp, file_size);                 
}


/*
 * get_file_type - derive file type from file name
 */
void
get_file_type(char *filename, char *filetype) 
{
    if (strstr(filename, ".html"))
	strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
	strcpy(filetype, "image/gif");
    else if (strstr(filename, ".png"))
	strcpy(filetype, "image/png");
    else if (strstr(filename, ".jpg"))
	strcpy(filetype, "image/jpeg");
    else
	strcpy(filetype, "text/plain");
}  

/*
 * serve_dynamic - run a CGI program on behalf of the client
 */
void
serve_dynamic(int fd, char *filename, char *cgiargs) 
{
    char buf[MAX_LINE], *emptylist[] = { NULL };

    /* Return first part of HTTP response */
    sprintf(buf, "HTTP/1.0 200 OK\r\n"); 
    sio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    sio_writen(fd, buf, strlen(buf));
  
    if (fork() == 0) { /* Child */ 
	/* Real server would set all CGI vars here */
	setenv("QUERY_STRING", cgiargs, 1); 
	dup2(fd, STDOUT_FILENO);         /* Redirect stdout to client */ 
	execve(filename, emptylist, environ); /* Run CGI program */ 
    }
    wait(NULL); /* Parent waits for and reaps child */ 
}

/*
 * client_error - returns an error message to the client
 */
void
client_error(int clientfd, char *cause, char *errnum, 
        char *short_msg, char *long_msg)
{
    char linebuf[MAX_LINE], body[MAX_BUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny web server Error</title>");
    strcat(body, "<body bgcolor=""ffffff"">\r\n");
    sprintf(linebuf, "%s: %s\r\n", errnum, short_msg);
    strcat(body, linebuf);
    sprintf(linebuf, "%s: %s\r\n", long_msg, cause);
    strcat(body, linebuf);

    /* Send the HTTP response */
    sprintf(linebuf, "HTTP/1.0 %s %s\r\n", errnum, short_msg);
    if (sio_writen(clientfd, linebuf, strlen(linebuf)) < 0)
        return;
    sprintf(linebuf, "Content-Type: text/html\r\n");
    if (sio_writen(clientfd, linebuf, strlen(linebuf)) < 0)
        return;
    sprintf(linebuf, "Content-Length: %zu\r\n\r\n", strlen(body));
    if (sio_writen(clientfd, linebuf, strlen(linebuf)) < 0)
        return;
    if (sio_writen(clientfd, body, strlen(body)) < 0)
        return;
}

