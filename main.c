#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <sys/mman.h>
#include <errno.h>
#include <time.h>

#define GB_32 (32UL * 1024 * 1024 * 1024)
#define MAX_SIZEBYTE 63
#define NUM_ITERATIONS 1000

// Precompute sizes in bytes
size_t sizes_in_bytes[MAX_SIZEBYTE + 1];

void precompute_sizes() {
    for (int i = 0; i <= MAX_SIZEBYTE; ++i) {
        if (i % 2 == 0) {
            sizes_in_bytes[i] = (size_t)1 << (i / 2 + 4);
        } else {
            sizes_in_bytes[i] = 3 * ((size_t)1 << ((i - 1) / 2 + 4));
        }
    }
}

// Function to increment the memory pointer
void *increment_mem_ptr(void **mem_ptr, int sizebyte) {
    size_t size = sizes_in_bytes[sizebyte];
    void *original_ptr = *mem_ptr;
    *mem_ptr = (void *)((char *)(*mem_ptr) + size);
    return original_ptr;
}

void fill_memory_with_ascii(void *mem_ptr, size_t size, int sizebyte) {
    char *ptr = (char *)mem_ptr;
    ptr[0] = (char)sizebyte; // Set the first byte to the sizebyte
    for (size_t i = 1; i < size; ++i) {
        ptr[i] = (char)(33 + ((i - 1) % 94)); // ASCII characters from 33 to 126
    }
}

void *thread_function(void *arg) {
    int core_id = *((int *)arg);
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) {
        perror("pthread_setaffinity_np");
        return NULL;
    }

    // Allocate 32GB of memory using mmap
    void *addr = mmap(NULL, GB_32, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (addr == MAP_FAILED) {
        perror("mmap");
        return NULL;
    }

    // Initialize memory pointer
    void *mem_ptr = addr;

    printf("Thread on core %02d: Allocated 32GB of memory at %p\n", core_id, addr);

    void *fst_string_ptr = NULL;
    void *snd_string_ptr = NULL;

    // Store ASCII characters in the allocated memory
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        int sizebyte = (rand() % 4);
        void *old_ptr = increment_mem_ptr(&mem_ptr, sizebyte);

        fill_memory_with_ascii(old_ptr, sizes_in_bytes[sizebyte], sizebyte);

        if (i == 0) {
            fst_string_ptr = old_ptr;
        } else if (i == 1) {
            snd_string_ptr = old_ptr;
        }
    }

    // Print the first 2 strings
    printf("Thread on core %02d: String 1 (sizebyte = %d) = %.*s\n", core_id, *(char *)fst_string_ptr, (int)sizes_in_bytes[*(char *)fst_string_ptr] - 1, (char *)fst_string_ptr + 1);
    printf("Thread on core %02d: String 2 (sizebyte = %d) = %.*s\n", core_id, *(char *)snd_string_ptr, (int)sizes_in_bytes[*(char *)snd_string_ptr] - 1, (char *)snd_string_ptr + 1);

    // Keep the memory allocated
    sleep(60);

    munmap(addr, GB_32);

    return NULL;
}

int get_physical_cores() {
    return sysconf(_SC_NPROCESSORS_ONLN);
}

int main() {
    // Seed the random number generator
    srand(time(NULL));

    // Precompute sizes in bytes
    precompute_sizes();

    int n = get_physical_cores();
    if (n <= 0) {
        fprintf(stderr, "Failed to get the number of physical cores\n");
        return 1;
    }

    int num_threads = 3 * n;
    pthread_t threads[num_threads];
    int core_ids[num_threads];

    for (int i = 0; i < num_threads; i++) {
        core_ids[i] = i % n;
        if (pthread_create(&threads[i], NULL, thread_function, &core_ids[i]) != 0) {
            perror("pthread_create");
            return 1;
        }
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
