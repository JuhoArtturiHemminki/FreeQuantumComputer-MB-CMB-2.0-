#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

#define TOTAL_LOOPS 78125000ULL

typedef struct {
    uint32_t total_ones;
    uint32_t tail_coefficient;
} ApcmWordEngine;

typedef struct {
    uint64_t start_idx;
    uint64_t end_idx;
} ThreadArg;

static inline void process_apcm_words(ApcmWordEngine *engine, uint64_t word0, uint64_t word1) {
    engine->total_ones += __builtin_popcountll(word0);
    engine->total_ones += __builtin_popcountll(word1);

    if (word1 & (1ULL << 63)) {
        uint64_t remaining = word1 & ~(1ULL << 63);
        if (remaining == 0) {
            engine->tail_coefficient = 63 + __builtin_clzll(word0);
        } else {
            engine->tail_coefficient = __builtin_clzll(remaining) - 1;
        }
    }
}

void* thread_worker(void* arg) {
    ThreadArg *range = (ThreadArg*)arg;
    volatile uint64_t word0 = 0xFFFFFFFFFFFFFFFFULL; 
    volatile uint64_t word1 = (1ULL << 63) | 1ULL;   
    ApcmWordEngine engine = {0, 0};

    for (uint64_t idx = range->start_idx; idx < range->end_idx; idx++) {
        process_apcm_words(&engine, word0, word1);
        __asm__ __volatile__("" :: "g"(&engine) : "memory");
        engine.total_ones = 0;
        engine.tail_coefficient = 0;
    }
    return NULL;
}

int main() {
    long num_threads = sysconf(_SC_NPROCESSORS_ONLN);
    if (num_threads < 1) num_threads = 1;

    printf("Käännetään ja käynnistetään MONISÄIKEINEN APCM-testi (pthreads)...\n");
    printf("Käytetään %ld rinnakkaista säiettä.\n", num_threads);

    pthread_t threads[num_threads];
    ThreadArg args[num_threads];
    uint64_t loops_per_thread = TOTAL_LOOPS / num_threads;

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (long i = 0; i < num_threads; i++) {
        args[i].start_idx = i * loops_per_thread;
        args[i].end_idx = (i == num_threads - 1) ? TOTAL_LOOPS : (i + 1) * loops_per_thread;
        pthread_create(&threads[i], NULL, thread_worker, &args[i]);
    }

    for (long i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double duration = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    uint64_t total_bits = TOTAL_LOOPS * 128ULL;
    double ns_per_bit = (duration / total_bits) * 1e9;
    double gbps_capacity = (total_bits / duration) / 1e9;

    printf("\n=== ULTRA SANAOPTIMOITU MONISÄIKEINEN BENCHMARK VALMIS ===\n");
    printf("Kokonaisaika:        %.6f sekuntia\n", duration);
    printf("Bittejä käsitelty:    %llu bittiä\n", total_bits);
    printf("Nopeus per raakabitti: %.4f nanosekuntia (ns) (kaikki ytimet)\n", ns_per_bit);
    printf("Kokonaiskaista:       %.2f Gbps / all combined cores\n", gbps_capacity);
    printf("========================================================\n");
    return 0;
}
