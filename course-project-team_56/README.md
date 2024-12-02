# Network File System

## Introduction

This is a the final course project of this course, a network file system built from scratch in C. A network file system is a form of DFS (Distributed File System) which is distributed into 3 major components. 

- Clients
- Naming Server
- Storage Server

Each of the components and their roles are explained below.


## Component Analysis

### Client

## Client Code Flow

1. **Initialization**: 
   - The client initializes by taking input for the Naming Server (NS) **IP address** and **port number**.
   - It retrieves its own IP address using the `obtain_client_ip()` function and dynamically allocates a port using `obtain_client_port()`.

2. **Connection Check**:
   - A continuous mechanism (such as `isNMConnected()`) ensures the client monitors the connection status of the Naming Server. This step is not explicitly implemented in the code but can be inferred for robustness.

3. **Command Loop**:
   - The client enters a **continuous loop** where it accepts user input for file operations (`CREATE`, `READ`, `WRITE`, etc.).

4. **Packet Initialization**:
   - For each command, the client initializes `PACKET` structures (`CLtoNS`, `CLtoSS`, etc.) to facilitate structured communication.

5. **Request to Naming Server**:
   - The client sends the request to the Naming Server using the `send_req_to_ns()` function.
   - The Naming Server responds:
     - For **privileged requests** (e.g., `LIST`, `INFO`), the NS processes the request directly.
     - For **non-privileged requests** (e.g., `CREATE`, `READ`), the NS returns the IP and port of the responsible Storage Server.

6. **Request to Storage Server**:
   - For non-privileged requests, the client connects to the Storage Server using `send_req_to_ss()`.
   - The client sends the file operation request to the Storage Server for execution.

7. **Response Handling**:
   - The client receives a response (ACK or error message) from either the NS or the SS.
   - If the response indicates an error (e.g., file not found, operation failed), the client handles it gracefully and provides feedback to the user.

8. **Audio Streaming**:
   - For `STREAM` requests, the client uses the `stream_audio_with_mpv()` function to handle received audio data.
   - The audio is written to a temporary `.wav` file and streamed using the `mpv` media player.

9. **Synchronization**:
   - Mutex locks (`recv_lock` and `send_lock`) ensure thread-safe send and receive operations during client-server communication.

10. **Termination**:
    - The client terminates when the user enters the `END` command.

---

## Supported Operations

The client supports the following commands:

- **CREATE <filepath> <filename>**: Create a new file at the specified path.
- **READ <filename>**: Read the contents of a file.
- **WRITE <filename> <data>**: Write data to a file.
- **DELETE <filename>**: Delete a file.
- **LIST**: List all files in the system.
- **STREAM <filename>**: Stream audio data from a file.
- **INFO <filename>**: Retrieve information about a file.
- **COPY <filename1> <filename2>**: Copy a file from one name to another.
- **END**: Exit the client program.

---

## Example Workflow for `CREATE` Request

1. User enters the command: `CREATE /documents report.txt`.
2. The client initializes a `PACKET` with request type `CREATE_REQUEST` and populates file paths.
3. The client sends the request to the Naming Server via `send_req_to_ns()`.
4. The Naming Server responds with the IP and port of the responsible Storage Server.
5. The client connects to the Storage Server using `send_req_to_ss()` and sends the `CREATE` request.
6. The Storage Server creates the file and returns an acknowledgment to the client.
7. The client displays the result and prompts for the next command.

---

## Summary of Key Functions

- **`obtain_client_ip()`**: Retrieves the client’s IP address.
- **`obtain_client_port()`**: Dynamically assigns an available port to the client.
- **`send_req_to_ns()`**: Sends requests to the Naming Server and retrieves responses.
- **`send_req_to_ss()`**: Sends requests to the Storage Server and retrieves responses.
- **`stream_audio_with_mpv()`**: Handles audio streaming using the `mpv` media player.
- **`init_PACKET()` and `init_Data_Packet()`**: Initialize packet structures for communication.
- **Mutex Locks** (`recv_lock`, `send_lock`): Ensure thread-safe operations.

---

## Conclusion

This flow ensures a modular and robust client design, capable of handling various file operations in a distributed file system. The client efficiently interacts with the Naming Server and Storage Servers while ensuring thread safety and user-friendly error handling.

### Naming Server

# Naming Server Code Flow

The Naming Server is a key component of the Distributed File System that coordinates between clients and Storage Servers (SS), manages metadata, and ensures data integrity. Below is the structured flow of the Naming Server's functionality.

---

## Flow of the Naming Server

1. **Initialization**:
   - The Naming Server initializes sockets, mutex locks, and data structures:
     - **SS List**: Tracks active Storage Servers (SS).
     - **Trie Structures**: Maintains metadata for files and directories.
     - **LRU Cache**: Optimizes frequent file lookups.
   - A primary socket binds to a dynamically assigned port and starts listening for connections.

2. **Connection Handling**:
   - A thread continuously accepts incoming connections:
     - **Client Connections**: Spawns a thread to handle requests.
     - **SS Connections**: Registers SS and synchronizes metadata.

3. **CREATE Request**:
   - Determines the least loaded SS using `create_request_finder`.
   - Sends SS details (IP and port) to the client.
   - Updates the metadata trie with the new file/directory.

4. **READ Request**:
   - Resolves the SS responsible for the requested file using `read_request_finder`.
   - Sends SS details to the client.

5. **WRITE Request**:
   - Maps the file to the appropriate SS for updates.
   - Updates metadata after a successful write.

6. **DELETE Request**:
   - Removes a file or directory from the metadata trie.
   - Sends details to the SS for deletion and updates backups.

7. **LIST Request**:
   - Retrieves all file and directory names from the trie.
   - Sends the list of files and directories to the client.

8. **COPY Request**:
   - Facilitates copying of files or directories between two SS instances.
   - Ensures synchronization and updates backups accordingly.

9. **Storage Server (SS) Registration**:
   - Registers new SS instances upon connection.
   - Synchronizes metadata (files, directories) using the trie structure.
   - Tracks backup SS instances for fault tolerance.

10. **Backup Management**:
    - Assigns backup SS instances to each active SS.
    - Balances backup load dynamically using `SSbackupMap_handler`.

11. **Metadata Management**:
    - **Trie**: Used to efficiently manage file and directory paths.
      - **Insert**: Adds files or directories.
      - **Search**: Finds files or directories.
      - **Retrieve All**: Retrieves all files and directories for `LIST` requests.

12. **LRU Cache**:
    - **Add**: Inserts frequently accessed records, evicting the least recently used.
    - **Retrieve**: Fetches metadata for cached files with constant-time access.

13. **Concurrency**:
    - Threads handle individual client and SS connections in parallel.
    - Mutex locks (`SS_lock`, `socket_Rlock`, `socket_Wlock`) ensure thread-safe access to shared resources.

14. **Error Handling**:
    - If metadata is unavailable, the Naming Server sends appropriate error messages to the client.
    - Ensures operations gracefully handle failures in client or SS communication.

15. **Example Workflow: CREATE Request**:
    - Client sends a `CREATE` request.
    - Naming Server identifies an SS and sends its details to the client.
    - Updates the trie to reflect the new file and synchronizes with backups.

---

## Summary

The Naming Server provides efficient metadata management using Tries and an LRU Cache, ensures robust coordination between clients and SS instances, and maintains fault tolerance with backup SS mechanisms. Its modular design and thread-safe operations make it scalable and reliable for distributed systems.


### Storage Server

# Storage Server Code Flow

The Storage Server handles file operations (e.g., read, write, create, delete) in the Distributed File System. Below is the structured flow of the Storage Server's functionality.

---

## Flow of the Storage Server

1. **Initialization**:
   - Initializes file storage structures using `initialise_SS_files`.
   - Prepares hash table for managing files (`File* files`) and directories.
   - Configures root directory for relative path operations.
   - Establishes socket communication with the Naming Server (NS).

2. **Connection Handling**:
   - Opens a socket to accept client connections.
   - Accepts connections in a loop, spawning detached threads (`handle_client`) to handle each client independently.

3. **CREATE Request**:
   - Distinguishes between file and directory creation.
   - Uses `mkdir` for directories and `creat` for files.
   - Updates metadata and notifies the NS about the operation's success or failure.

4. **READ Request**:
   - Checks if the requested file exists.
   - Uses locks (`pthread_mutex`) to prevent conflicts during concurrent reads and writes.
   - Streams file data to the client in chunks, ensuring the NS is informed of the operation status.

5. **WRITE Request**:
   - Supports both synchronous and asynchronous writes:
     - **Synchronous Write**: Writes data to the file immediately.
     - **Asynchronous Write**: Introduces a delay (`usleep`) between data chunks for smoother operations.
   - Ensures thread-safe writes using `pthread_mutex` and `pthread_cond`.

6. **DELETE Request**:
   - Handles both files and directories:
     - **Files**: Marks files as deleted, signals waiting threads, and removes the file.
     - **Directories**: Recursively deletes all files and subdirectories.
   - Sends success or failure status to both the client and NS.

7. **LIST Request**:
   - Traverses the file system to list all files and directories.
   - Sends a concatenated list to the client for display.

8. **STREAM Request**:
   - Streams file data to the client in real-time.
   - Ensures write operations are locked during the stream.

9. **INFO Request**:
   - Retrieves metadata (e.g., size, permissions, owner) using `stat`.
   - Sends the metadata details to the client in a structured format.

10. **Concurrency Management**:
    - Uses `pthread_mutex` and `pthread_cond` for synchronization between threads.
    - Each client is handled in a separate detached thread to ensure non-blocking operations.

11. **File Hashing**:
    - Files are indexed using a hash function (`hash_iNode`) for efficient lookups.
    - Linear probing resolves hash collisions.

12. **Error Handling**:
    - Handles errors gracefully for all requests, including file not found, invalid paths, and client disconnects.
    - Sends appropriate error responses to both the client and NS.

13. **Path Management**:
    - Supports relative-to-absolute and absolute-to-relative path conversions to ensure consistency with the root directory.

14. **Logging and Debugging**:
    - Outputs debug messages for critical events (e.g., client connections, file operations).
    - Provides detailed error messages for troubleshooting.

15. **Example Workflow: WRITE Request**:
    - Client sends a `WRITE` request with file data.
    - The server locks the file, writes data in chunks, and updates metadata.
    - Upon completion, sends a success message to both the client and NS.

---

## Summary

The Storage Server ensures efficient and secure handling of file operations in the distributed system. Its modular design and robust error handling make it a reliable component for storage management and client interactions.


## Miscellaneous Details

## Conclusion

