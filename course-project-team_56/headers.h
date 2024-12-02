#ifndef HEADERS_H
#define HEADERS_H

// #define _GNU_SOURCE
// #define _POSIX_C_SOURCE
// #define _XOPEN_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include<time.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<errno.h>
#include<stdbool.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<netdb.h>
#include<fcntl.h>
#include<sys/stat.h>
#include <ifaddrs.h>
#include <stdbool.h>
#include<sys/time.h>
#include<dirent.h>
#include <sys/wait.h>

// error codes etc. ////////////////////////////////////////////////////////////////////////////////

#define SUCCESS 0
#define CLIENT_TIMEOUT 1
#define SS_TIMEOUT 2
#define SS_NOT_FOUND 3
#define FILE_NOT_FOUND 4
#define FILE_ALREADY_EXISTS 5
#define DIR_NOT_FOUND 6
#define DIR_ALREADY_EXISTS 7
#define FILE_BEING_WRITTEN 8
#define FILE_BEING_READ 9
#define INVALID_REQUEST 10

// limits on various stuff

#define SS_MAX 16
#define MAX_THREADS_SS 16
#define MAX_THREADS_NM 16
#define MAX_FILENAME_SIZE 1024
#define MAX_DATA_CHUNK_SIZE 1024

#define SS_CHECK_INTERVAL 5

#define MAX_FILEPATH_SIZE 1024
#define MAXIMUM_FILES 10007
#define MAX_DATA_SIZE 1000000
#define MAX_SEND_SIZE 16384
#define HASH_PRIME 10007
#define PACKET_SIZE 16 

#define DATA_PACKET_STRUCT 2
#define PACKET_STRUCT 1
#define SS_INIT_STRUCT 3
#define STATUS_CHECK_STRUCT 4

// request related definitions and structures and functions 
#define CREATE_REQUEST 0
#define READ_REQUEST 1
#define WRITE_REQUEST 2
#define DELETE_REQUEST 3
#define LIST_REQUEST 4
#define STREAM_REQUEST 5
#define INFO_REQUEST 6
#define COPY_REQUEST 7
#define SS_INIT 8
#define STATUS_CHECK 9
#define ASYNC_WRITE 10

#define RED "\033[0;31m"
#define RESET "\033[0m"


typedef struct st_File
{
    unsigned long long int iNode;
    char filepath[MAX_FILEPATH_SIZE];
    pthread_mutex_t lock;
    pthread_cond_t read_cv;
    pthread_cond_t write_cv;
    int active_clients ;
    int write_clients ;
    int read_clients ;
    bool is_deleted ;
} File ;

typedef struct PACKET
{
    int status ;
    int request_type ;
    char IP[20];
    int PORT ;
    char path1[MAX_FILEPATH_SIZE];
    char path2[MAX_FILEPATH_SIZE];
    int operationNumber ;

} PACKET;

typedef struct Data_Packet
{
    int status ;
    int request_type ;
    int number_of_packets ;
    char data[MAX_SEND_SIZE];
} Data_Packet ;


typedef struct st_NM_LOG{
    char request_ip[NI_MAXHOST];
    int request_port;
    char response_ip[NI_MAXHOST];
    int response_port;
    char file_name[MAX_FILEPATH_SIZE];
    char dir_name[MAX_FILEPATH_SIZE];
    int request_type;
    bool status;
    char* time;
    int count;      // to add a new log entry, increment this
}st_NM_LOG;

typedef struct st_NM_LOG* NM_LOG;

void add_log_entry(NM_LOG log, char* request_ip, int request_port, char* response_ip, int response_port, char* file_name, char* dir_name, int request_type, bool status, char* time, int count);

void print_log_entry(NM_LOG log);

// connection related functions and structures //////////////////////////////////////////////////


char* obtain_client_ip();

void print_server_ip();

// file related functions and structures //////////////////////////////////////////////////////////    

// miscellaneous functions and structures //////////////////////////////////////////////////////////

char* getTimeWithMicroseconds();

int* initSS(char** files, char** dirs, char** permitted, int n);


// queue implementation, if required

typedef struct st_Que{
    char** Quu;
    int front;
    int rear;
    int q_size;
    int capacity;
}st_Que;

typedef struct st_Que* Que;

Que create_q();

void en_q(Que Q, char* dir);

int d_q(Que Q);

void destroy_q(Que Q);

bool is_present(char* name, char** arr, int n);

void change_relative_to_absolute(char* path);

char* get_wifi_ip();

char* get_current_ip();

char* print_server_ip_SSside();

#endif
