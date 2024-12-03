#ifndef NM_H
#define NM_H

#include"../headers.h"

// Search effficiency related functions and structures //////////////////////////////////////////

#define ALPHABET_SIZE 128

typedef struct TrieNode {
    struct TrieNode* child[ALPHABET_SIZE];
    bool isEndOfWord;
}TrieNode;

struct TrieNode* getNode();

void insert(struct TrieNode* root, const char* key);

bool search(struct TrieNode* root, const char* key);

char* isSubstring(struct TrieNode* root, char* sub);

void deleteString(struct TrieNode* root, const char* str);

typedef struct st_SS_list{

    char client_ip[1025];
    int client_port;
    char NM_ip[1025];
    int NM_port;
    TrieNode* files;
    TrieNode* dirs;
    int backups_stored;     // number of other SS's backups stored
    int backupID_1;
    int backupID_2;
    int file_count;
    int dir_count;
    bool in_use;
    pthread_mutex_t lock;
    int SS_Client_socket;
    int SS_NM_fd;
    struct sockaddr_in SS_addr;

}st_SS_list;

typedef struct st_SS_list* SS_list;

void retrieveIP(char* host);

typedef struct st_thread_args{
    int socket;
    // char* ip;
    // int port;
    PACKET request_packet;
}st_thread_args;

typedef struct st_thread_args* thread_args;

// bool doesExist(char* file, SS_list S);

int add_SS(int client_port, int NM_port, char* client_IP, char* NM_IP);

void initialiseSS();

int read_request_finder(char* file);

int write_request_finder(char* file);

int dir_request_finder(char* dir);

int create_request_finder(char *file);

void* SS_handler(void* arg);

void* status_handler(void* args);

void cpyFile(char* file_name1, char* file_name2, int source, int destination);

// void* checkSS(void* arg);

void* client_handler(void* arg);   

int countWords(TrieNode* root); // Function to get count of number of words in the Trie

char** getAllWords(TrieNode* root, int* wordCount); // Function to return all words in the trie

void freeResult(char** result, int count);  // Free the memory allocated for the result array

char** findWordsWithSubstring(struct TrieNode* root, char* sub, int* resultCount) ; // Function to find all words that contain the given substring

void printWords(TrieNode* root);

char* processString(const char* input);

char* sub_strings(const char* str1, const char* str2);

// for both get all words and find words with substring, a single free function is enough

#endif

