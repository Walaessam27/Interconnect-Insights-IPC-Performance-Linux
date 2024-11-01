#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <unistd.h>

#define SHM_SIZE 4096  
void starting(double *packet1, double *packet2, int N) {
    for (int i = 0; i < N; i++) {
        packet1[i] = (double)rand() / (double)(RAND_MAX / N);
        packet2[i] = (double)rand() / (double)(RAND_MAX / 10);}}

void serial(double *packet1, double *packet2, double *result, int N) {
    for (int i = 0; i < N; i++) {
        result[i] = pow(packet1[i], packet2[i]);}}

void parallelsh(double *packet1, double *packet2, double *result, int N, int M) {
    int shmid = shmget(IPC_PRIVATE, N * sizeof(double), IPC_CREAT | 0666);
    double *shmem = (double*)shmat(shmid, NULL, 0);
    pid_t pid;
    for (int i = 0; i < M; i++) {
        pid = fork();
        if (pid == 0) {
            int start = i * (N / M);
            int end = (i == M - 1) ? N : start + (N / M);
            for (int j = start; j < end; j++) {
                shmem[j] = pow(packet1[j], packet2[j]);
            }
            shmdt(shmem);
            exit(0);}}
    for (int i = 0; i < M; i++) wait(NULL);
    for (int i = 0; i < N; i++) result[i] = shmem[i];
    shmctl(shmid, IPC_RMID, NULL);}

void parallelmes(double *packet1, double *packet2, double *result, int N, int M) {
    char fifo_name[50];
    pid_t pid;
    for (int i = 0; i < M; i++) {
        snprintf(fifo_name, sizeof(fifo_name), "/tmp/my_fifo_%d", i);  
        mkfifo(fifo_name, 0666);
        pid = fork();
        if (pid == 0) {  
            int start = i * (N / M);
            int end = (i == M - 1) ? N : start + (N / M);
            int fd = open(fifo_name, O_WRONLY);

            for (int j = start; j < end; j++) {
                double res = pow(packet1[j], packet2[j]);
                write(fd, &res, sizeof(double)); }
            close(fd);exit(0);}}
    for (int i = 0; i < M; i++) {
        snprintf(fifo_name, sizeof(fifo_name), "/tmp/my_fifo_%d", i);
        int fd = open(fifo_name, O_RDONLY);
        
        int start = i * (N / M);
        int end = (i == M - 1) ? N : start + (N / M);
        for (int j = start; j < end; j++) {
            read(fd, &result[j], sizeof(double)); }
        close(fd);
        unlink(fifo_name);}
    for (int i = 0; i < M; i++) wait(NULL);}

int comparing(double *arr1, double *arr2, int N) {
    for (int i = 0; i < N; i++) {
        if (fabs(arr1[i] - arr2[i]) > 1e-9) return 0;}
    return 1;}
int main() {
    // Student ID 1: 12027854
    // Student ID 2: 12029152
    int N = (12027854 + 12029152) / 2;  // 12028503
    int M = (N % 10) + 5;  //  8

    double *packet1 = (double *)malloc(N * sizeof(double));
    double *packet2 = (double *)malloc(N * sizeof(double));
    double *result_packet1 = (double *)malloc(N * sizeof(double));
    double *result_packet2 = (double *)malloc(N * sizeof(double));
    double *result_packet3 = (double *)malloc(N * sizeof(double));
    starting(packet1, packet2, N);
    clock_t start = clock();
    serial(packet1, packet2, result_packet1, N);
    clock_t end = clock();
    printf("Serial Time: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);

    start = clock();
    parallelsh(packet1, packet2, result_packet2, N, M);
    end = clock();
    printf("Parallel (Shared Memory) Time: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);
    printf("Shared Memory: %s\n", comparing(result_packet1, result_packet2, N) ? "Match" : "Mismatch");

    start = clock();
    parallelmes(packet1, packet2, result_packet3, N, M);
    end = clock();
    printf("Parallel (Message Passing) Time: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);
    printf("Message Passing: %s\n", comparing(result_packet1, result_packet3, N) ? "Match" : "Mismatch");
    free(packet1); free(packet2); free(result_packet1); free(result_packet2); free(result_packet3);
    return 0;
}
