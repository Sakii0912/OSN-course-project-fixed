#ifndef LRU_H
#define LRU_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define CACHE_CAPACITY 10 // Change as needed

// Node structure for doubly linked list
typedef struct CacheNode {
    char *fileName;
    int SS_id;
    struct CacheNode *prev, *next;
} CacheNode;

// Hash map structure
typedef struct HashMap {
    CacheNode **table;
    int size;
} HashMap;

typedef struct LRUCache {
    CacheNode *head, *tail;
    HashMap *hashMap;
    int capacity, currentSize;
} LRUCache;

// Hash function
unsigned int hash(const char *str, int size);

// Create a new cache node
CacheNode *createCacheNode(const char *fileName, const char *ip, int port);

// Initialize hash map
HashMap *initHashMap(int size);

// Initialize LRU cache
LRUCache *initLRUCache(int capacity);

// Move node to front of the list
void moveToHead(LRUCache *cache, CacheNode *node);

// Remove the least recently used node
void removeTail(LRUCache *cache);

// Add a new node to the cache
void addToCache(LRUCache *cache, const char *fileName, const char *ip, int port);

// Get file details from the cache
CacheNode *getFromCache(LRUCache *cache, const char *fileName);

void printCache(LRUCache *cache);

#endif