#include"headers.h"

// char* getTimeWithMicroseconds() {
//     // Allocate memory for the result string
//     char *timeString = malloc(30 * sizeof(char));
//     if (timeString == NULL) {
//         return NULL; // Allocation failed
//     }

//     // Get the current time and microseconds
//     struct timeval currentTime;
//     gettimeofday(&currentTime, NULL);

//     // Convert to local time structure
//     struct tm *timeInfo = localtime(&currentTime.tv_sec);

//     // Format the time in HH:MM:SS format
//     strftime(timeString, 30, "%H:%M:%S", timeInfo);

//     // Add microseconds to the formatted time string
//     sprintf(timeString + 8, ".%06ld", currentTime.tv_usec);

//     return timeString;
// }

// Function to get and print the public IP address of the server
void print_server_ip() {
    struct ifaddrs *ifaddr, *ifa;
    char host[NI_MAXHOST];      // ignore the error, it is safe to ignore it

    // Retrieve the list of network interfaces
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return;
    }

    // Loop through interfaces and print the public IP (ignoring loopback and internal interfaces)
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET) {
            continue;
        }
        getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
        if (strstr(host, "127.") == NULL) {  // Ignore loopback IPs
            printf("Naming Server Public IP: %s\n", host);
            break;
        }
    }

    freeifaddrs(ifaddr);

    return;
}

char* get_wifi_ip() {
    struct ifaddrs *ifaddr, *ifa;
    char *ip = malloc(INET_ADDRSTRLEN);

    if (ip == NULL) {
        perror("Memory allocation failed");
        return NULL;
    }

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        free(ip);
        return NULL;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;

        // Check if it's an IPv4 address
        if (ifa->ifa_addr->sa_family == AF_INET) {
            // Replace "en0" with the name of your Wi-Fi interface if necessary
            if (strcmp(ifa->ifa_name, "en0") == 0) {
                struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
                inet_ntop(AF_INET, &(sa->sin_addr), ip, INET_ADDRSTRLEN);
                freeifaddrs(ifaddr);
                return ip;  // Return the IP as a string
            }
        }
    }

    freeifaddrs(ifaddr);
    free(ip);
    return NULL;  // Return NULL if no IP is found
}

char* get_current_ip() {
    struct ifaddrs *ifaddr, *ifa;
    char *ip = malloc(INET_ADDRSTRLEN); // Allocate memory for the IP address string

    if (ip == NULL) {
        perror("Memory allocation failed");
        return NULL;
    }

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs failed");
        free(ip);
        return NULL;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;

        // Check if the address is IPv4
        if (ifa->ifa_addr->sa_family == AF_INET) {
            // Get the interface name and check if it's active (not loopback)
            if (strcmp(ifa->ifa_name, "lo") != 0) { // Exclude the loopback interface
                struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
                inet_ntop(AF_INET, &(sa->sin_addr), ip, INET_ADDRSTRLEN);

                // Free resources and return the IP
                freeifaddrs(ifaddr);
                return ip;
            }
        }
    }

    freeifaddrs(ifaddr);
    free(ip);
    return NULL; // Return NULL if no IP is found
}

// queue implementation //////////////////////////////////////////////////////////////

#define size 1000

Que create_q(){
    Que Q = (Que)malloc(sizeof(st_Que));

    Q->Quu = (char**)malloc(sizeof(char*) * size);
    for(int i = 0; i<size; i++){
        Q->Quu[i] = (char*)malloc(sizeof(char) * size);
    }
    Q->front = 0;
    Q->rear = -1;  
    Q->q_size = 0;   

    return Q;
}

void en_q(Que Q, char* entry){
    Q->rear = (Q->rear + 1)%size;
    Q->q_size++;
    strcpy(Q->Quu[Q->rear], entry);

    return;
}

void destroy_q(Que Q){
    for(int i = 0 ; i<size; i++){
        free(Q->Quu[i]);
    }
    free(Q->Quu);
    free(Q);
}

int d_q(Que Q){
    if(Q->q_size == 0){
        return -1;
    }
    int ret = Q->front;
    Q->front = (Q->front + 1)%size;
    Q->q_size--;
    return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////