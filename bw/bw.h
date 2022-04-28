#pragma once

#include <stddef.h>
#include <pthread.h>
#include <stdint.h>
#define buffer_malloc __malloc
#define buffer_free __free

#define PAGESIZE 4096

#define EXEC_1_TIMES(x)   x 
#define EXEC_2_TIMES(x)   x x
#define EXEC_4_TIMES(x)   EXEC_2_TIMES(EXEC_2_TIMES(x))
#define EXEC_8_TIMES(x)   EXEC_2_TIMES(EXEC_4_TIMES(x))
#define EXEC_16_TIMES(x)  EXEC_2_TIMES(EXEC_8_TIMES(x))
#define EXEC_32_TIMES(x)   EXEC_2_TIMES(EXEC_16_TIMES(x))
#define EXEC_64_TIMES(x)   EXEC_2_TIMES(EXEC_32_TIMES(x))
#define EXEC_128_TIMES(x)   EXEC_2_TIMES(EXEC_64_TIMES(x))
#define EXEC_256_TIMES(x)   EXEC_2_TIMES(EXEC_128_TIMES(x))

#define UNROLL_LOOPLOOP(t,x)  EXEC_##t##_TIMES(x)
#define UNROLL_LOOP(t,x) UNROLL_LOOPLOOP(t,x)
#define UNROLL(x) UNROLL_LOOP(LOOPS, x)

typedef struct {
    pthread_t id;
    char* start;
    char* end;
    size_t stride;
    pthread_attr_t attr;
    uint64_t start_cycle, stop_cycle;
} thread_info_t;

typedef struct {
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    int count;
    int crossed;
}barrier_t;

extern barrier_t barrier;

enum RWMode:int {
    READ = 0,
    WRITE = 1
};

enum SRMode:int {
    SEQ = 0,
    RANDOM = 1
};

enum AccessMode:int {
    GROUPED = 0,
    INTERLEAVED = 1
};

void barrier_init(barrier_t* b, int count);
void barrier_cross(barrier_t* b);

char* create_buffer(const char* path, size_t& sz);

thread_info_t* create_thread_info(char* buffer, size_t sz, int threads, size_t stride, AccessMode m );


void read_hub(thread_info_t* infos, int threads, size_t stride, SRMode srmode);


void* read_nt_64(void* arg);
void* read_nt_128(void* arg);
void* read_nt_256(void* arg);
void* read_nt_512(void* arg);



