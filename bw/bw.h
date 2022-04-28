#pragma once

#include <stddef.h>
#include <pthread.h>
#include <stdint.h>
#ifdef PMEM
#include "libpmem.h"
#endif
#define buffer_malloc __malloc
#define buffer_free __free


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
    char* buffer;
} thread_info_t;

typedef struct {
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    int count;
    int crossed;
}barrier_t;

extern barrier_t barrier;
extern size_t access_size; 
extern size_t nb_accesses; 

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

thread_info_t* create_thread_info(char* buffer, size_t sz, int threads,  AccessMode m );


void read_hub(thread_info_t* infos, int threads,  SRMode srmode);


void* read_seq(void* arg);
void* read_random(void* arg);



void write_hub(thread_info_t* infos, int threads,  SRMode srmode);


void* write_seq(void* arg);
void* write_random(void* arg);

inline uint64_t find_start(thread_info_t* infos, int threads){
    uint64_t min = 0xffffffffffffffff;
    for(int i = 0; i < threads; i++){
        if(infos[i].start_cycle < min) min = infos[i].start_cycle;
    }
    return min;
}
inline uint64_t find_stop(thread_info_t* infos, int threads){
    uint64_t max = 0x0;
    for(int i = 0; i < threads; i++){
        if(infos[i].stop_cycle >max) max = infos[i].start_cycle;
    }
    return max;
}


