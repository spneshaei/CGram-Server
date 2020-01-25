//
//  main.c
//  CGramServer
//
//  Created by Seyyed Parsa Neshaei on 12/20/19.
//  Copyright © 2019 Seyyed Parsa Neshaei. All rights reserved.
//

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <time.h>
#include "cJSON.h"
#define MAX 1000000
#define PORT 12345
#define SA struct sockaddr

typedef struct User {
    char username[1000];
    char token[20];
    char currentChannel[1000];
} User;

User allUsers[MAX];
int currentAllUsersIndex = 0;

void splitString(char str[1000000], char newString[1000][1000], int *countOfWords) {
    int j = 0, count = 0;
    for(int i = 0; i <= strlen(str); i++) {
        if (str[i] == '\0' || str[i] == ' ' || str[i] == ',' || str[i] == '\n') {
            newString[count][j] = '\0';
            count += 1;
            j = 0;
        } else {
            newString[count][j] = str[i];
            j++;
        }
    }
    *countOfWords = count;
}

void writeToFile(char *fileName, char *data) {
    FILE *fptr = fopen(fileName, "w");
    unsigned long len = strlen(data);
    for (int i = 0; i < len; i++) {
        fputc(data[i], fptr);
    }
    fputc('\0', fptr);
    fclose(fptr);
}

void readFromFile(char *fileName, char *data) {
    FILE *fptr = fopen(fileName, "r");
    if (fptr != NULL) {
        int i = 0;
        while (1) {
            char c = fgetc(fptr);
            if (c == EOF) {
                fclose(fptr);
                return;
            }
            data[i] = c;
            i++;
        }
    }
    fclose(fptr);
}

void generateToken(char *token, int size) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJK...";
    if (size) {
        --size;
        for (size_t n = 0; n < size; n++) {
            int key = rand() % (int) (sizeof charset - 1);
            token[n] = charset[key];
        }
        token[size] = '\0';
    }
}

void badRequest(char *result) {
    strcpy(result, "{\"type\":\"Error\",\"content\":\"Invalid request format.\"}");
}

int userHasGivenPasswordInJSON(char *user, char *pass, cJSON *root) {
    cJSON *users = cJSON_GetObjectItemCaseSensitive(root, "users");
    cJSON *userJSON = NULL;
    cJSON_ArrayForEach(userJSON, users) {
        cJSON *username = cJSON_GetObjectItemCaseSensitive(userJSON, "username");
        cJSON *password = cJSON_GetObjectItemCaseSensitive(userJSON, "password");
        if (cJSON_IsString(username) && cJSON_IsString(password)) {
            if (username -> valuestring && strcmp(username -> valuestring, user) == 0) {
                if (!(password->valuestring) || strcmp(password->valuestring, pass) != 0) {
                    return 0;
                }
                return 1;
            }
        }
    }
    return 0;
}

int channelExitsInJSON(char *channelName, cJSON *root) {
    cJSON *channels = cJSON_GetObjectItemCaseSensitive(root, "channels");
    cJSON *channelJSON = NULL;
    cJSON_ArrayForEach(channelJSON, channels) {
        cJSON *name = cJSON_GetObjectItemCaseSensitive(channelJSON, "name");
        if (cJSON_IsString(name)) {
            if (name -> valuestring && strcmp(name -> valuestring, channelName) == 0) {
                return 1;
            }
        }
    }
    return 0;
}

int userExitsInJSON(char *user, cJSON *root) {
    cJSON *users = cJSON_GetObjectItemCaseSensitive(root, "users");
    cJSON *userJSON = NULL;
    cJSON_ArrayForEach(userJSON, users) {
        cJSON *username = cJSON_GetObjectItemCaseSensitive(userJSON, "username");
        if (cJSON_IsString(username)) {
            if (username -> valuestring && strcmp(username -> valuestring, user) == 0) {
                return 1;
            }
        }
    }
    return 0;
}

int userIDHavingGivenToken(char *token) { // returns -1 if nothing found, index of user in allUsers if found
    for (int i = 0; i < currentAllUsersIndex; i++) {
        if (strcmp(allUsers[i].token, token) == 0) {
            return i;
        }
    }
    return -1;
}

void doLogin(char *username, char *password, char *result) {
    char data[100000] = {}, fileName[] = "users.txt";
    readFromFile(fileName, data);
    if (strcmp(data, "") == 0) {
        strcpy(result, "{\"type\":\"Error\",\"content\":\"this username is not available.\"}");
        return;
    }
    cJSON *root = cJSON_Parse(data);
    if (root == NULL) {
        strcpy(result, "{\"type\":\"Error\",\"content\":\"this username is not available.\"}");
        return;
    }
    if (!userExitsInJSON(username, root)) {
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Username is not valid.\"}");
        return;
    }
    if (!userHasGivenPasswordInJSON(username, password, root)) {
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Wrong password.\"}");
        return;
    }
    char token[20] = {};
    generateToken(token, 15);
    User user = {};
    strcpy(user.username, username);
    strcpy(user.token, token);
    if (currentAllUsersIndex == MAX) {
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Maximum number of concurrent users reached.\"}");
        return;
    }
    allUsers[currentAllUsersIndex] = user;
    currentAllUsersIndex++;
    strcpy(result, "{\"type\":\"AuthToken\",\"content\":\"");
    strcat(result, token);
    strcat(result, "\"}");
}

void doRegister(char *username, char *password, char *result) {
    char data[100000] = {}, fileName[] = "users.txt";
    readFromFile(fileName, data);
    if (strcmp(data, "") == 0) {
        strcat(data, "{\"users\": [{ \"username\": \"");
        strcat(data, username);
        strcat(data, "\", \"password\": \"");
        strcat(data, password);
        strcat(data, "\"}]}");
        writeToFile(fileName, data);
        strcpy(result, "{\"type\":\"Successful\",\"content\":\"\"}");
        return;
    }
    cJSON *root = cJSON_Parse(data);
    if (root == NULL) {
        strcat(data, "{\"users\": [{ \"username\": \"");
        strcat(data, username);
        strcat(data, "\", \"password\": \"");
        strcat(data, password);
        strcat(data, "\"}]}");
        writeToFile(fileName, data);
        strcpy(result, "{\"type\":\"Successful\",\"content\":\"\"}");
        return;
    }
    if (!userExitsInJSON(username, root)) {
        cJSON *users = cJSON_GetObjectItemCaseSensitive(root, "users");
        cJSON *obj = cJSON_CreateObject();
        cJSON_AddStringToObject(obj, "username", username);
        cJSON_AddStringToObject(obj, "password", password);
        cJSON_AddItemToArray(users, obj);
        strcpy(data, cJSON_Print(root));
        writeToFile(fileName, data);
        strcpy(result, "{\"type\":\"Successful\",\"content\":\"\"}");
    } else {
        strcpy(result, "{\"type\":\"Error\",\"content\":\"this username is not available.\"}");
    }
}

void doLogout(char *token, char *result) {
    int id = userIDHavingGivenToken(token);
    if (id == -1) {
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Authentication failed!\"}");
        return;
    }
    strcpy(allUsers[id].username, "");
    strcpy(allUsers[id].token, "");
    strcpy(result, "{\"type\":\"Successful\",\"content\":\"\"}");
}

void doCreateChannel(char *channelName, char *token, char *result) {
    char data[MAX] = {}, fileName[] = "channels.txt";
    readFromFile(fileName, data);
    int id = userIDHavingGivenToken(token);
    if (strcmp(data, "") == 0) {
        strcat(data, "{\"channels\": [{ \"name\": \"");
        strcat(data, channelName);
        strcat(data, "\", \"members\": [\"");
        strcat(data, allUsers[id].username);
        strcat(data, "\"], \"messages\": []}]}");
        if (id == -1) {
            // TODO: Error message wrong
            strcpy(result, "{\"type\":\"Error\",\"content\":\"Channel name is not available.\"}");
            return;
        }
        if (strcmp(allUsers[id].currentChannel, "") != 0) {
            strcpy(result, "{\"type\":\"Error\",\"content\":\"You are in another channel.\"}");
            return;
        }
        strcpy(allUsers[id].currentChannel, channelName);
        writeToFile(fileName, data);
        strcpy(result, "{\"type\":\"Successful\",\"content\":\"\"}");
        return;
    }
    printf("DATA::: %s\n", data);
    cJSON *root = cJSON_Parse(data);
    if (root == NULL) {
        strcat(data, "{\"channels\": [{ \"name\": \"");
        strcat(data, channelName);
        strcat(data, "\", \"members\": [\"");
        strcat(data, allUsers[id].username);
        strcat(data, "\"], \"messages\": []}]}");
        if (id == -1) {
            // TODO: Error message wrong
            strcpy(result, "{\"type\":\"Error\",\"content\":\"Channel name is not available.\"}");
            return;
        }
        if (strcmp(allUsers[id].currentChannel, "") != 0) {
            strcpy(result, "{\"type\":\"Error\",\"content\":\"You are in another channel.\"}");
            return;
        }
        strcpy(allUsers[id].currentChannel, channelName);
        writeToFile(fileName, data);
        strcpy(result, "{\"type\":\"Successful\",\"content\":\"\"}");
        return;
    }
    if (channelExitsInJSON(channelName, root)) {
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Channel name is not available.\"}");
        return;
    }
    if (id == -1) {
        // TODO: Error message wrong
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Channel name is not available.\"}");
        return;
    }
    if (strcmp(allUsers[id].currentChannel, "") != 0) {
        strcpy(result, "{\"type\":\"Error\",\"content\":\"You are in another channel.\"}");
        return;
    }
    cJSON *channels = cJSON_GetObjectItemCaseSensitive(root, "channels");
    cJSON *channel = cJSON_CreateObject();
    cJSON *members = cJSON_CreateArray();
    cJSON *member = cJSON_CreateString(allUsers[id].username);
    cJSON *messages = cJSON_CreateArray();
    cJSON *name = cJSON_CreateString(channelName);
    cJSON_AddItemToArray(members, member);
    cJSON_AddItemToObject(channel, "name", name);
    cJSON_AddItemToObject(channel, "members", members);
    cJSON_AddItemToObject(channel, "messages", messages);
    cJSON_AddItemToArray(channels, channel);
    strcpy(data, cJSON_Print(root));
    writeToFile(fileName, data);
    strcpy(result, "{\"type\":\"Successful\",\"content\":\"\"}");
}

void doLeave(char *token, char *result) {
    int id = userIDHavingGivenToken(token);
    if (id == -1) {
        // TODO: Error message wrong
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Error\"}");
        return;
    }
    if (strcmp(allUsers[id].currentChannel, "") == 0) {
        strcpy(result, "{\"type\":\"Error\",\"content\":\"You aren\'t in any channel\"}");
        return;
    }
    char data[MAX] = {}, fileName[] = "channels.txt";
    readFromFile(fileName, data);
    cJSON *root = cJSON_Parse(data);
    if (root == NULL) {
        // TODO: Error message wrong
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Error\"}");
        return;
    }
    char channelName[1000] = {};
    strcpy(channelName, allUsers[id].currentChannel);
    cJSON *channels = cJSON_GetObjectItemCaseSensitive(root, "channels");
    cJSON *channel = NULL;
    cJSON_ArrayForEach(channel, channels) {
        cJSON *name = cJSON_GetObjectItemCaseSensitive(channel, "name");
        if (name->valuestring && (strcmp(name->valuestring, channelName) == 0)) {
            cJSON *members = cJSON_GetObjectItemCaseSensitive(channel, "members");
            cJSON *member = NULL;
            cJSON_ArrayForEach(member, members) {
                if (member->valuestring && strcmp(member->valuestring, allUsers[id].username) == 0) {
                    cJSON_Delete(member);
                    strcpy(data, cJSON_Print(root));
                    writeToFile(fileName, data);
                    strcpy(result, "{\"type\":\"Successful\",\"content\":\"\"}");
                    return;
                }
            }
            break;
        }
    }
    // TODO: Error message wrong
    strcpy(result, "{\"type\":\"Error\",\"content\":\"Error\"}");
}

void doJoinChannel(char *channelName, char *token, char *result) {
    int id = userIDHavingGivenToken(token);
    if (id == -1) {
        // TODO: Error message wrong
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Error\"}");
        return;
    }
    if (strcmp(allUsers[id].currentChannel, "") != 0) {
        strcpy(result, "{\"type\":\"Error\",\"content\":\"You are in another channel.\"}");
        return;
    }
    char data[MAX] = {}, fileName[] = "channels.txt";
    readFromFile(fileName, data);
    cJSON *root = cJSON_Parse(data);
    if (root == NULL) {
        // TODO: Error message wrong
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Error\"}");
        return;
    }
    cJSON *channels = cJSON_GetObjectItemCaseSensitive(root, "channels");
    cJSON *channel = NULL;
    cJSON_ArrayForEach(channel, channels) {
        cJSON *name = cJSON_GetObjectItemCaseSensitive(channel, "name");
        if (name->valuestring && (strcmp(name->valuestring, channelName) == 0)) {
            cJSON *members = cJSON_GetObjectItemCaseSensitive(channel, "members");
            cJSON *member = cJSON_CreateString(allUsers[id].username);
            cJSON_AddItemToArray(members, member);
            strcpy(allUsers[id].currentChannel, channelName);
            strcpy(data, cJSON_Print(root));
            writeToFile(fileName, data);
            strcpy(result, "{\"type\":\"Successful\",\"content\":\"\"}");
            return;
        }
    }
    strcpy(result, "{\"type\":\"Error\",\"content\":\"Channel not found.\"}");
}

void doRefresh(char *token, char *result) {
    int id = userIDHavingGivenToken(token);
    if (id == -1) {
        // TODO: Error message wrong
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Error\"}");
        return;
    }
    if (strcmp(allUsers[id].currentChannel, "") == 0) {
        strcpy(result, "{\"type\":\"Error\",\"content\":\"You aren\'t in any channel\"}");
        return;
    }
    char data[MAX] = {}, fileName[] = "channels.txt";
    readFromFile(fileName, data);
    cJSON *root = cJSON_Parse(data);
    if (root == NULL) {
        // TODO: Error message wrong
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Error\"}");
        return;
    }
    char channelName[1000] = {};
    strcpy(channelName, allUsers[id].currentChannel);
    cJSON *channels = cJSON_GetObjectItemCaseSensitive(root, "channels");
    cJSON *channel = NULL;
    cJSON_ArrayForEach(channel, channels) {
        cJSON *name = cJSON_GetObjectItemCaseSensitive(channel, "name");
        if (name->valuestring && (strcmp(name->valuestring, channelName) == 0)) {
            strcpy(result, "{\"type\":\"List\",\"content\":[");
            cJSON *messages = cJSON_GetObjectItemCaseSensitive(channel, "messages");
            cJSON *message = NULL;
            int isFirstTime = 1;
            cJSON_ArrayForEach(message, messages) {
                if (isFirstTime) {
                    isFirstTime = 0;
                } else {
                    strcat(result, ",");
                }
                cJSON *sender = cJSON_GetObjectItemCaseSensitive(message, "sender");
                cJSON *content = cJSON_GetObjectItemCaseSensitive(message, "content");
                strcat(result, "{\"sender\":\"");
                strcat(result, sender->valuestring);
                strcat(result, "\",\"content\":\"");
                strcat(result, content->valuestring);
                strcat(result, "\"}");
            }
            strcat(result, "]}");
            return;
        }
    }
    strcpy(result, "{\"type\":\"Error\",\"content\":\"Channel not found.\"}");
}

void doSend(char *originalMessage, char *token, char *result) {
    int id = userIDHavingGivenToken(token);
    if (id == -1) {
        // TODO: Error message wrong
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Error\"}");
        return;
    }
    if (strcmp(allUsers[id].currentChannel, "") == 0) {
        // TODO: Study \' or ' (also using replace and find in other places...)
        strcpy(result, "{\"type\":\"Error\",\"content\":\"You aren\'t in any channel\"}");
        return;
    }
    char data[MAX] = {}, fileName[] = "channels.txt";
    readFromFile(fileName, data);
    cJSON *root = cJSON_Parse(data);
    if (root == NULL) {
        // TODO: Error message wrong
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Error\"}");
        return;
    }
    char channelName[1000] = {};
    strcpy(channelName, allUsers[id].currentChannel);
    cJSON *channels = cJSON_GetObjectItemCaseSensitive(root, "channels");
    cJSON *channel = NULL;
    cJSON_ArrayForEach(channel, channels) {
        cJSON *name = cJSON_GetObjectItemCaseSensitive(channel, "name");
        if (name->valuestring && (strcmp(name->valuestring, channelName) == 0)) {
            cJSON *messages = cJSON_GetObjectItemCaseSensitive(channel, "messages");
            cJSON *message = cJSON_CreateObject();
            cJSON *sender = cJSON_CreateString(allUsers[id].username);
            cJSON *content = cJSON_CreateString(originalMessage);
            cJSON_AddItemToObject(message, "sender", sender);
            cJSON_AddItemToObject(message, "content", content);
            cJSON_AddItemToArray(messages, message);
            strcpy(data, cJSON_Print(root));
            writeToFile(fileName, data);
            strcpy(result, "{\"type\":\"Successful\",\"content\":\"\"}");
            return;
        }
    }
    // TODO: Error message wrong
    strcpy(result, "{\"type\":\"Error\",\"content\":\"Error\"}");
}

void doChannelMembers(char *token, char *result) {
    int id = userIDHavingGivenToken(token);
    if (id == -1) {
        // TODO: Error message wrong
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Error\"}");
        return;
    }
    if (strcmp(allUsers[id].currentChannel, "") == 0) {
        strcpy(result, "{\"type\":\"Error\",\"content\":\"You aren\'t in any channel\"}");
        return;
    }
    char data[MAX] = {}, fileName[] = "channels.txt";
    readFromFile(fileName, data);
    cJSON *root = cJSON_Parse(data);
    if (root == NULL) {
        // TODO: Error message wrong
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Error\"}");
        return;
    }
    char channelName[1000] = {};
    strcpy(channelName, allUsers[id].currentChannel);
    cJSON *channels = cJSON_GetObjectItemCaseSensitive(root, "channels");
    cJSON *channel = NULL;
    cJSON_ArrayForEach(channel, channels) {
        cJSON *name = cJSON_GetObjectItemCaseSensitive(channel, "name");
        if (name->valuestring && (strcmp(name->valuestring, channelName) == 0)) {
            strcpy(result, "{\"type\":\"List\",\"content\":[");
            cJSON *members = cJSON_GetObjectItemCaseSensitive(channel, "members");
            cJSON *member = NULL;
            int isFirstTime = 1;
            cJSON_ArrayForEach(member, members) {
                if (isFirstTime) {
                    isFirstTime = 0;
                } else {
                    strcat(result, ",");
                }
                strcat(result, "\"");
                strcat(result, member->valuestring);
                strcat(result, "\"");
            }
            strcat(result, "]}");
            return;
        }
    }
    strcpy(result, "{\"type\":\"Error\",\"content\":\"Channel not found.\"}");
}

void process(char *request) {
    if (strcmp(request, "") == 0) {
        return;
    }
    char parts[1000][1000] = {};
    int countOfParts = 0;
    splitString(request, parts, &countOfParts);
    
    if (countOfParts < 0) {
        badRequest(request);
        return;
    }
    
    char firstPart[1000], secondPart[1000];
    strcpy(firstPart, parts[0]);
    strcpy(secondPart, parts[1]);
    if (strcmp(firstPart, "") != 0) {
        printf(firstPart);
        printf("\n");
    }
    
    
    if (strcmp(firstPart, "login") == 0) {
        doLogin(secondPart, parts[3], request);
    } else if (strcmp(firstPart, "register") == 0) {
        doRegister(secondPart, parts[3], request);
    } else if (strcmp(firstPart, "logout") == 0) {
        doLogout(secondPart, request);
    } else if (strcmp(firstPart, "create") == 0 && strcmp(secondPart, "channel") == 0) {
        doCreateChannel(parts[2], parts[4], request);
    } else if (strcmp(firstPart, "leave") == 0) {
        doLeave(secondPart, request);
    } else if (strcmp(firstPart, "join") == 0 && strcmp(secondPart, "channel") == 0) {
        doJoinChannel(parts[2], parts[4], request);
    } else if (strcmp(firstPart, "refresh") == 0) {
        doRefresh(secondPart, request);
    } else if (strcmp(firstPart, "send") == 0) { // TODO: Bug: Comma and Space in message not fixed!!
        doSend(secondPart, parts[3], request);
    } else if (strcmp(firstPart, "channel") == 0 && strcmp(secondPart, "members") == 0) {
        doChannelMembers(parts[2], request);
    } else {
        badRequest(request);
    }
}

void chat(int client_socket, int server_socket)
{
    char buffer[MAX];
//    int n;
//    while (true) {
        bzero(buffer, sizeof(buffer));
    
    // TODO: Maintanance
//    gets(buffer);
    
        // Read the message from client and copy it to buffer
    
    // TODO: Maintanance
        recv(client_socket, buffer, sizeof(buffer), 0);
    
        // Print buffer which contains the client message
//        printf("From client: %s\t To client : ", buffer);
    
    
    
//        if (strcmp(buffer, "") == 0) {
//            continue;
//        }
    
    
        if (strcmp(buffer, "") != 0) {
            printf("Before processing \"%s\"\n", buffer);
        }
    
        process(buffer);
//        bzero(buffer, sizeof(buffer));
//        n = 0;
//        // Copy server message to the buffer
//        while ((buffer[n++] = getchar()) != '\n')
//            ;
        
        // Send the buffer to client
    
    // TODO: Maintanance
        if (strcmp(buffer, "") != 0) {
            printf("Before sending \"%s\"\n", buffer);
            send(client_socket, buffer, sizeof(buffer), 0);
            printf("Right after sending \"%s\"\n", buffer);
        }
//        if (strcmp(buffer, "") != 0) {
//            printf("SEnt!\n");
//        }
    
//    printf("%s\n", buffer);
    
        // If the message starts with "exit" then server exits and chat ends
//        if (strncmp("exit", buffer, 4) == 0) {
//            printf("Server stopping...\n");
//            return;
//        }
//    }
        
//        shutdown(server_socket, SHUT_RDWR);
//    }
}

void connectServer() {
    int server_socket, client_socket;
    struct sockaddr_in server, client;
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        printf("Socket creation failed...\n");
        // TODO: Maintanance
//        exit(0);
    }
    else
        printf("Socket successfully created..\n");
        ;
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY); // the only diff line
    server.sin_port = htons(PORT);
    if ((bind(server_socket, (SA*)&server, sizeof(server))) != 0) {
        printf("Socket binding failed...\n");
        // TODO: Maintanance
//        exit(0);
    }
    else
        printf("Socket successfully bound..\n");
    
    if ((listen(server_socket, 5)) != 0) {
        printf("Listen failed...\n");
        // TODO: Maintanance
//        exit(0);
    }
    else
        printf("Server listening..\n");
    
    while (1) {
        socklen_t len = sizeof(client);
        client_socket = accept(server_socket, (SA*)&client, &len);
        if (client_socket < 0) {
            printf("Server accceptance failed...\n");
            // TODO: Maintanance
            //        exit(0);
        }
        else
            printf("Server acccepted the client..\n");
        
//        chat(client_socket, server_socket);
        
        char buffer[MAX];
        bzero(buffer, sizeof(buffer));
        recv(client_socket, buffer, sizeof(buffer), 0);
        process(buffer);
        send(client_socket, buffer, strlen(buffer)+1, 0);
        close(client_socket);
    }
    
    
//    shutdown(server_socket, SHUT_RDWR);
}

int main(int argc, const char * argv[]) {
    srand((unsigned int) time(NULL));
    
//    while (1) {
//        char str[10000];
//        gets(str);
//        process(str);
//        puts(str);
//    }
    
//    while (1) {
        connectServer();
//        sleep(500);
//    }
    
    return 0;
}
