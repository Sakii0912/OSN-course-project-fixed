#include "../headers.h"
#include "NM.h"

extern SS_list S;
extern int server_fd;
extern pthread_mutex_t socket_Rlock;
extern pthread_mutex_t socket_Wlock;
extern pthread_mutex_t socket_Rlock_2;
extern pthread_mutex_t socket_Wlock_2;
extern pthread_mutex_t SS_lock;

extern int SS_count;
extern pthread_mutex_t SS_count_lock;
extern pthread_mutex_t log_lock;

extern char NS_IP[20];
extern int NS_PORT;

void cpyFile(char* file_name1, char* file_name2, int source, int destination);

void SSbackupMap_handler(int SS_number);

char* processString(const char* input) {
    if (!input) return NULL; // Handle NULL input
    
    // Check if the string starts with "./"
    if (strncmp(input, "./", 2) == 0) {
        return strdup(input); // Return a copy of the original string
    }

    // Allocate memory for the new string "./" + input + '\0'
    size_t newSize = strlen(input) + 3; // 3 = 2 for "./" + 1 for '\0'
    char* result = (char*)malloc(newSize);
    if (!result) {
        perror("Failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    // Prepend "./" to the input string
    strcpy(result, "./");
    strcat(result, input);

    return result;
}

// Comparison function for qsort
int compareStrings(const void *a, const void *b)
{
    const char *str1 = *(const char **)a;
    const char *str2 = *(const char **)b;
    return strcmp(str1, str2);
}

// Function to get and print the public IP address of the server
void retrieveIP(char *host)
{
    struct ifaddrs *ifaddr, *ifa;

    if (host == NULL)
    {
        host = (char *)malloc(sizeof(char) * NI_MAXHOST);
    }

    // Retrieve the list of network interfaces
    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        return;
    }

    // Loop through interfaces and find the public IP (ignoring loopback)
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET)
        {
            continue;
        }
        getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
        if (strstr(host, "127.") == NULL)
        { // Ignore loopback IPs
            printf("Naming Server Public IP: %s\n", host);
            break;
        }
    }

    // Use getsockname to retrieve the port number of the server socket

    freeifaddrs(ifaddr);

    return;
}

// Function to initialize the SS list

void initialiseSS()
{
    if (S == NULL)
    {
        S = (SS_list)malloc(sizeof(st_SS_list) * SS_MAX);
    }
    else
    {
        printf("SS already initialized; error\n");
        return;
    }
    for (int i = 0; i < SS_MAX; i++)
    {
        S[i].in_use = false;
        S[i].client_port = -1;
        S[i].NM_port = -1;
        S[i].file_count = 0;
        S[i].dir_count = 0;
        S[i].files = getNode();
        S[i].dirs = getNode();
        insert(S[i].dirs, ".");        // insert the root directory
        S[i].backups_stored = 0;
        S[i].backupID_1 = -1;
        S[i].backupID_2 = -1;
        S[i].SS_Client_socket = -1;
        pthread_mutex_init(&S[i].lock, NULL);
    }
    return;
}

// Function to add a new SS to the SS list, return the SS number
int add_SS(int client_port, int NM_port, char* client_IP, char* NM_IP){
    pthread_mutex_lock(&SS_lock);

    for(int i = 0; i<SS_MAX; i++)
    {
        if((S[i].in_use == false) && (S[i].client_port == -1)){
            S[i].in_use = true;
            S[i].client_port = client_port;
            S[i].NM_port = NM_port;
            strcpy(S[i].client_ip, NM_IP);
            strcpy(S[i].NM_ip, NM_IP);

            pthread_mutex_unlock(&SS_lock);
            SSbackupMap_handler(i);
            return i;
        }
        
    }
    pthread_mutex_unlock(&SS_lock);

    return -1;
}

// function to handle the SS backup mapping process
void SSbackupMap_handler(int SS_number)
{

    pthread_mutex_lock(&SS_lock);
    pthread_mutex_lock(&SS_count_lock);

    if (SS_count == 1)
    {
        pthread_mutex_unlock(&SS_count_lock);
        pthread_mutex_unlock(&SS_lock);
        return;
    }
    else if (SS_count == 2)
    {

        int inuse1 = -1;
        int inuse2 = -1;

        for (int i = 0; i < SS_MAX; i++)
        {
            if (S[i].in_use)
            {
                if (inuse1 == -1)
                {
                    inuse1 = i;
                }
                else
                {
                    inuse2 = i;
                    break;
                }
            }
        }

        if ((S[inuse1].backupID_1 == -1) && (S[inuse1].backupID_2 == -1))
        {
            S[inuse1].backupID_1 = inuse2;
        }

        if ((S[inuse2].backupID_1 != -1) && (S[inuse2].backupID_2 == -1))
        {
            S[inuse2].backupID_2 = inuse2;
        }

        pthread_mutex_unlock(&SS_lock);
        pthread_mutex_unlock(&SS_count_lock);

        return;
    }
    else
    {

        int to_be_backedup = -1;

        for (int i = 0; i < SS_MAX; i++)
        {
            if (S[i].in_use)
            {
                if ((S[i].backupID_1) == -1 && (S[i].backupID_2 == -1))
                {
                    to_be_backedup = i;
                    break;
                }
            }
        }

        int backup1 = -1;
        int backup2 = -1;

        for (int i = 0; i < SS_MAX; i++)
        { // search for the inuse directory with the least backups
            if (S[i].in_use)
            {
                if (backup1 == -1)
                {
                    backup1 = i;
                }
                else if (backup2 == -1)
                {
                    backup2 = i;
                    break;
                }
                else
                {
                    if (S[backup1].backups_stored > S[backup2].backups_stored)
                    {
                        backup1 = i;
                    }
                    else
                    {
                        backup2 = i;
                    }
                }
            }
        }

        for (int i = 0; i < SS_MAX; i++)
        { // search for the inuse directory with second least backups
            if (S[i].in_use)
            {
                if (i != backup1)
                {
                    if (backup2 == -1)
                    {
                        backup2 = i;
                    }
                    else
                    {
                        if (S[backup1].backups_stored > S[backup2].backups_stored)
                        {
                            backup1 = i;
                        }
                        else
                        {
                            backup2 = i;
                        }
                    }
                }
            }
        }

        S[to_be_backedup].backupID_1 = backup1;
        S[to_be_backedup].backupID_2 = backup2;

        pthread_mutex_unlock(&SS_lock);
        pthread_mutex_unlock(&SS_count_lock);

        return;
    }
}

// function for read requests

int read_request_finder(char *file)
{ // return -1 if file not found, else return the SS number

    pthread_mutex_lock(&SS_lock);

    for (int i = 0; i < SS_MAX; i++)
    {
        if (S[i].files != NULL)
        {
            if (search(S[i].files, file))
            {

                if (S[i].in_use)
                {
                    pthread_mutex_unlock(&SS_lock);
                    return i;
                }

                if (S[i].backupID_1 != -1)
                {
                    if (search(S[S[i].backupID_1].files, file))
                    {
                        pthread_mutex_unlock(&SS_lock);
                        return S[i].backupID_1;
                    }
                }

                if (S[i].backupID_2 != -1)
                {
                    if (search(S[S[i].backupID_2].files, file))
                    {
                        pthread_mutex_unlock(&SS_lock);
                        return S[i].backupID_2;
                    }
                }
            }
        }
    }

    pthread_mutex_unlock(&SS_lock);

    return -1;
}

// function for write requests

int dir_write_request_finder(char* file){
    pthread_mutex_lock(&SS_lock);

    for(int i = 0; i<SS_MAX; i++){
        if(S[i].in_use){
            if(search(S[i].dirs, file)){
                pthread_mutex_unlock(&SS_lock);
                return i;
            }
        }
    }

    pthread_mutex_unlock(&SS_lock);

    return -1;
}

int write_request_finder(char *file)
{ // return -1 if file not found, else return the SS number; use same as create request and delete request

    pthread_mutex_lock(&SS_lock);

    for (int i = 0; i < SS_MAX; i++)
    {

        if (S[i].in_use)
        {
            if (search(S[i].files, file))
            {

                pthread_mutex_unlock(&SS_lock);
                return i;
            }
        }
    }

    pthread_mutex_unlock(&SS_lock);

    return -1;
}

int dir_request_finder(char *dir)
{

    pthread_mutex_lock(&SS_lock);

    for (int i = 0; i < SS_MAX; i++)
    {

        if (S[i].in_use)
        {
            if (search(S[i].dirs, dir))
            {

                pthread_mutex_unlock(&SS_lock);
                return i;
            }
        }
    }

    pthread_mutex_unlock(&SS_lock);

    return -1;
}

int create_request_finder(char *file)
{
    if(strcmp(file, ".") != 0){
        return dir_request_finder(file);
    }
    int min = __INT_MAX__;
    int index = 0;
    for(int i = 0; i<SS_MAX; i++){
        if(S[i].in_use){
            if((S[i].file_count + S[i].dir_count) < min){
                min = S[i].file_count + S[i].dir_count;
                index = i;
            }
        }
    }
    return index;
}

// Function to copy a file from one SS to another, for the purpose of databackup
void cpyFile(char* file_name1, char* file_name2, int source, int destination)
{
    
    // have to send a read request to the file in the source SS, and a write request to the file in the destination SS

    // read request to the source SS

    int struct_type = PACKET_STRUCT;
    PACKET read_packet;
    read_packet.request_type = READ_REQUEST;
    strcpy(read_packet.path1, file_name1);

    struct sockaddr_in source_addr;
    int source_fd;

    if ((source_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("Socket creation failed\n");
        return;
    }

    // Server address configuration
    memset(&source_addr, 0, sizeof(source_addr));
    source_addr.sin_family = AF_INET;
    source_addr.sin_port = htons(S[source].client_port);

    // Convert IP to binary form
    if (inet_pton(AF_INET, S[source].client_ip, &source_addr.sin_addr) <= 0)
    {
        printf("Invalid IP address\n");
        close(source_fd);
        return;
    }

    // Connect to the server
    if (connect(source_fd, (struct sockaddr *)&source_addr, sizeof(source_addr)) < 0)
    {
        printf("Connection failed\n");
        close(source_fd);
        return;
    }

    pthread_mutex_lock(&socket_Rlock_2);

    pthread_mutex_lock(&socket_Wlock_2);
    send(source_fd, &struct_type, sizeof(int), 0);
    send(source_fd, &read_packet, sizeof(PACKET), 0);
    pthread_mutex_unlock(&socket_Wlock_2);

    int counter = 0;

    Data_Packet buffer_packet;

    char** data = NULL;

    while(1){

        recv(source_fd, &struct_type, sizeof(int), 0);
        if(struct_type == PACKET_STRUCT){
            recv(source_fd, &read_packet, sizeof(PACKET), 0);
            break;
        }
        else{
            recv(source_fd, &buffer_packet, sizeof(Data_Packet), 0);
            counter++;
            if(counter == 1){
                data = (char**)malloc(sizeof(char*)*(buffer_packet.number_of_packets));
                for(int i = 0; i<buffer_packet.number_of_packets; i++){
                    data[i] = (char*)malloc(sizeof(char)* MAX_SEND_SIZE);
                }
                strcpy(data[0], buffer_packet.data);
                continue;
            }
            strcpy(data[counter-1], buffer_packet.data);
            if(counter == buffer_packet.number_of_packets){
                recv(source_fd, &struct_type, sizeof(int), 0);
                recv(source_fd, &read_packet, sizeof(PACKET), 0);
                break;
            }

        }

    }   

    close(source_fd);

    printf("Data received\n");

    printf("--------------------\n");
    for(int i = 0; i<counter; i++){
        printf("%s", data[i]);
    }
    printf("--------------------\n");

    pthread_mutex_unlock(&socket_Rlock_2);

    // write request to the destination SS

    struct_type = PACKET_STRUCT;
    PACKET write_packet;
    write_packet.request_type = WRITE_REQUEST;
    strcpy(write_packet.path1, file_name2);

    int dest_fd;
    struct sockaddr_in dest_addr;

    if ((dest_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("Socket creation failed\n");
        return;
    }

    // Server address configuration
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(S[destination].client_port);

    // Convert IP to binary form
    if (inet_pton(AF_INET, S[destination].client_ip, &dest_addr.sin_addr) <= 0)
    {
        printf("Invalid IP address\n");
        close(dest_fd);
        return;
    }

    // Connect to the server
    if (connect(dest_fd, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0)
    {
        printf("Connection failed\n");
        close(dest_fd);
        return;
    }
    
    pthread_mutex_lock(&socket_Wlock_2);
    send(dest_fd, &struct_type, sizeof(int), 0);
    send(dest_fd, &write_packet, sizeof(PACKET), 0);

    Data_Packet write_buffer_packet;
    write_buffer_packet.number_of_packets = counter;

    struct_type = DATA_PACKET_STRUCT;

    for(int i = 0; i<counter; i++){
        strcpy(write_buffer_packet.data, data[i]);
        send(dest_fd, &struct_type, sizeof(int), 0);
        send(dest_fd, &write_buffer_packet, sizeof(Data_Packet), 0);
    }

    pthread_mutex_unlock(&socket_Wlock_2);

    printf("Data sent\n");

    close(dest_fd);

    return;

}

char* sub_strings(const char* str1, const char* str2){  // str1 - str2
    char* pos = strstr(str1, str2);
    int result_len = strlen(str1) - strlen(str2);
    char* result = (char*)malloc(result_len + 1);
    strcpy(result, pos + strlen(str2));
    result[result_len] = '\0';
    return result;
}

// Function to handle the SS requests
