#include "client_helper.h"

int main(){
    char ns_ip[INET_ADDRSTRLEN];
    printf("%s", "Enter the IP address of the naming server: ");
    scanf("%s", ns_ip);
    printf("%s", "Enter the port number of the naming server: ");
    int ns_port;
    scanf("%d", &ns_port);
	char client_ip_addr[INET_ADDRSTRLEN];
    strcpy(client_ip_addr, obtain_client_ip());
    int client_port = obtain_client_port();
    printf("\nClient IP: %s\n", client_ip_addr);
    printf("Client Port: %d\n\n", client_port);
    printf("Enter the request you want to send\n");
    printf("You can send the following requests\n");
    printf("Note: Click enter after the request which will prompt you to enter the file name:\n");
    printf("-----------------------------------------\n");
    printf("CREATE <filepath> <filename>\n");
    printf("READ <filename>\n");
    printf("WRITE <filename> <data>\n");
    printf("DELETE <filename>\n");
    printf("LIST\n");
    printf("STREAM\n");
    printf("INFO <filename>\n");
    printf("COPY <filename1> <filename2>\n");
    printf("ASYNC_WRITE <filename>\n");
    printf("END\n");
    printf("-----------------------------------------\n");
    while(1){
        printf("\nYou can keep requesting or type END to end the connection\n");
        char request[1024];
        scanf("%s", request);
        if(strcmp(request, "END") == 0){
            break;
        }
        PACKET* CLtoNS = init_PACKET(0, -1, "", "");
        PACKET* NStoCL = init_PACKET(0, -1, "", "");
        PACKET* CLtoSS = init_PACKET(0, -1, "", "");
        
        if(CLtoNS == NULL || NStoCL == NULL || CLtoSS == NULL){
            printf("Packet initialization failed\n");
            exit(EXIT_FAILURE);
        }
        if(strcmp(request,"CREATE") == 0){
            CLtoNS->request_type = 0;
            CLtoSS->request_type = 0;
            printf("Enter the path in which you want to store the file: ");
            char path[MAX_FILEPATH_SIZE];
            scanf("%s", path);
            printf("Enter the filename: ");
            char filename[MAX_FILENAME_SIZE];
            scanf("%s", filename);
            strcpy(CLtoNS->path1, path);
            strcpy(CLtoNS->path2, filename);
            strcat(path, "/");
            strcat(path, filename);
            CLtoSS->request_type = CREATE_REQUEST;
            strcpy(CLtoSS->path1, path);
            printf("input taken\n");
            if(send_req_to_ns(ns_ip, ns_port, CLtoNS, NStoCL) != 0){
                printf("Request to NS failed\n");
                exit(EXIT_FAILURE);
            }
            if(NStoCL->status == 0){
                printf("File created successfully in NS\n");
            }
            else{
                printf("File creation failed, NS status code %d\n", NStoCL->status);
                continue;
            }
            int ss_port = NStoCL->PORT;
            char ss_ip[INET_ADDRSTRLEN];
            strcpy(ss_ip, NStoCL->IP);
            if(send_req_to_ss(ss_ip, ss_port, CLtoSS) != 0){
                printf("Request to SS failed\n");
                exit(EXIT_FAILURE);
            }
        }
        else if (strcmp(request, "READ") == 0) {
            CLtoNS->request_type = 1;
            CLtoSS->request_type = 1;
            printf("Enter the filename: ");
            char filename[MAX_FILENAME_SIZE];
            scanf("%s", filename);
            strcpy(CLtoNS->path1, filename);
            strcpy(CLtoSS->path1, filename);
            if (send_req_to_ns(ns_ip, ns_port, CLtoNS, NStoCL) != 0) {
                printf("Request to NS failed\n");
                exit(EXIT_FAILURE);
            }
            if (NStoCL->status == 0) {
                printf("File read successfully from NS\n");
            } 
            else {
                printf("File read failed, NS status code %d\n", NStoCL->status);
                continue;
            }
            int ss_port = NStoCL->PORT;
            char ss_ip[INET_ADDRSTRLEN];
            strcpy(ss_ip, NStoCL->IP);
            if (send_req_to_ss(ss_ip, ss_port, CLtoSS) != 0) {
                printf("Request to SS failed\n");
                exit(EXIT_FAILURE);
            }
        } 
        else if (strcmp(request, "WRITE") == 0) {
            CLtoNS->request_type = 2;
            CLtoSS->request_type = 2;
            printf("Enter the filename: ");
            char filename[MAX_FILENAME_SIZE];
            scanf("%s", filename);
            strcpy(CLtoNS->path1, filename);
            strcpy(CLtoSS->path1, filename);
            if (send_req_to_ns(ns_ip, ns_port, CLtoNS, NStoCL) != 0) {
                printf("Request to NS failed\n");
                exit(EXIT_FAILURE);
            }
            if (NStoCL->status == 0) {
                printf("File requested successfully from NS\n");
            } 
            else {
                printf("File request failed from NS: status code %d\n", NStoCL->status);
                continue;
            }
            int ss_port = NStoCL->PORT;
            char ss_ip[INET_ADDRSTRLEN];
            strcpy(ss_ip, NStoCL->IP);
            if (send_req_to_ss(ss_ip, ss_port, CLtoSS) != 0) {
                printf("Request to SS failed\\n");
                exit(EXIT_FAILURE);
            }
        } 
        else if (strcmp(request, "DELETE") == 0) {
            CLtoNS->request_type = 3;
            CLtoSS->request_type = 3;
            printf("Enter the filename: ");
            char filename[MAX_FILENAME_SIZE];
            scanf("%s", filename);
            strcpy(CLtoNS->path1, filename);
            strcpy(CLtoSS->path1, filename);
            if (send_req_to_ns(ns_ip, ns_port, CLtoNS, NStoCL) != 0) {
                printf("Request to NS failed\n");
                exit(EXIT_FAILURE);
            }
            if (NStoCL->status == 0) {
                printf("File deleted successfully from NS\n");
            } 
            else {
                printf("File deletion failed, NS status code %d\n", NStoCL->status);
                continue;
            }
            int ss_port = NStoCL->PORT;
            char ss_ip[INET_ADDRSTRLEN];
            strcpy(ss_ip, NStoCL->IP);
            if (send_req_to_ss(ss_ip, ss_port, CLtoSS) != 0) {
                printf("Request to SS failed\n");
                exit(EXIT_FAILURE);
            }
        } 
        else if (strcmp(request, "LIST") == 0) { // this isnt complete, need to somehow get the list of files from ns
            CLtoNS->request_type = 4;
            CLtoSS->request_type = 4;
            if (send_req_to_ns(ns_ip, ns_port, CLtoNS, NStoCL) != 0) {
                printf("Request to NS failed\n");
                exit(EXIT_FAILURE);
            }
            if (NStoCL->status == 0) {
                printf("List of files retrieved successfully from NS\n");
            } 
            else {
                printf("Failed to list files, NS status code %d\n", NStoCL->status);
                continue;
            }
        } 
        else if (strcmp(request, "STREAM") == 0) {
            CLtoNS->request_type = 5;
            CLtoSS->request_type = 5;
            printf("Enter the filename: ");
            char filename[MAX_FILENAME_SIZE];
            scanf("%s", filename);
            strcpy(CLtoNS->path1, filename);
            strcpy(CLtoSS->path1, filename);
            if (send_req_to_ns(ns_ip, ns_port, CLtoNS, NStoCL) != 0) {
                printf("Request to NS failed");
                exit(EXIT_FAILURE);
            }
            if (NStoCL->status == 0) {
                printf("Streaming data successfully from NS\n");
            } 
            else {
                printf("Failed to stream data, NS status code %d\n", NStoCL->status);
                continue;
            }
            int ss_port = NStoCL->PORT;
            char ss_ip[INET_ADDRSTRLEN];
            strcpy(ss_ip, NStoCL->IP);
            if (send_req_to_ss(ss_ip, ss_port, CLtoSS) != 0) {
                printf("Request to SS failed\n");
                exit(EXIT_FAILURE);
            }
        } 
        else if (strcmp(request, "INFO") == 0) {
            CLtoNS->request_type = 6;
            CLtoSS->request_type = 6;
            printf("Enter the filename: ");
            char filename[MAX_FILENAME_SIZE];
            scanf("%s", filename);
            strcpy(CLtoNS->path1, filename);
            strcpy(CLtoSS->path1, filename);
            if (send_req_to_ns(ns_ip, ns_port, CLtoNS, NStoCL) != 0) {
                printf("Request to NS failed\n");
                exit(EXIT_FAILURE);
            }
            if (NStoCL->status == 0) {
                printf("File info retrieved successfully from NS\n");
            } 
            else {
                printf("Failed to get file info from NS, status code %d\n", NStoCL->status);
                continue;
            }
            int ss_port = NStoCL->PORT;
            char ss_ip[INET_ADDRSTRLEN];
            strcpy(ss_ip, NStoCL->IP);
            if (send_req_to_ss(ss_ip, ss_port, CLtoSS) != 0) {
                printf("Request to SS failed\n");
                exit(EXIT_FAILURE);
            }
        } 
        else if (strcmp(request, "COPY") == 0) {
            CLtoNS->request_type = 7;
            CLtoSS->request_type = 7;
            printf("Enter the source filename: ");
            char filename1[MAX_FILENAME_SIZE];
            scanf("%s", filename1);
            printf("Enter the destination filename: ");
            char filename2[MAX_FILENAME_SIZE];
            scanf("%s", filename2);
            strcpy(CLtoNS->path1, filename1);
            strcpy(CLtoNS->path2, filename2);
            if (send_req_to_ns(ns_ip, ns_port, CLtoNS, NStoCL) != 0) {
                printf("Request to NS failed");
                exit(EXIT_FAILURE);
            }
            if (NStoCL->status == 0) {
                printf("File copied successfully in NS\n");
            } 
            else {
                printf("File copy failed, NS status code %d\n", NStoCL->status);
            }
        }
        else if(strcmp(request, "ASYNC_WRITE") == 0){
            CLtoNS->request_type = 10;
            CLtoSS->request_type = 10;
            printf("Enter the filename: ");
            char filename[MAX_FILENAME_SIZE];
            scanf("%s", filename);
            strcpy(CLtoNS->path1, filename);
            strcpy(CLtoSS->path1, filename);
            if(send_req_to_ns(ns_ip, ns_port, CLtoNS, NStoCL) != 0){
                printf("Request to NS failed\n");
                exit(EXIT_FAILURE);
            }
            if(NStoCL->status == 0){
                printf("File requested successfully from NS\n");
            }
            else{
                printf("File request failed from NS: status code %d\n", NStoCL->status);
                continue;
            }
            int ss_port = NStoCL->PORT;
            char ss_ip[INET_ADDRSTRLEN];
            strcpy(ss_ip, NStoCL->IP);
            if(send_req_to_ss(ss_ip, ss_port, CLtoSS) != 0){
                printf("Request to SS failed\n");
                exit(EXIT_FAILURE);
            }
        }
        else{
            printf("Invalid request.... Try again\n");
            continue;
        }
    }
    return 0;
}