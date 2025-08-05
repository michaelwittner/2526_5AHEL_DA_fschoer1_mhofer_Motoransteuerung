/*
 * main.c - Schritt03 Easy
 * 
 * Dieses Programm empfängt Kommandos über die serielle Schnittstelle
 * und wertet sie mit zwei Threads aus:
 *  1. Thread: Empfängt Strings und legt sie bei CR in eine Message Queue
 *  2. Thread: Liest Befehle aus der Queue und führt Aktionen aus
 *
 * Unterstützte Befehle:
 *  - "LED ROT" oder "LED rot" -> rote LED an
 *  - "OFF" -> alle LEDs aus
 *
 * Weitere Befehle können leicht in parseCommand() hinzugefügt werden.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <ctype.h>

#define QUEUE_SIZE 10
#define CMD_MAX_LEN 64

char messageQueue[QUEUE_SIZE][CMD_MAX_LEN];
int queueHead = 0;
int queueTail = 0;
pthread_mutex_t queueMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queueCond = PTHREAD_COND_INITIALIZER;

// --- LED Funktionen (simuliert) ---
void ledRedOn() { printf("[LED] Rot an\n"); }
void ledGreenOn() { printf("[LED] Gruen an\n"); }
void allLedsOff() { printf("[LED] Alle aus\n"); }

// --- Hilfsfunktion zum Einfügen in die Queue ---
void enqueueCommand(const char* cmd) {
    pthread_mutex_lock(&queueMutex);
    strncpy(messageQueue[queueTail], cmd, CMD_MAX_LEN-1);
    messageQueue[queueTail][CMD_MAX_LEN-1] = '\0';
    queueTail = (queueTail + 1) % QUEUE_SIZE;
    pthread_cond_signal(&queueCond);
    pthread_mutex_unlock(&queueMutex);
}

// --- Hilfsfunktion zum Auslesen der Queue ---
int dequeueCommand(char* buffer) {
    pthread_mutex_lock(&queueMutex);
    while (queueHead == queueTail) {
        pthread_cond_wait(&queueCond, &queueMutex);
    }
    strncpy(buffer, messageQueue[queueHead], CMD_MAX_LEN);
    queueHead = (queueHead + 1) % QUEUE_SIZE;
    pthread_mutex_unlock(&queueMutex);
    return 1;
}

// --- Befehlsauswertung ---
void parseCommand(const char* cmd) {
    if (strcasecmp(cmd, "LED ROT") == 0 || strcasecmp(cmd, "LED rot") == 0) {
        ledRedOn();
    } else if (strcasecmp(cmd, "OFF") == 0) {
        allLedsOff();
    } else {
        printf("Unbekannter Befehl: %s\n", cmd);
    }
}

// --- Thread: Empfängt Daten (Simulation statt echte UART) ---
void* receiverThread(void* arg) {
    char buffer[CMD_MAX_LEN];
    while (1) {
        if (fgets(buffer, sizeof(buffer), stdin)) {
            buffer[strcspn(buffer, "\r\n")] = '\0';
            if (strlen(buffer) > 0) {
                enqueueCommand(buffer);
            }
        }
        usleep(10000);
    }
    return NULL;
}

// --- Thread: Liest Queue und wertet Kommandos aus ---
void* commandThread(void* arg) {
    char buffer[CMD_MAX_LEN];
    while (1) {
        if (dequeueCommand(buffer)) {
            parseCommand(buffer);
        }
    }
    return NULL;
}

int main() {
    pthread_t recvThread, cmdThread;

    pthread_create(&recvThread, NULL, receiverThread, NULL);
    pthread_create(&cmdThread, NULL, commandThread, NULL);

    pthread_join(recvThread, NULL);
    pthread_join(cmdThread, NULL);

    return 0;
}
