#include "../headers.h"
#include "NM.h"

// Helper function to find a word starting from the given Trie node
bool findWord(struct TrieNode* node, char* buffer, int depth) {
    if (!node) return false;
    
    // If this node marks the end of a word, return the word built so far
    if (node->isEndOfWord) {
        buffer[depth] = '\0';
        return true;
    }

    // Recursively search for any child node that forms a word
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (node->child[i]) {
            buffer[depth] = (char)i;
            if (findWord(node->child[i], buffer, depth + 1)) {
                return true;
            }
        }
    }

    return false;
}

// Modified function to check if there is any string in the Trie containing `sub`
char* isSubstring(struct TrieNode* root, char* sub) {
    struct TrieNode* curr = root;

    char* temp = sub;
    // Traverse the Trie based on `sub` characters
    while (*sub) {
        int index = *sub;  // ASCII value
        if (!curr->child[index]) {
            printf("No word found\n");
            return NULL;
        }
        curr = curr->child[index];
        sub++;
    }

    // Allocate buffer to store the matched word
    char* matchedWord = malloc(1000);  // Assuming word length will not exceed 1000 characters
    // if (!matchedWord) return false;

    sub = temp;  // Reset the pointer to the start of the substring
    // Copy the substring into the buffer to start building the result word
    strncpy(matchedWord, sub, strlen(sub));
    printf("%s %ld\n", matchedWord, strlen(sub));
    
    // Find the rest of the word if any word exists
    if (findWord(curr, matchedWord, strlen(sub))) {
        return matchedWord;  // Return the found word containing `sub`
    } else {
        free(matchedWord);
        return NULL;
    }
}
// Make sure to free the returned string after use, if not NULL

///////////////////////////////////////////////////////////////////////////////////////////

// Function to create a new Trie node
struct TrieNode* getNode() {
    struct TrieNode* node = 
      (struct TrieNode*)malloc(sizeof(struct TrieNode));
    node->isEndOfWord = false;
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        node->child[i] = NULL;
    }
    return node;
}

// Function to insert a key into the Trie
void insert(struct TrieNode* root, const char* key) {
    struct TrieNode* curr = root;
    while (*key) {
        int index = *key; // Use the ASCII value directly
        if (!curr->child[index]) {
            curr->child[index] = getNode();
        }
        curr = curr->child[index];
        key++;
    }
    curr->isEndOfWord = true;
}

// Function to search a key in the Trie
bool search(struct TrieNode* root, const char* key) {
    struct TrieNode* curr = root;
    while (*key) {
        int index = *key; // Use the ASCII value directly
        if (!curr->child[index]) {
            return false;
        }
        curr = curr->child[index];
        key++;
    }
    return (curr != NULL && curr->isEndOfWord);
}

// Recursive helper function to delete a string from the Trie
bool deleteHelper(struct TrieNode* node, const char* str, int depth) {
    if (!node) return false;

    // Base case: reached end of the string
    if (str[depth] == '\0') {
        // Unmark the end of the word
        if (node->isEndOfWord) {
            node->isEndOfWord = false;

            // Check if the node has no children
            for (int i = 0; i < ALPHABET_SIZE; i++) {
                if (node->child[i]) return false; // Node has children, do not delete
            }
            return true; // Node has no children and is not end of any word
        }
        return false; // Word not present as end of any word
    }

    // Recursive case: find the character index and proceed to delete
    int index = str[depth];
    if (deleteHelper(node->child[index], str, depth + 1)) {
        // Free the child node and set it to NULL
        free(node->child[index]);
        node->child[index] = NULL;

        // Check if this node is now a non-end node with no children
        if (!node->isEndOfWord) {
            for (int i = 0; i < ALPHABET_SIZE; i++) {
                if (node->child[i]) return false; // Node has other children, do not delete
            }
            return true; // Node can be deleted as it's no longer part of another word
        }
    }

    return false;
}

// Function to delete a string from the Trie
void deleteString(struct TrieNode* root, const char* str) {
    deleteHelper(root, str, 0);
}

// Function to get count of number of words in the Trie

int countWords(TrieNode* root) {
    if (root == NULL) {
        return 0;
    }

    int count = 0;

    // If the current node marks the end of a word, increment the count
    if (root->isEndOfWord) {
        count++;
    }

    // Recur for all children (alphabet size 128)
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (root->child[i] != NULL) {
            count += countWords(root->child[i]);
        }
    }

    return count;
}

// Helper function to collect words from the trie
void collectWords(TrieNode* node, char* buffer, int level, char** result, int* index) {
    if (node == NULL) return;

    // If we have reached the end of a word, add it to the result
    if (node->isEndOfWord) {
        buffer[level] = '\0';  // Null-terminate the word
        result[*index] = strdup(buffer);  // Save the word in the result array
        (*index)++;
    }

    // Recur for each child
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (node->child[i] != NULL) {
            buffer[level] = (char)i;  // Set the character for this position
            collectWords(node->child[i], buffer, level + 1, result, index);
        }
    }
}

int countWordsWithSubstring(struct TrieNode* root, const char* sub);

// Function to find all words that contain the given substring
// Function to find all words that contain the given substring
char** findWordsWithSubstring(struct TrieNode* root, char* sub, int* resultCount) {
    struct TrieNode* curr = root;
    const char* tempSub = sub;
    int index = 0;
    *resultCount = 0;

    // Traverse the Trie based on `sub` characters
    while (*sub) {
        index = *sub;  // ASCII value
        if (!curr->child[index]) {
            // No match for the substring
            return NULL;
        }
        curr = curr->child[index];
        sub++;
    }

    // List to store words containing the substring
    char** result = (char**)malloc(sizeof(char*) * (countWordsWithSubstring(root, sub)));  // Initial allocation (resize if needed)
    char matchedWord[MAX_FILENAME_SIZE];
    int resultIndex = 0;

    // Append the substring to the matchedWord buffer
    strncpy(matchedWord, tempSub, sub - tempSub);  // Copy the full substring into matchedWord
    matchedWord[sub - tempSub] = '\0';  // Ensure it's null-terminated

    // After finding the substring, now collect all words from the current node
    // We traverse the trie to find all the words starting from the current node
    collectWords(curr, matchedWord, strlen(matchedWord), result, &resultIndex);

    *resultCount = resultIndex;  // Set the number of words found
    return result;
}


// Function to return all words in the trie
char** getAllWords(TrieNode* root, int* wordCount) {
    if (root == NULL) return NULL;

    // Start with an initial guess for word count
    *wordCount = 0;
    char** result = (char**)malloc(sizeof(char*) * 1000);  // Allocate space for 1000 words (resize as needed)

    char buffer[MAX_FILENAME_SIZE];  // Buffer to store each word during traversal
    int index = 0;

    // Collect all words in the trie
    collectWords(root, buffer, 0, result, &index);

    *wordCount = index;  // Set the total number of words found
    return result;
}

// Function to free the memory allocated for the result array
void freeResult(char** result, int count) {
    for (int i = 0; i < count; i++) {
        free(result[i]);  // Free each string
    }
    free(result);  // Free the array of strings
}


// Helper function to perform DFS and count the words starting from a given node
int countWordsFromNode(struct TrieNode* node) {
    if (node == NULL) return 0;

    int count = 0;

    // If we reach the end of a word, increment the count
    if (node->isEndOfWord) {
        count++;
    }

    // Recur for each child node
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (node->child[i] != NULL) {
            count += countWordsFromNode(node->child[i]);
        }
    }

    return count;
}

// Function to count how many words in the trie contain the given substring
int countWordsWithSubstring(struct TrieNode* root, const char* sub) {
    struct TrieNode* curr = root;
    const char* tempSub = sub;
    int index = 0;

    // Traverse the Trie based on the substring characters
    while (*sub) {
        index = *sub;  // ASCII value of the character
        if (!curr->child[index]) {
            // No match for the substring
            return 0;
        }
        curr = curr->child[index];
        sub++;
    }

    // Now we are at the node corresponding to the end of the substring
    // Perform DFS to count all words from this point
    return countWordsFromNode(curr);
}


// Helper function to recursively traverse and print words in the Trie
void printWordsHelper(TrieNode* node, char* prefix, int level) {
    if (node->isEndOfWord) {
        prefix[level] = '\0'; // Null-terminate the string
        printf("%s\n", prefix);
    }

    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (node->child[i]) {
            prefix[level] = (char)i;
            printWordsHelper(node->child[i], prefix, level + 1);
        }
    }
}

// Function to print all words in the Trie
void printWords(TrieNode* root) {
    char prefix[1000]; // Buffer to store words (adjust size as needed)
    printWordsHelper(root, prefix, 0);
}