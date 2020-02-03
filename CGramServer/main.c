//
//  main.c
//  CGramServer
//
//  Created by Seyyed Parsa Neshaei on 12/20/19.
//  Copyright © 2019 Seyyed Parsa Neshaei. All rights reserved.
//

#include <sys/ioctl.h>
#include <stdio.h>
#include <termios.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <time.h>
#include "cJSON.h"
#define MAX 10000
#define PORT 12345
#define SA struct sockaddr

typedef struct User {
    char username[1000];
    char token[20];
    char currentChannel[1000];
} User;

static struct termios term, oterm;

static int getch(void);

User allUsers[MAX];
int currentAllUsersIndex = 0;

char asciiArt[5][200];

char *replaceWord(const char *s, const char *oldW,
                  const char *newW) {
    char *result;
    int i, cnt = 0;
    int newWlen = strlen(newW);
    int oldWlen = strlen(oldW);
    
    // Counting the number of times old word
    // occur in the string
    for (i = 0; s[i] != '\0'; i++)
    {
        if (strstr(&s[i], oldW) == &s[i])
        {
            cnt++;
            
            // Jumping to index after the old word.
            i += oldWlen - 1;
        }
    }
    
    // Making new string of enough length
    result = (char *)malloc(i + cnt * (newWlen - oldWlen) + 1);
    
    i = 0;
    while (*s)
    {
        // compare the substring with the result
        if (strstr(s, oldW) == s)
        {
            strcpy(&result[i], newW);
            i += newWlen;
            s += oldWlen;
        }
        else
            result[i++] = *s++;
    }
    
    result[i] = '\0';
    return result;
}

void insertString(char* destination, int pos, char* stringToInsert) {
    char* strC = (char*)malloc(strlen(destination) + strlen(stringToInsert) + 1);
    strncpy(strC,destination,pos);
    strC[pos] = '\0';
    strcat(strC, stringToInsert);
    strcat(strC, destination + pos);
    strcpy(destination, strC);
    free(strC);
}

void occurences(char *str, char *toSearch, int *numberOfOccurences, int occurencesArr[500]) {
    if (strlen(str) < strlen(toSearch)) {
        return;
    }
    *numberOfOccurences = 0;
    unsigned long stringLen = strlen(str), searchLen = strlen(toSearch);
    for (int i = 0; i <= stringLen - searchLen; i++) {
        int found = 1;
        for (int j = 0; j < searchLen; j++) {
            if (str[i + j] != toSearch[j]) {
                found = 0;
                break;
            }
        }
        if (found) {
            occurencesArr[*numberOfOccurences] = i;
            (*numberOfOccurences)++;
        }
    }
}

void splitStringWithoutSpace(char str[1000000], char newString[1000][1000], int *countOfWords) {
    int j = 0, count = 0;
    for(int i = 0; i <= strlen(str); i++) {
        if (str[i] == '\0' || str[i] == ',' || str[i] == '\n') {
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

void splitStringByDoubleQuotes(char str[1000000], char newString[1000][1000], int *countOfWords) {
    int j = 0, count = 0;
    for(int i = 0; i <= strlen(str); i++) {
        if (str[i] == '\0' || str[i] == '\"' || str[i] == '\n') {
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

void findSubstring(char *destination, const char *source, int beg, int n)
{
    // extracts n characters from source string starting from beg index
    // and copy them into the destination string
    while (n > 0)
    {
        *destination = *(source + beg);
        
        destination++;
        source++;
        n--;
    }
    
    // null terminate destination string
    *destination = '\0';
}

int stringContainsWord(char *string, char *word) {
    int c = -1;
    char newString[1000][1000] = {};
    splitString(string, newString, &c);
    if (c < 0) {
        return 0;
    }
    for (int i = 0; i < c; i++) {
        if (strcmp(newString[i], word) == 0) {
            return 1;
        }
    }
    return 0;
}

int numberOfOccurencesOfWordInString(char *word, char *string) {
    int c = -1;
    int result = 0;
    char newString[1000][1000] = {};
    splitString(string, newString, &c);
    if (c < 0) {
        return 0;
    }
    for (int i = 0; i < c; i++) {
        if (strcmp(newString[i], word) == 0) {
            result += 1;
        }
    }
    return result;
}

void badRequest(char *result) {
    strcpy(result, "{\"type\":\"Error\",\"content\":\"Invalid request format.\"}");
}

int userHasGivenPasswordInJSON_cJSON(char *user, char *pass, cJSON *root) {
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

int userHasGivenPasswordInJSON(char *user, char *pass, char *root) {
    char toSearch[200] = {};
    strcpy(toSearch, "\"username\":\t\"");
    strcat(toSearch, user);
    strcat(toSearch, "\",");
    int numberOfOccurences = -1;
    int occurencesArr[1000] = {};
    occurences(root, toSearch, &numberOfOccurences, occurencesArr);
    if (numberOfOccurences < 1) {
        return 0;
    }
    unsigned long startPosition = occurencesArr[0] + 32 + strlen(user);
    const int rootPassSize = 500;
    char rootPass[rootPassSize] = {};
    int i = 0;
    while (1) {
        if (i >= rootPassSize) {
            return 0;
        }
        if (root[startPosition + i] == '\"' || root[startPosition + i] == '\"') {
            break;
        }
        rootPass[i] = root[startPosition + i];
        i++;
    }
    rootPass[i] = '\0';
    if (strcmp(rootPass, pass) == 0) {
        return 1;
    }
    return 0;
}


int memberExistsInJSON_cJSON(char *theMemberName, cJSON *root, int shouldCountMinusOnes) {
    cJSON *members = cJSON_GetObjectItemCaseSensitive(root, "members");
    cJSON *member = NULL;
    cJSON_ArrayForEach(member, members) {
        cJSON *memberName = cJSON_GetObjectItemCaseSensitive(member, "name");
        cJSON *hasSeen = cJSON_GetObjectItemCaseSensitive(member, "hasSeen");
        if (!memberName || !hasSeen) continue;
        if (cJSON_IsString(memberName)) {
            if (shouldCountMinusOnes) {
                if (memberName -> valuestring && strcmp(memberName -> valuestring, theMemberName) == 0) {
                    return 1;
                }
            } else {
                if (memberName -> valuestring && strcmp(memberName -> valuestring, theMemberName) == 0 && hasSeen->valueint != -1) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

// Assumes name of users and channels are different

int memberExistsInJSON(char *theMemberName, char *channelName, char *root, int shouldCountMinusOnes) {
    char string[MAX] = {};
    strcpy(string, root);
    char toSearch[200] = {};
    strcpy(toSearch, "\"name\":\t\"");
    strcat(toSearch, channelName);
    strcat(toSearch, "\",");
    int numberOfOccurences = -1;
    int occurencesArr[1000] = {};
    occurences(string, toSearch, &numberOfOccurences, occurencesArr);
    if (numberOfOccurences < 1) {
        return 0;
    }
    int cutPos = occurencesArr[0] + 5;
    char substring[1000] = {};
    findSubstring(substring, string, cutPos, (int)(strlen(string) - cutPos));
    strcpy(string, substring);
    strcpy(toSearch, "\"name\":\t\"");
    strcat(toSearch, theMemberName);
    occurences(string, toSearch, &numberOfOccurences, occurencesArr);
    if (numberOfOccurences < 1) {
        return 0;
    }
    int memberNamePos = occurencesArr[0];
    strcpy(toSearch, "\"hasSeen\": -1");
    occurences(string, toSearch, &numberOfOccurences, occurencesArr);
    int hasSeenMinus = occurencesArr[0];
    strcpy(toSearch, "\"messages\":");
    occurences(string, toSearch, &numberOfOccurences, occurencesArr);
    int messagesPos = occurencesArr[0];
    if (shouldCountMinusOnes) {
        if (memberNamePos < messagesPos) {
            return 1;
        } else {
            return 0;
        }
    } else {
        if (memberNamePos < hasSeenMinus && hasSeenMinus < messagesPos) {
            return 0;
        } else if (memberNamePos < messagesPos) {
            return 1;
        } else {
            return 0;
        }
    }
}

int channelExistsInJSON_cJSON(char *channelName, cJSON *root) {
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

int channelExistsInJSON(char *channelName, char *root) {
    char string[MAX] = {};
    strcpy(string, root);
    char toSearch[200] = {};
    strcpy(toSearch, "\"name\":\t\"");
    strcat(toSearch, channelName);
    strcat(toSearch, "\",");
    int numberOfOccurences = -1;
    int occurencesArr[1000] = {};
    occurences(string, toSearch, &numberOfOccurences, occurencesArr);
    if (numberOfOccurences < 1) {
        return 0;
    }
    return 1;
}



int userExistsInJSON_cJSON(char *user, cJSON *root) {
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

int userExistsInJSON(char *user, char *root) {
    char toSearch[200] = {};
    strcpy(toSearch, "\"username\":\t\"");
    strcat(toSearch, user);
    strcat(toSearch, "\",");
    int numberOfOccurences = -1;
    int occurencesArr[1000] = {};
    occurences(root, toSearch, &numberOfOccurences, occurencesArr);
    if (numberOfOccurences < 1) {
        return 0;
    }
    return 1;
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
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Username is not valid.\"}");
        return;
    }
//    cJSON *root = cJSON_Parse(data);
//    if (root == NULL) {
//        strcpy(result, "{\"type\":\"Error\",\"content\":\"Username is not valid.\"}");
//        return;
//    }
    if (!userExistsInJSON(username, data)) {
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Username is not valid.\"}");
        return;
    }
    if (!userHasGivenPasswordInJSON(username, password, data)) {
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
        strcat(data, "{\n\t\"users\":\t[{\n\t\t\t\"username\":\t\"");
        strcat(data, username);
        strcat(data, "\",\n\t\t\t\"password\":\t\"");
        strcat(data, password);
        strcat(data, "\"\n\t\t}]\n}");
        writeToFile(fileName, data);
        strcpy(result, "{\"type\":\"Successful\",\"content\":\"\"}");
        return;
    }
//    cJSON *root = cJSON_Parse(data);
//    if (root == NULL) {
//        strcat(data, "{\"users\": [{ \"username\": \"");
//        strcat(data, username);
//        strcat(data, "\", \"password\": \"");
//        strcat(data, password);
//        strcat(data, "\"}]}");
//        writeToFile(fileName, data);
//        strcpy(result, "{\"type\":\"Successful\",\"content\":\"\"}");
//        return;
//    }
    if (!userExistsInJSON(username, data)) {
        // add in 13
        char x[500] = {};
        strcpy(x, "{\n\t\t\t\"username\":\t\"");
        strcat(x, username);
        strcat(x, "\",\n\t\t\t\"password\":\t\"");
        strcat(x, password);
        strcat(x, "\"\n\t\t}, ");
        insertString(data, 13, x);
//        cJSON *users = cJSON_GetObjectItemCaseSensitive(root, "users");
//        cJSON *obj = cJSON_CreateObject();
//        cJSON_AddStringToObject(obj, "username", username);
//        cJSON_AddStringToObject(obj, "password", password);
//        cJSON_AddItemToArray(users, obj);
//        strcpy(data, cJSON_Print(root));
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
        strcat(data, "{\n\t\"channels\":\t[{\n\t\t\t\"name\":\t\"");
        strcat(data, channelName);
        strcat(data, "\",\n\t\t\t\"members\":\t[{\n\t\t\t\t\t\"name\":\t\"");
        strcat(data, allUsers[id].username);
        strcat(data, "\",\n\t\t\t\t\t\"hasSeen\":\t0\n\t\t\t\t}],\n\t\t\t\"messages\":\t[]\n\t\t}]\n}");
        if (id == -1) {
            // TODO: Error message wrong
            strcpy(result, "{\"type\":\"Error\",\"content\":\"Channel name is not available.\"}");
            return;
        }
//        if (strcmp(allUsers[id].currentChannel, "") != 0) {
//            strcpy(result, "{\"type\":\"Error\",\"content\":\"You are in another channel.\"}");
//            return;
//        }
        strcpy(allUsers[id].currentChannel, channelName);
        writeToFile(fileName, data);
        strcpy(result, "{\"type\":\"Successful\",\"content\":\"\"}");
        return;
    }
//    printf("DATA::: %s\n", data);
////    cJSON *root = cJSON_Parse(data);
//    if (root == NULL) {
//        strcat(data, "{\"channels\": [{ \"name\": \"");
//        strcat(data, channelName);
//        strcat(data, "\", \"members\": [{\"name\":\"");
//        strcat(data, allUsers[id].username);
//        strcat(data, "\", \"hasSeen\": 0}], \"messages\": []}]}");
//        if (id == -1) {
//            // TODO: Error message wrong
//            strcpy(result, "{\"type\":\"Error\",\"content\":\"Channel name is not available.\"}");
//            return;
//        }
////        if (strcmp(allUsers[id].currentChannel, "") != 0) {
////            strcpy(result, "{\"type\":\"Error\",\"content\":\"You are in another channel.\"}");
////            return;
////        }
//        strcpy(allUsers[id].currentChannel, channelName);
//        writeToFile(fileName, data);
//        strcpy(result, "{\"type\":\"Successful\",\"content\":\"\"}");
//        return;
//    }
    if (channelExistsInJSON(channelName, data)) {
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Channel name is not available.\"}");
        return;
    }
    if (id == -1) {
        // TODO: Error message wrong
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Channel name is not available.\"}");
        return;
    }
//    if (strcmp(allUsers[id].currentChannel, "") != 0) {
//        strcpy(result, "{\"type\":\"Error\",\"content\":\"You are in another channel.\"}");
//        return;
//    }
    
    strcpy(allUsers[id].currentChannel, channelName);
    char x[500] = {};
    strcpy(x, "{\n\t\t\t\"name\":\t\"");
    strcat(x, channelName);
    strcat(x, "\",\n\t\t\t\"members\":\t[");
    strcat(x, "{\n\t\t\t\t\t\"name\":\t\"");
    strcat(x, allUsers[id].username);
    strcat(x, "\",\n\t\t\t\t\t\"hasSeen\":\t-1");
    strcat(x, "\n\t\t\t\t}],\n\t\t\t\"messages\":\t[]\n\t\t}, ");
    insertString(data, 16, x);
    
    
//    strcpy(allUsers[id].currentChannel, channelName);
//    cJSON *channels = cJSON_GetObjectItemCaseSensitive(root, "channels");
//    cJSON *channel = cJSON_CreateObject();
//    cJSON *members = cJSON_CreateArray();
//    cJSON *member = cJSON_CreateObject();
//    cJSON_AddStringToObject(member, "name", allUsers[id].username);
//    cJSON_AddNumberToObject(member, "hasSeen", 0);
//    cJSON *messages = cJSON_CreateArray();
//    cJSON *name = cJSON_CreateString(channelName);
//    cJSON_AddItemToArray(members, member);
//    cJSON_AddItemToObject(channel, "name", name);
//    cJSON_AddItemToObject(channel, "members", members);
//    cJSON_AddItemToObject(channel, "messages", messages);
//    cJSON_AddItemToArray(channels, channel);
//    strcpy(data, cJSON_Print(root));
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
//    if (strcmp(allUsers[id].currentChannel, "") == 0) {
//        strcpy(result, "{\"type\":\"Error\",\"content\":\"You aren\'t in any channel\"}");
//        return;
//    }
    char data[MAX] = {}, fileName[] = "channels.txt";
    readFromFile(fileName, data);
//    cJSON *root = cJSON_Parse(data);
//    if (root == NULL) {
//        // TODO: Error message wrong
//        strcpy(result, "{\"type\":\"Error\",\"content\":\"Error\"}");
//        return;
//    }
    char channelName[1000] = {};
    strcpy(channelName, allUsers[id].currentChannel);
    char mainString[500] = {};
    strcat(mainString, "\"name\":\t\"");
    strcat(mainString, allUsers[id].username);
    strcat(mainString, "\",\n\t\t\t\t\t\"hasSeen\":\t0");
    char newString[500] = {};
    strcat(newString, "\"name\":\t\"");
    strcat(newString, allUsers[id].username);
    strcat(newString, "\",\n\t\t\t\t\t\"hasSeen\":\t-1");
    replaceWord(data, mainString, newString);
    writeToFile(fileName, data);
    strcpy(allUsers[id].currentChannel, "");
    strcpy(result, "{\"type\":\"Successful\",\"content\":\"\"}");
    
//    cJSON *channels = cJSON_GetObjectItemCaseSensitive(root, "channels");
//    cJSON *channel = NULL;
//    cJSON_ArrayForEach(channel, channels) {
//        cJSON *name = cJSON_GetObjectItemCaseSensitive(channel, "name");
//        if (name->valuestring && (strcmp(name->valuestring, channelName) == 0)) {
//            cJSON *members = cJSON_GetObjectItemCaseSensitive(channel, "members");
//            cJSON *memberr = NULL;
//            cJSON_ArrayForEach(memberr, members) {
//                cJSON *member = cJSON_GetObjectItemCaseSensitive(memberr, "name");
//                if (!member) continue;
//                if (member->valuestring && strcmp(member->valuestring, allUsers[id].username) == 0) {
//                    cJSON *hasSeen = cJSON_GetObjectItemCaseSensitive(memberr, "hasSeen");
//                    cJSON_SetIntValue(hasSeen, -1);
//                    strcpy(data, cJSON_Print(root));
//                    writeToFile(fileName, data);
//                    strcpy(allUsers[id].currentChannel, "");
//                    strcpy(result, "{\"type\":\"Successful\",\"content\":\"\"}");
//                    return;
//                }
//            }
//            break;
//        }
//    }
    // TODO: Error message wrong
//    strcpy(result, "{\"type\":\"Error\",\"content\":\"Error\"}");
}

void doJoinChannel(char *channelName, char *token, char *result) {
    int id = userIDHavingGivenToken(token);
    if (id == -1) {
        // TODO: Error message wrong
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Error\"}");
        return;
    }
//    if (strcmp(allUsers[id].currentChannel, "") != 0) {
//        strcpy(result, "{\"type\":\"Error\",\"content\":\"You are in another channel.\"}");
//        return;
//    }
    char data[MAX] = {}, fileName[] = "channels.txt";
    readFromFile(fileName, data);
    
    char stringToSearchFor[MAX] = {};
    strcpy(stringToSearchFor, "\"name\":\t\"");
    strcat(stringToSearchFor, channelName);
    strcat(stringToSearchFor, "\",\n\t\t\t\"members\":\t[");
    int numberOfOccurences = -1;
    int occurencesArr[100] = {};
    occurences(data, stringToSearchFor, &numberOfOccurences, occurencesArr);
    if (numberOfOccurences < 1) {
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Channel not found.\"}");
        return;
    }
    strcpy(allUsers[id].currentChannel, channelName);
    unsigned long firstPos = occurencesArr[0] + strlen(stringToSearchFor);
    char stringToInsert[500];
    strcpy(stringToInsert, "{\n\t\t\t\t\t\"name\":\t\"");
    strcat(stringToInsert, allUsers[id].username);
    strcat(stringToInsert, "\",\n\t\t\t\t\t\"hasSeen\":\t0\n\t\t\t\t},");
    insertString(data, (int)firstPos, stringToInsert);
    writeToFile(fileName, data);
    
    strcpy(result, "{\"type\":\"Successful\",\"content\":\"\"}");
    
    
//    cJSON *root = cJSON_Parse(data);
//    if (root == NULL) {
//        // TODO: Error message wrong
//        strcpy(result, "{\"type\":\"Error\",\"content\":\"Error\"}");
//        return;
//    }
//    strcpy(allUsers[id].currentChannel, channelName);
//    cJSON *channels = cJSON_GetObjectItemCaseSensitive(root, "channels");
//    cJSON *channel = NULL;
//    cJSON_ArrayForEach(channel, channels) {
//        cJSON *name = cJSON_GetObjectItemCaseSensitive(channel, "name");
//        if (name->valuestring && (strcmp(name->valuestring, channelName) == 0)) {
//            if (!memberExistsInJSON(allUsers[id].username, channelName, data, 1)) {
//                cJSON *members = cJSON_GetObjectItemCaseSensitive(channel, "members");
////                cJSON *member = cJSON_CreateString(allUsers[id].username);
//                cJSON *member = cJSON_CreateObject();
//                cJSON_AddStringToObject(member, "name", allUsers[id].username);
//                cJSON_AddNumberToObject(member, "hasSeen", 0);
//                cJSON_AddItemToArray(members, member);
//                strcpy(allUsers[id].currentChannel, channelName);
//                strcpy(data, cJSON_Print(root));
//                writeToFile(fileName, data);
//                strcpy(result, "{\"type\":\"Successful\",\"content\":\"\"}");
//                return;
//            } else {
//                cJSON *members = cJSON_GetObjectItemCaseSensitive(channel, "members");
//                cJSON *member = NULL;
//                cJSON_ArrayForEach(member, members) {
//                    cJSON *memberName = cJSON_GetObjectItemCaseSensitive(member, "name");
//                    if (memberName && cJSON_IsString(memberName) && (strcmp(memberName->valuestring, allUsers[id].username) == 0)) {
//                        cJSON *memberHasSeen = cJSON_GetObjectItemCaseSensitive(member, "hasSeen");
//                        if (memberHasSeen) {
//                            cJSON_SetIntValue(memberHasSeen, 0);
//                            strcpy(result, "{\"type\":\"Successful\",\"content\":\"\"}");
//                            return;
//                        }
//                    }
//                }
//            }
//            strcpy(result, "{\"type\":\"Successful\",\"content\":\"\"}");
//            return;
//        }
//    }
//    strcpy(result, "{\"type\":\"Error\",\"content\":\"Channel not found.\"}");
}



int hasSeenOfMemberGiven(char *theMemberName, cJSON *members) {
    cJSON *member = NULL;
    cJSON_ArrayForEach(member, members) {
        cJSON *memberName = cJSON_GetObjectItemCaseSensitive(member, "name");
        if (!memberName) {
            continue;
        }
        if (memberName->valuestring && (strcmp(memberName->valuestring, theMemberName) == 0)) {
            cJSON *memberHasSeen = cJSON_GetObjectItemCaseSensitive(member, "hasSeen");
            if (memberHasSeen && cJSON_IsNumber(memberHasSeen)) {
                return memberHasSeen->valueint;
            }
        }
    }
    return 0;
}

void setHasSeenOfMemberGiven(int num, char *theMemberName, cJSON *members) {
    cJSON *member = NULL;
    cJSON_ArrayForEach(member, members) {
        cJSON *memberName = cJSON_GetObjectItemCaseSensitive(member, "name");
        if (!memberName) {
            continue;
        }
        if (memberName->valuestring && (strcmp(memberName->valuestring, theMemberName) == 0)) {
            cJSON *memberHasSeen = cJSON_GetObjectItemCaseSensitive(member, "hasSeen");
            if (memberHasSeen && cJSON_IsNumber(memberHasSeen)) {
                cJSON_SetIntValue(memberHasSeen, num);
            }
        }
    }
}

void doSearchBetweenMessages(char *textToSearch, char *token, char *result) {
    int id = userIDHavingGivenToken(token);
    if (id == -1) {
        // TODO: Error message wrong
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Error\"}");
        return;
    }
    //    if (strcmp(allUsers[id].currentChannel, "") == 0) {
    //        strcpy(result, "{\"type\":\"Error\",\"content\":\"You aren\'t in any channel\"}");
    //        return;
    //    }
    char data[MAX] = {}, fileName[] = "channels.txt";
    readFromFile(fileName, data);
    
    int numberOfOccurences = -1;
    int occurencesArr[500] = {};
    char toSearch[200] = {};
    strcpy(toSearch, "\"name\":\t\"");
    strcat(toSearch, allUsers[id].currentChannel);
    strcat(toSearch, "\",");
    occurences(data, toSearch, &numberOfOccurences, occurencesArr);
    if (numberOfOccurences < 1) {
        // TODO: Error message wrong
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Error\"}");
        return;
    }
    int outputOfStageOne = occurencesArr[0];
    strcpy(toSearch, "\"messages\"");
    occurences(data, toSearch, &numberOfOccurences, occurencesArr);
    if (numberOfOccurences < 1) {
        // TODO: Error message wrong
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Error\"}");
        return;
    }
    int outputOfStageTwo = -1;
    for (int i = 0; i < numberOfOccurences; i++) {
        if (occurencesArr[i] > outputOfStageOne) {
            outputOfStageTwo = occurencesArr[i];
            break;
        }
    }
    if (outputOfStageTwo == -1) {
        // TODO: Error message wrong
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Error\"}");
        return;
    }
    strcpy(toSearch, "]");
    occurences(data, toSearch, &numberOfOccurences, occurencesArr);
    int outputOfStageThree = -1;
    for (int i = 0; i < numberOfOccurences; i++) {
        if (occurencesArr[i] > outputOfStageTwo) {
            outputOfStageThree = occurencesArr[i];
            break;
        }
    }
    
    char stringToBeInserted[MAX] = {};
    findSubstring(stringToBeInserted, data, outputOfStageTwo + 12, outputOfStageThree - (outputOfStageTwo + 12) + 1);
    replaceWord(stringToBeInserted, "\n", "");
    // stringToBeInserted is the whole content that should be parsed
    if (strlen(stringToBeInserted) < 4) {
        strcpy(result, "{\"type\":\"List\",\"content\":[]}");
        return;
    }
    char line[MAX] = {};
    strcpy(line, stringToBeInserted);
    strcpy(result, "{\"type\":\"List\",\"content\":[");
    char subStringParts[1000][1000] = {};
    int countOfWords = -1;
    splitStringByDoubleQuotes(line, subStringParts, &countOfWords);
    if (countOfWords < 1) {
        // TODO: Error message wrong
        strcat(result, "]}");
        return;
    }
    for (int i = 0; i < countOfWords; i += 4) {
        if (i != 0) {
            strcat(result, ",");
        }
        strcat(result, "{\"sender\":\"");
        strcat(result, subStringParts[i + 1]);
        strcat(result, "\",\"content\":\"");
        strcat(result, subStringParts[i + 3]);
        strcat(result, "\"}");
    }
}

int tMC = 0;

void doRefresh(char *token, char *result) {
    int id = userIDHavingGivenToken(token);
    if (id == -1) {
        // TODO: Error message wrong
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Error\"}");
        return;
    }
    //    if (strcmp(allUsers[id].currentChannel, "") == 0) {
    //        strcpy(result, "{\"type\":\"Error\",\"content\":\"You aren\'t in any channel\"}");
    //        return;
    //    }
    char data[MAX] = {}, fileName[] = "channels.txt";
    readFromFile(fileName, data);
    
    int numberOfOccurences = -1;
    int occurencesArr[500] = {};
    char toSearch[200] = {};
    strcpy(toSearch, "\"name\":\t\"");
    strcat(toSearch, allUsers[id].currentChannel);
    strcat(toSearch, "\",");
    occurences(data, toSearch, &numberOfOccurences, occurencesArr);
    if (numberOfOccurences < 1) {
        // TODO: Error message wrong
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Error\"}");
        return;
    }
    int outputOfStageOne = occurencesArr[0];
    strcpy(toSearch, "\"messages\"");
    occurences(data, toSearch, &numberOfOccurences, occurencesArr);
    if (numberOfOccurences < 1) {
        // TODO: Error message wrong
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Error\"}");
        return;
    }
    int outputOfStageTwo = -1;
    for (int i = 0; i < numberOfOccurences; i++) {
        if (occurencesArr[i] > outputOfStageOne) {
            outputOfStageTwo = occurencesArr[i];
            break;
        }
    }
    if (outputOfStageTwo == -1) {
        // TODO: Error message wrong
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Error\"}");
        return;
    }
    strcpy(toSearch, "]");
    occurences(data, toSearch, &numberOfOccurences, occurencesArr);
    int outputOfStageThree = -1;
    for (int i = 0; i < numberOfOccurences; i++) {
        if (occurencesArr[i] > outputOfStageTwo) {
            outputOfStageThree = occurencesArr[i];
            break;
        }
    }
    
    char stringToBeInserted[MAX] = {};
    findSubstring(stringToBeInserted, data, outputOfStageTwo + 12, outputOfStageThree - (outputOfStageTwo + 12) + 1);
    replaceWord(stringToBeInserted, "\n", "");
    strcpy(result, "{\"type\":\"List\",\"content\":");
    strcat(result, stringToBeInserted);
    strcat(result, "}");
}

void doRefresh_cJSON(char *token, char *result) {
    int id = userIDHavingGivenToken(token);
    if (id == -1) {
        // TODO: Error message wrong
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Error\"}");
        return;
    }
//    if (strcmp(allUsers[id].currentChannel, "") == 0) {
//        strcpy(result, "{\"type\":\"Error\",\"content\":\"You aren\'t in any channel\"}");
//        return;
//    }
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
        cJSON *membersJSON = cJSON_GetObjectItemCaseSensitive(channel, "members");
        if (!membersJSON) {
            continue;
        }
        if (name->valuestring && (strcmp(name->valuestring, channelName) == 0)) {
            strcpy(result, "{\"type\":\"List\",\"content\":[");
            int hasSeen = hasSeenOfMemberGiven(allUsers[id].username, membersJSON);
            cJSON *messages = cJSON_GetObjectItemCaseSensitive(channel, "messages");
            cJSON *message = NULL;
            int isFirstTime = 1, i = 0, totalMessagesCount = 0;
            cJSON_ArrayForEach(message, messages) {
                // TODO: Here
//                if (i < hasSeen) {
//                    i++;
//                    continue;
//                }
                totalMessagesCount++;
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
            if (tMC == 0) {
                tMC = totalMessagesCount;
            } else {
                tMC++;
            }
            strcat(result, "]}");
            setHasSeenOfMemberGiven(totalMessagesCount, allUsers[id].username, membersJSON);
            strcpy(data, cJSON_Print(root));
            writeToFile(fileName, data);
            return;
        }
    }
    strcpy(result, "{\"type\":\"Error\",\"content\":\"Channel not found.\"}");
}


void doSend_cJSON(char *originalMessage, char *token, char *result) {
    int id = userIDHavingGivenToken(token);
    if (id == -1) {
        // TODO: Error message wrong
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Error\"}");
        return;
    }
    //    if (strcmp(allUsers[id].currentChannel, "") == 0) {
    //        // TODO: Study \' or ' (also using replace and find in other places...)
    //        strcpy(result, "{\"type\":\"Error\",\"content\":\"You aren\'t in any channel\"}");
    //        return;
    //    }
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

void doSend(char *originalMessage, char *token, char *result) {
    int id = userIDHavingGivenToken(token);
    if (id == -1 || strcmp(originalMessage, "") == 0) {
        // TODO: Error message wrong
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Error\"}");
        return;
    }
    //    if (strcmp(allUsers[id].currentChannel, "") == 0) {
    //        // TODO: Study \' or ' (also using replace and find in other places...)
    //        strcpy(result, "{\"type\":\"Error\",\"content\":\"You aren\'t in any channel\"}");
    //        return;
    //    }
    char data[MAX] = {}, fileName[] = "channels.txt";
    readFromFile(fileName, data);
    
    int numberOfOccurences = -1;
    int occurencesArr[500] = {};
    char toSearch[200] = {};
    strcpy(toSearch, "\"name\":\t\"");
    strcat(toSearch, allUsers[id].currentChannel);
    strcat(toSearch, "\",");
    occurences(data, toSearch, &numberOfOccurences, occurencesArr);
    if (numberOfOccurences < 1) {
        // TODO: Error message wrong
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Error\"}");
        return;
    }
    int outputOfStageOne = occurencesArr[0];
    strcpy(toSearch, "\"messages\"");
    occurences(data, toSearch, &numberOfOccurences, occurencesArr);
    if (numberOfOccurences < 1) {
        // TODO: Error message wrong
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Error\"}");
        return;
    }
    int outputOfStageTwo = -1;
    for (int i = 0; i < numberOfOccurences; i++) {
        if (occurencesArr[i] > outputOfStageOne) {
            outputOfStageTwo = occurencesArr[i];
            break;
        }
    }
    if (outputOfStageTwo == -1) {
        // TODO: Error message wrong
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Error\"}");
        return;
    }
    strcpy(toSearch, "]");
    occurences(data, toSearch, &numberOfOccurences, occurencesArr);
    int outputOfStageThree = -1;
    for (int i = 0; i < numberOfOccurences; i++) {
        if (occurencesArr[i] > outputOfStageTwo) {
            outputOfStageThree = occurencesArr[i];
            break;
        }
    }
    
    if (outputOfStageThree - outputOfStageTwo <= 15) {
        char newMessage[500] = {};
        strcpy(newMessage, "{\n\t\t\t\t\t\"sender\":\t\"");
        strcat(newMessage, allUsers[id].username);
        strcat(newMessage, "\",\n\t\t\t\t\t\"content\":\t\"");
        strcat(newMessage, originalMessage);
        strcat(newMessage, "\"\n\t\t\t\t}");
        insertString(data, outputOfStageThree, newMessage);
    } else {
        char newMessage[500] = {};
        strcpy(newMessage, ",{\n\t\t\t\t\t\"sender\":\t\"");
        strcat(newMessage, allUsers[id].username);
        strcat(newMessage, "\",\n\t\t\t\t\t\"content\":\t\"");
        strcat(newMessage, originalMessage);
        strcat(newMessage, "\"\n\t\t\t\t}");
        insertString(data, outputOfStageThree, newMessage);
    }
    writeToFile(fileName, data);
    strcpy(result, "{\"type\":\"Successful\",\"content\":\"\"}");
}

void doSearchBetweenMembers(char *textToSearch, char *token, char *result) {
    int id = userIDHavingGivenToken(token);
    if (id == -1) {
        // TODO: Error message wrong
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Error\"}");
        return;
    }
    char data[MAX] = {}, fileName[] = "channels.txt";
    readFromFile(fileName, data);
    if (memberExistsInJSON(textToSearch, allUsers[id].currentChannel, data, 0)) {
        strcpy(result, "{\"type\":\"Successful\",\"content\":\"\"}");
    } else {
        // TODO: Error message wrong
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Error\"}");
    }
}

void doChannelMembers(char *token, char *result) {
    int id = userIDHavingGivenToken(token);
    if (id == -1) {
        // TODO: Error message wrong
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Error\"}");
        return;
    }
    //    if (strcmp(allUsers[id].currentChannel, "") == 0) {
    //        strcpy(result, "{\"type\":\"Error\",\"content\":\"You aren\'t in any channel\"}");
    //        return;
    //    }
    char data[MAX] = {}, fileName[] = "channels.txt";
    readFromFile(fileName, data);
    char toSearch[500] = {};
    strcpy(toSearch, "\"name\":\t\"");
    strcat(toSearch, allUsers[id].currentChannel);
    strcat(token, "\"");
    int numberOfOccurences = -1;
    int occurencesArr[500] = {};
    occurences(data, toSearch, &numberOfOccurences, occurencesArr);
    if (numberOfOccurences < 1) {
        // TODO: Error message wrong
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Error\"}");
        return;
    }
    int fPO = occurencesArr[0];
    strcpy(toSearch, "\"members\"");
    occurences(data, toSearch, &numberOfOccurences, occurencesArr);
    if (numberOfOccurences < 1) {
        // TODO: Error message wrong
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Error\"}");
        return;
    }
    int x = -1;
    for (int i = 0; i < numberOfOccurences; i++) {
        if (occurencesArr[i] > fPO) {
            x = occurencesArr[i];
            break;
        }
    }
    if (x < 1) {
        // TODO: Error message wrong
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Error\"}");
        return;
    }
    
    strcpy(toSearch, "]");
    occurences(data, toSearch, &numberOfOccurences, occurencesArr);
    if (numberOfOccurences < 1) {
        // TODO: Error message wrong
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Error\"}");
        return;
    }
    int y = -1;
    for (int i = 0; i < numberOfOccurences; i++) {
        if (occurencesArr[i] > x) {
            y = occurencesArr[i];
            break;
        }
    }
    if (y < 1) {
        // TODO: Error message wrong
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Error\"}");
        return;
    }
    
    char a[500] = {};
    findSubstring(a, data, x, (y - x) + 1);
    strcpy(toSearch, "\"name\":\t");
    occurences(a, toSearch, &numberOfOccurences, occurencesArr);
    int hasSeenOccurencesArr[500] = {};
    strcpy(toSearch, "\"hasSeen\":\t");
    occurences(a, toSearch, &numberOfOccurences, hasSeenOccurencesArr);
    char members[500][100];
    int countOfMembers = 0;
    for (int i = 0; i < numberOfOccurences; i++) {
        if (a[hasSeenOccurencesArr[i] + 12] != '-') {
            char name[100] = {};
            int c = 0;
            while (1) {
                if (a[occurencesArr[i] + 9 + c] == '\"' || a[occurencesArr[i] + 9 + c] == '\0') {
                    break;
                }
                name[c] = a[occurencesArr[i] + 9 + c];
                c++;
            }
            strcpy(members[i], name);
            countOfMembers++;
        }
    }
    
    strcpy(result, "{\"type\":\"List\",\"content\":[");
    for (int i = 0; i < countOfMembers; i++) {
        if (i != 0) {
            strcat(result, ", ");
        }
        strcat(result, "\"");
        strcat(result, members[i]);
        strcat(result, "\"");
    }
    strcat(result, "]}");
}

void doChannelMembers_cJSON(char *token, char *result) {
    int id = userIDHavingGivenToken(token);
    if (id == -1) {
        // TODO: Error message wrong
        strcpy(result, "{\"type\":\"Error\",\"content\":\"Error\"}");
        return;
    }
//    if (strcmp(allUsers[id].currentChannel, "") == 0) {
//        strcpy(result, "{\"type\":\"Error\",\"content\":\"You aren\'t in any channel\"}");
//        return;
//    }
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
                cJSON *memberN = cJSON_GetObjectItemCaseSensitive(member, "name");
                cJSON *hasSeenN = cJSON_GetObjectItemCaseSensitive(member, "hasSeen");
                if (!memberN || !hasSeenN || (hasSeenN->valueint == -1)) {
                    continue;
                }
                if (isFirstTime) {
                    isFirstTime = 0;
                } else {
                    strcat(result, ",");
                }
                strcat(result, "\"");
                strcat(result, memberN->valuestring);
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
        printf("Server received a '%s' type of request from the client\n", firstPart);
    }
    
    
    if (strcmp(firstPart, "login") == 0) {
        doLogin(secondPart, parts[3], request);
    } else if (strcmp(firstPart, "register") == 0) {
        doRegister(secondPart, parts[3], request);
    } else if (strcmp(firstPart, "logout") == 0) {
        doLogout(secondPart, request);
    } else if (strcmp(firstPart, "create") == 0 && strcmp(secondPart, "channel") == 0) {
        doCreateChannel(parts[2], parts[4], request);
    } else if (strcmp(firstPart, "searchBetweenMembers") == 0) {
        doSearchBetweenMembers(secondPart, parts[3], request);
    } else if (strcmp(firstPart, "searchBetweenMessages") == 0) {
        doSearchBetweenMessages(secondPart, parts[3], request);
    } else if (strcmp(firstPart, "leave") == 0) {
        doLeave(secondPart, request);
    } else if (strcmp(firstPart, "join") == 0 && strcmp(secondPart, "channel") == 0) {
        doJoinChannel(parts[2], parts[4], request);
    } else if (strcmp(firstPart, "refresh") == 0) {
        doRefresh(secondPart, request);
    } else if (strcmp(firstPart, "send") == 0) {
//        doSend(secondPart, parts[3], request);
        char newParts[1000][1000] = {};
        int dummy = 0;
        splitStringWithoutSpace(request, newParts, &dummy);
        char ourStrWithSendWord[1000] = {};
        strcpy(ourStrWithSendWord, newParts[0]);
        unsigned long len = strlen(ourStrWithSendWord);
        for (int i = 0; i < len; i++) {
            ourStrWithSendWord[i] = ourStrWithSendWord[i + 5];
        }
        doSend(ourStrWithSendWord, parts[countOfParts - 2], request);
    } else if (strcmp(firstPart, "channel") == 0 && strcmp(secondPart, "members") == 0) {
        doChannelMembers(parts[2], request);
    } else {
        badRequest(request);
    }
    
    printf("Server sent the result '%s' to the client\n", request);
    
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
        printf("Server socket successfully created\n");
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
        printf("Socket successfully bound\n");
    
    if ((listen(server_socket, 5)) != 0) {
        printf("Listen failed...\n");
        // TODO: Maintanance
//        exit(0);
    }
    else
        printf("Server listening to client requests on port %d\n", PORT);
    
    while (1) {
        socklen_t len = sizeof(client);
        client_socket = accept(server_socket, (SA*)&client, &len);
        if (client_socket < 0) {
            printf("Server accceptance failed or breakpoint set\n");
            // TODO: Maintanance
            //        exit(0);
        }
        else
            printf("Server acccepted a request from the client\n");
        
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

void initializeAsciiArt() {
    strcpy(asciiArt[0], " ______     ______     ______     ______     __    __    \n");
    strcpy(asciiArt[1], "/\\  ___\\   /\\  ___\\   /\\  == \\   /\\  __ \\   /\\ \"-./  \\   \n");
    strcpy(asciiArt[2], "\\ \\ \\____  \\ \\ \\__ \\  \\ \\  __<   \\ \\  __ \\  \\ \\ \\-./\\ \\  \n");
    strcpy(asciiArt[3], " \\ \\_____\\  \\ \\_____\\  \\ \\_\\ \\_\\  \\ \\_\\ \\_\\  \\ \\_\\ \\ \\_\\ \n");
    strcpy(asciiArt[4], "  \\/_____/   \\/_____/   \\/_/ /_/   \\/_/\\/_/   \\/_/  \\/_/ \n");
}

enum Colors { RED = 35, GREEN = 32, YELLOW = 33, BLUE = 34, CYAN = 36 } appColor = RED;

void makeBoldColor() {
    char x[100] = {'\0'};
    sprintf(x, "\033[1;%dm", appColor);
    printf("%s", x);
}

void makeColor() {
    char x[100] = {'\0'};
    sprintf(x, "\033[0;%dm", appColor);
    printf("%s", x);
}

void resetFont() {
    printf("\x1b[0m");
}

static int getch() {
    int c = 0;
    
    tcgetattr(0, &oterm);
    memcpy(&term, &oterm, sizeof(term));
    term.c_lflag &= ~(ICANON | ECHO);
    term.c_cc[VMIN] = 1;
    term.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &term);
    c = getchar();
    tcsetattr(0, TCSANOW, &oterm);
    if (c == 27) {
        resetFont();
        exit(0);
    }
    return c;
}

int terminalColumns = 80, terminalLines = 24;

void setupTerminalDimensions() {
#ifdef TIOCGSIZE
    struct ttysize ts;
    ioctl(STDIN_FILENO, TIOCGSIZE, &ts);
    terminalColumns = ts.ts_cols;
    terminalLines = ts.ts_lines;
#elif defined(TIOCGWINSZ)
    struct winsize ts;
    ioctl(STDIN_FILENO, TIOCGWINSZ, &ts);
    terminalColumns = ts.ws_col;
    terminalLines = ts.ws_row;
#endif
}

void printStringCentered(char *str) {
    int long len = strlen(str);//unicodeStrlen(str);
    int width = (int)(len + (terminalColumns - len) / 2);
    printf("%*s", width, str);
}

int main(int argc, const char * argv[]) {
    srand((unsigned int) time(NULL));
    initializeAsciiArt();
    switch (argc) {
        case 0:
        case 1:
            system("clear");
            printf("\n");
            makeBoldColor();
            for (int i = 0; i < 5; i++) {
                printf("%s", asciiArt[i]);
            }
            printf("\n\n\n");
            printStringCentered("CGram Server v1.0");
            printf("\n\n\n");
            printStringCentered("This server is designed to work alongside CGram v2.0 on macOS systems.");
            printf("\n\n");
            printStringCentered("To reset all of the users and channels\' data on the server,");
            printf("\n");
            printStringCentered("simply pass \'--reset\' while opening CGram Server using Terminal.");
            printf("\n\n\n");
            resetFont();
            connectServer();
            break;
            
        case 2:
            if (strcmp(argv[1], "--reset") == 0) {
                makeBoldColor();
                printf("\n\n");
                printStringCentered("Are you sure you want to reset all the data on the server?");
                printf("\n");
                printStringCentered("If yes, press 'y', and if not, press 'n'.\n\n");
                resetFont();
                while (1) {
                    char c = getch();
                    switch (c) {
                        case 'y':
                        case 'Y':
                            remove("channels.txt");
                            remove("users.txt");
                            printf("\n\n");
                            makeBoldColor();
                            printStringCentered("Reset all data done. Press any key to return.");
                            printf("\n\n\n");
                            resetFont();
                            getch();
                            return 0;
                            break;
                        case 'n':
                        case 'N':
                            printf("\n");
                            return 0;
                            break;
                        default:
                            break;
                    }
                }
            }
            break;
        default:
            break;
    }
    
//    while (1) {
//        char str[10000];
//        gets(str);
//        process(str);
//        puts(str);
//    }
    
//    while (1) {
    
//        sleep(500);
//    }
    
    return 0;
}
