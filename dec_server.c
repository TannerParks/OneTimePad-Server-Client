#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>


// https://replit.com/@cs344/83clientc?lite=true#client.c

void error(const char *msg) {
    perror(msg);
    exit(1);
}


// Struct for the socket
void setupAddressStruct(struct sockaddr_in* address, int portNumber){
 
  // Clear address struct
  memset((char*) address, '\0', sizeof(*address)); 

  // Network capable
  address->sin_family = AF_INET;
  // Port number
  address->sin_port = htons(portNumber);
  // Allow a client at any address to connect
  address->sin_addr.s_addr = INADDR_ANY;
}


void decrypt(char *cipher_text, char *key_text, char *decrypted_text) {
    int num_plain;
    int num_key;
    int num_decrypt;

    // Turn plaintext/key characters to integers
    for (int i=0; i < strlen(cipher_text); i++) {

        if (cipher_text[i] == ' ') { // Checks for space
            num_plain = 26;
        }
        else {
            num_plain = cipher_text[i] - 'A'; // Sets letter equal to index in alphabet
        }

        if (key_text[i] == ' ') {
            num_key = 26;
        }
        else {
            num_key = key_text[i] - 'A'; // Sets letter equal to index in alphabet
        }

        // Convert decrypted integer back to a character
        num_decrypt = (num_plain - num_key) % 27;

        if (num_decrypt < 0) {
            num_decrypt += 27;
        }
        if (num_decrypt == 26) {
            decrypted_text[i] = ' ';
        }
        else {
            decrypted_text[i] = num_decrypt + 'A';
        }
    }

}

int main(int argc, char *argv[]) {
    int connectionSocket, charsRead, childStatus;
    //char buffer[256]
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t sizeOfClientInfo = sizeof(clientAddress);

    pid_t spawnPid;

    char plain_text[80000]; // Max chars in txt files is 69333 so I just added another 10k
    char key_text[80000];
    char combined[160000];  // Combined plain text and key
    char decrypted_text[80000];

    int decryption_server;  // Decryption server tag for verification
    int text_length;    // Length of the plain + key text
    //int plain_length;

    char buffer[1024];

    char *context = NULL;
    char *text;

    if (argc < 2) {
        fprintf(stderr, "USAGE: %s port &\n", argv[0]);
        exit(1);
    }

    // Socket
    int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket < 0) {
        error("ERROR opening socket");
    }

    setupAddressStruct(&serverAddress, atoi(argv[1]));

    // Bind socket
    if (bind(listenSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        error("ERROR on binding");
    }

    listen(listenSocket, 5); // Listen for up to 5 connections

    while(1) {
        // Generate socket used for communication
        connectionSocket = accept(listenSocket, (struct sockaddr *)&clientAddress, &sizeOfClientInfo);
        if (connectionSocket < 0) {
            error("ERROR on accept");
        }

        spawnPid = fork();
        switch(spawnPid) {
            case -1:
                error("fork() failed!");
                exit(1);
                break;
            
            case 0:
                // confirm to client that this is decryption and not encryption
                decryption_server = 1;
                write(connectionSocket, &decryption_server, sizeof(int));

                // Get text length from client
                text_length = 0;
                read(connectionSocket, &text_length, sizeof(int));

                memset(combined, '\0', sizeof(combined));

                // Read message from the socket
                charsRead = 0;
                while (charsRead < text_length) {
                    charsRead += recv(connectionSocket, buffer, 1000, 0);
                    strcat(combined, buffer);
                    memset(buffer, '\0', sizeof(buffer));
                }
                if (charsRead < 0) {
                    error("ERROR reading from socket");
                }

                // Separate plain text and key
                text = strtok_r(combined, "&", &context);
                strcpy(plain_text, text);
                text = strtok_r(NULL, "&", &context);
                strcpy(key_text, text);

                // Decryption
                decrypt(plain_text, key_text, decrypted_text);

                // Send decryption to client
                charsRead = send(connectionSocket, decrypted_text, sizeof(decrypted_text)-1, 0);
                if (charsRead < 0) {
                    error("ERROR writing to socket");
                }

                // Reset everything for reuse
                memset(combined, '\0', 160000);
                memset(plain_text, '\0', 80000);
                memset(key_text, '\0', 80000);
                memset(decrypted_text, '\0', 80000);

                close(connectionSocket);    // Close connection socket
                break;
            
            default:
                spawnPid = waitpid(spawnPid, &childStatus, WNOHANG);
                break;
        }
    }

    close(listenSocket);    // Close listening socket
    return 0;
}