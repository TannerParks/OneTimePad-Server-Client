#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>  // ssize_t
#include <sys/socket.h> // send(),recv()
#include <netdb.h>      // gethostbyname()


// https://replit.com/@cs344/83clientc?lite=true#client.c

// A lot of this code is copied from the replit link we were given and I used benrprince's github to help with how
// to send 1000 characters at a time and also to verify that we were in the right server


// Error function used for reporting issues
void error(const char *msg, int exit_val) { 
  perror(msg); 
  exit(exit_val); 
} 


// Set up the address struct
void setupAddressStruct(struct sockaddr_in* address, int portNumber){
 
  // Clear out the address struct
  memset((char*) address, '\0', sizeof(*address)); 

  // Network capable
  address->sin_family = AF_INET;
  // Store the port number
  address->sin_port = htons(portNumber);

  // Get the DNS entry
  struct hostent* hostInfo = gethostbyname("localhost"); 
  if (hostInfo == NULL) { 
    fprintf(stderr, "CLIENT: ERROR, no such host\n"); 
    exit(0); 
  }
  // Copy the first IP address from the DNS entry to sin_addr.s_addr
  memcpy((char*) &address->sin_addr.s_addr, 
        hostInfo->h_addr_list[0],
        hostInfo->h_length);
}


void check_bad(char *plain, char *key) {
    // Checks for anything except space and capital letters
    for (int i=0; i < strlen(plain); i++) {
        if(((plain[i] < 65) && plain[i] != 32) || (plain[i] > 90)) {
            error("CLIENT: ERROR bad characters in plain text", 1);
        }
    }
    for (int i=0; i < strlen(plain); i++) {
        if(((plain[i] < 65) && plain[i] != 32) || (plain[i] > 90)) {
            error("CLIENT: ERROR bad characters in key", 1);
        }
    }
}


int main(int argc, char *argv[]) {
    int socketFD, portNumber, charsWritten, charsRead, server;
    struct sockaddr_in serverAddress;

    char plain_text[80000]; // Max chars in txt files is 69333 so I just added another 10k
    char key_text[80000];
    char combined[160000];

    FILE *plain_file;
    FILE *key_file;

    int text_length;
    int plain_length;

    char buffer[1024];

    if (argc < 4) { 
        fprintf(stderr,"USAGE: %s text key port\n", argv[0]); 
        exit(0);
    }

    socketFD = socket(AF_INET, SOCK_STREAM, 0); 
    if (socketFD < 0){
        error("CLIENT: ERROR opening socket", 2);
    }

    // Set up server address struct
    setupAddressStruct(&serverAddress, atoi(argv[3]));

    // Connect to server
    if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
        error("CLIENT: ERROR connecting", 2);
    }

    // Check that we're at the right server
    read(socketFD, &server, sizeof(int));
    //printf("After read\n");
    if (server != 0) {
        error("CLIENT: ERROR wrong server", 2);
    }

    // Clear out the buffer arrays
    memset(plain_text, '\0', sizeof(plain_text));
    memset(key_text, '\0', sizeof(key_text));

    // Get plain text from file
    plain_file = fopen(argv[1], "r");
    fgets(plain_text, sizeof(plain_text)-1, plain_file);
    plain_text[strcspn(plain_text, "\n")] = '\0';
    fclose(plain_file);
    
    // Get key form file
    key_file = fopen(argv[2], "r");
    fgets(key_text, sizeof(key_text)-1, key_file);
    key_text[strcspn(key_text, "\n")] = '\0';
    fclose(key_file);

    strcat(combined, plain_text);
    strcat(combined, "&");  // Separate text and key for later
    strcat(combined, key_text);

    // Check if any file has bad characters in them then terminate if so
    check_bad(plain_text, key_text);

    // Check if key file is shorter than plain text file
    if(strlen(key_text) < strlen(plain_text)) {
        error("CLIENT: Key is too short", 1);
    }

    text_length = strlen(combined);
    write(socketFD, &text_length, sizeof(int));

    plain_length = strlen(plain_text);

    // Send plain text to the server
    charsWritten = send(socketFD, combined, strlen(combined), 0);
    if (charsWritten < 0){
      error("CLIENT: ERROR writing plain text to socket", 2);
    }
    if (charsWritten < strlen(plain_text)){
      printf("CLIENT: WARNING: Not all plain text data written to socket!\n");
    }

    // Clear buffer for reuse
    memset(plain_text, '\0', sizeof(plain_text));

    charsRead = 0;
    while (charsRead < plain_length) {
        charsRead += recv(socketFD, buffer, 1000, 0);
        //printf("%d\n", charsRead);
        strcat(plain_text, buffer);
        memset(buffer, '\0', sizeof(buffer));
    }
    //printf("After read from socket\n");
    if (charsRead < 0) {
        error("CLIENT: ERROR reading from socket", 2);
    }
    printf("%s\n", plain_text);

    close(socketFD);
    return 0;
}