#include "LRU.h"

unsigned int hash(const char *str, int size) {
    unsigned int hash = 5381;
    while (*str)
        hash = ((hash << 5) + hash) + *str++;
    return hash % size;
}

// Create a new cache node
CacheNode *createCacheNode(const char *fileName, int SS_id) {
    CacheNode *node = (CacheNode *)malloc(sizeof(CacheNode));
    node->fileName = strdup(fileName);
    node->SS_id = SS_id;
    node->prev = node->next = NULL;
    return node;
}

// Initialize hash map
HashMap *initHashMap(int size) {
    HashMap *hashMap = (HashMap *)malloc(sizeof(HashMap));
    hashMap->table = (CacheNode **)calloc(size, sizeof(CacheNode *));
    hashMap->size = size;
    return hashMap;
}

// Initialize LRU cache
LRUCache *initLRUCache() {
    LRUCache *cache = (LRUCache *)malloc(sizeof(LRUCache));
    cache->head = cache->tail = NULL;
    cache->hashMap = initHashMap(CACHE_CAPACITY * 2);
    cache->capacity = CACHE_CAPACITY;
    cache->currentSize = 0;
    return cache;
}

// Move node to front of the list
void moveToHead(LRUCache *cache, CacheNode *node) {
    if (cache->head == node) return;

    // Remove node from its current position
    if (node->prev) node->prev->next = node->next;
    if (node->next) node->next->prev = node->prev;

    if (cache->tail == node) cache->tail = node->prev;

    // Insert node at the head
    node->prev = NULL;
    node->next = cache->head;

    if (cache->head) cache->head->prev = node;
    cache->head = node;

    if (!cache->tail) cache->tail = node;
}

// Remove the least recently used node
void removeTail(LRUCache *cache) {
    if (!cache->tail) return;

    CacheNode *node = cache->tail;
    if (node->prev) node->prev->next = NULL;
    else cache->head = NULL;

    cache->tail = node->prev;

    unsigned int index = hash(node->fileName, cache->hashMap->size);
    cache->hashMap->table[index] = NULL;

    free(node->fileName);
    free(node);
    cache->currentSize--;
}

// Add a new node to the cache
void addToCache(LRUCache *cache, const char *fileName, int SS_id) {
    unsigned int index = hash(fileName, cache->hashMap->size);
    CacheNode *existingNode = cache->hashMap->table[index];

    if (existingNode) {
        // Update existing node
        existingNode->SS_id = SS_id;
        moveToHead(cache, existingNode);
    } else {
        // Evict if at capacity
        if (cache->currentSize == cache->capacity) {
            removeTail(cache);
        }

        // Add new node
        CacheNode *newNode = createCacheNode(fileName, SS_id);
        cache->hashMap->table[index] = newNode;
        newNode->next = cache->head;
        if (cache->head) cache->head->prev = newNode;
        cache->head = newNode;

        if (!cache->tail) cache->tail = newNode;

        cache->currentSize++;
    }
}

// Get file details from the cache
CacheNode *getFromCache(LRUCache *cache, const char *fileName) {
    unsigned int index = hash(fileName, cache->hashMap->size);
    CacheNode *node = cache->hashMap->table[index];

    if (node) {
        moveToHead(cache, node);
    }

    return node;
}

void printCache(LRUCache *cache) {
    CacheNode *current = cache->head;
    printf("Cache contents:\n");
    while (current) {
        printf("%s -> SS_id: %d\n", current->fileName, current->SS_id);
        current = current->next;
    }
    printf("----\n");
}
