# NetCoDE - Network based Collaborative Document Editor 

## Overview

This project is a **Document Management System** that allows multiple clients to **concurrently read and write** to specific lines in documents. The system supports **multiple clients**, with robust synchronization mechanisms using **mutexes** and **semaphores** to prevent race conditions and ensure smooth document access.

Admins have special privileges, including the ability to create new documents. The system also implements a **queue for deferred requests**, allowing read/write operations to be queued and processed in **FIFO** order when the requested resource becomes available.

---

## Key Features

### Document Management
- **Documents** are represented by the `Document` structure, which contains:
  - Document ID and name.
  - Content divided into multiple lines (up to `MAX_LINES`).
  - **Mutex locks** and **semaphores** for each line to handle concurrent access safely.
  
### User Management
- **Users** are represented by the `User` structure, which includes:
  - User ID, username, and password.
  - Admin status (admins can create new documents).

### Concurrency Handling
- The system supports multiple clients trying to read or write to specific lines of documents concurrently.
  - **Mutexes**: Each line is protected by a mutex to avoid race conditions during reads and writes.
  - **Semaphores**: Used to coordinate access to lines for multiple readers.
  - **Reader-Writer Locking**: Allows multiple readers to access a line simultaneously, but ensures exclusive access for writes.

### Queue for Deferred Requests
- If a document line is locked for writing or being read while a write is requested, the request is deferred and added to a **queue**.
- Queued requests are processed in **FIFO** order once the line becomes available.

---

## Major Functions

### `initQueue` and `enqueue/dequeue`
- **Queue Initialization**: Initializes the queue for deferred requests.
- **Enqueue/Dequeue Operations**: Requests are enqueued if the line they need is busy and are dequeued and processed once the line becomes available.

### `createDocument`
- **Creates a new document** and initializes mutexes and semaphores for each line in the document to manage concurrent access.

### `readDocument`
- **Reads a specific line** from a document.
- If the line is locked for writing, the request is **deferred** to the queue until the line becomes available.

### `writeDocument`
- **Writes content** to a specific line in a document, ensuring correct positioning and handling differences in line length.

### `grantAccess`
- Handles **access requests** (read/write) for specific document lines.
- Checks if the requested line is available and locks it for the client if possible. If the line is busy, the request is added to the queue for later processing.

---

## Client Handling

### Client Handler (`client_handler`)
- Each client is served by a **separate thread**.
- Clients send requests to **read** or **write** lines from documents. Admin clients can also create new documents.
- If a requested line is busy, the request is deferred to the queue for later processing.

### Server Main Loop (`main`)
- The server listens for new client connections and spawns a **new thread** for each client.
- Each client request is processed asynchronously. If any document line is busy, the request is added to the queue for deferred execution.

---

## Workflow Example

1. A client connects to the server and requests to **read** or **write** a specific line in a document.
2. The server checks if the requested line is available:
   - If available, the server grants access (read or write) and locks the line using mutexes.
   - If not available, the request is **deferred** and added to the **queue**.
3. When the requested line becomes free, queued requests are processed in **FIFO order**.
   
---

## Thread Safety

- **Mutexes**: Ensure that only one client can modify a line at a time, preventing data races.
- **Semaphores**: Allow multiple readers to access a line simultaneously but block writes until all readers are finished.
- **Queue**: Holds pending requests, ensuring they are processed as soon as the requested resource becomes available.

---

## Technologies Used
- **C Programming Language** for core logic and synchronization.
- **POSIX Threads (pthreads)** for thread management.
- **Mutexes** and **Semaphores** for concurrency control.
  
---

## How to Run the Program

1. Compile the program using a compatible C compiler (e.g., GCC):
   ```bash
   gcc -o server server.c -lpthread
   ```

2. Run the server.
   ```bash
   ./server
   ```
   
4. Connect clients to the server and start sending requests for reading/writing lines or creating documents (admins only).
