#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <pthread.h>
#include <regex.h>
#include "utils/hashmap.c"
#include "utils/routes.c"

#define EXIT_SUCCESS 0
#define PORT 2130
#define BUFFER_SIZE 104857600

hash_table* mime_types = {0};
route_table* routes = {0};


void create_mime_types(){
    mime_types = hash_table_create();
    hash_table_set(mime_types, "html",   "text/html");
    hash_table_set(mime_types, "txt",    "text/plain");
    hash_table_set(mime_types, "jpg",    "image/jpeg");
    hash_table_set(mime_types, "jpeg",   "image/jpeg");
    hash_table_set(mime_types, "png",    "image/png");
    hash_table_set(mime_types, "css",    "text/css");
    hash_table_set(mime_types, "js",     "text/javascript");
}

void create_routes(){
    routes = route_table_create();

    route* index = malloc(sizeof(route));
    index->template="routes/index.html";
    index->base="";
    index->route_type=STATIC;
   
    route* blogs = malloc(sizeof(route));
    blogs->template="routes/blogs.html",
    blogs->route_type=STATIC,
    blogs->base="blogs";
    
    route* blog = malloc(sizeof(route));
    blog->template="routes/blog.html",
    blog->route_type=DYNAMIC,
    blog->base="blog";

    route_table_set(routes,index->base, index);
    //route_table_set(routes,blogs.base, &blogs);
    route_table_set(routes,blog->base, blog);
}

route* get_route(char* url){
    char base[32] = ""; 
    int i = 0;
    char* p = &url[0];
    while(*p != '\0') {
        if(*p != '/'){
            base[i] = *p;
        } 
        else {
            base[i] = '\0';
            break;
        }
        //process the current char
        ++p; 
        i++;
    }

    return route_table_get(routes, base);
}

void post_error(char* error) {
	perror(error);
	exit(EXIT_FAILURE);
}

const char *get_file_extension(const char *file_name) {
    const char *dot = strrchr(file_name, '.');
    if (!dot || dot == file_name) {
        return "";
    }
    return dot + 1;
}

const char *get_mime_type(const char *file_ext) {
    const char* mime_type = hash_table_get( mime_types, file_ext );
    if(mime_type!= NULL){
       return mime_type; 
    }
    return "application/octet-stream";
}

char *url_decode(const char *src) {
    size_t src_len = strlen(src);
    char *decoded = malloc(src_len + 1);
    size_t decoded_len = 0;

    // decode %2x to hex
    for (size_t i = 0; i < src_len; i++) {
        if (src[i] == '%' && i + 2 < src_len) {
            int hex_val;
            sscanf(src + i + 1, "%d", &hex_val);
            decoded[decoded_len++] = hex_val;
            i += 2;
        } else {
            decoded[decoded_len++] = src[i];
        }
    }

    // add null terminator
    decoded[decoded_len] = '\0';
    return decoded;
}

void build_http_response(const char *file_name,
			 const char *file_ext,
			 char *response,
			 size_t *response_len)
{
    const char *mime_type = get_mime_type(file_ext);
    char *header = (char *)malloc(BUFFER_SIZE * sizeof(char));
    snprintf(header, BUFFER_SIZE , 
	     "HTTP/1.1 200 OK\r\n"
	     "Content-Type: %s\r\n"
	     "\r\n",
	     mime_type
    );

    int file_fd = open(file_name, O_RDONLY); if(file_fd == -1 ) {
	snprintf(response, BUFFER_SIZE , 
		 "HTTP/1.1 404 Not Found\r\n"
		 "Content-Type: text/plain\r\n"
		 "\r\n"
		 "404 Not Found"
	);
	*response_len = strlen(response);
	return;
    }
    struct stat file_stat;
    fstat(file_fd, &file_stat);
    off_t file_size = file_stat.st_size;
    *response_len = 0;
    memcpy(response, header, strlen(header));
    *response_len += strlen(header);


    ssize_t bytes_read;
    while ((bytes_read = read(file_fd,
			      response + *response_len,
			      BUFFER_SIZE - *response_len)) > 0 ){
	*response_len += bytes_read;
    }
    free(header);
    close(file_fd);
}
void build_route_response(route *route,
			 const char *file_ext,
			 char *response,
			 size_t *response_len)
{
    const char *mime_type = get_mime_type(file_ext);
    char *header = (char *)malloc(BUFFER_SIZE * sizeof(char));
    snprintf(header, BUFFER_SIZE , 
	     "HTTP/1.1 200 OK\r\n"
	     "Content-Type: %s\r\n"
	     "\r\n",
	     mime_type
    );

    int file_fd = open(route->template, O_RDONLY); if(file_fd == -1 ) {
	snprintf(response, BUFFER_SIZE , 
		 "HTTP/1.1 404 Not Found\r\n"
		 "Content-Type: text/plain\r\n"
		 "\r\n"
		 "404 Not Found"
	);
	*response_len = strlen(response);
	return;
    }
    struct stat file_stat;
    fstat(file_fd, &file_stat);
    off_t file_size = file_stat.st_size;
    *response_len = 0;
    memcpy(response, header, strlen(header));
    *response_len += strlen(header);


    ssize_t bytes_read;
    while ((bytes_read = read(file_fd,
			      response + *response_len,
			      BUFFER_SIZE - *response_len)) > 0 ){
	*response_len += bytes_read;
    }
    free(header);
    close(file_fd);
}

void *handle_client(void *arg) {
    int client_fd = *((int *)arg);
    char *buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));

    // receive request data from client and store into buffer
    ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);
    if (bytes_received > 0) {
        // check if request is GET
        regex_t regex;
        regcomp(&regex, "^GET /([^ ]*) HTTP/1", REG_EXTENDED);
        regmatch_t matches[2];

        if (regexec(&regex, buffer, 2, matches, 0) == 0) {
            // extract filename from request and decode URL
            buffer[matches[1].rm_eo] = '\0';
            const char *url_encoded_file_name = buffer + matches[1].rm_so;

            //printf("URL: %s\n",url_encoded_file_name);
            char *file_name = url_decode(url_encoded_file_name);

            // get file extension
            char file_ext[64];
            strcpy(file_ext, get_file_extension(file_name));

            // build HTTP response
            char *response = (char *)malloc(BUFFER_SIZE * 2 * sizeof(char));
            size_t response_len;
            //const char* route = hash_table_get(routes, file_name);
            route* route = get_route(file_name);
            if(route != NULL){
                build_route_response(
                route, "html", response, &response_len);
            }
            else {
                build_http_response(file_name, file_ext, response, &response_len);
            }

            // send HTTP response to client
            send(client_fd, response, response_len, 0);

            free(response);
            free(file_name);
        }
        regfree(&regex);
    }
    
    close(client_fd);
    free(arg);
    free(buffer);
    return NULL;
}

void handle_server() {
    printf("START");
    int server_fd;
    struct sockaddr_in server_addr;
    if ((server_fd = socket(AF_INET, SOCK_STREAM,0) ) < 0) {
        perror("SOCKET FAILED");
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    int yes=1;
    //char yes='1'; // Solaris people use this

    // lose the pesky "Address already in use" error message
    if (setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof yes) == -1) {
        perror("setsockopt");
        exit(1);
    } 
    if(bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
	post_error("BIND FAILED");
    }
    int listener = listen(server_fd, 10);
    
    if(listener == -1){
        post_error("Listened FAILED");
    }
    printf("SOCKET CREATED");
    while(1) {
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);

	int *client_fd = malloc(sizeof(int));

	if ((*client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len)) < 0) {
	    perror("ACCEPT FAILED");
	    continue; 
	}
	pthread_t thread_id;
        /*This function creates  a separate thread that handles the client separately here.*/
	pthread_create(&thread_id,NULL, handle_client, (void *)client_fd);
	pthread_detach(thread_id);
    }
    close(server_fd);
}

int main(void)
{
        create_mime_types();
        create_routes();
	handle_server();
	return EXIT_SUCCESS;
}
