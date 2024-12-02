#include "../headers.h"
#include "../errors.h"

#define IP_SIZE 30
#define SS_PORT 0
#define SS_PORT_FOR_NS 0
#define ASYNCHRONOUS_TIMING 100


#define RED "\033[0;31m"
#define RESET "\033[0m"
#define MAX_FILEPATH_SIZE 1024
#define MAXIMUM_FILES 10007
#define MAX_DATA_SIZE 1000000
#define HASH_PRIME 10007
#define PACKET_STRUCT 1
#define DATA_PACKET_STRUCT 2


long long int number_of_threads;
pthread_mutex_t number_of_threads_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t recv_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t send_lock = PTHREAD_MUTEX_INITIALIZER;
File* files;

void initialise_SS_files();
File* initialise_File(char* file_name);
void free_File(File* file);
int hash_iNode(unsigned long long int iNode);
void* handle_client(void* client_fd);
void Read(int client, PACKET buffer , int NS_fd);
void Write(int client, PACKET buffer, char** data, int NS_fd, int number_of_chunks);
void Write_Asynchronous(int client, PACKET buffer, char** data, int NS_fd, int n);
void Delete(int client, PACKET buffer, int NS_fd);
void Stream(int client, PACKET buffer, int NS_fd);
void Info(int client, PACKET buffer, int NS_fd);
// void Copy(int client, PACKET buffer, int NS_fd);
void Create(int client, PACKET buffer, int NS_fd);
void send_PACKET(int fd, int status, int request_type, char* IP, int PORT, char* path1, char* path2);
void send_DataPacket(int fd ,int status, int request_type, int number_of_packets, char* data);
void remove_directory(const char *path, int NS_fd, char* IP, int PORT);
void send_STATUS(int client, int status, int request_type, char* IP, int PORT, char* path1, char* path2);

int NS_sender_fd;
char ns_ip[IP_SIZE];
int ns_port;
int ns_side_port;
char root[MAX_FILEPATH_SIZE];
int client_side_port;
char ss_server_ip[IP_SIZE];
// Should search for optimum data structure for storing the files and directories. --Done
// Should create an array of struct for files, which has info on locks related to read, write, etc. and also other file related info. --Done

// Should do the same for the directories. --No need to do this

void change_absolute_to_relative(char* path) {
    size_t root_len = strlen(root);

    // Check if `path` starts with `root`
    if (strncmp(path, root, root_len) == 0) {
        // Strip the `root` part from `path` to make it relative
        memmove(path, path + root_len, strlen(path + root_len) + 1); // +1 to include null terminator
    } else {
        // If the path does not start with `root`, then it is error
    }
}

void change_relative_to_absolute(char* path) {
    char absolute_path[MAX_FILEPATH_SIZE]; // Buffer to hold the absolute path

    // Copy the root directory as the base for the absolute path
    strcpy(absolute_path, root);

    // If the path is already absolute (starts with '/'), just append it to root
    if (path[0] == '/') {
        strncat(absolute_path, path, MAX_FILEPATH_SIZE - strlen(absolute_path) - 1);
    } else {
        // If the path is relative, prepend root and a '/'
        strncat(absolute_path, "/", MAX_FILEPATH_SIZE - strlen(absolute_path) - 1);
        strncat(absolute_path, path, MAX_FILEPATH_SIZE - strlen(absolute_path) - 1);
    }

    // Copy the result back to the original path
    strncpy(path, absolute_path, MAX_FILEPATH_SIZE - 1);
    path[MAX_FILEPATH_SIZE - 1] = '\0'; // Ensure null-termination

    // Ensure the path ends with a trailing '/' if it's not already there
    // if (path[strlen(path) - 1] != '/') {
    //     strncat(path, "/", MAX_FILEPATH_SIZE - strlen(path) - 1);
    // }
}

int main()
{
    // Initialising the storage server
    initialise_SS_files();
    char** Files = malloc(MAXIMUM_FILES * sizeof(char*));
    for(int i = 0; i < MAXIMUM_FILES; i++)
    {
        Files[i] = malloc(MAX_FILEPATH_SIZE * sizeof(char));
    }
    char** dirs = malloc(MAXIMUM_FILES * sizeof(char*));
    for(int i = 0; i < MAXIMUM_FILES; i++)
    {
        dirs[i] = malloc(MAX_FILEPATH_SIZE * sizeof(char));
    }
    char** permitted = malloc(MAXIMUM_FILES * sizeof(char*));
    for(int i = 0; i < MAXIMUM_FILES; i++)
    {
        permitted[i] = malloc(MAX_FILEPATH_SIZE * sizeof(char));
    }
    int* arr = initSS(Files, dirs, permitted, MAXIMUM_FILES);
    // for (int i = 0; i < arr[0]; i++) {
    //     printf("File: %s\n", files[i]);
    // }
    // for (int i = 0; i < arr[1]; i++) {
    //     printf("Directory: %s\n", dirs[i]);
    // }
    // printf("Enter the root directory: ");
    if(arr == NULL)
    {
        printf(RED "Failed to initialise the storage server\n" RESET);
        return -1;
    }
    printf("Enter the IP address and PORT of Naming Server: ");
    char NS_IP[IP_SIZE];
    int NS_PORT;
    scanf("%s %d", NS_IP, &NS_PORT);
    ns_port = NS_PORT;
    strcpy(ns_ip, NS_IP);

    printf("Enter the root directory: ");
    scanf("%s", root);
    fflush(stdout);
    // Initializing the socket for the Naming Server
    NS_sender_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (NS_sender_fd < 0) {
        printf(RED "Socket creation failed: %s\n" RESET, strerror(errno));
        return -1;
    }

    // Starting the storage server for accepting requests from clients
    int SS_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (SS_fd < 0) {
        printf(RED "Socket creation failed: %s\n" RESET, strerror(errno));
        return -1;
    }

    // printf("error here 1\n");
    fflush(stdout);
    // // Making the socket non-blocking
    // int flags = fcntl(SS_fd, F_GETFL, 0);
    // if (flags < 0) {
    //     printf(RED "Fcntl failed: %s\n" RESET, strerror(errno));
    //     return -1;
    // }

    // if (fcntl(SS_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
    //     printf(RED "Fcntl failed: %s\n" RESET, strerror(errno));
    //     return -1;
    // }

    // Binding the storage server socket
    struct sockaddr_in ss_addr;
    ss_addr.sin_family = AF_INET;
    ss_addr.sin_port = htons(SS_PORT);
    char* ip = print_server_ip_SSside();
    strcpy(ss_server_ip, ip);
    free(ip);
    ss_addr.sin_addr.s_addr = inet_addr(ss_server_ip);

    if (bind(SS_fd, (struct sockaddr*)&ss_addr, sizeof(ss_addr)) < 0) {
        printf(RED "Bind failed: %s\n" RESET, strerror(errno));
        return -1;
    }
    struct sockaddr_in ss_add1;
    socklen_t ss_addr1_len = sizeof(ss_add1);
    if(getsockname(SS_fd, (struct sockaddr*)&ss_add1, &ss_addr1_len) < 0)
    {
        printf(RED "Getsockname failed: %s\n" RESET, strerror(errno));
        return -1;
    }
    client_side_port = ntohs(ss_add1.sin_port);
    inet_ntop(AF_INET, &ss_add1.sin_addr, ss_server_ip, sizeof(ss_server_ip));
    printf("SS_SIDE_PORT: %d\n", client_side_port);
    // inet_ntop(AF_INET, &ss_add1.sin_addr, ss_server_ip, sizeof(ss_server_ip));
    print_server_ip();
    // printf("SS_SERVER_IP: %s\n", ss_server_ip);

    // Non-blocking socket configuration
    // int flags1 = fcntl(NS_sender_fd, F_GETFL, 0);
    // if (flags < 0) {
    //     printf(RED "Fcntl failed: %s\n" RESET, strerror(errno));
    //     return -1;
    // }

    // if (fcntl(NS_sender_fd, F_SETFL, flags1 | O_NONBLOCK) < 0) {
    //     printf(RED "Fcntl failed: %s\n" RESET, strerror(errno));
    //     return -1;
    // }

    // Binding to the local port for communication with the Naming Server
    struct sockaddr_in ns_bind_addr;
    ns_bind_addr.sin_family = AF_INET;
    ns_bind_addr.sin_port = htons(SS_PORT_FOR_NS);
    ns_bind_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(NS_sender_fd, (struct sockaddr*)&ns_bind_addr, sizeof(ns_bind_addr)) < 0) {
        printf(RED "Bind failed: %s\n" RESET, strerror(errno));
        return -1;
    }
    
    struct sockaddr_in ns_side;
    socklen_t ns_addr_len = sizeof(ns_side);
    if(getsockname(NS_sender_fd, (struct sockaddr*)&ns_side, &ns_addr_len) < 0)
    {
        printf(RED "Getsockname failed: %s\n" RESET, strerror(errno));
        return -1;
    }
    ns_side_port = ntohs(ns_side.sin_port);
    printf("NS_SIDE_PORT: %d\n", ntohs(ns_side.sin_port));

    // Connecting to the Naming Server
    struct sockaddr_in ns_connect_addr;
    ns_connect_addr.sin_family = AF_INET;
    ns_connect_addr.sin_port = htons(NS_PORT);
    ns_connect_addr.sin_addr.s_addr = inet_addr(NS_IP);

    // while(1)
    // {
    //     int res = connect(NS_sender_fd, (struct sockaddr*)&ns_connect_addr, sizeof(ns_connect_addr));
    //     if(res < 0 && errno == EINPROGRESS)
    //     {
    //         continue;
    //     }
    //     if(res == 0) 
    //     {
    //         printf("Connected to the Naming Server\n");
    //         break;
    //     }
    //     break;
    // }
    connect(NS_sender_fd, (struct sockaddr*)&ns_connect_addr, sizeof(ns_connect_addr));
    // Send the number of files and directories to the Naming Server
    int number_of_files_and_directories = arr[0] + arr[1];
    int number_of_files = arr[0];
    int number_of_directories = arr[1];
    PACKET files_and_directories;

    pthread_mutex_lock(&send_lock);
    int no = SS_INIT_STRUCT;
    send(NS_sender_fd, &no, sizeof(no), 0);
    files_and_directories.status = SUCCESS;
    files_and_directories.request_type = SS_INIT;
    strcmp(files_and_directories.IP, ss_server_ip);
    files_and_directories.PORT = client_side_port;

    send(NS_sender_fd, &files_and_directories, sizeof(files_and_directories), 0);
    send(NS_sender_fd, &number_of_files, sizeof(number_of_files), 0);
    // send(NS_sender_fd, &number_of_files_and_directories, sizeof(number_of_files_and_directories), 0);

    // Sending the files and directories to the Naming Server
    for (int i = 0; i < number_of_files; i++){
        if (Files[i] != NULL) {
            files_and_directories.status = SUCCESS;
            files_and_directories.request_type = SS_INIT;
            strcpy(files_and_directories.path1, Files[i]);
            no = PACKET_STRUCT;
            send(NS_sender_fd, &no, sizeof(no), 0);
            send(NS_sender_fd, &files_and_directories, sizeof(files_and_directories), 0);
        }
    }
    send(NS_sender_fd, &number_of_directories, sizeof(number_of_directories), 0);
    for (int i = 0; i < number_of_directories; i++) {
        if (dirs[i] != NULL) {
            files_and_directories.status = SUCCESS;
            files_and_directories.request_type = SS_INIT;
            strcpy(files_and_directories.path1, dirs[i]);
            no = PACKET_STRUCT;
            send(NS_sender_fd, &no, sizeof(no), 0);
            send(NS_sender_fd, &files_and_directories, sizeof(files_and_directories), 0);
        }
    }
    pthread_mutex_unlock(&send_lock);
    close(NS_sender_fd); // Closing the connection with the Naming Server

    for(int i = 0; i < MAXIMUM_FILES; i++)
    {
        free(Files[i]);
    }
    free(Files);
    for(int i = 0; i < MAXIMUM_FILES; i++)
    {
        free(dirs[i]);
    }
    free(dirs);
    for(int i = 0; i < MAXIMUM_FILES; i++)
    {
        free(permitted[i]);
    }
    free(permitted);
    free(arr);

    // Listening for incoming connections
    if (listen(SS_fd, SOMAXCONN) < 0) {
        printf(RED "Listen failed: %s\n" RESET, strerror(errno));
        return -1;
    }

    printf("Storage Server started successfully\n");
    number_of_threads = 0;
    socklen_t ss_addr_len = sizeof(ss_addr);
    while(1)
    {
        printf("Waiting for client connection\n");
        struct sockaddr_in client;
        socklen_t client_addr_len = sizeof(client);
        int client_fd = accept(SS_fd, (struct sockaddr*)&client, &client_addr_len);
        // CONTAINS THE FILE_DESCRIPTOR OF THE CONNECTING CLIENT
        if(client_fd < 0)
        {
            printf(RED "Accept failed: %s\n" RESET, strerror(errno));
            return -1;
        }
        int *client_fd_ptr = malloc(sizeof(int));
        if (client_fd_ptr == NULL) {
            perror("Memory allocation failed");
            close(client_fd);
            continue;
        }
        *client_fd_ptr = client_fd;

        // Using detached threads so that server doesn't has to worry about joining the threads.
        printf("Client connected2\n");
        pthread_t thread ;
        pthread_attr_t temporary_attr ;
        pthread_attr_init(&temporary_attr);
        pthread_attr_setdetachstate(&temporary_attr, PTHREAD_CREATE_DETACHED);
        
        pthread_mutex_lock(&number_of_threads_lock);
        number_of_threads += 1;
        pthread_mutex_unlock(&number_of_threads_lock);
        
        pthread_create(&thread, &temporary_attr, handle_client, client_fd_ptr);
    }
       
    while(number_of_threads > 0)
    {
        sleep(1);
    }

    pthread_mutex_destroy(&number_of_threads_lock);
    printf("All threads have been closed\n");
    return 0;
}

void* handle_client(void* client_fd)
{
    int client = *(int*)client_fd;
    free(client_fd);
    printf("Client connected\n");
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    if (getpeername(client, (struct sockaddr*)&client_addr, &client_addr_len) < 0) {
        printf(RED "Getpeername failed: %s\n" RESET, strerror(errno));
        return NULL;
    }
    char client_IP[IP_SIZE];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_IP, IP_SIZE);
    int client_port = ntohs(client_addr.sin_port);

    PACKET buffer;
    int recv_bytes;
    int ch;
    char recv_ch = PACKET_STRUCT;

    // printf("Client IP: %s\n", client_IP);
    pthread_mutex_lock(&recv_lock);

    printf("Client connected with IP: %s and port: %d\n", client_IP, client_port);
    
    recv_bytes = recv(client, &ch, sizeof(ch), 0);
    if (recv_bytes < 0) {
        // if (errno == EWOULDBLOCK || errno == EAGAIN) {
        //     pthread_mutex_unlock(&recv_lock);
        //     return NULL;
        // }
        printf(RED "Recv failed: %s\n" RESET, strerror(errno));
        pthread_mutex_unlock(&recv_lock);
        return NULL;
    }
    if (recv_bytes == 0) {
        // Client disconnected
        // send_PACKET(NS_sender_fd, CLIENT_TIMEOUT, 5, client_IP, client_port, NULL, NULL);
        send_STATUS(NS_sender_fd, CLIENT_TIMEOUT, -1, client_IP, client_port, NULL, NULL);
        close(client);
        pthread_mutex_lock(&number_of_threads_lock);
        number_of_threads -= 1;
        pthread_mutex_unlock(&number_of_threads_lock);
        pthread_mutex_unlock(&recv_lock);
        return NULL;
    }
    if (recv_bytes != sizeof(ch) || ch != recv_ch) {
        // Send an error message to the client
        printf(RED "Invalid request\n" RESET);
        pthread_mutex_unlock(&recv_lock);
        return NULL;
    }

    // while (1) {
    recv_bytes = recv(client, &buffer, sizeof(buffer), 0);
    if (recv_bytes < 0) {
        printf(RED "Recv failed: %s\n" RESET, strerror(errno));
        pthread_mutex_unlock(&recv_lock);
        return NULL;
    }
    if (recv_bytes == 0) {
        // Client disconnected
        // send_PACKET(NS_sender_fd, CLIENT_TIMEOUT, 5, client_IP, client_port, NULL, NULL);
        send_STATUS(NS_sender_fd, CLIENT_TIMEOUT, -1, client_IP, client_port, NULL, NULL);
        close(client);
        pthread_mutex_lock(&number_of_threads_lock);
        number_of_threads -= 1;
        pthread_mutex_unlock(&number_of_threads_lock);
        pthread_mutex_unlock(&recv_lock);
        return NULL;
    }
    if(recv_bytes != sizeof(buffer))
    {
        // Send an error message to the client
        printf(RED "Invalid request\n" RESET);
        pthread_mutex_unlock(&recv_lock);
        return NULL;
    }
    // }
    char **data1 = NULL;
    int counter = 0;
    Data_Packet data_buffer ;
    int type = 0;
    if(buffer.request_type == WRITE_REQUEST || buffer.request_type == ASYNC_WRITE)
    {

        printf("Write request received\n");

        while(1)
        {
            counter += 1;
            recv_bytes = recv(client, &type, sizeof(int), 0);
            if (recv_bytes == 0) {
                // Client disconnected
                // send_PACKET(NS_sender_fd, CLIENT_TIMEOUT, 5, client_IP, client_port, NULL, NULL);
                send_STATUS(NS_sender_fd, CLIENT_TIMEOUT, -1, client_IP, client_port, NULL, NULL);
                close(client);
                pthread_mutex_lock(&number_of_threads_lock);
                number_of_threads -= 1;
                pthread_mutex_unlock(&number_of_threads_lock);
                pthread_mutex_unlock(&recv_lock);
                return NULL;
            }
            if (recv_bytes < 0) {
                send_STATUS(NS_sender_fd, CLIENT_TIMEOUT, buffer.request_type, client_IP, client_port, NULL, NULL);  
                send_PACKET(client, CLIENT_TIMEOUT, buffer.request_type, client_IP, client_port, NULL, NULL);
                printf(RED "Recv failed: %s\n" RESET, strerror(errno));
                pthread_mutex_unlock(&recv_lock);
                return NULL;
            }
            if (recv_bytes != sizeof(type) || type != DATA_PACKET_STRUCT) {
                // Send an error message to the client
                send_STATUS(NS_sender_fd, CLIENT_TIMEOUT, buffer.request_type, client_IP, client_port, NULL, NULL);
                send_PACKET(client, CLIENT_TIMEOUT, buffer.request_type, client_IP, client_port, NULL, NULL);
                printf(RED "Invalid request\n" RESET);
                pthread_mutex_unlock(&recv_lock);
                return NULL;
            }
            recv_bytes = recv(client, &data_buffer , sizeof(data_buffer), 0);
            if (recv_bytes == 0) {
                // Client disconnected
                send_STATUS(NS_sender_fd, CLIENT_TIMEOUT, -1, client_IP, client_port, NULL, NULL);
                close(client);
                pthread_mutex_lock(&number_of_threads_lock);
                number_of_threads -= 1;
                pthread_mutex_unlock(&number_of_threads_lock);
                pthread_mutex_unlock(&recv_lock);
                return NULL;
            }
            if (recv_bytes < 0) {
                printf(RED "Recv failed: %s\n" RESET, strerror(errno));
                pthread_mutex_unlock(&recv_lock);
                return NULL;
            }
            if(counter == 1)
            {
                data1 = (char**)malloc(sizeof(char*) * data_buffer.number_of_packets);
                if(data1 == NULL)
                {
                    printf(RED "Error in allocating malloc memory for data1\n");
                    //Send the error to the client and NS both of them.
                    return NULL;
                }
                for(int i = 0 ; i < data_buffer.number_of_packets ; i += 1)
                {
                    data1[i] = (char*)malloc(sizeof(char) * MAX_SEND_SIZE);
                    if(data1[i] == NULL)
                    {
                        printf(RED "Error in allocating memory for data1[ %d ]\n", i);
                        // Send the errors to the client as well as the NS.
                        return NULL;
                    }
                }
            }
            strcpy(data1[counter - 1], data_buffer.data);
            if(counter == data_buffer.number_of_packets)
            {
                printf("Data received\n");
                printf("Number of packets: %d\n", data_buffer.number_of_packets);
                break;
            }
        }
        for(int i = 0 ; i < counter; i += 1)
        {
            printf("Data: %s\n", data1[i]);
        }
    }

    pthread_mutex_unlock(&recv_lock);

    printf("Request received from client\n");
    printf("Packet details\n");
    printf("Request type: %d\n", buffer.request_type);
    printf("Path1: %s\n", buffer.path1);
    printf("Path2: %s\n", buffer.path2);

    change_relative_to_absolute(buffer.path1); // Changing paths according to the root directory of ss

    printf("Path1: %s\n", buffer.path1);
    if (buffer.request_type == CREATE_REQUEST) {
        Create(client, buffer, NS_sender_fd);
    } else if (buffer.request_type == READ_REQUEST) {
        Read(client, buffer, NS_sender_fd);
    } else if (buffer.request_type == WRITE_REQUEST) {
        Write(client, buffer, data1, NS_sender_fd, counter);
    } else if (buffer.request_type == DELETE_REQUEST) {
        Delete(client, buffer, NS_sender_fd);
    } else if (buffer.request_type == STREAM_REQUEST) {
        Stream(client, buffer, NS_sender_fd);
    } else if (buffer.request_type == INFO_REQUEST) {
        Info(client, buffer, NS_sender_fd);
    } else if (buffer.request_type == COPY_REQUEST) {
        // Copy(client, buffer, NS_sender_fd);
    }else if(buffer.request_type == ASYNC_WRITE) {
        Write_Asynchronous(client, buffer, data1, NS_sender_fd, counter);
    }else {
        // Handle unknown request type
        printf(RED "Unknown request type\n" RESET);
        send_STATUS(NS_sender_fd, INVALID_REQUEST, buffer.request_type, client_IP, client_port, buffer.path1, buffer.path2);
        send_PACKET(client, INVALID_REQUEST, buffer.request_type, client_IP, client_port, buffer.path1, buffer.path2);
    }

    if(buffer.request_type != 10) close(client);
    pthread_mutex_lock(&number_of_threads_lock);
    number_of_threads -= 1;
    pthread_mutex_unlock(&number_of_threads_lock);

    if(buffer.request_type != ASYNC_WRITE) close(client);
    pthread_exit(NULL);
    return NULL;  // Ensure function returns NULL at the end
}


int hash_iNode(unsigned long long int iNode)
{
    return iNode % HASH_PRIME;
}

void initialise_SS_files()
{
    files = (File*)malloc(sizeof(File) * MAXIMUM_FILES);
    if(files == NULL)
    {
        printf(RED "Malloc failed: %s\n" RESET, strerror(errno));
        return;
    }
    for(int i = 0 ; i < MAXIMUM_FILES; i++)
    {
        files[i].iNode = 0;
        files[i].filepath[0] = '\0';
        pthread_mutex_init(&files[i].lock, NULL);
        pthread_cond_init(&files[i].read_cv, NULL);
        pthread_cond_init(&files[i].write_cv, NULL);
        files[i].active_clients = 0;
        files[i].write_clients = 0;
        files[i].read_clients = 0;
        files[i].is_deleted = false;
    }
}

File* initialise_File(char* file_path) // Should modify the index assiging part
{
    struct stat meta_file ;
    if(stat(file_path, &meta_file) < 0)
    {
        printf(RED "Stat failed: %s\n" RESET, strerror(errno));

        // Do error handling

        return NULL;
    }

    printf("iNode: %lu\n", meta_file.st_ino);

    // Linear Hashing of iNode for files
    int get_Hash_index = hash_iNode(meta_file.st_ino);
    int counter = 0;
    if(get_Hash_index >= MAXIMUM_FILES)
    {
        get_Hash_index = 0;
        // printf("get_Hash_index: %d\n", get_Hash_index);
    }

    // printf("%d\n", get_Hash_index);
    int k = get_Hash_index;

    while(files[get_Hash_index].iNode != meta_file.st_ino)
    {
        // printf("get_Hash_index: %d\n", get_Hash_index);
        get_Hash_index = (get_Hash_index + 1) % MAXIMUM_FILES;
        counter += 1;
        if(counter == MAXIMUM_FILES)
        {
            // Do error handling
            // return NULL;
            break;
        }
    }
    if(counter == MAXIMUM_FILES)
    {
        counter = 0;
        get_Hash_index = k;
        while(files[get_Hash_index].iNode != 0)
        {
            get_Hash_index = (get_Hash_index + 1) % MAXIMUM_FILES;
            counter += 1;
            if(counter == MAXIMUM_FILES)
            {
                send_STATUS(NS_sender_fd, FILE_NOT_FOUND, -1, NULL, 0, file_path, NULL);
                return NULL;
            }
        }
    }
    if(meta_file.st_ino == files[get_Hash_index].iNode)
    {
        return &files[get_Hash_index];
    }
    
    File* file = &files[get_Hash_index];
    file->iNode = meta_file.st_ino;
    strcpy(file->filepath, file_path);
    pthread_mutex_init(&file->lock, NULL);
    pthread_cond_init(&file->read_cv, NULL);
    pthread_cond_init(&file->write_cv, NULL);
    file->active_clients = 0;
    file->write_clients = 0;
    file->read_clients = 0;
    file->is_deleted = false;
    return file;
}

void free_File(File* file)
{
    pthread_mutex_destroy(&file->lock);
    pthread_cond_destroy(&file->read_cv);
    pthread_cond_destroy(&file->write_cv);
    file->iNode = 0;
    file->filepath[0] = '\0';
    file->active_clients = 0;
    file->write_clients = 0;
    file->read_clients = 0;
    file->is_deleted = false;
}

void Read(int client, PACKET buffer, int NS_fd)
{
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    if(getpeername(client, (struct sockaddr*)&client_addr, &client_addr_len) < 0)
    {
        printf(RED "Getpeername failed: %s\n" RESET, strerror(errno));
        return;
    }
    char client_IP[IP_SIZE];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_IP, IP_SIZE);
    int client_port = ntohs(client_addr.sin_port);
    
    // Check if the file exists in the storage server.
    int file_fd = open(buffer.path1, O_RDONLY);
    if(file_fd < 0)
    {
        // Send the status of the request to the NS.
        // send_PACKET(NS_fd, FILE_NOT_FOUND, 5, client_IP, client_port, buffer.path1, NULL);
        send_STATUS(NS_fd, FILE_NOT_FOUND, 5, client_IP, client_port, buffer.path1, NULL);
        // Send an error message to the client.
        send_PACKET(client, FILE_NOT_FOUND, 5, NULL, 0, buffer.path1, NULL);
        return;
    }
    File* file = initialise_File(buffer.path1);
    // If it does, then check if the file is being written to.
    pthread_mutex_lock(&file->lock);
    int flag = 0;
    while(file->write_clients > 0)
    {
        if(flag == 0)
        {
            send_PACKET(client, FILE_BEING_WRITTEN, READ_REQUEST, NULL, 0, buffer.path1, NULL);
            // Send the status of the request to the NS.
            // send_PACKET(NS_fd, FILE_BEING_WRITTEN, 5, client_IP, client_port, buffer.path1, NULL);
            flag = 1;
        }
        pthread_cond_wait(&file->read_cv, &file->lock);
        if(file->is_deleted)
        {
            // Send an error message to the client.
            send_PACKET(client, FILE_NOT_FOUND, READ_REQUEST, NULL, 0, buffer.path1, NULL);
            // Send the status of the request to the NS.
            // send_PACKET(NS_fd, FILE_NOT_FOUND, 5, client_IP, client_port, buffer.path1, NULL);
            send_STATUS(NS_fd, FILE_NOT_FOUND, READ_REQUEST, client_IP, client_port, buffer.path1, NULL);
            pthread_mutex_unlock(&file->lock);
            close(file_fd);
            return;
        }
    }
    file->read_clients += 1;
    pthread_mutex_unlock(&file->lock);

    // Read from the file and send it to the client 
    int total_chunks = 0;
    struct stat meta_data ;
    if (fstat(file_fd, &meta_data) < 0) 
    {
        // Send response to client and NS.
        send_PACKET(client, FILE_NOT_FOUND, READ_REQUEST, NULL, 0, buffer.path1, NULL);
        // send_PACKET(NS_fd, FILE_NOT_FOUND, 5, client_IP, client_port, buffer.path1, NULL);
        send_STATUS(NS_fd, FILE_NOT_FOUND, READ_REQUEST, client_IP, client_port, buffer.path1, NULL);
        printf(RED "Failed to get file size: %s\n" RESET, strerror(errno));
        close(file_fd);
        return;
    }
    if(meta_data.st_size % MAX_SEND_SIZE == 0)
    {
        total_chunks = meta_data.st_size / MAX_SEND_SIZE;
    }
    else {
        total_chunks = (meta_data.st_size / MAX_SEND_SIZE) + 1;
    }
    int chunks_sent = 0;
    int bytes_read, bytes_sent;
    char data[MAX_SEND_SIZE];
    while ((bytes_read = read(file_fd, data, sizeof(data))) > 0) {
        // printf("debug statement\n");
        send_DataPacket(client, SUCCESS, 1, total_chunks, data);
        chunks_sent++;
    }
    // Send the status of the request to the NS.
    // send_PACKET(NS_fd, SUCCESS, READ_REQUEST, client_IP, client_port, buffer.path1, NULL);
    send_STATUS(NS_fd, SUCCESS, READ_REQUEST, client_IP, client_port, buffer.path1, NULL);
    // Send the status of the request to the client.
    send_PACKET(client, SUCCESS, READ_REQUEST, NULL, 0, buffer.path1, NULL);
    pthread_mutex_lock(&file->lock);
    file->read_clients -= 1;
    pthread_cond_signal(&file->read_cv);
    pthread_cond_signal(&file->write_cv);
    pthread_mutex_unlock(&file->lock);

    close(file_fd);
}

void Write_Asynchronous(int client, PACKET buffer, char** data, int NS_fd, int n) {
     struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    if(getpeername(client, (struct sockaddr*)&client_addr, &client_addr_len) < 0)
    {
        printf(RED "Getpeername failed: %s\n" RESET, strerror(errno));
        return;
    }
    char client_IP[IP_SIZE];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_IP, IP_SIZE);
    int client_port = ntohs(client_addr.sin_port);

    // Check if the file exists in the storage server. If it exists, then truncate and write to it.
    int file_fd = open(buffer.path1, O_WRONLY | O_TRUNC);
    if(file_fd < 0)
    {
        // Send an error message to the client.
        send_PACKET(client, FILE_NOT_FOUND, ASYNC_WRITE, NULL, 0, buffer.path1, NULL);
        // Send the status of the request to the NS.
        // send_PACKET(NS_fd, FILE_NOT_FOUND, 5, client_IP, client_port, buffer.path1, NULL);
        send_STATUS(NS_fd, FILE_NOT_FOUND, ASYNC_WRITE, client_IP, client_port, buffer.path1, NULL);
        return;
    }

    File* file = initialise_File(buffer.path1);

    pthread_mutex_lock(&file->lock);
    int flag = 0;
    while(file->write_clients > 0 || file->read_clients > 0)
    {
        if(flag == 0)
        {
            send_PACKET(client, FILE_BEING_READ, ASYNC_WRITE, NULL, 0, buffer.path1, NULL);
            // send_PACKET(NS_fd, FILE_BEING_READ, 5, client_IP, client_port, buffer.path1, NULL);
            flag = 1;
        }
        pthread_cond_wait(&file->write_cv, &file->lock);
        if(file->is_deleted)
        {
            // Send an error message to the client.
            // send_PACKET(client, FILE_NOT_FOUND, ASYNC_WRITE, NULL, 0, buffer.path1, NULL);
            // Send the status of the request to the NS.
            // send_PACKET(NS_fd, FILE_NOT_FOUND, 5, client_IP, client_port, buffer.path1, NULL);
            send_STATUS(NS_fd, FILE_NOT_FOUND, ASYNC_WRITE, client_IP, client_port, buffer.path1, NULL);
            pthread_mutex_unlock(&file->lock);
            close(file_fd);
            return;
        }
    }
    file->write_clients += 1;
    pthread_mutex_unlock(&file->lock);

    // Write to the file and send the status of the request to the NS.
    int total_chunks = 0;
    while(total_chunks < n)
    {
        int bytes_written = write(file_fd, data[total_chunks], strlen(data[total_chunks]));
        usleep(ASYNCHRONOUS_TIMING);
        if(bytes_written < 0)
        {
            // Send an error message to the client.
            // send_PACKET(client, FILE_NOT_FOUND, ASYNC_WRITE, NULL, 0, buffer.path1, NULL);
            // Send the status of the request to the NS.
            // send_PACKET(NS_fd, FILE_NOT_FOUND, ASYNC_WRITE, client_IP, client_port, buffer.path1, NULL);
            send_STATUS(NS_fd, FILE_NOT_FOUND, ASYNC_WRITE, client_IP, client_port, buffer.path1, NULL);
            pthread_mutex_lock(&file->lock);
            file->write_clients -= 1;
            pthread_cond_signal(&file->write_cv);
            pthread_mutex_unlock(&file->lock);
            close(file_fd);
            return;
        }
        total_chunks += 1;
    }
    
    // Send the status of the request to the NS.
    // send_PACKET(NS_fd, SUCCESS, ASYNC_WRITE, client_IP, client_port, buffer.path1, NULL);
    send_STATUS(NS_fd, SUCCESS, ASYNC_WRITE, client_IP, client_port, buffer.path1, NULL);
    // Send the status of the request to the client.
    // send_PACKET(client, SUCCESS, 2, NULL, 0, buffer.path1, NULL);

    pthread_mutex_lock(&file->lock);
    file->write_clients -= 1;
    pthread_cond_signal(&file->write_cv);
    pthread_cond_signal(&file->read_cv);
    pthread_mutex_unlock(&file->lock);
    close(file_fd);
}

void Write(int client, PACKET buffer, char** data, int NS_fd, int n) // Should do the asynchronous write part.
{
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    if(getpeername(client, (struct sockaddr*)&client_addr, &client_addr_len) < 0)
    {
        for(int i = 0; i < n; i++)
        {
            free(data[i]);
        }
        free(data);
        printf(RED "Getpeername failed: %s\n" RESET, strerror(errno));
        return;
    }
    char client_IP[IP_SIZE];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_IP, IP_SIZE);
    int client_port = ntohs(client_addr.sin_port);

    // Check if the file exists in the storage server. If it exists, then truncate and write to it.
    int file_fd = open(buffer.path1, O_WRONLY | O_APPEND | O_TRUNC);
    if(file_fd < 0)
    {
        // Send an error message to the client.
        send_PACKET(client, FILE_NOT_FOUND, WRITE_REQUEST, NULL, 0, buffer.path1, NULL);
        // Send the status of the request to the NS.
        // send_PACKET(NS_fd, FILE_NOT_FOUND, 5, client_IP, client_port, buffer.path1, NULL);
        send_STATUS(NS_fd, FILE_NOT_FOUND, WRITE_REQUEST, client_IP, client_port, buffer.path1, NULL);
        for(int i = 0; i < n; i++)
        {
            free(data[i]);
        }
        free(data);
        return;
    }

    File* file = initialise_File(buffer.path1);

    pthread_mutex_lock(&file->lock);
    int flag = 0;
    while(file->write_clients > 0 || file->read_clients > 0)
    {
        if(flag == 0)
        {
            send_PACKET(client, FILE_BEING_READ, WRITE_REQUEST, NULL, 0, buffer.path1, NULL);
            // send_PACKET(NS_fd, FILE_BEING_READ, 5, client_IP, client_port, buffer.path1, NULL);
            flag = 1;
        }
        pthread_cond_wait(&file->write_cv, &file->lock);
        if(file->is_deleted)
        {
            // Send an error message to the client.
            send_PACKET(client, FILE_NOT_FOUND, WRITE_REQUEST, NULL, 0, buffer.path1, NULL);
            // Send the status of the request to the NS.
            // send_PACKET(NS_fd, FILE_NOT_FOUND, WRITE_REQUEST, client_IP, client_port, buffer.path1, NULL);
            send_STATUS(NS_fd, FILE_NOT_FOUND, WRITE_REQUEST, client_IP, client_port, buffer.path1, NULL);
            pthread_mutex_unlock(&file->lock);
            for(int i = 0; i < n; i++)
            {
                free(data[i]);
            }
            free(data);
            close(file_fd);
            return;
        }
    }
    file->write_clients += 1;
    pthread_mutex_unlock(&file->lock);

    printf("Write Started\n");
    // Write to the file and send the status of the request to the NS.
    int total_chunks = 0;
    while(total_chunks < n)
    {
        printf("Data: %s\n", data[total_chunks]);
        int bytes_written = write(file_fd, data[total_chunks], strlen(data[total_chunks]));
        if(bytes_written < 0)
        {
            // Send an error message to the client.
            send_PACKET(client, FILE_NOT_FOUND, WRITE_REQUEST, NULL, 0, buffer.path1, NULL);
            // Send the status of the request to the NS.
            // send_PACKET(NS_fd, FILE_NOT_FOUND, WRITE_REQUEST, client_IP, client_port, buffer.path1, NULL);
            send_STATUS(NS_fd, FILE_NOT_FOUND, WRITE_REQUEST, client_IP, client_port, buffer.path1, NULL);
            pthread_mutex_lock(&file->lock);
            file->write_clients -= 1;
            pthread_cond_signal(&file->write_cv);
            pthread_mutex_unlock(&file->lock);
            close(file_fd);
            for(int i = 0; i < n; i++)
            {
                free(data[i]);
            }
            free(data);
            return;
        }
        total_chunks += 1;
    }
    
    // Send the status of the request to the NS.
    // send_PACKET(NS_fd, SUCCESS, WRITE_REQUEST, client_IP, client_port, buffer.path1, NULL);
    send_STATUS(NS_fd, SUCCESS, WRITE_REQUEST, client_IP, client_port, buffer.path1, NULL);
    // Send the status of the request to the client.
    send_PACKET(client, SUCCESS, WRITE_REQUEST, NULL, 0, buffer.path1, NULL);

    pthread_mutex_lock(&file->lock);
    file->write_clients -= 1;
    pthread_cond_signal(&file->write_cv);
    pthread_cond_signal(&file->read_cv);
    pthread_mutex_unlock(&file->lock);
    for(int i = 0; i < n; i++)
    {
        free(data[i]);
    }
    free(data);
    close(file_fd);
}

int is_file(char *path)
{
    // Get the substring after the last '/'.
    char *file_name = strrchr(path, '/');
    if(file_name == NULL)
    {
        return 1;
    }
    if(strstr(file_name, ".") == NULL)
    {
        return 1;
    }
    return 0;
}
void Create(int client, PACKET buffer, int NS_fd)
{
    printf("Create happening\n");
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    if(getpeername(client, (struct sockaddr*)&client_addr, &client_addr_len) < 0)
    {
        printf(RED "Getpeername failed: %s\n" RESET, strerror(errno));
        return;
    }
    char client_IP[IP_SIZE];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_IP, IP_SIZE);
    int client_port = ntohs(client_addr.sin_port);

    printf("Client details: %s %d\n", client_IP, client_port);
    // Figure whether the path belongs to the file or directory.
    int flag = is_file(buffer.path1);
    if(flag == 1)
    {
        printf("Directory\n");
        // Do the respective stuff related to the directory.
        if(mkdir(buffer.path1, 0777) < 0)
        {
            // Send an error message to the client.
            send_PACKET(client, FILE_ALREADY_EXISTS, CREATE_REQUEST, NULL, 0, buffer.path1, NULL);
            // Send the status of the request to the NS.
            // send_PACKET(NS_fd, FILE_ALREADY_EXISTS, CREATE_REQUEST, client_IP, client_port, buffer.path1, NULL);
            send_STATUS(NS_fd, FILE_ALREADY_EXISTS, CREATE_REQUEST, client_IP, client_port, buffer.path1, NULL);
            return;
        }
        // Send the status of the request to the NS.
        // send_PACKET(NS_fd, SUCCESS, CREATE_REQUEST, client_IP, client_port, buffer.path1, NULL);
        send_STATUS(NS_fd, SUCCESS, CREATE_REQUEST, client_IP, client_port, buffer.path1, NULL);
        // Send the status of the request to the client.
        send_PACKET(client, SUCCESS, CREATE_REQUEST, NULL, 0, buffer.path1, NULL);
        return;
    }

    // Check if the file already exists in the storage server.
    int file_fd = open(buffer.path1, O_RDONLY);
    if(file_fd >= 0)
    {
        close(file_fd);
        // Send an error message to the client.
        send_PACKET(client, FILE_ALREADY_EXISTS, CREATE_REQUEST, NULL, 0, buffer.path1, NULL);
        // Send the status of the request to the NS.
        // send_PACKET(NS_fd, FILE_ALREADY_EXISTS, CREATE_REQUEST, client_IP, client_port, buffer.path1, NULL);
        send_STATUS(NS_fd, FILE_ALREADY_EXISTS, CREATE_REQUEST, client_IP, client_port, buffer.path1, NULL);
        return;
    }
    printf("File is new\n");
    printf("Path: %s\n", buffer.path1);
    int create_fd = creat(buffer.path1, 0644);
    if(create_fd < 0)
    {
        // Send an error message to the client.
        send_PACKET(client, FILE_ALREADY_EXISTS, CREATE_REQUEST, NULL, 0, buffer.path1, NULL);
        // Send the status of the request to the NS.
        // send_PACKET(NS_fd, FILE_ALREADY_EXISTS, CREATE_REQUEST, client_IP, client_port, buffer.path1, NULL);
        send_STATUS(NS_fd, FILE_ALREADY_EXISTS, CREATE_REQUEST, client_IP, client_port, buffer.path1, NULL);
        return;
    }

    // Send the status of the request to the NS.
    // send_PACKET(NS_fd, SUCCESS, CREATE_REQUEST, client_IP, client_port, buffer.path1, NULL);
    send_STATUS(NS_fd, SUCCESS, CREATE_REQUEST, client_IP, client_port, buffer.path1, NULL);
    // Send the status of the request to the client.
    send_PACKET(client, SUCCESS, CREATE_REQUEST, NULL, 0, buffer.path1, NULL);

    close(create_fd);
}

void remove_directory(const char *path, int NS_fd, char* IP, int Port) {
    struct dirent *entry;
    DIR *dir = opendir(path);

    if (!dir)  // If directory doesn't exist
    {
        // Send the status of the request to the NS.
        // send_PACKET(NS_fd, DIR_NOT_FOUND, 5, IP, Port, (char*)path, NULL);  // Cast to char*
        send_STATUS(NS_fd, DIR_NOT_FOUND, DELETE_REQUEST, IP, Port, (char*)path, NULL); 
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        char full_path[1024];
        struct stat statbuf;

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        stat(full_path, &statbuf);

        if (S_ISDIR(statbuf.st_mode)) {
            remove_directory(full_path, NS_fd, IP, Port);  // Recursively remove subdirectory
        } else {
            File* file = initialise_File(full_path);
            file->is_deleted = true;
            pthread_cond_signal(&file->read_cv);
            pthread_cond_signal(&file->write_cv);
            remove(full_path);  // Remove file
            free_File(file);
        }
    }
    closedir(dir);
    rmdir(path);  // Finally remove the directory itself
}

void Delete(int client, PACKET buffer, int NS_fd)
{
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    if(getpeername(client, (struct sockaddr*)&client_addr, &client_addr_len) < 0)
    {
        printf(RED "Getpeername failed: %s\n" RESET, strerror(errno));
        return;
    }
    char client_IP[IP_SIZE];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_IP, IP_SIZE);
    int client_port = ntohs(client_addr.sin_port);

    struct stat meta_data;
    if(stat(buffer.path1, &meta_data) < 0)
    {
        // Send an error message to the client.
        send_PACKET(client, FILE_NOT_FOUND, DELETE_REQUEST, NULL, 0, buffer.path1, NULL);
        // Send the status of the request to the NS.
        // send_PACKET(NS_fd, FILE_NOT_FOUND, 5, client_IP, client_port, buffer.path1, NULL);
        send_STATUS(NS_fd, FILE_NOT_FOUND, DELETE_REQUEST, client_IP, client_port, buffer.path1, NULL);
        return;
    }
    if(S_ISDIR(meta_data.st_mode))
    {
        // Do the respective stuff related to the directory.
        remove_directory(buffer.path1, NS_fd, client_IP, client_port);

        // Send the status of the request to the NS.
        // send_PACKET(NS_fd, SUCCESS, DELETE_REQUEST, client_IP, client_port, buffer.path1, NULL);
        send_STATUS(NS_fd, SUCCESS, DELETE_REQUEST, client_IP, client_port, buffer.path1, NULL);
        // Send the status of the request to the client.
        send_PACKET(client, SUCCESS, DELETE_REQUEST, NULL, 0, buffer.path1, NULL);
        return ;
    }
    // Delete the file.
    File* file = initialise_File(buffer.path1);
    if(file == NULL) printf("File is NULL\n");
    printf("Details of the file: %s %llu\n", file->filepath, file->iNode);
    file->is_deleted = true;
    pthread_cond_signal(&file->read_cv);
    pthread_cond_signal(&file->write_cv);

    remove(buffer.path1);
    free_File(file);
    // Send the status of the request to the NS.
    // send_PACKET(NS_fd, SUCCESS, DELETE_REQUEST, client_IP, client_port, buffer.path1, NULL);
    send_STATUS(NS_fd, SUCCESS, DELETE_REQUEST, client_IP, client_port, buffer.path1, NULL);
    // Send the status of the request to the client.
    send_PACKET(client, SUCCESS, DELETE_REQUEST, NULL, 0, buffer.path1, NULL);

}

void Stream(int client, PACKET buffer, int NS_fd)
{
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    if(getpeername(client, (struct sockaddr*)&client_addr, &client_addr_len) < 0)
    {
        printf(RED "Getpeername failed: %s\n" RESET, strerror(errno));
        return;
    }
    char client_IP[IP_SIZE];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_IP, IP_SIZE);
    int client_port = ntohs(client_addr.sin_port);

    int file_fd = open(buffer.path1, O_RDONLY);
    if(file_fd < 0)
    {
        // Send an error message to the client.
        send_PACKET(client, FILE_NOT_FOUND, STREAM_REQUEST, NULL, 0, buffer.path1, NULL);
        // Send the status of the request to the NS.
        // send_PACKET(NS_fd, FILE_NOT_FOUND, 5, client_IP, client_port, buffer.path1, NULL);
        send_STATUS(NS_fd, FILE_NOT_FOUND, STREAM_REQUEST, client_IP, client_port, buffer.path1, NULL);
        return;
    }

    File* file = initialise_File(buffer.path1);

    pthread_mutex_lock(&file->lock);
    int flag = 0;
    while(file->write_clients > 0)
    {
        if(flag == 0)
        {
            send_PACKET(client, FILE_BEING_WRITTEN, STREAM_REQUEST, NULL, 0, buffer.path1, NULL);
            // send_PACKET(NS_fd, FILE_NOT_FOUND, 5, client_IP, client_port, buffer.path1, NULL);
            flag = 1;
        }
        pthread_cond_wait(&file->read_cv, &file->lock);
        if(file->is_deleted)
        {
            // Send an error message to the client.
            send_PACKET(client, FILE_NOT_FOUND, STREAM_REQUEST, NULL, 0, buffer.path1, NULL);
            send_STATUS(NS_fd, FILE_NOT_FOUND, STREAM_REQUEST, client_IP, client_port, buffer.path1, NULL);
            // Send the status of the request to the NS.
            pthread_mutex_unlock(&file->lock);
            close(file_fd);
            return;
        }
    }

    file->read_clients += 1;
    pthread_mutex_unlock(&file->lock);

    // Read from the file and send it to the client
    int total_chunks = 0;
    struct stat meta_data ;
    if (fstat(file_fd, &meta_data) < 0) 
    {
        // Send response to client and NS.
        // send_PACKET(NS_fd, FILE_NOT_FOUND, STREAM_REQUEST, client_IP, client_port, buffer.path1, NULL);
        send_STATUS(NS_fd, FILE_NOT_FOUND, STREAM_REQUEST, client_IP, client_port, buffer.path1, NULL);
        send_PACKET(client, FILE_NOT_FOUND, STREAM_REQUEST, NULL, 0, buffer.path1, NULL);
        printf(RED "Failed to get file size: %s\n" RESET, strerror(errno));
        close(file_fd);
        return;
    }
    total_chunks = (meta_data.st_size + MAX_DATA_SIZE - 1) / MAX_DATA_SIZE;
    int chunks_sent = 0;
    int bytes_read, bytes_sent;
    char data[MAX_DATA_SIZE];
    while ((bytes_read = read(file_fd, data, sizeof(data))) > 0) {
        send_DataPacket(client, SUCCESS, 1, total_chunks, data);
        // if (bytes_sent < 0) {
        //     // Handle send error
        //     printf(RED "Send to client failed: %s\n" RESET, strerror(errno));
        //     close(file_fd);
        //     pthread_mutex_lock(&file->lock);
        //     file->read_clients -= 1;
        //     pthread_cond_signal(&file->read_cv);
        //     pthread_cond_signal(&file->write_cv);
        //     pthread_mutex_unlock(&file->lock);
        //     return;
        // }
        chunks_sent++;
    }
    // Send the status of the request to the NS.
    // send_PACKET(NS_fd, SUCCESS, 1, client_IP, client_port, buffer.path1, NULL);
    send_STATUS(NS_fd, SUCCESS, STREAM_REQUEST, client_IP, client_port, buffer.path1, NULL);
    // Send the status of the request to the client.
    send_PACKET(client, SUCCESS, STREAM_REQUEST, NULL, 0, buffer.path1, NULL);


    pthread_mutex_lock(&file->lock);
    file->read_clients -= 1;
    pthread_cond_signal(&file->read_cv);
    pthread_mutex_unlock(&file->lock);
    close(file_fd);
}


void Info(int client, PACKET buffer , int NS_fd)
{
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    if(getpeername(client, (struct sockaddr*)&client_addr, &client_addr_len) < 0)
    {
        // send_PACKET(NS_fd, FILE_NOT_FOUND, INFO_REQUEST, NULL, 0, buffer.path1, NULL);
        send_STATUS(NS_fd, FILE_NOT_FOUND, INFO_REQUEST, NULL, 0, buffer.path1, NULL);
        printf(RED "Getpeername failed: %s\n" RESET, strerror(errno));
        return;
    }

    char client_IP[IP_SIZE];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_IP, IP_SIZE);
    int client_port = ntohs(client_addr.sin_port);

    struct stat meta_data ;
    if(stat(buffer.path1, &meta_data) < 0)
    {
        // Send an error message to the client.
        send_PACKET(client, FILE_NOT_FOUND, INFO_REQUEST, NULL, 0, buffer.path1, NULL);
        // Send the status of the request to the NS.
        // send_PACKET(NS_fd, FILE_NOT_FOUND, 6, client_IP, client_port, buffer.path1, NULL);
        send_STATUS(NS_fd, FILE_NOT_FOUND, INFO_REQUEST, client_IP, client_port, buffer.path1, NULL);
        return;
    }

    char data[MAX_SEND_SIZE];

    // Should add more fields of the meta of the file.
    snprintf(data, MAX_SEND_SIZE, "File size: %lld\n File Permissions: %o\n File Owner: %d\n File Group: %d\n Last Access Time: %s Last Modification Time: %s Last Status Change Time: %s\n", 
         (long long)meta_data.st_size, meta_data.st_mode, meta_data.st_uid, meta_data.st_gid, 
         ctime(&meta_data.st_atime), ctime(&meta_data.st_mtime), ctime(&meta_data.st_ctime));


    // Send the data to the client.
    send_DataPacket(client, SUCCESS, INFO_REQUEST, 1, data);

    // Send the status of the request to the NS.
    // send_PACKET(NS_fd, SUCCESS, 6, client_IP, client_port, buffer.path1, NULL);
    send_STATUS(NS_fd, SUCCESS, INFO_REQUEST, client_IP, client_port, buffer.path1, NULL);
}

void send_PACKET(int fd, int status, int request_type, char* IP, int PORT, char* path1, char* path2)
{
    PACKET buffer ;
    buffer.status = status;
    buffer.request_type = request_type;
    if(IP != NULL)
    {
        strcpy(buffer.IP, IP);
    }else {
        buffer.IP[0] = '\0';
    }
    buffer.PORT = PORT;
    if(path1 != NULL)
    {
        strcpy(buffer.path1, path1);
    }else{
        buffer.path1[0] = '\0';
    }
    if(path2 != NULL)
    {
        strcpy(buffer.path2, path2);
    }
    else{
        buffer.path2[0] = '\0';
    }
    pthread_mutex_lock(&send_lock);
    int header = PACKET_STRUCT;
    int send_bytes;
    // while(1)
    // {
    send_bytes = send(fd, &header, sizeof(header), 0);
    if(send_bytes < 0)
    {
        // if(errno == EWOULDBLOCK || errno == EAGAIN)
        // {
        //     continue;
        // }
        printf(RED "Send failed: %s\n" RESET, strerror(errno));
        return;
    }
    if(send_bytes != sizeof(header))
    {
        printf(RED "Invalid request\n" RESET);
        return;
    }
    // }
    // while(1)
    // {
    send_bytes = send(fd, &buffer, sizeof(buffer), 0);
    if(send_bytes < 0)
    {
        printf(RED "Send failed: %s\n" RESET, strerror(errno));
        return;
    }
    if(send_bytes != sizeof(buffer))
    {
        printf(RED "Invalid request\n" RESET);
        return;
    }
    pthread_mutex_unlock(&send_lock);
    return ;
}

void send_DataPacket(int fd ,int status, int request_type, int number_of_packets, char* data)
{
    Data_Packet buffer ;
    buffer.status = status;
    buffer.request_type = request_type;
    buffer.number_of_packets = number_of_packets;
    if(data != NULL)
    {
        strcpy(buffer.data, data);
    }else{
        buffer.data[0] = '\0';
    }
    int send_bytes ;
    pthread_mutex_lock(&send_lock);
    int header = DATA_PACKET_STRUCT;
    // while(1)
    // {
        send_bytes = send(fd, &header, sizeof(header), 0);
        if(send_bytes < 0)
        {
            printf(RED "Send failed: %s\n" RESET, strerror(errno));
            return;
        }
        if(send_bytes != sizeof(header))
        {
            printf(RED "Invalid request\n" RESET);
            return;
        }
    // }
    // while(1)
    // {
    send_bytes = send(fd, &buffer, sizeof(buffer), 0);
    if(send_bytes < 0)
    {
        printf(RED "Send failed: %s\n" RESET, strerror(errno));
        return;
    }
    if(send_bytes != sizeof(buffer))
    {
        printf(RED "Invalid request\n" RESET);
        return;
    }
    // }
    pthread_mutex_unlock(&send_lock);
    return ;
}

void send_STATUS(int fd, int status, int request_type, char* IP, int PORT, char* path1, char* path2) {
    struct sockaddr_in NS_server;
    NS_server.sin_family = AF_INET;
    NS_server.sin_port = htons(ns_port);
    printf("NS_IP: %s\n", ns_ip);
    NS_server.sin_addr.s_addr = inet_addr(ns_ip);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf(RED "Socket creation failed: %s\n" RESET, strerror(errno));
        return;
    }

    // Binding to the local port (use ss_ss_addr if necessary)
    struct sockaddr_in ss_ss_addr;
    ss_ss_addr.sin_family = AF_INET;
    ss_ss_addr.sin_port = htons(0);
    ss_ss_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr*)&ss_ss_addr, sizeof(ss_ss_addr)) < 0) {
        printf(RED "Bind failed: %s\n" RESET, strerror(errno));
        close(sock);
        return;
    }

    if (connect(sock, (struct sockaddr*)&NS_server, sizeof(NS_server)) < 0) {
        printf(RED "Connect failed: %s\n" RESET, strerror(errno));
        close(sock);
        return;
    }

    PACKET buffer = {0};  // Zero-initialize buffer
    buffer.status = status;
    buffer.request_type = request_type;

    if (IP != NULL) {
        strncpy(buffer.IP, IP, sizeof(buffer.IP) - 1);
        buffer.IP[sizeof(buffer.IP) - 1] = '\0';
    } else {
        buffer.IP[0] = '\0';
    }
    buffer.PORT = PORT;

    if (path1 != NULL) {
        strncpy(buffer.path1, path1, sizeof(buffer.path1) - 1);
        buffer.path1[sizeof(buffer.path1) - 1] = '\0';
    } else {
        buffer.path1[0] = '\0';
    }

    if (path2 != NULL) {
        strncpy(buffer.path2, path2, sizeof(buffer.path2) - 1);
        buffer.path2[sizeof(buffer.path2) - 1] = '\0';
    } else {
        buffer.path2[0] = '\0';
    }

    pthread_mutex_lock(&send_lock);
    int header = STATUS_CHECK_STRUCT;
    if (send(sock, &header, sizeof(header), 0) != sizeof(header)) {
        printf(RED "Send failed: %s\n" RESET, strerror(errno));
        pthread_mutex_unlock(&send_lock);
        close(sock);
        return;
    }

    if (send(sock, &buffer, sizeof(buffer), 0) != sizeof(buffer)) {
        printf(RED "Send failed: %s\n" RESET, strerror(errno));
        pthread_mutex_unlock(&send_lock);
        close(sock);
        return;
    }
    pthread_mutex_unlock(&send_lock);
    close(sock);
}
