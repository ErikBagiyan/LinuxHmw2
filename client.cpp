#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstring>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>

int functionID, a, b, res;

std::mutex mtx;
std::condition_variable cv;

void compute() {
    const char* shm_name = "/rpc_shm";
    int shm_size = 4 * sizeof(int);
    int fd = shm_open(shm_name, O_RDWR | O_CREAT, 0666);

    if (fd == -1) {
        std::cerr << "Shared memory cannot be opened: " << std::strerror(errno) << std::endl;
        exit(errno);
    }

    if (ftruncate(fd, shm_size) == -1) {
        std::cerr << "Shared memory cannot be resized: " << std::strerror(errno) << std::endl;
        exit(errno);
    }

    int* adr = (int*) mmap(nullptr, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (adr == MAP_FAILED) {
        std::cerr << "MMAP FAILED: " << std::strerror(errno) << std::endl;
        exit(errno);
    }

    close(fd);

    const char* semName = "/rpc_sem";
    sem_t* sem = sem_open(semName, O_CREAT, 0666, 0);

    if (sem == SEM_FAILED) {
        std::cerr << "Semaphore cannot be opened: " << std::strerror(errno) << std::endl;
        exit(errno);
    }

    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock);
    }

    switch (functionID) {
        case 0:
            res = a + b;
            break;
        case 1:
            res = a - b;
            break;
        case 2:
            res = a * b;
            break;
        case 3:
            res = a / b;
            break;
        default:
            std::cerr << "Invalid function ID: " << functionID << std::endl;
            exit(1);
    }

    adr[3] = res;

    sem_post(sem);

    munmap(adr, shm_size);
    sem_close(sem);
}

int main() {
    std::thread t(compute);

    std::cout << "Please enter the function identifier (0: add, 1: sub, 2: mul, 3: div): ";
    std::cin >> functionID;

    std::cout << "Please enter the arguments: ";
    std::cin >> a >> b;

    const char *shm_name = "/rpc_shm";
    int shm_size = 4 * sizeof(int);
    int fd = shm_open(shm_name, O_RDWR, 0);

    if (fd == -1) {
        std::cerr << "Shared memory cannot be opened: " << std::strerror(errno) << std::endl;
        exit(errno);
    }

    int *adr = (int *) mmap(nullptr, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (adr == MAP_FAILED) {
        std::cerr << "MMAP FAILED: " << std::strerror(errno) << std::endl;
        exit(errno);
    }

    close(fd);

    adr[0] = functionID;
    adr[1] = a;
    adr[2] = b;

    cv.notify_one();

    sem_t *sem = sem_open(semName, O_CREAT, 0666, 0);

    if (sem == SEM_FAILED) {
        std::cerr << "Semaphore cannot be opened: " << std::strerror(errno) << std::endl;
        exit(errno);
    }

    sem_wait(sem);

    int res = adr[3];

    std::cout << "The result is: " << res << std::endl;

    sem_close(sem);
    sem_unlink(semName);
    munmap(adr, shm_size);
    shm_unlink(shm_name);

    t.join();

    return 0;
}

