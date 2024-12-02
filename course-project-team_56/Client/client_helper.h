#include"../headers.h"

PACKET* init_PACKET(int status, int request_type, char* file_name1, char* file_name2);

Data_Packet* init_Data_Packet(int status, int request_type, int num_packets);

void free_memory(void* ptr);

char* obtain_client_ip();

int obtain_client_port();

void stream_audio_with_mpv(const char* audio_data, size_t data_size);

int send_req_to_ss(char* ss_ip, int ss_port, PACKET* CLtoSS);

int send_req_to_ns(char* server_ip, int server_port, PACKET* CLtoNS, PACKET* NStoCL);