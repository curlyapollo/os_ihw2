#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// объявление констант
const int SHM_SIZE = 1024;
const char *SEM_NAME = "/treasure_semaphore";
const char *SHM_NAME = "/treasure_shm";

// объявление глобальных переменных
int *shm_ptr;
sem_t *sem;

// обработчик сигналов
void sig_handler(int signo) {
    // удаляем семафор и разделяемую память
    sem_unlink(SEM_NAME);
    shm_unlink(SHM_NAME);
    exit(0);
}

// функция поиска клада
void search(int id, int start, int end) {
    // вывод информации о начале поисков
    printf("Group %d searches for treasure on section %d-%d\n", id, start, end);
    // имитация поиска
    sleep(rand() % 3 + 1);
    // проверка на нахождение клада
    if ((rand() % 100) < 25) {
        // вывод информации о нахождении клада
        printf("Group %d found treasure on section %d-%d\n", id, start, end);
        // завершение поисков
        sem_post(sem);
        // завершение работы группы
        exit(0);
    }
}

// функция группы пиратов
void group(int id, int start, int end) {
    // цикл поиска клада на каждом участке
    while (start <= end) {
        // поиск клада на текущем участке
        search(id, start, start + 9);
        // переход на следующий участок
        start += 10;
    }
}

int main(int argc, char *argv[]) {
    // обработка сигнала прерывания
    signal(SIGINT, sig_handler);
    // получение количества групп и участков
    int groups, sections;
    printf("Enter number of groups and sections: ");
    scanf("%d %d", &groups, &sections);
    // создание семафора и разделяемой памяти
    sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    ftruncate(shm_fd, SHM_SIZE);
    shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    // инициализация начала и конца текущего участка
    int start = 0, end = 9;
    // создание дочерних процессов для групп пиратов
    for (int i = 0; i < groups; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // процесс группы пиратов
            group(i + 1, start, end);
        } else if (pid > 0) {
            // процесс родительского процесса
            // установка начала и конца следующего участка
            start += 10;
            end += 10;
        } else {
            // ошибка создания процесса
            fprintf(stderr, "Error creating process for group %d\n", i + 1);
            exit(1);
        }
    }
    // ожидание завершения работы всех групп пиратов
    for (int i = 0; i < groups; i++) {
        wait(NULL);
    }
    // удаление семафора и разделяемой памяти
    sem_unlink(SEM_NAME);
    shm_unlink(SHM_NAME);
    // вывод информации о завершении поисков
    printf("All groups finished searching for treasure\n");
    return 0;
}