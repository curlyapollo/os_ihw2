#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/mman.h>
#include <signal.h>

#define NUM_GROUPS 3 // Количество групп пиратов
#define NUM_AREAS 5 // Количество участков на острове
#define TREASURE_AREA 2 // Участок, на котором находится клад
#define MEM_SIZE sizeof(int) * NUM_GROUPS * NUM_AREAS // Размер разделяемой памяти

int *shared_mem; // Указатель на разделяемую память
sem_t *semaphores[NUM_GROUPS]; // Массив семафоров для каждой группы

void cleanup() {
    // Удаление семафоров
    for (int i = 0; i < NUM_GROUPS; i++) {
        sem_close(semaphores[i]);
        sem_unlink("/semaphore_group");
    }

    // Удаление разделяемой памяти
    munmap(shared_mem, MEM_SIZE);
}

void sighandler(int signum) {
    printf("\nПрограмма завершена пользователем\n");
    exit(0);
}

int main() {
    signal(SIGINT, sighandler);

    // Создание и инициализация семафоров для каждой группы
    for (int i = 0; i < NUM_GROUPS; i++) {
        semaphores[i] = sem_open("/semaphore_group", O_CREAT, 0777, 1);
        if (semaphores[i] == SEM_FAILED) {
            perror("Ошибка создания семафора");
            exit(1);
        }
    }

    // Создание разделяемой памяти
    shared_mem = mmap(NULL, MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (shared_mem == MAP_FAILED) {
        perror("Ошибка создания разделяемой памяти");
        exit(1);
    }

    // Инициализация разделяемой памяти
    for (int i = 0; i < NUM_GROUPS; i++) {
        for (int j = 0; j < NUM_AREAS; j++) {
            shared_mem[i * NUM_AREAS + j] = 0;
        }
    }

    // Создание дочерних процессов для каждой группы пиратов
    for (int i = 0; i < NUM_GROUPS; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("Ошибка создания дочернего процесса");
            exit(1);
        } else if (pid == 0) {
            // Дочерний процесс

            // Инициализация случайного начального участка для группы
            int start_area = rand() % NUM_AREAS;

            while (1) {
                // Захват семафора для группы
                sem_wait(semaphores[i]);

                // Поиск клада на текущем участке
                if (shared_mem[i * NUM_AREAS + start_area] == 0) {
                    printf("Группа %d нашла клад на участке %d\n", i + 1, start_area + 1);
                    shared_mem[i * NUM_AREAS + start_area] = 1;

                    // Проверка, найден ли клад на нужном участке
                    if (start_area == TREASURE_AREA) {
                        printf("Клад найден на участке %d! Прекращение работы\n", TREASURE_AREA + 1);

                        // Освобождение семафоров и разделяемой памяти
                        cleanup();

                        exit(0);
                    }
                } else {
                    printf("Группа %d не нашла клад на участке %d\n", i + 1, start_area + 1);
                }

                // Освобождение семафора для группы
                sem_post(semaphores[i]);

                // Переход на следующий участок
                start_area = (start_area + 1) % NUM_AREAS;
                sleep(1);
            }
        }
    }

    // Ожидание завершения всех дочерних процессов
    for (int i = 0; i < NUM_GROUPS; i++) {
        wait(NULL);
    }

    // Освобождение семафоров и разделяемой памяти
    cleanup();

    return 0;
}