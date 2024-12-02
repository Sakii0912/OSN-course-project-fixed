#include "client_helper.h"


pthread_mutex_t recv_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t send_lock = PTHREAD_MUTEX_INITIALIZER;
int send1 = 1;
int send2 = 2;

PACKET *init_PACKET(int status, int request_type, char *file_name1, char *file_name2)
{
    PACKET *packet = (PACKET *)malloc(sizeof(PACKET));
    packet->status = status;
    packet->request_type = request_type;
    strcpy(packet->path1, file_name1);
    strcpy(packet->path2, file_name2);
    return packet;
}

Data_Packet *init_Data_Packet(int status, int request_type, int num_packets)
{
    Data_Packet *packet = (Data_Packet *)malloc(sizeof(Data_Packet));
    packet->status = status;
    packet->request_type = request_type;
    packet->number_of_packets = num_packets;
    return packet;
}

void free_memory(void *ptr)
{
    free(ptr);
}

char *obtain_client_ip()
{
    struct ifaddrs *ifaddr, *ifa;
    char *host = (char *)malloc(sizeof(char) * NI_MAXHOST);

    // Retrieve the list of network interfaces
    if (getifaddrs(&ifaddr) == -1)
    {
        printf("getifaddrs");
        return NULL;
    }

    // Loop through interfaces and print the public IP (ignoring loopback and internal interfaces)
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET)
        {
            continue;
        }
        getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
        if (strstr(host, "127.") == NULL)
        { // Ignore loopback IPs
            // printf("Server Public IP: %s\n", host);
            break;
        }
    }

    freeifaddrs(ifaddr);
    return host;
}

int obtain_client_port()
{
    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Create a socket
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("Socket creation failed");
        return -1;
    }

    // Bind the socket to a port
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = INADDR_ANY;
    client_addr.sin_port = 0; // Bind to any available port

    if (bind(client_fd, (struct sockaddr *)&client_addr, sizeof(client_addr)) == -1)
    {
        printf("Bind failed");
        close(client_fd);
        return -1;
    }

    // Get the port number
    if (getsockname(client_fd, (struct sockaddr *)&client_addr, &addr_len) == -1)
    {
        printf("getsockname");
        close(client_fd);
        return -1;
    }

    return ntohs(client_addr.sin_port);
}

// Client side receiving part

void stream_audio_with_mpv(const char *audio_data, size_t data_size)
{
    const char *temp_file = "temp_audio_file.wav";

    FILE *file = fopen(temp_file, "wb");
    if (file == NULL)
    {
        printf("Failed to open temporary file for writing");
        return;
    }

    if (fwrite(audio_data, sizeof(char), data_size, file) != data_size)
    {
        printf("Failed to write audio data to temporary file");
        fclose(file);
        return;
    }

    fclose(file);

    pid_t pid = fork();
    if (pid == 0)
    {
        execlp("mpv", "mpv", "--no-video", "--audio-display=no", temp_file, (char *)NULL);

        printf("Failed to execute mpv");
        return;
    }
    else if (pid < 0)
    {
        printf("Fork failed");
        return;
    }

    wait(NULL);

    if (remove(temp_file) != 0)
    {
        printf("Failed to remove temporary file");
    }
}

int send_req_to_ss(char *ss_ip, int ss_port, PACKET *CLtoSS)
{ // connect and send req to ss
    int ss_fd;
    struct sockaddr_in server_addr;

    CLtoSS->PORT = ss_port;
    strcpy(CLtoSS->IP, ss_ip);
    // Create socket
    if ((ss_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("Socket creation failed\n");
        return -1;
    }

    // Server address configuration
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(ss_port);

    // Convert IP to binary form
    if (inet_pton(AF_INET, ss_ip, &server_addr.sin_addr) <= 0)
    {
        printf("Invalid IP address\n");
        close(ss_fd);
        return -1; 
    }
    printf("Storage server IP and PORT: %s and %d\n", ss_ip, ss_port);

    // Connect to the server
    if (connect(ss_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("Connection failed\n");
        close(ss_fd);
        return -1;
    }
    printf("Connected To SS\n");
    int send_bytes = 0;
    // Send the request to the server
    if (CLtoSS->request_type != WRITE_REQUEST)
    {
        
        send_bytes = send(ss_fd, &send1, sizeof(int), 0);                  
        if(send_bytes < 0){
            printf("send_req_to_ss: send error near like 160\n");
        }
        send_bytes = send(ss_fd, CLtoSS, sizeof(*CLtoSS), 0);
        if (send_bytes < 0)
        {
            printf("send_req_to_ss: send error near like 164\n");
        }
        int typeofstruct;
        int bytes3 = recv(ss_fd, &typeofstruct, sizeof(int), 0);
        if(bytes3 < 0){
            printf("send_req_to_ss: recv error near like 173\n");
            return -1;
        }
        printf("Type of struct: %d\n", typeofstruct);
        int numpackets = 0;
        if(typeofstruct == PACKET_STRUCT){
            PACKET *SStoCL = init_PACKET(0, -1, "", "");
            if (SStoCL == NULL)
            {
                printf("Packet initialization failed\n");
                return -1;
            }
            int bytes_recieved;
            bytes_recieved = recv(ss_fd, SStoCL, sizeof(*SStoCL), 0);
            if(bytes_recieved < 0){
                printf("Response from SS failed\n");
                return -1;
            }
        }
        else if(typeofstruct == DATA_PACKET_STRUCT){
            Data_Packet *SStoCL = init_Data_Packet(0, CLtoSS->request_type, -1);
            if (SStoCL == NULL)
            {
                printf("Packet initialization failed\n");
                return -1;
            }
            int bytes_recieved;
            bytes_recieved = recv(ss_fd, SStoCL, sizeof(*SStoCL), 0);
            if(bytes_recieved > 0)
            {
                numpackets = SStoCL->number_of_packets - 1;
                if (SStoCL->request_type != STREAM_REQUEST)
                {
                    printf("Response from SS:\n %s\n", SStoCL->data);
                }
                else
                {
                    stream_audio_with_mpv(SStoCL->data, sizeof(SStoCL->data));
                }
            }
            else{
                printf("Failed to recieve data packet\n");
                return -1;
            }
        }
        
        while (numpackets > 0)
        { // and this also if infinite loop
            sleep(1);
            printf("Num of packets left %d\n", numpackets);
            bytes3 = recv(ss_fd, &typeofstruct, sizeof(int), 0);
            if(bytes3 < 0){
                printf("send_req_to_ss: recv error near like 173\n");
                return -1;
            }
            printf("Type of struct: %d\n", typeofstruct);
            if (typeofstruct == PACKET_STRUCT)
            {
                PACKET *SStoCL = init_PACKET(0, -1, "", "");
                if (SStoCL == NULL)
                {
                    printf("Packet initialization failed\n");
                    return -1;
                }
                int bytes_recieved;
                while ((bytes_recieved = recv(ss_fd, SStoCL, sizeof(*SStoCL), 0)) > 0)
                {
                    printf("Response from SS: Status code %d\n", SStoCL->status);
                }
                free(SStoCL);
                // break;
            }
            if (typeofstruct == DATA_PACKET_STRUCT)
            {
                Data_Packet *SStoCL = init_Data_Packet(0, CLtoSS->request_type, -1);
                if (SStoCL == NULL)
                { 
                    printf("Packet initialization failed\n");
                    return -1;
                }
                int bytes_recieved;
                bytes_recieved = recv(ss_fd, SStoCL, sizeof(*SStoCL), 0);
                if(bytes_recieved > 0)
                {
                    if (SStoCL->request_type != STREAM_REQUEST)
                    {
                        printf("Response from SS: %s\n", SStoCL->data);
                    }
                    else
                    {
                        stream_audio_with_mpv(SStoCL->data, sizeof(SStoCL->data));
                    }
                }
                else{
                    printf("Failed to recieve data packet\n");
                    return -1;
                }
                free(SStoCL);
                // break;
            }
            numpackets--;
        }
    }
    else
    {
        // sending the data
        
        send_bytes = send(ss_fd, &send1, sizeof(int), 0);                       
        send_bytes = send(ss_fd, CLtoSS, sizeof(PACKET), 0);
        

        printf("Enter the data (empty newline to stop writing data)\n");
        char inpdata[MAX_DATA_SIZE];
        int count_packets = 0;
        scanf("%s", inpdata);
        int sizeofdata = strlen(inpdata);
        printf("Size of data entered is %d\n", sizeofdata);
        fflush(stdout);
        if(sizeofdata % MAX_SEND_SIZE == 0){
            count_packets = sizeofdata / MAX_SEND_SIZE;
        }
        else{
            count_packets = (sizeofdata / MAX_SEND_SIZE) + 1;
        }
        int offset = 0; // be carefull with this function too, could result in infinite loops.
        Data_Packet *write_data = init_Data_Packet(0, 2, -1);
        write_data->number_of_packets = count_packets;
        int numofpackets = 0;
        while (sizeofdata > 0)
        {
            numofpackets++;
            if (write_data == NULL)
            {
                printf("Data Packet initialization failed\n");
                return -1;
            }
            int chunk_size = MAX_SEND_SIZE;
            if (sizeofdata < chunk_size)
            {
                chunk_size = sizeofdata;
            }
            strncpy(write_data->data, inpdata + offset, chunk_size);
            write_data->data[chunk_size] = '\0'; // Null-terminate the chunk
            printf("Data chunk: %s\n", write_data->data);

            printf("Send2: %d\n", send2);
            send_bytes = send(ss_fd, &send2, sizeof(int), 0); 
            if (send_bytes <= 0) {
                printf("Failed to send chunk identifier\n");
                
                free(write_data);
                break;
            }
            send_bytes = send(ss_fd, write_data, sizeof(*write_data), 0);
            if (send_bytes <= 0)
            {
                printf("Failed to send data chunk\n");
                
                free(write_data);
                break;
            }
            

            sizeofdata -= chunk_size;
            offset += chunk_size;
        }
        free(write_data);
        printf("Sent all data as individual packets\n");
        printf("Num of packets %d\n", numofpackets);
        fflush(stdout);

        // recieving acknowledgement
        int struct_type;
        
        int bytes = recv(ss_fd, &struct_type, sizeof(int), 0);
        
        if (bytes < 0)
        {
            printf("rend_req_to_ss : recv failed around 266\n");
            return -1;
        }
        if (struct_type == PACKET_STRUCT)
        {
            int bytes_recieved;
            PACKET *SStoCL = init_PACKET(0, -1, "", "");
            if (SStoCL == NULL)
            {
                printf("Packet initialization failed\n");
                return -1;
            }
            
            while ((bytes_recieved = recv(ss_fd, SStoCL, sizeof(*SStoCL), 0)) > 0)
            {
                printf("Response from SS: Status code %d\n", SStoCL->status);
            }
            
        }
        else
        {
            printf("Unexpected struct type recieved. Expecting PACKET, recieved Data_Packet\n");
            return -1;
        }
    }
    // Receive the response from the server
    close(ss_fd);
    return 0;
}

int send_req_to_ns(char *server_ip, int server_port, PACKET *CLtoNS, PACKET *NStoCL)
{ // connect and send req to ns

    printf("Request type: %d\n", CLtoNS->request_type);

    int ns_fd;
    struct sockaddr_in server_addr;

    // Create socket
    if ((ns_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("Socket creation failed\n");
        return -1;
    }

    // Server address configuration
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    // Convert IP to binary form
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0)
    {
        printf("Invalid IP address\n");
        close(ns_fd);
        return -1;
    }

    // Connect to the server
    if (connect(ns_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("Connection failed\n");
        close(ns_fd);
        return -1;
    }
    printf("Connected to NS\n");

    int struct_type = PACKET_STRUCT;
    int bytes = send(ns_fd, &struct_type, sizeof(int), 0);
    if (bytes < 0)
    {
        printf("send_req_to_ns: send error\n");
        return -1;
    }
    bytes = send(ns_fd, CLtoNS, sizeof(*CLtoNS), 0);
    if (bytes < 0)
    {
        printf("send_req_to_ns: send error\n");
        return -1;
    }
    printf("Request sent to NS %d\n", bytes);
    fflush(stdout);
    if(bytes < 0){
        printf("send_req_to_ns: send error\n");
    }
    fflush(stdout);
    // Receive the response from the server

    printf("Sent the actual struct\n");

    if (CLtoNS->request_type != 4)
    {
        bytes = recv(ns_fd, &struct_type, sizeof(int), 0);
        if (bytes < 0)
        {
            printf("send_req_to_ns: recv error around 332\n");
            return -1;
        }
        if (struct_type == 1)
        {
            bytes = recv(ns_fd, NStoCL, sizeof(PACKET), 0);
            if (bytes < 0)
            {
                printf("send_req_to_ns: recv error around 346\n");
                return -1;
            }
            if (NStoCL->status == 0)
            {
                printf("File requested successfully from NS\n");
            }
            else
            {
                printf("File request failed from NS: status code %d\n", NStoCL->status);
                return 0;
            }
            printf("Recieved PACKET struct with the ip and port %s, %d\n", NStoCL->IP, NStoCL->PORT);
        }
        else
        {
            printf("Mismatch in recieved struct: recieved Data_Packet instead of PACKET\n");
            return -1;
        }
    }
    else
    {
        
        bytes = recv(ns_fd, &struct_type, sizeof(int), 0);
        
        if (bytes < 0)
        {
            printf("send_req_to_ns: recv error around 348\n");
            return -1;
        }
        if (struct_type == 2)
        {
            Data_Packet *list_elem = init_Data_Packet(0, 4, -1);
            if (list_elem == NULL)
            {
                printf("Request to SS initialization failed\n");
                return -1;
            }
            
            bytes = recv(ns_fd, list_elem, sizeof(*list_elem), 0);
            
            while (bytes > 0)
            {
                printf("Response from NS: %s\n", list_elem->data);
                
                bytes = recv(ns_fd, list_elem, sizeof(*list_elem), 0);
                
            }
            free((void *)list_elem);
            if (bytes < 0)
            {
                return -1;
            }
        }
        else
        {
            printf("Mismatch in recieved struct: recieved PACKET instead of Data_Packet\n");
            return -1;
        }
    }

    // Print the response status
    printf("Response status from NS: %d\n", NStoCL->status);

    close(ns_fd);
    return 0;
}