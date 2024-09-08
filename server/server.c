// local buffer file content
// reader writer lock for clients for each line
// mutex for file updation - no writer is allowed but readers are allowed for reading from local buffer except the line which is being updated from local buffer also

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h> 

#define MAX_DOCUMENTS 10
#define MAX_USERS 100
#define MAX_LINES 1000

clock_t start, end;
double cpu_time_used;

typedef enum { READ, WRITE } AccessType;

typedef struct {
    int id;
    char docName[255];
    char content[MAX_LINES][1024]; // Maximum 1024 characters per line
    pthread_mutex_t lock[MAX_LINES]; // Mutex lock for each line
    sem_t semaphore[MAX_LINES];
} Document;

typedef struct {
    int id;
    char username[50];
    char password[50];
    int admin;
} User;


// Define QueueNode structure for queue implementation
typedef struct QueueNode {
    int docId;
    char content[1024];
    char docName[255];
    int userId;
    int line_number;
    AccessType accessType;
    struct QueueNode* next;
} QueueNode;

// Define Queue structure
typedef struct {
    QueueNode* front;
    QueueNode* rear;
} Queue;

Document documents[MAX_DOCUMENTS];
User users[MAX_USERS];
int next_document_id = 1;
int next_user_id = 1;
pthread_mutex_t queueMutex = PTHREAD_MUTEX_INITIALIZER;
Queue accessQueue;

// Initialize queue
void initQueue() {
    accessQueue.front = NULL;
    accessQueue.rear = NULL;
}

// Enqueue operation
void enqueue(int docId, int userId, int line_number, AccessType accessType, char content[1024]) {
    QueueNode* newNode = (QueueNode*)malloc(sizeof(QueueNode));
    newNode->docId = docId;
    newNode->userId = userId;
    newNode->line_number = line_number;
    newNode->accessType = accessType;
    strcpy(newNode->content, content);
    newNode->next = NULL;

    pthread_mutex_lock(&queueMutex);
    if (accessQueue.front == NULL) {
        accessQueue.front = newNode;
        accessQueue.rear = newNode;
    } else {
        accessQueue.rear->next = newNode;
        accessQueue.rear = newNode;
    }
    pthread_mutex_unlock(&queueMutex);
}

// Dequeue operation - search node with given line no and operation and then dequeue
QueueNode* dequeue() {
    pthread_mutex_lock(&queueMutex);
    if (accessQueue.front == NULL) {
        pthread_mutex_unlock(&queueMutex);
        return NULL;
    }

    QueueNode* temp = accessQueue.front;
    accessQueue.front = accessQueue.front->next;
    if (accessQueue.front == NULL) {
        accessQueue.rear = NULL;
    }
    pthread_mutex_unlock(&queueMutex);

    return temp;
}

// Function to initialize documents and users
void initialize() {
    for (int i = 0; i < MAX_DOCUMENTS; i++) {
        documents[i].id = -1;
    }
    for (int i = 0; i < MAX_USERS; i++) {
        users[i].id = -1;
    }
}

// Function to create a new document
void createDocument(char filename[1024]) {
    int docId = next_document_id++;
    documents[docId].id = docId;
    // printf("Enter document name : ");
    strcpy(documents[docId].docName, filename);
    // Initialize mutex locks for each line
    for (int i = 0; i < MAX_LINES; i++) {
        pthread_mutex_init(&documents[docId].lock[i], NULL);
        sem_init(&documents[docId].semaphore[i],0,0);
    }
    printf("Document created successfully!!\n");
}


void signal_not_available(){
	// do you want to read or write another line?
	printf("Signal not available\n");
	printf("Do you want to read or write another line?\n");
}

void readDocument(int docId, int line_number){
	FILE *file = fopen(documents[docId].docName, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: Unable to open file '%s'\n", documents[docId].docName);
        return;
    }

    // Get the line number from the command-line argument
    
    if (line_number <= 0) {
        fprintf(stderr, "Error: Invalid line number\n");
        fclose(file);
        return;
    }

    // Read lines until reaching the desired line
    char line[1024];
    int current_line = 0;
    while (fgets(line, sizeof(line), file) != NULL) {
        current_line++;
        if (current_line == line_number) {
            printf("Line %d: %s", line_number, line);
            fclose(file);
            return;
        }
    }

    // If the desired line number exceeds the total number of lines
    fprintf(stderr, "Error: Line number %d exceeds total number of lines\n", line_number);
    fclose(file);
}

void writeDocument(int docId, int line_number, char new_content[1024]) {
    // Implement document writing logic
    printf("I am in writeDocument\n");
    FILE *file = fopen(documents[docId].docName, "r+");
    if (file == NULL) {
        fprintf(stderr, "Error: Unable to open file '%s'\n", documents[docId].docName);
        return;
    }

    // Get the new content to be written
    // char new_content[1024] = "This is a new content";
    // printf("Enter the new content: ");
    // fgets(new_content, sizeof(new_content), stdin);

    // Move to the beginning of the file
    rewind(file);

    // Variables for reading lines
    char line[1024];
    long int position = 0;
    int current_line = 0;

    // Find the position of the line to overwrite
    while (fgets(line, sizeof(line), file) != NULL) {
        current_line++;
        if (current_line == line_number) {
            position = ftell(file) - strlen(line); // Get the position before reading the line
            break;
        }
    }

    // If the specified line is not found
    if (current_line != line_number) {
        fprintf(stderr, "Error: Line number %d not found\n", line_number);
        fclose(file);
        return;
    }

    // Move to the position of the line to overwrite
    fseek(file, position, SEEK_SET);

    // Calculate the difference in length between the new content and the existing line
    int length_difference = strlen(new_content) - strlen(line)+1;

    // Extend the line if the new content is longer than the existing line
    if (length_difference > 0) {

        // Add spaces to extend the line
        for (int i = 0; i < strlen(line); i++) {
            if(i==strlen(line)-1)
                break;
            fputc(new_content[i], file);
        }
        printf("Content of line %d extended with new content\n", line_number);
    } else if(length_difference<0){
        // Overwrite the line with new content
        fputs(new_content, file);

        // Remove any extra characters from the existing line by writing spaces
        for (int i = 0; i < -length_difference-1; i++) {
            fputc(' ', file);
        }

        printf("Content of line %d overwritten with new content\n", line_number);
    } else{
        printf("same length\n");
        fputs(new_content, file);
    }

    // Close the file
    fclose(file);
}

void grantAccess(int docId, int line_number, AccessType accessType, char new_content[1024]) {
    int userId = 0;
    // if (docId < 0 || docId >= MAX_DOCUMENTS || documents[docId].id == -1 || userId < 0 || userId >= MAX_USERS || users[userId].id == -1) {
    if (docId < 0 || docId >= MAX_DOCUMENTS || documents[docId].id == -1 ) { 
        return; // Invalid document or user
    }

    // Grant access based on access type (READ or WRITE)
    if (accessType == READ) {
        // Implement logic for read access
        // pthread_mutex_lock(&(documents[docId].lock[line_number]));
        if (pthread_mutex_trylock(&(documents[docId].lock[line_number-1])) == 0) {
        	pthread_mutex_unlock(&(documents[docId].lock[line_number-1]));
            sem_post(&(documents[docId].semaphore[line_number-1]));
            printf("Access granted for reading\n");
            readDocument(docId, line_number);
            sem_wait(&(documents[docId].semaphore[line_number-1]));
            // printf("Content of line %d: %s\n", line_number, documents[docId].content[line_number]);
            
        } else {
            // If mutex is locked, enqueue the request and send signal SIGUSR1
            enqueue(docId, userId, line_number, accessType,new_content);
            // raise(SIGUSR1);
            signal_not_available();
        }
    } else if (accessType == WRITE) {
        // Implement logic for write access
        // pthread_mutex_lock(&(documents[docId].lock[line_number]));
        int value;
        sem_getvalue(&documents[docId].semaphore[line_number-1], &value);
        if (pthread_mutex_trylock(&(documents[docId].lock[line_number-1])) == 0 && value==0){
        // if (pthread_mutex_trylock(&(documents[docId].lock[line_number-1])) == 0 && sem_trywait(&(documents[docId].semaphore[line_number-1])) == -1) {
            printf("Access granted for writing\n");
            writeDocument(docId, line_number, new_content);
            // sem_post(&(documents[docId].semaphore[line_number-1])); 
            pthread_mutex_unlock(&(documents[docId].lock[line_number-1]));
        } else {
            // If mutex is locked or semaphore value is not zero, enqueue the request and send signal SIGUSR1
            enqueue(docId, userId, line_number, accessType,new_content);
            // raise(SIGUSR1);
            signal_not_available();
        }
    }
}

void* client_handler(void* arg){
    char username[50];
     // Buffer to store received message
    int client_socket = *(int *)arg;

    // Read message from the client
    int bytes_received = recv(client_socket, username, sizeof(username), 0);
    if (bytes_received <= 0) {
        perror("Error receiving message from client");
        close(client_socket);
        return NULL;
    }

    // Null-terminate the received message
    username[bytes_received] = '\0';
    // printf("username : %s %d %d\n",username,strcmp(username,"admin"),bytes_received);
    // char admin[] = {'a','d','m','i','n','\0'};
    if(bytes_received==6 && username[0]=='a' && username[1]=='d' && username[2]=='m' && username[3]=='i' && username[4]=='n'){
        // Read message from the client
        char buffer[1024];
        bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            perror("Error receiving message from client");
            close(client_socket);
            return NULL;
        }

        // Null-terminate the received message
        buffer[bytes_received] = '\0';
        createDocument(buffer);
    } else{

        char buffer[1024];
        // Assuming the client sends the message in the format: "document_name operation_choice line_number"
        char docname[255];
        int choice;
        int line_number;
        char new_content[1024];
        
        // Read message from the client
        bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            perror("Error receiving message from client");
            close(client_socket);
            return NULL;
        }

        // Null-terminate the received message
        buffer[bytes_received] = '\0';

        // Extract document name, operation choice, and line number from the received message
        sscanf(buffer, "%s %d %d", docname, &choice, &line_number);

        // Process the received information and grant access accordingly
        int flag = 0, ind=0;
        for(int i = 0; i < MAX_DOCUMENTS; i++) {
            if(documents[i].id == -1) {
                ind = i;
                flag = 1;
                break;
            }
            else if(strcmp(docname, documents[i].docName) != 0) {
                ind = i;
                flag = 1;
            } 
            else if(strcmp(docname, documents[i].docName)==0) {
                ind = i;
                flag = 0;
                break;
            }
        }

        if(flag == 1) {
            documents[ind].id = ind+1;
            strcpy(documents[ind].docName, docname);
        }

        switch(choice) {
            case 1:
                grantAccess(ind, line_number, READ, "");
                break;
            case 2:
                bytes_received = recv(client_socket, new_content, sizeof(new_content), 0);
                if (bytes_received <= 0) {
                    perror("Error receiving message from client");
                    close(client_socket);
                    return NULL;
                }

                // Null-terminate the received message
                new_content[bytes_received] = '\0';
                grantAccess(ind, line_number, WRITE, new_content);
                break;
            default:
                printf("Invalid choice\n");
                break;
        }

    }

        // Get the end time after processing client request
        end = clock();

        // Calculate the time taken for processing the request
        cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
        printf("Time taken: %f seconds\n", cpu_time_used);

    return NULL;
}

int main() {
    initialize();

    int server_socket;
    struct sockaddr_in server_addr;

    

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Initialize server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    // Bind socket to address
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_socket, 5) < 0) {
        perror("Listening failed");
        exit(EXIT_FAILURE);
    }

    // Accept client connections
	while (1) {
	    int client_socket;
	    struct sockaddr_in client_addr;
	    int client_addr_len = sizeof(client_addr);

	    // Accept connection
        start = clock();
	    client_socket = accept(server_socket, (struct sockaddr*)&client_addr, (socklen_t*)&client_addr_len);
	    if (client_socket < 0) {
	        perror("Acceptance failed");
	        continue;
	    }

	    

	    // Spawn a new thread to handle the client
	    pthread_t client_thread;
	    pthread_create(&client_thread, NULL, client_handler, (void*)&client_socket);




        // Process pending requests from the queue
        QueueNode* request;
        while ((request = dequeue()) != NULL) {
            printf("Processing pending request...\n");
            grantAccess(request->docId, request->line_number, request->accessType, request->content);
            free(request);
        }
	}


    // Close server socket
    close(server_socket);

    return 0;
}
