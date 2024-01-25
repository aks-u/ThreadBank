#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#include "header.h"

extern pthread_mutex_t mutex;

// Helper function to get the balance of an account
int getBalance(char* filePath) {
    int balance;
    pthread_mutex_lock(&mutex);
    FILE *file = fopen(filePath, "r");
    if (file != NULL) {
        fscanf(file, "%d", &balance);
        fclose(file);
        pthread_mutex_unlock(&mutex);
        return balance;
    } else {
        pthread_mutex_unlock(&mutex);
        return -1;      // Return -1 if the file couldn't be opened
    }
}

// Helper function to deposit money into an account
int deposit(char* filePath, int amount) {
    int balance = getBalance(filePath);
    if (balance == -1 || amount < 0) {
        return -1;      // Return -1 if getBalance failed to open the file, or the amount is negative
    }

    pthread_mutex_lock(&mutex);
    FILE *file = fopen(filePath, "w");
    if (file != NULL) {
        balance += amount;
        fprintf(file, "%d", balance);
        fclose(file);
        pthread_mutex_unlock(&mutex);
        return balance;         // Return the updated balance after deposit
    } else {
        pthread_mutex_unlock(&mutex);
        return -1;          // Return -1 if the file couldn't be opened
    }
}

// Helper function to withdraw money from an account
int withdraw(char* filePath, int amount) {
    int balance = getBalance(filePath);
    if (balance == -1 || amount > balance || amount < 0) {
        return -1;      // Return -1 if getBalance failed to open the file, or withdrawal amount is greater than balance, or the amount is negative
    }
    pthread_mutex_lock(&mutex);
    FILE *file = fopen(filePath, "w");
    if (file != NULL) {
        balance -= amount;
        fprintf(file, "%d", balance);
        fclose(file);
        pthread_mutex_unlock(&mutex);
        return balance;     // Return the updated balance after withdrawal
    } else {
        pthread_mutex_unlock(&mutex);
        return -1;      // Return -1 if the file couldn't be opened
    }
}

// Function to handle commands received from a client
int handleCommand(char* buf, int clientFD) {
    int accno1, accno2, amount, balance;
    char response[50];
    char filePath[50];
    char filePath2[50];

    // Process the command. First letter determines the command
    switch (buf[0]) {
    // Quit
    case 'q': 
        strcpy(response, "ok: Quit");
        break;


    // List account value
    case 'l': 
        if (sscanf(buf,"l %d", &accno1) == 1) {
            snprintf(filePath, sizeof(filePath), "./accounts/%d.txt", accno1);

            // Check if file exists
            if (access(filePath, F_OK) != -1) {
                // File exists
                balance = getBalance(filePath);
                if (balance == -1) {
                    sprintf(response, "%s", "fail: Couldn't list account value");
                }

            } else {
                // File doesn't exist, create it with balance set to 0
                pthread_mutex_lock(&mutex);
                FILE *file = fopen(filePath, "w");
                if (file != NULL) {
                    balance = 0;
                    fprintf(file, "%d", balance);
                    fclose(file);
                } else {
                    sprintf(response, "%s", "fail: Couldn't list account value");
                }
                pthread_mutex_unlock(&mutex);
            }
            sprintf(response, "%s %d", "ok: Account value is", balance);
        } else {
            sprintf(response, "%s", "fail: Couldn't list account value");
        }
        break;


    // Withdraw money from an account
    case 'w': 
        if (sscanf(buf,"w %d %d", &accno1, &amount) == 2) {
            if (withdraw(filePath, amount) == -1) {
                sprintf(response, "%s", "fail: Unable to withdraw money");
                break;
            }
            sprintf(response, "%s %d %s %d", "ok: Withdrew", amount, "from account", accno1);
        } else {
            sprintf(response, "%s", "fail: Unable to withdraw money");
        }
        break;


    // Transfer money from one account to another
    case 't': 
        if (sscanf(buf,"t %d %d %d", &accno1, &accno2, &amount) == 3) {
            snprintf(filePath, sizeof(filePath), "./accounts/%d.txt", accno1);
            snprintf(filePath2, sizeof(filePath2), "./accounts/%d.txt", accno2);
            
            // Check that both files exist
            if (access(filePath, F_OK) == -1 || access(filePath2, F_OK) == -1) {
                sprintf(response, "%s", "fail: Unable to transfer money");
                break;
            }

            // Check that withdrawal from account 1 succedes
            if ((balance = withdraw(filePath, amount)) == -1) {
                sprintf(response, "%s", "fail: Unable to transfer money");
                break;
            }
            
            // Check that depositing to account 2 succedes
            if (deposit(filePath2, amount) == -1) {
                deposit(filePath, amount);          // If not, return the money back to account 1
                sprintf(response, "%s", "fail: Unable to transfer money");
                break;
            }
            sprintf(response, "%s %d %s %d %s %d", "ok: Transferred", amount, "from account", accno1, "to", accno2);
        } else {
            sprintf(response, "%s", "fail: Unable to transfer money");
        }
        break;


    // Deposit to an account
    case 'd': 
        if (sscanf(buf,"d %d %d", &accno1, &amount) == 2) {
            snprintf(filePath, sizeof(filePath), "./accounts/%d.txt", accno1);
            if (deposit(filePath, amount) == -1) {
                sprintf(response, "%s", "fail: Unable to deposit money");
                break;
            }
            sprintf(response, "%s %d %s %d", "ok: Deposited", amount, "to account", accno1);
        } else {
            sprintf(response, "%s", "fail: Unable to deposit money");
        }
        break;


    // Unknown command
    default: 
        strcpy(response, "fail: Unknown command");
        break;
    }

    // Send response message to client (ok/fail)
    send(clientFD, response, strlen(response), 0);

    return 0;
}

    


