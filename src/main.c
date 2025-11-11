#include <stdio.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>
#include <netdb.h>
#include "threadpool.h"




#define BUFFER_SIZE 1000
#define SIZE 2000
#define FILTER_SIZE 100

typedef struct CLIENT_INFO{
    struct sockaddr_in sockInfo;
    int sockfd;
} CLINET_INFO ;

void* dealWith (void * cin_information);
void responseError(int fd , int error );
void addToEnd(char * req , int* size);
void openFilter(char file_path[]);
void responseSever(CLINET_INFO cinIn,struct hostent *hp , char* req, int port );
bool IpFound(char* ip_filter , char* ip_host);


char filterHostName[FILTER_SIZE][50];
char filterIPNumber[FILTER_SIZE][50];
int len_hostName=0;
int len_ipNumber=0;

bool isNumber(char* number){   // check if the user enter a number and if is a positive or not

    for (int i =0 ; i< strlen(number) ; i++){
        if( number[i]== '0' && i==0){       // that the number is not 0
            return false;
        }

        if (number[i] == '-' && i ==0){   // that is not negative
            return false;
        }

        if (!isdigit(number[i])){  //
            return false;
        }

    }
    return true;
}


int main(int argc , char* argv[]) {

    if(argc!= 5){   //// 5
        printf("Usage: proxyServer <port> <pool-size> <max-number-of-request> <filter>\n");
        return 1;
    }

    if( (isNumber(argv[1]) == false) ||  (isNumber(argv[2]) == false) || (isNumber(argv[3]) == false)){
        printf("Usage: proxyServer <port> <pool-size> <max-number-of-request> <filter>\n");
        return 1;
    }

    if (strtoul(argv[2],NULL,10) > 65535){
        printf("Usage: proxyServer <port> <pool-size> <max-number-of-request> <filter>\n");

        return 1;
    }

//    for (int i = 0; i < FILTER_SIZE; i++) {
//        filterHostName[i] = NULL;
//        filterIPNumber[i] = NULL;
//    }

    struct sockaddr_in server_info;
    int wlc_socket_fd ;
    in_port_t port = (in_port_t)strtoul(argv[1],NULL,10);
    size_t pool_size= strtoul(argv[2],NULL,10);
    size_t max_tasks = strtoul( argv[3],NULL,10);
    char file_name[100];
    strcpy(file_name , argv[4]);


    openFilter(file_name);
    threadpool *tp = create_threadpool((int)pool_size);

    //num_of_tasks = num_of_tasks > max_tasks ? max_tasks : num_of_tasks;
    memset(&server_info,0,sizeof(struct sockaddr_in) );

    server_info.sin_family = AF_INET;
    server_info.sin_port = htons(port);
    server_info.sin_addr.s_addr = inet_addr("127.0.0.1");

    if ((wlc_socket_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        perror("error: <socket>\n");
        return 1;
    }

    if ((bind(wlc_socket_fd , (struct sockaddr*)&server_info , sizeof (struct sockaddr_in))) == -1){
        close(wlc_socket_fd);
        perror("error: <bind>\n");
        return 1;
    }

    if (listen (wlc_socket_fd , 5 ) == -1){
        close(wlc_socket_fd);
        perror("error: <listen>\n");
        return 1;
    }


    int counter =0;
    for ( size_t i =0 ; i < max_tasks; i++){
        CLINET_INFO *cin_inf = (CLINET_INFO*)malloc(sizeof (CLINET_INFO));
        socklen_t struct_len = sizeof (struct sockaddr_in);
        (cin_inf->sockfd) = accept(wlc_socket_fd , (struct sockaddr*) &cin_inf->sockInfo , &struct_len);

        if ((cin_inf->sockfd) == -1){
            perror("error: <accept>\n");
            continue;  /// <0
        }
        counter++;
        dispatch(tp , (void*)dealWith  , (void*)cin_inf);

        if (counter == max_tasks)
            break;
    }

    destroy_threadpool(tp);
    close(wlc_socket_fd);
    return 0;
}



void* dealWith (void * cin_information) {

    CLINET_INFO cinIn = *(CLINET_INFO *) cin_information;
    struct hostent *hp;
    char buffer[BUFFER_SIZE];
    char *copy;
    char *req ; // this wat will I send to the server
    size_t read_byt;
    size_t cont = 0;
    char *host;
    int port;
    int size = 3000;
    int check = 0;
    char* pos;
    int length;
    copy = (char*) malloc((sizeof(char) * size));

    if (copy == NULL){
        responseError(cinIn.sockfd , 500);
        close(cinIn.sockfd);
        pthread_exit(NULL);
    }

    memset(buffer, 0, sizeof(buffer));
    memset(copy, 0, size * sizeof(char));



    while (1) {
        read_byt = read(cinIn.sockfd, buffer, BUFFER_SIZE - 1);

        if (read_byt == 0){
            return NULL;
        }

        cont = cont+read_byt;

        if (cont >= size){
            size = size * 2 ;
            copy = (char*) realloc(copy,size * sizeof(char));

            if (copy == NULL){
                responseError(cinIn.sockfd , 500);
                return NULL;
            }
        }

        if (read_byt == -1) {
            responseError(cinIn.sockfd , 500);
            return NULL;
        }

        if (strstr(buffer, "\r\n\r\n") != NULL) {
            strcpy(copy, buffer);
            break;
        }
            // if
        else{
            strcpy(copy, buffer);
            if ((pos = strstr(copy, "\r\n\r\n")) != NULL){
                length = pos - copy;
                check =1;
                break;

            }}
        memset(buffer, 0, sizeof(buffer));
    }

    req = (char*) malloc((sizeof(char) * size));

    if (req == NULL){
        responseError(cinIn.sockfd , 500);
        close(cinIn.sockfd);
        return NULL;
    }

    if (check == 1){
        memcpy(req, copy, length);
        strcat(req , "\r\n\r\n");
    }
    else{
        strcpy(req, copy);
    }

    char headerHost [101];

    host = strstr(copy, "Host:");
    if (host != NULL){
        memcpy(headerHost, host, 100);
        headerHost[100] = '\0';
    }



    char *line = strtok(copy, "\r\n");

    if (line == NULL) {
        // do something
    }

    char *method = strtok(line, " ");
    char *path = strtok(NULL, " ");
    char *protocol = strtok(NULL, " ");


    if (method == NULL || path == NULL || protocol == NULL) {
        // send 400 "Bad Request" respond, as in file 400.pdf
        responseError(cinIn.sockfd, 400);
        close(cinIn.sockfd);
        return NULL;
    }


    if (strcmp(protocol, "HTTP/1.1") != 0 && strcmp(protocol, "HTTP/1.0") != 0) {
        responseError(cinIn.sockfd, 400);
        close(cinIn.sockfd);
        return NULL;
    }

    if (host == NULL) {
        responseError(cinIn.sockfd, 400);
        close(cinIn.sockfd);
        return NULL;
    }

    if (strcmp(method, "GET") != 0) {
        //return error message 501
        responseError(cinIn.sockfd, 501);
        close(cinIn.sockfd);
        return NULL;
    }


    char *hName = strtok(headerHost, "\r\n");
    hName = hName+5;
    char* h_name = strtok(hName , ":");
    char* str_port = strtok(NULL, "");
    if (str_port != NULL ) {
        port = atoi(str_port);       // Convert string to integer
    } else {
        port = atoi("80");
    }

    while (isspace(*h_name)) {
        h_name++;
    }

    if ((hp = gethostbyname(h_name )) == NULL) {   // get the ip using the name
        herror("gethostbyname\n");
        responseError(cinIn.sockfd, 404);
        close(cinIn.sockfd);
        return NULL;
    }

    struct in_addr **addr_list;
    addr_list = (struct in_addr **)hp->h_addr_list;
    // Convert binary IP address to a string
    char *ip_str = inet_ntoa((*addr_list[0]));

    for (int i = 0; i < FILTER_SIZE && i< len_hostName ; i++) {
        //error message 403 “Forbidden” as in file 403.pdf
        if (strcmp(filterHostName[i], h_name) == 0 ) {
            responseError(cinIn.sockfd, 403);
            close(cinIn.sockfd);
            return NULL;
        }
    }

    bool found = false;
    for (int i = 0; i < FILTER_SIZE && i< len_ipNumber; i++) {
        //error message 403 “Forbidden” as in file 403.pdf
        found =IpFound(filterIPNumber[i] , ip_str);
        if (found == true) {
            responseError(cinIn.sockfd, 403);
            close(cinIn.sockfd);
            return NULL;
        }
    }


    char *con = strstr(req, "Connection:");
    if (con == NULL){
        addToEnd( req , &size);
    }
    else{
        char *toDelete = "keep-alive";
        char *del = "Connection:";
        char *ptr ;
        char *end;
        if ((ptr = strstr(req, toDelete)) != NULL){
            addToEnd( req , &size );
            memmove(ptr, ptr + strlen(toDelete), strlen(ptr + strlen(toDelete)) + 1);
            if ((end  = strstr(con, "\r\n")) != NULL){
                memmove(end, end + strlen("\r\n"), strlen(ptr + strlen("\r\n")) + 1);
            }
            char *ttr = strstr(req, del);
            if (ttr != NULL) {
                memmove(ttr, ttr + strlen(del)+1, strlen(ttr + (strlen(del)) + 1)+1);
            }

        }}

    responseSever(cinIn, hp ,req, port );
    free(copy);
    free(req);
    close(cinIn.sockfd);
    free(cin_information);

    return NULL;
}


void responseError(int fd , int num_error ) {

    char str[1000];
    char coLen[1000];
    char body[500];
    int index =0;
    size_t len ;
    time_t current_time;
    struct tm *time_info;

    time(&current_time);
    time_info = gmtime(&current_time); // Use gmtime for UTC time

    // Format the date
    char date_str[40]; // Make sure it's large enough to hold the formatted date
    strftime(date_str, sizeof(date_str), "Date: %a, %d %b %Y %H:%M:%S GMT\r\n", time_info);

    memset(str, 0, sizeof(str));
    memset(coLen, 0, sizeof(coLen));
    memset(body, 0, sizeof(body));


    char *error_messages[] = {
            "400 Bad Request",
            "403 Forbidden",
            "404 Not Found",
            "500 Internal Server Error",
            "501 Not supported"
    };

    if (num_error == 400) {
        strcpy(body,
               "<HTML><HEAD><TITLE>400 Bad Request</TITLE></HEAD>\r\n<BODY><H4>400 Bad request</H4>\r\nBad Request.\r\n</BODY></HTML>");
        len = strlen(body);
        sprintf(coLen, "Content-Length: %zu\r\n", len);
        index = 0;
    } else if (num_error == 403) {
        strcpy(body,
               "<HTML><HEAD><TITLE>403 Forbidden</TITLE></HEAD>\r\n<BODY><H4>403 Forbidden</H4>\r\nAccess denied.\r\n</BODY></HTML>");
        len = strlen(body);
        sprintf(coLen, "Content-Length: %zu\r\n", len);
        index = 1;

    } else if (num_error == 404) {
        strcpy(body,
               "<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD>\r\n<BODY><H4>404 Not Found</H4>\r\nFile not found.\r\n</BODY></HTML>");
        len = strlen(body);
        sprintf(coLen, "Content-Length: %zu\r\n", len);
        index = 2;

    } else if (num_error == 500) {
        strcpy(body,
               "<HTML><HEAD><TITLE>500 Internal Server Error</TITLE></HEAD>\r\n<BODY><H4>500 Internal Server Error</H4>\r\nSome server side error.\r\n</BODY></HTML>");
        len = strlen(body);
        sprintf(coLen, "Content-Length: %zu\r\n", len);
        index = 3;


    } else if (num_error == 501) {
        strcpy(body,
               "<HTML><HEAD><TITLE>501 Not supported</TITLE></HEAD>\r\n<BODY><H4>501 Not supported</H4>\r\nMethod is not supported.\r\n</BODY></HTML>");
        len = strlen(body);
        sprintf(coLen, "Content-Length: %zu\r\n", len);
        index = 4;
    }


    strcpy(str, "HTTP/1.1 ");
    strcat(str, error_messages[index]);
    strcat(str, "\r\nServer: webserver/1.0\r\n");
    strcat(str, date_str);
    strcat(str, "Content-Type: text/html\r\n");
    strcat(str, coLen);
    strcat(str, "Connection: close\r\n\r\n");
    strcat(str, body);

    if (write(fd, str, strlen(str)) < 0) {
        perror("error: <write>\n");
        close(fd);
        return;
    }

}


void openFilter(char file_path[]){

    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        perror("error: <fopen>\n");
        exit(EXIT_FAILURE);
    }

    int i = 0;
    int j = 0;
    char buffer[100];
    memset(buffer , 0 , sizeof(buffer));
    while (fgets(buffer, sizeof(buffer)-1, file) != NULL) {
        size_t len = strcspn(buffer, "\r\n"); // Find the position of '\n' if it exists
        buffer[len] = '\0';

        if (isdigit(buffer[0])){
            strcpy(filterIPNumber[j], buffer);
            len_ipNumber++;
            j++;
        }
        else{
            strcpy(filterHostName[i], buffer);
            len_hostName++;
            i++;}
        memset(buffer , 0, sizeof(buffer));
    }

    fclose(file);
}

void addToEnd(char * req , int* size){
    size_t len = strlen(req);
    char *toDelete = "\r\n\r\n";
    char add[] = "Connection: close\r\n\r\n";
    int len_add = strlen(add);
    char *ptr = strstr(req, toDelete);
    if (ptr != NULL) {
        memmove(ptr, ptr + strlen(toDelete), strlen(ptr + strlen(toDelete)) + 1);
    }
    if (len + len_add +2>= (*size)){
        (*size) = (*size) +100;
        req =(char*) realloc(req,(sizeof(char)) * (*(size)));
    }

    strcat(req,"\r\n");
    strcat(req,add);
}

void responseSever(CLINET_INFO cinIn,struct hostent *hp , char* req, int port ){

    struct sockaddr_in server_address;
    size_t bytesRead;
    size_t bytesWrite;

    unsigned char buffer[SIZE] ;


    int server_fd ;
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("error: <socket>\n");
        responseError(cinIn.sockfd , 500);
        close(cinIn.sockfd);
        close(server_fd );
        return;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port); // Replace PORT_NUMBER with the server's port

    server_address.sin_addr.s_addr = ((struct in_addr *) (hp->h_addr))->s_addr;

    // connect the socket to server
    if (connect(server_fd , (struct sockaddr *) &server_address, sizeof(server_address)) == -1) {
        perror("error: <connect>\n");
        responseError(cinIn.sockfd , 500);
        close(cinIn.sockfd);
        close(server_fd );
        return;
    }


//    int headerEnd ;
 //   char *bodyStart = NULL;
 //   int start ;


    bytesWrite= write( server_fd, req, strlen(req));
    if (bytesWrite < 0 ) {
        responseError(cinIn.sockfd , 500);
        close(cinIn.sockfd);
        return;
    }

    //memset(buffer, 0, sizeof(buffer));
    while ((bytesRead = read(server_fd,  buffer, SIZE-1 )) > 0){
        if (bytesRead < SIZE){
            buffer[bytesRead+1] = '\0';
        }

        write(cinIn.sockfd, buffer, bytesRead);

        //memset(buffer, 0, sizeof(buffer));
    }

    if (bytesRead == -1){
        perror("error: <read>\n");
        responseError(cinIn.sockfd , 500);
        close(cinIn.sockfd);
        close(server_fd);
        return;
    }
    close(server_fd);
}


bool IpFound(char* ip_filter , char* ip_host){
    char *token;
    char *subnet_ip;
    char *prefixLength;
    char ipCopy[16];
    strcpy(ipCopy, ip_filter);
    uint32_t subnetMask = 0xFFFFFFFF;

    token = strtok(ipCopy, "/");
    subnet_ip= token;
    if (token == NULL){
        prefixLength = "32";
    }
    else {
        token = strtok(NULL, "/");
        prefixLength = token;
    }

    int bitsToClear = 32 - atoi(prefixLength);
    subnetMask = subnetMask >> bitsToClear;  // shift right to create correct mask

    uint32_t subnet = inet_addr(subnet_ip);  // this is already in network byte order
    uint32_t network = subnet & subnetMask;  // applying mask to find the network address
    uint32_t host = inet_addr(ip_host);      // this should also be in network byte order

    if ((host & subnetMask) == network) {
        return true;
    } else {
        return false;
    }
}
