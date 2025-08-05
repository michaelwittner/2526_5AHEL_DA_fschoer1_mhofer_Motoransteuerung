/**
 * main.c - Raspberry Pi / Microcontroller Serielle Kommunikation mit RT-Threads (Schritt03)
 *
 * Aufgabenstellung:
 * 1. Ein Thread empfängt Strings über die serielle Schnittstelle.
 *    - Wenn ein Carriage Return (CR, '\r') empfangen wird, wird der komplette String
 *      als Nachricht in eine Message Queue gelegt.
 * 2. Der Main-Thread liest Nachrichten aus der Message Queue und wertet Kommandos aus.
 *    - Unterstützte Kommandos:
 *        "LED ROT" / "LED rot" -> LED rot einschalten
 *        "OFF" -> Alle LEDs ausschalten
 *    - Leicht erweiterbar für zusätzliche Befehle
 */

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_MSG_LEN 128
#define QUEUE_SIZE 10

char messageQueue[QUEUE_SIZE][MAX_MSG_LEN];
int queueStart = 0;
int queueEnd = 0;

pthread_mutex_t queueMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queueCond = PTHREAD_COND_INITIALIZER;

// Hilfsfunktion: Nachricht in Queue legen
void enqueueMessage(const char *msg) {
    pthread_mutex_lock(&queueMutex);
    strncpy(messageQueue[queueEnd], msg, MAX_MSG_LEN);
    queueEnd = (queueEnd + 1) % QUEUE_SIZE;
    pthread_cond_signal(&queueCond);
    pthread_mutex_unlock(&queueMutex);
}

// Hilfsfunktion: Nachricht aus Queue lesen
void dequeueMessage(char *buffer) {
    pthread_mutex_lock(&queueMutex);
    while (queueStart == queueEnd) {
        pthread_cond_wait(&queueCond, &queueMutex);
    }
    strncpy(buffer, messageQueue[queueStart], MAX_MSG_LEN);
    queueStart = (queueStart + 1) % QUEUE_SIZE;
    pthread_mutex_unlock(&queueMutex);
}

// Hilfsfunktion: String zu Uppercase konvertieren
void strToUpper(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = toupper((unsigned char)str[i]);
    }
}

// Funktion zur Befehlsauswertung
void processCommand(const char *cmd) {
    char upperCmd[MAX_MSG_LEN];
    strncpy(upperCmd, cmd, MAX_MSG_LEN);
    upperCmd[MAX_MSG_LEN-1] = '\0';
    strToUpper(upperCmd);

    if (strcmp(upperCmd, "LED ROT") == 0) {
        printf("Kommando erkannt: LED ROT -> LED einschalten\n");
        // TODO: Hier GPIO/LED einschalten
    } else if (strcmp(upperCmd, "OFF") == 0) {
        printf("Kommando erkannt: OFF -> Alle LEDs ausschalten\n");
        // TODO: Hier GPIO/LED ausschalten
    } else {
        printf("Unbekanntes Kommando: %s\n", cmd);
    }
}

// Thread für seriellen Empfang
void* serialReceiveThread(void* arg) {
    char buffer[MAX_MSG_LEN];
    int pos = 0;
    int ch;
    
    while (1) {
        ch = getchar(); // UART-Leseoperation simuliert
        if (ch == '\r') {
            buffer[pos] = '\0';
            enqueueMessage(buffer);
            pos = 0;
        } else if (ch >= 32 && ch <= 126) {
            if (pos < MAX_MSG_LEN - 1) {
                buffer[pos++] = (char)ch;
            }
        }
        usleep(1000);
    }
    return NULL;
}

int main() {
    pthread_t recvThread;
    pthread_create(&recvThread, NULL, serialReceiveThread, NULL);

    char msg[MAX_MSG_LEN];
    while (1) {
        dequeueMessage(msg);
        printf("Empfangenes Kommando: %s\n", msg);
        processCommand(msg);
    }

    pthread_join(recvThread, NULL);
    return 0;
}