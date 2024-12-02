#include "../headers.h"
#include "NM.h"
#include "LRU.h"

LRUCache* cache;

SS_list S;
int server_fd;
int server_fd_2; // for writing to the SS's
pthread_mutex_t socket_Rlock;
pthread_mutex_t socket_Wlock;
pthread_mutex_t socket_Rlock_2;
pthread_mutex_t socket_Wlock_2;
pthread_mutex_t SS_lock;

int SS_count;
pthread_mutex_t SS_count_lock;
pthread_mutex_t log_lock;

char NS_IP[20];
int NS_PORT;

// LRUCache *cache;

/// Function to check if the SS's are connected
// void* checkSS(void* arg){

//     while(1){

//         for(int i = 0; i<SS_MAX; i++){
//             if(S[i].in_use){
//                 ifgetsockopt(S[i].SS_socket, SOL_SOCKET, SO_ERROR, &(S[i].SS_addr), &addr_len) == -1) == -1){
//                     printf("SS %d is not connected\n", i);
//                     S[i].in_use = false;
//                     close(S[i].SS_socket);
//                 }
//             }
//         }

//         sleep(SS_CHECK_INTERVAL);
//     }

//     return NULL;
// }

// Function to keep checking for confirmation packets from the SS's
// void* checkSS(void* arg){

//     while(1){

//         for(int i = 0; i<SS_MAX; i++){
//             if(S[i].in_use){
//                 int struct_type;
//                 if(recv(S[i].SS_socket, &struct_type, sizeof(int), MSG_DONTWAIT) == -1){
//                     continue; // no packet received
//                 }else{
//                     if(struct_type == PACKET_STRUCT){
//                         PACKET packet;
//                         if(recv(S[i].SS_socket, &packet, sizeof(PACKET), 0) == -1){
//                             perror("Receive failed");
//                         }
//                         // add to the log, either that an operation was successful or not

//                         // if it's an asynchronous write, send confirmation back to the client, using the IP and port in the packet

//                         if(packet.request_type == ASYNC_WRITE){
//                             int client_fd = socket(AF_INET, SOCK_STREAM, 0);
//                             if(client_fd == -1){
//                                 perror("Socket creation failed");
//                                 exit(EXIT_FAILURE);
//                             }

//                             struct sockaddr_in client_addr;
//                             memset(&client_addr, 0, sizeof(client_addr));
//                             client_addr.sin_family = AF_INET;
//                             client_addr.sin_port = htons(packet.PORT);
//                             if(inet_pton(AF_INET, packet.IP, &client_addr.sin_addr) <= 0){
//                                 perror("Invalid address/ Address not supported");
//                                 exit(EXIT_FAILURE);
//                             }

//                             if(connect(client_fd, (struct sockaddr *)&client_addr, sizeof(client_addr)) == -1){
//                                 perror("Connect failed");
//                                 exit(EXIT_FAILURE);
//                             }

//                             int struct_type = PACKET_STRUCT;

//                             pthread_mutex_lock(&socket_Wlock);

//                             send(client_fd, &struct_type, sizeof(int), 0);
//                             send(client_fd, &packet, sizeof(PACKET), 0);

//                             pthread_mutex_unlock(&socket_Wlock);

//                             close(client_fd);
//                         }

//                     }
//                 }
//             }

//             sleep(1);
//         }

//     }
// }

void *status_handler(void *args){
    PACKET packet;

    int ss_fd = *(int *)args;
    // int struct_type;
    int recv_bytes = recv(ss_fd, &packet, sizeof(PACKET), 0);
    if (recv_bytes < 0)
    {
        printf("recv failed\n");
        return NULL;
    }
    // printf("Struct type: %d\n", struct_type);
    printf("Status handler connected\n");
    printf("Status : %d\n", packet.status);
    printf("Request type : %d\n", packet.request_type);

    pthread_mutex_unlock(&socket_Rlock);

    pthread_exit(NULL);
}

// Function to handle the client requests
void *client_handler(void *arg){

    int client_fd = *(int *)arg;
    printf("Client thread connected\n");
    char *temp_name_for_processing = (char *)malloc(sizeof(char) * MAX_FILEPATH_SIZE);
    // recv the actual data_packet struct
    PACKET packet;

    recv(client_fd, &packet, sizeof(PACKET), 0);
    printf("Path recieved: %s\n", packet.path1);
    printf("File recieved: %s\n", packet.path2);

    if (packet.request_type == CREATE_REQUEST)
    {
        printf("Create request received\n");
        pthread_mutex_unlock(&socket_Rlock);

        if (strcmp(packet.path1, ".") == 0)
        {
            // do nothing
        }
        else
        {
            temp_name_for_processing = processString(packet.path1);
            strcpy(packet.path1, temp_name_for_processing);
        }

        printf("modified path recieved: %s\n", packet.path1);

        // LRU cache implementation in create request.
        CacheNode* entry = getFromCache(cache, packet.path1)
        int SS_id;
        if(entry){
            SS_id = entry->SS_id;
            printf("Cache hit for path: %s\n", packet.path1);
        }
        else{
            SS_id = create_request_finder(packet.path1);
            if (SS_id == -1)
            {
                // send the error message to the client

                printf("Create request failed\n");

                int struct_type = PACKET_STRUCT;
                pthread_mutex_lock(&socket_Wlock);

                send(client_fd, &struct_type, sizeof(int), 0);
                PACKET response_packet;
                response_packet.status = FILE_NOT_FOUND;
                response_packet.request_type = STATUS_CHECK;

                strcpy(response_packet.IP, packet.IP);
                response_packet.PORT = packet.PORT;
                strcpy(response_packet.path1, packet.path1);
                send(client_fd, &response_packet, sizeof(PACKET), 0);
                pthread_mutex_unlock(&socket_Wlock);

                close(client_fd);

                return NULL;
            }
            else
            {

                printf("Create request found\n");
                addToCache(cache, packet.path1, SS_id);
                // send the success packet to the client, with the IP and port of the SS
                int struct_type = PACKET_STRUCT;
                pthread_mutex_lock(&socket_Wlock);
                send(client_fd, &struct_type, sizeof(int), 0);
                PACKET response_packet;
                response_packet.status = SUCCESS;
                response_packet.request_type = STATUS_CHECK;
                strcpy(response_packet.IP, S[SS_id].client_ip);
                response_packet.PORT = S[SS_id].client_port;
                strcpy(response_packet.path1, packet.path1);
                send(client_fd, &response_packet, sizeof(PACKET), 0);
                pthread_mutex_unlock(&socket_Wlock);

                printf("Create request successful, sent back to client\n");

                close(client_fd);

                // add the file to the SS's trie

                char path_to_add[MAX_FILEPATH_SIZE];
                strcpy(path_to_add, packet.path1);
                strcat(path_to_add, "/");
                strcat(path_to_add, packet.path2);
                char path[MAX_FILEPATH_SIZE];
                strcpy(path, packet.path2);

                if (strstr(path, ".") == NULL)
                {
                    insert(S[SS_id].dirs, path_to_add);
                }
                else
                {
                    insert(S[SS_id].files, path_to_add);
                }

                // send the request packet to the backup SS, with the naming server IP and port
                struct_type = PACKET_STRUCT;
                // pthread_mutex_lock(&socket_Wlock);
                PACKET backup_packet;
                backup_packet.request_type = CREATE_REQUEST;
                strcpy(backup_packet.IP, NS_IP);
                backup_packet.PORT = NS_PORT;
                strcpy(backup_packet.path1, packet.path1);
                strcpy(backup_packet.path2, packet.path2);
                if (S[SS_id].backupID_1 != -1)
                {
                    pthread_mutex_lock(&socket_Wlock);

                    // connect(S[S[SS_id].backupID_1].SS_Client_socket, (struct sockaddr *)&(S[S[SS_id].backupID_1].SS_addr), sizeof(S[S[SS_id].backupID_1].SS_addr));

                    send(S[S[SS_id].backupID_1].SS_Client_socket, &struct_type, sizeof(int), 0);
                    send(S[S[SS_id].backupID_1].SS_Client_socket, &backup_packet, sizeof(PACKET), 0);
                    pthread_mutex_unlock(&socket_Wlock);
                }
                if (S[SS_id].backupID_2 != -1)
                {
                    pthread_mutex_lock(&socket_Wlock);
                    send(S[S[SS_id].backupID_2].SS_Client_socket, &struct_type, sizeof(int), 0);
                    send(S[S[SS_id].backupID_2].SS_Client_socket, &backup_packet, sizeof(PACKET), 0);
                    pthread_mutex_unlock(&socket_Wlock);
                }

                return NULL;
            }
        }
    }
    else if (packet.request_type == DELETE_REQUEST)
    {

        pthread_mutex_unlock(&socket_Rlock);

        printf("Delete request received\n");

        // could be a file or a directory

        int flag_dir = 0;

        if (strcmp(packet.path1, ".") == 0)
        {
            // do nothing
        }
        else
        {
            temp_name_for_processing = processString(packet.path1);
            strcpy(packet.path1, temp_name_for_processing);
        }
        CacheNode* entry = getFromCache(cache, packet.path1);
        int SS_id;
        if (entry) {
            SS_id = entry->SS_id;
            printf("Cache hit for path: %s\n", packet.path1);
        } 
        else{
            SS_id = write_request_finder(packet.path1); // file
            if (SS_id == -1)
            {
                SS_id = dir_request_finder(packet.path1); // directory
                flag_dir = 1;
            }

            printf("SS_id: %d\n", SS_id);

            if (SS_id == -1)
            {

                printf("Delete request failed\n");

                // send the error packet to the client
                int struct_type = PACKET_STRUCT;
                pthread_mutex_lock(&socket_Wlock);
                send(client_fd, &struct_type, sizeof(int), 0);
                PACKET response_packet;
                strcpy(S[SS_id].client_ip, packet.IP);
                response_packet.status = FILE_NOT_FOUND;
                response_packet.request_type = STATUS_CHECK;
                strcpy(response_packet.IP, packet.IP);
                response_packet.PORT = S[SS_id].client_port;
                strcpy(response_packet.path1, packet.path1);
                send(client_fd, &response_packet, sizeof(PACKET), 0);
                pthread_mutex_unlock(&socket_Wlock);

                close(client_fd);

                return NULL;
            }
            else
            {
                addToCache(cache, packet.path1, SS_id);
                // send the success packet to the client, with the IP and port of the SS
                int struct_type = PACKET_STRUCT;
                pthread_mutex_lock(&socket_Wlock);
                send(client_fd, &struct_type, sizeof(int), 0);
                PACKET response_packet;
                response_packet.status = SUCCESS;
                response_packet.request_type = STATUS_CHECK;
                strcpy(response_packet.IP, S[SS_id].client_ip);
                response_packet.PORT = S[SS_id].client_port;
                strcpy(response_packet.path1, packet.path1);
                send(client_fd, &response_packet, sizeof(PACKET), 0);
                // addToCache(cache, packet.path1, S[SS_id].client_ip, S[SS_id].client_port);
                // printCache(cache);
                pthread_mutex_unlock(&socket_Wlock);

                close(client_fd);

                if (flag_dir)
                { // it's a directory
                    deleteString(S[SS_id].dirs, packet.path1);
                }
                else
                {
                    deleteString(S[SS_id].files, packet.path1);
                }

                // send the request packet to the backup SS, with the naming server IP and port
                struct_type = PACKET_STRUCT;
                // pthread_mutex_lock(&socket_Wlock);
                PACKET backup_packet;
                backup_packet.request_type = DELETE_REQUEST;
                strcpy(backup_packet.IP, NS_IP);
                backup_packet.PORT = NS_PORT;
                strcpy(backup_packet.path1, packet.path1);
                strcpy(backup_packet.path2, packet.path2);
                // if(S[SS_id].backupID_1 != -1){
                //     pthread_mutex_lock(&socket_Wlock);
                //     connect(S[S[SS_id].backupID_1].SS_Client_socket, (struct sockaddr *)&(S[S[SS_id].backupID_1].SS_addr), sizeof(S[S[SS_id].backupID_1].SS_addr));
                //     send(S[S[SS_id].backupID_1].SS_Client_socket, &struct_type, sizeof(int), 0);
                //     send(S[S[SS_id].backupID_1].SS_Client_socket, &backup_packet, sizeof(PACKET), 0);
                //     pthread_mutex_unlock(&socket_Wlock);
                // }
                // if(S[SS_id].backupID_2 != -1){
                //     pthread_mutex_lock(&socket_Wlock);
                //     connect(S[S[SS_id].backupID_2].SS_Client_socket, (struct sockaddr *)&(S[S[SS_id].backupID_2].SS_addr), sizeof(S[S[SS_id].backupID_2].SS_addr));
                //     send(S[S[SS_id].backupID_2].SS_Client_socket, &struct_type, sizeof(int), 0);
                //     send(S[S[SS_id].backupID_2].SS_Client_socket, &backup_packet, sizeof(PACKET), 0);
                //     pthread_mutex_unlock(&socket_Wlock);
                // }

                return NULL;
            }
        }
    }
    else if ((packet.request_type == READ_REQUEST) || (packet.request_type == STREAM_REQUEST) || (packet.request_type == INFO_REQUEST))
    {

        pthread_mutex_unlock(&socket_Rlock);

        if (strcmp(packet.path1, ".") == 0)
        {
            // do nothing
        }
        else
        {
            temp_name_for_processing = processString(packet.path1);
            strcpy(packet.path1, temp_name_for_processing);
        }

        CacheNode* entry = getFromCache(cache, packet.path1);
        int SS_id;
        if(entry){
            SS_id = entry->SS_id;
            printf("Cache hit for path: %s\n", packet.path1);
        }
        else{

            

            SS_id = read_request_finder(packet.path1);
            if (SS_id == -1)
            {
                // send the error packet to the client
                int struct_type = PACKET_STRUCT;
                pthread_mutex_lock(&socket_Wlock);
                send(client_fd, &struct_type, sizeof(int), 0);
                PACKET response_packet;
                response_packet.status = FILE_NOT_FOUND;
                response_packet.request_type = STATUS_CHECK;
                strcpy(response_packet.IP, packet.IP);
                response_packet.PORT = packet.PORT;
                strcpy(response_packet.path1, packet.path1);
                send(client_fd, &response_packet, sizeof(PACKET), 0);
                pthread_mutex_unlock(&socket_Wlock);

                close(client_fd);

                return NULL;
            }
            else
            {
                addToCache(cache, packet.path1, SS_id);
                // send the success packet to the client, with the IP and port of the SS
                int struct_type = PACKET_STRUCT;
                pthread_mutex_lock(&socket_Wlock);
                send(client_fd, &struct_type, sizeof(int), 0);
                PACKET response_packet;
                response_packet.status = SUCCESS;
                response_packet.request_type = STATUS_CHECK;
                strcpy(response_packet.IP, S[SS_id].client_ip);
                response_packet.PORT = S[SS_id].client_port;
                strcpy(response_packet.path1, packet.path1);
                send(client_fd, &response_packet, sizeof(PACKET), 0);
                // addToCache(cache, packet.path1, S[SS_id].client_ip, S[SS_id].client_port);
                // printCache(cache);
                pthread_mutex_unlock(&socket_Wlock);
            }

            close(client_fd);
            return NULL;
        }
    }
    else if (packet.request_type == WRITE_REQUEST)
    {

        pthread_mutex_unlock(&socket_Rlock);

        printf("Write request received\n");

        if (strcmp(packet.path1, ".") == 0)
        {
            // do nothing
        }
        else
        {
            temp_name_for_processing = processString(packet.path1);
            strcpy(packet.path1, temp_name_for_processing);
        }

        CacheNode* entry = getFromCache(cache, packet.path1);
        int SS_id;
        if(entry){
            SS_id = entry->SS_id;
            printf("Cache hit for path: %s\n", packet.path1);
        }
            else{
            SS_id = write_request_finder(packet.path1);

            printf("SS_id: %d\n", SS_id);

            if (SS_id == -1)
            {
                // send the error packet to the client
                int struct_type = PACKET_STRUCT;
                pthread_mutex_lock(&socket_Wlock);
                send(client_fd, &struct_type, sizeof(int), 0);
                PACKET response_packet;
                response_packet.status = FILE_NOT_FOUND;
                response_packet.request_type = STATUS_CHECK;
                strcpy(response_packet.IP, packet.IP);
                response_packet.PORT = packet.PORT;
                strcpy(response_packet.path1, packet.path1);
                send(client_fd, &response_packet, sizeof(PACKET), 0);
                pthread_mutex_unlock(&socket_Wlock);

                close(client_fd);

                return NULL;
            }
            else
            {
                addToCache(cache, packet.path1, SS_id);
                // send the success packet to the client, with the IP and port of the SS
                int struct_type = PACKET_STRUCT;
                pthread_mutex_lock(&socket_Wlock);
                send(client_fd, &struct_type, sizeof(int), 0);
                PACKET response_packet;
                response_packet.status = SUCCESS;
                response_packet.request_type = STATUS_CHECK;
                strcpy(response_packet.IP, S[SS_id].client_ip);
                response_packet.PORT = S[SS_id].client_port;
                strcpy(response_packet.path1, packet.path1);
                send(client_fd, &response_packet, sizeof(PACKET), 0);
                // addToCache(cache, packet.path1, S[SS_id].client_ip, S[SS_id].client_port);
                // printCache(cache);
                pthread_mutex_unlock(&socket_Wlock);

                printf("Write request successful, sent back to client\n");

                close(client_fd);

                // send the request packet to the backup SS, with the naming server IP and port

                // handle the backup SS, asynch write

                return NULL;
            }
        }
    }
    else if (packet.request_type == LIST_REQUEST)
    {
        pthread_mutex_unlock(&socket_Rlock);

        if (strcmp(packet.path1, ".") == 0)
        {
            // do nothing
        }
        else
        {
            temp_name_for_processing = processString(packet.path1);
            strcpy(packet.path1, temp_name_for_processing);
        }

        char **files = NULL;
        char **dirs = NULL;
        int file_count = 0;
        int dir_count = 0;
        files = getAllWords(S[0].files, &file_count);
        dirs = getAllWords(S[0].dirs, &dir_count);

        // create a data packet and send it to the client, in the data packet, send data which has all the files and directories concatenated, separated by '\n'

        Data_Packet D;
        for (int i = 0; i < file_count; i++)
        {
            strcpy(D.data, files[i]);
            strcat(D.data, "\n");
        }

        for (int i = 0; i < dir_count; i++)
        {
            strcpy(D.data, dirs[i]);
            strcat(D.data, "\n");
        }

        // send the data packet to the client

        int struct_type = DATA_PACKET_STRUCT;
        pthread_mutex_lock(&socket_Wlock);
        send(client_fd, &struct_type, sizeof(int), 0);
        send(client_fd, &D, sizeof(Data_Packet), 0);
        pthread_mutex_unlock(&socket_Wlock);

        close(client_fd);

        return NULL;
    }
    else if (packet.request_type == COPY_REQUEST)
    {
        pthread_mutex_unlock(&socket_Rlock);

        printf("Copy request received\n");

        if (strcmp(packet.path1, ".") == 0)
        {
            // do nothing
        }
        else
        {
            temp_name_for_processing = processString(packet.path1);
            strcpy(packet.path1, temp_name_for_processing);
        }

        if (strcmp(packet.path2, ".") == 0)
        {
            // do nothing
        }
        else
        {
            temp_name_for_processing = processString(packet.path1);
            strcpy(packet.path2, temp_name_for_processing);
        }

        int SS_id_1; // source SS
        int SS_id_2; // destination SS
        bool is1_file = false;
        bool is2_file = false;

        // have to check for combinations of file and directory

        SS_id_1 = read_request_finder(packet.path1);
        if (SS_id_1 == -1)
        {
            SS_id_1 = dir_request_finder(packet.path1);
        }
        else
        {
            is1_file = true;
        }

        SS_id_2 = write_request_finder(packet.path2);
        if (SS_id_2 == -1)
        {
            SS_id_2 = dir_request_finder(packet.path2);
        }
        else
        {
            is2_file = true;
        }

        if (SS_id_1 == -1 || SS_id_2 == -1)
        {
            // send the error packet to the client
            int struct_type = PACKET_STRUCT;
            pthread_mutex_lock(&socket_Wlock);
            send(client_fd, &struct_type, sizeof(int), 0);
            PACKET response_packet;
            response_packet.status = FILE_NOT_FOUND;
            response_packet.request_type = STATUS_CHECK;
            strcpy(response_packet.IP, packet.IP);
            response_packet.PORT = packet.PORT;
            strcpy(response_packet.path1, packet.path1);
            send(client_fd, &response_packet, sizeof(PACKET), 0);
            pthread_mutex_unlock(&socket_Wlock);

            close(client_fd);

            return NULL;
        }

        // destination can't be a file when the source is a directory

        if (!is1_file && is2_file)
        {
            // send the error packet to the client
            int struct_type = PACKET_STRUCT;
            pthread_mutex_lock(&socket_Wlock);
            send(client_fd, &struct_type, sizeof(int), 0);
            PACKET response_packet;
            response_packet.status = FILE_NOT_FOUND;
            response_packet.request_type = STATUS_CHECK;
            strcpy(response_packet.IP, packet.IP);
            response_packet.PORT = packet.PORT;
            strcpy(response_packet.path1, packet.path1);
            send(client_fd, &response_packet, sizeof(PACKET), 0);
            pthread_mutex_unlock(&socket_Wlock);
            return NULL;
        }

        // send the success packet to the client, with the IP and port of the SS

        int struct_type = PACKET_STRUCT;
        pthread_mutex_lock(&socket_Wlock);
        send(client_fd, &struct_type, sizeof(int), 0);
        PACKET response_packet;
        response_packet.status = SUCCESS;
        response_packet.request_type = STATUS_CHECK;
        strcpy(response_packet.IP, S[SS_id_1].client_ip);
        response_packet.PORT = S[SS_id_1].client_port;
        strcpy(response_packet.path1, packet.path1);
        send(client_fd, &response_packet, sizeof(PACKET), 0);
        pthread_mutex_unlock(&socket_Wlock);

        if (is1_file && is2_file)
        { // send a read request to the source, write to the destination

            printf("Copying file to file\n");

            cpyFile(packet.path1, packet.path2, SS_id_1, SS_id_2);
        }
        else if (is1_file && !is2_file)
        { // send a read request to the source, write to the destination

            // first create the file in the destination

            int struct_type = PACKET_STRUCT;
            PACKET create_packet;
            create_packet.request_type = CREATE_REQUEST;
            strcpy(create_packet.IP, NS_IP);
            create_packet.PORT = NS_PORT;
            strcpy(create_packet.path1, packet.path2);
            strcpy(create_packet.path2, packet.path2);

            pthread_mutex_lock(&socket_Wlock);
            send(S[SS_id_2].SS_Client_socket, &struct_type, sizeof(int), 0);
            send(S[SS_id_2].SS_Client_socket, &create_packet, sizeof(PACKET), 0);
            pthread_mutex_unlock(&socket_Wlock);

            // send the read request to the source
        }
    }
    else if (packet.request_type == STATUS_CHECK)
    {

        printf("Status check request received\n");

        pthread_mutex_unlock(&socket_Rlock);

        return NULL;
    }

    pthread_mutex_unlock(&socket_Rlock);
    return NULL;
}

// Function to handle the SS requests
void *SS_handler(void *arg){

    printf("SS thread created\n");

    int client_fd = *(int *)arg;

    // receive the initial data packet from the SS

    PACKET SS_info_packet;

    recv(client_fd, &SS_info_packet, sizeof(PACKET), 0);

    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    if (getpeername(client_fd, (struct sockaddr *)&client_addr, &addr_len) == -1)
    {
        perror("getpeername failed");
        return NULL;
    }

    // Convert IP address to human-readable format
    char client_ip[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip)) == NULL)
    {
        perror("inet_ntop failed");
        return NULL;
    }

    // Get the port number
    int client_port = ntohs(client_addr.sin_port);

    printf("SS connected with IP: %s and port: %d\n", client_ip, client_port);
    // close(client_fd);

    // add the SS to the SS list

    int SS_id = add_SS(SS_info_packet.PORT, client_port, SS_info_packet.IP, client_ip);

    printf("%d\n", SS_id);

    int number_of_files;

    recv(client_fd, &number_of_files, sizeof(int), 0);

    printf("Number of files: %d\n", number_of_files);

    int recv_type;
    PACKET file_packet;

    int counter = 0;

    while (1)
    {

        if (counter == number_of_files)
        {
            break;
        }

        recv(client_fd, &recv_type, sizeof(int), 0);

        if (recv_type == PACKET_STRUCT)
        {

            recv(client_fd, &file_packet, sizeof(PACKET), 0);

            // add file to the SS trie

            insert(S[SS_id].files, processString(file_packet.path1));

            counter++;
        }
    }

    // recv the number of directories

    int number_of_dirs;

    printf("Receiving directories\n");

    recv(client_fd, &number_of_dirs, sizeof(int), 0);

    counter = 0;

    while (1)
    {

        if (counter == number_of_dirs)
        {
            break;
        }

        recv(client_fd, &recv_type, sizeof(int), 0);

        if (recv_type == PACKET_STRUCT)
        {

            recv(client_fd, &file_packet, sizeof(PACKET), 0);

            // add file to the SS trie

            insert(S[SS_id].dirs, processString(file_packet.path1));

            counter++;
        }
    }

    // add the data_packet to the SS
    // close(client_fd);

    printf("SS added succesfully\n");

    printWords(S[SS_id].files);
    printf("\n");
    printWords(S[SS_id].dirs);

    pthread_mutex_unlock(&socket_Rlock);

    return NULL;
}

// thread function to handle the program exit

// void* exit_handler(void* arg){
//     char c;
//     while(1){
//         scanf("%c", &c);
//         if(c == 'q'){
//             break;
//         }
//     }

//     // destroy the socket
//     // close(server_fd);

//     // destroy the mutexes
//     pthread_mutex_destroy(&socket_Rlock);
//     pthread_mutex_destroy(&socket_Wlock);

//     exit(0);
// }
// void print_server_ip() {
//     struct ifaddrs *ifaddr, *ifa;
//     char host[1025];      // ignore the error, it is safe to ignore it

//     // Retrieve the list of network interfaces
//     if (getifaddrs(&ifaddr) == -1) {
//         perror("getifaddrs");
//         return;
//     }

//     // Loop through interfaces and print the public IP (ignoring loopback and internal interfaces)
//     for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
//         if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET) {
//             continue;
//         }
//         getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
//         if (strstr(host, "127.") == NULL) {  // Ignore loopback IPs
//             printf("Naming Server Public IP: %s\n", host);
//             break;
//         }
//     }

//     freeifaddrs(ifaddr);

//     return;
// }

int main(){
    cache = initLRUCache();
    S = NULL;
    initialiseSS();
    // cache = initLRUCache(CACHE_CAPACITY);
    // if(cache == NULL){
    //     printf("Cache creation failed\n");
    //     return -1;
    // }
    pthread_mutex_init(&socket_Rlock, NULL);
    pthread_mutex_init(&socket_Wlock, NULL);
    pthread_mutex_init(&SS_lock, NULL);
    pthread_mutex_init(&SS_count_lock, NULL);
    pthread_mutex_init(&log_lock, NULL);

    struct sockaddr_in server_addr, client_addr, server_addr_2;
    socklen_t addr_len = sizeof(client_addr);

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Bind to all interfaces
    server_addr.sin_port = 0;                 // Dynamically choose port

    // Bind socket
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // repeat the same for the second socket

    // Create socket

    if ((server_fd_2 = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr_2, 0, sizeof(server_addr_2));
    server_addr_2.sin_family = AF_INET;
    server_addr_2.sin_addr.s_addr = INADDR_ANY; // Bind to all interfaces
    server_addr_2.sin_port = 0;                 // Dynamically choose port

    // Bind socket
    if (bind(server_fd_2, (struct sockaddr *)&server_addr_2, sizeof(server_addr_2)) == -1)
    {
        perror("Bind failed");
        close(server_fd_2);
        exit(EXIT_FAILURE);
    }

    // Get the dynamically assigned port
    socklen_t len = sizeof(server_addr);
    if (getsockname(server_fd, (struct sockaddr *)&server_addr, &len) == -1)
    {
        perror("getsockname failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Print the server IP and port
    // printf("Naming Server IP: %s\n", inet_ntoa(server_addr.sin_addr));
    print_server_ip();
    printf("Naming Server Port: %d\n", ntohs(server_addr.sin_port));

    // Store the server IP and port
    strcpy(NS_IP, inet_ntoa(server_addr.sin_addr));
    NS_PORT = ntohs(server_addr.sin_port);

    // Listen for incoming connections
    if (listen(server_fd, 10) == -1)
    {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Accept incoming connections

    while (1)
    {

        printf("Waiting for client connection\n");

        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);

        // acquire the lock

        pthread_mutex_lock(&socket_Rlock);

        printf("Client connected\n");
        int struct_type;
        // while(1)
        // {
        //     if(recv(client_fd, &struct_type, sizeof(int), 0) == -1){
        //         if(errno == EAGAIN || errno == EWOULDBLOCK){
        //             continue;
        //         }
        //         perror("Receive failed");
        //         close(client_fd);
        //         close(server_fd);
        //         exit(EXIT_FAILURE);
        //     }
        // }
        recv(client_fd, &struct_type, sizeof(int), 0);

        printf("Struct type: %d\n", struct_type);

        if (struct_type == PACKET_STRUCT)
        {
            pthread_t client_thread;
            pthread_create(&client_thread, NULL, client_handler, (void *)&client_fd);
            if (pthread_detach(client_thread) != 0)
            {
                perror("Thread detach failed");
                close(client_fd);
                close(server_fd);
                exit(EXIT_FAILURE);
            }
        }

        if (struct_type == SS_INIT_STRUCT)
        {
            pthread_t SS_thread;
            pthread_create(&SS_thread, NULL, SS_handler, (void *)&client_fd);
            if (pthread_detach(SS_thread) != 0)
            {
                perror("Thread detach failed");
                close(client_fd);
                close(server_fd);
                exit(EXIT_FAILURE);
            }
        }

        // accept the header (int) and then create a new thread to handle the request
        if (struct_type == STATUS_CHECK_STRUCT)
        {
            pthread_t status_thread;
            pthread_create(&status_thread, NULL, status_handler, (void *)&client_fd);
            if (pthread_detach(status_thread) != 0)
            {
                perror("Thread detach failed");
                close(client_fd);
                close(server_fd);
                exit(EXIT_FAILURE);
            }
        }
    }

    // destroy the socket
    close(server_fd);

    // destroy the mutexes
    pthread_mutex_destroy(&socket_Rlock);
    pthread_mutex_destroy(&socket_Wlock);
    pthread_mutex_destroy(&SS_lock);
    pthread_mutex_destroy(&SS_count_lock);
    pthread_mutex_destroy(&log_lock);

    return 0;
}