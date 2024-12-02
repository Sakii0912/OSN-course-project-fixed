// #define _XOPEN_SOURCE
#include "../headers.h"
#include <stdbool.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool is_present(char* name, char** arr, int n) {
    for (int i = 0; i < n; i++) {
        if (strcmp(name, arr[i]) == 0) {
            return true;
        }
    }
    return false;
}

int get_user_confirmation(const char* path, const char* type) {
    char response[10];
    printf("Do you want to add this %s: %s? (yes/no): ", type, path);
    fgets(response, sizeof(response), stdin);

    // Remove trailing newline
    response[strcspn(response, "\n")] = '\0';

    return (strcmp(response, "yes") == 0);
}

// Main function with user prompting
int* initSS(char** files, char** dirs, char** permitted, int n) {
    char cwd[MAX_FILEPATH_SIZE]; // Buffer to store the current working directory
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd failed");
        return NULL;
    }

    int* arr = (int*)malloc(2 * sizeof(int));  // Dynamically allocate memory for the array
    if (arr == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }
    arr[0] = 0;  // Initialize file count
    arr[1] = 0;  // Initialize directory count

    Que Q = create_q();  // Assume create_q is correctly defined
    en_q(Q, ".");  // Start with the current directory

    char temp[MAX_FILEPATH_SIZE];  // Temporary buffer for path manipulation

    while (Q->q_size != 0) {
        int pt = d_q(Q);

        DIR* D = opendir(Q->Quu[pt]);
        if (!D) {
            fprintf(stderr, "Failed to open directory: %s\n", Q->Quu[pt]);
            continue;
        }

        struct dirent* dir;
        while ((dir = readdir(D)) != NULL) {
            if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
                continue;  // Skip "." and ".."
            }

            snprintf(temp, sizeof(temp), "%s/%s", Q->Quu[pt], dir->d_name);

            if (dir->d_type == DT_DIR) {
                if (get_user_confirmation(temp, "directory")) {
                    en_q(Q, temp);  // Add directory to the queue
                    dirs[arr[1]] = (char*)malloc(sizeof(char) * MAX_FILEPATH_SIZE);
                    if (dirs[arr[1]] == NULL) {
                        fprintf(stderr, "Memory allocation failed for directory\n");
                        continue;
                    }
                    strcpy(dirs[arr[1]++], temp);
                }
            } else if (dir->d_type == DT_REG) {
                if (get_user_confirmation(temp, "file")) {
                    files[arr[0]] = (char*)malloc(sizeof(char) * MAX_FILEPATH_SIZE);
                    if (files[arr[0]] == NULL) {
                        fprintf(stderr, "Memory allocation failed for file\n");
                        continue;
                    }
                    strcpy(files[arr[0]++], temp);
                }
            }
        }

        closedir(D);
    }

    // Uncomment these lines to print results if needed
    // for (int i = 0; i < arr[0]; i++) {
    //     printf("File: %s\n", files[i]);
    // }
    // for (int i = 0; i < arr[1]; i++) {
    //     printf("Directory: %s\n", dirs[i]);
    // }
    // destroy_q(Q);  // Assume destroy_q is correctly defined

    return arr;
}

char* print_server_ip_SSside() {
    char* ip = (char*)malloc(INET_ADDRSTRLEN);  // Allocate memory for the IP address string
    struct ifaddrs *ifaddr, *ifa;
    if(ip == NULL){
        ip = (char*)malloc(sizeof(char) * 16); // Allocate memory for the IP address string
    }
         // ignore the error, it is safe to ignore it

    // Retrieve the list of network interfaces
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return NULL;
    }

    // Loop through interfaces and print the public IP (ignoring loopback and internal interfaces)
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET) {
            continue;
        }
        getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), ip, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
        if (strstr(ip, "127.") == NULL) {  // Ignore loopback IPs
            printf("SS_SERVER_IP: %s\n", ip);
            break;
        }
    }

    freeifaddrs(ifaddr);

    return ip;
}

