#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>

#define NUM_PIRATES 5
#define NUM_AREAS 10

typedef struct {
    int found;
    sem_t semaphore;
} Area;

Area *areas;
sem_t *pirate_semaphores;
sem_t *treasure_semaphore;

void cleanup() {
    int i;
    // Удаляем разделяемую память и семафоры
    munmap(areas, sizeof(Area) * NUM_AREAS);
    shm_unlink("/treasure_map");
    for (i = 0; i < NUM_PIRATES; i++) {
        sem_destroy(&pirate_semaphores[i]);
    }
    sem_destroy(treasure_semaphore);
    sem_unlink("/treasure_sem");
    for (i = 0; i < NUM_PIRATES; i++) {
        sem_unlink("/pirate_sem" + i);
    }
}

void sigint_handler(int sig) {
    printf("Program interrupted\n");
    cleanup();
    exit(0);
}

int main() {
    int i, j, fd;
    pid_t pid;

    // Создаем разделяемую память для участков острова
    fd = shm_open("/treasure_map", O_CREAT | O_RDWR, 0666);
    ftruncate(fd, sizeof(Area) * NUM_AREAS);
    areas = mmap(NULL, sizeof(Area) * NUM_AREAS, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    // Создаем семафоры для каждого пирата и для клада
    pirate_semaphores = (sem_t *) malloc(sizeof(sem_t) * NUM_PIRATES);
    for (i = 0; i < NUM_PIRATES; i++) {
        sem_init(&pirate_semaphores[i], 1, 0);
        sem_unlink("/pirate_sem" + i);
    }
    treasure_semaphore = sem_open("/treasure_sem", O_CREAT | O_EXCL, 0666, 0);

    // Создаем дочерние процессы
    for (i = 0; i < NUM_PIRATES; i++) {
        pid = fork();
        if (pid == 0) { // Дочерний процесс
            // Находим свой участок острова и начинаем поиски клада
            for (j = i; j < NUM_AREAS; j += NUM_PIRATES) {
                printf("Pirate %d searching area %d\n", i, j);
                sleep(rand() % 3); // Имитируем поиски
                if (areas[j].found) {
                    printf("Pirate %d found treasure in area %d\n", i, j);
                    sem_post(treasure_semaphore); // Сообщаем о нахождении клада
                    break;
                }
                sem_post(&pirate_semaphores[i]); // Сообщаем о завершении поисков на этом участке
            }
            exit(0);
        }
    }

    // Родительский процесс ждет сообщения о нахождении клада или завершении поисков всех пиратов
    while (1) {
        sem_wait(treasure_semaphore); // Ожидаем сообщения о нахождении клада
        printf("Treasure found!\n");
        for (i = 0; i < NUM_PIRATES; i++) {
            sem_post(&pirate_semaphores[i]); // Прерываем работу всех пиратов
        }
        break;
    }
    for (i = 0; i < NUM_PIRATES; i++) {
        sem_wait(&pirate_semaphores[i]); // Ожидаем завершения работы каждого пирата
    }

    // Удаляем разделяемую память и семафоры
    cleanup();

    return 0;
}