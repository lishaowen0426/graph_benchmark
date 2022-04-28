#include "bw.h"
#include "util.h"
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
typedef void* (*func_t)(void* arg);


static size_t access_count;

static thread_local __uint128_t g_lehmer64_state;
static void init_seed(unsigned int i){
   srand(i);
   g_lehmer64_state = rand();

}
static uint64_t lehmer64() {
  g_lehmer64_state *= 0xda942042e4dd58b5;
  return g_lehmer64_state >> 64;
}



char* create_buffer(const char* path, size_t& sz){
    //check if the file exists
    std::ifstream ifs(path);
    if(ifs.good()){
      die("the buffer file exists!"); 
    }
    
    sz = ( sz + 4096 -1 )/4096*4096; // align size to pagesize

    int fd = open(path,O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
    if(fd == -1) {
        die("open file failed\n");
    }
    off_t offt = lseek(fd, sz-1, SEEK_SET);
    if(offt == -1) die("lseek failed");

    int result = write(fd, "", 1);
    if(result == -1) die("write failed");

#ifndef PMEM
    void* addr = mmap(0, sz, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if(addr == MAP_FAILED) die("mmap failed");
#else
    void* addr = pmem_map_file(path, 0,0,0777, NULL,NULL);
    if(!pmem_is_pmem(addr,sz)){
        die("File is not in pmem");
    }
#endif
    memset(addr, 0, sz);
    return (char*)addr;

}



thread_info_t* create_thread_info(char* buffer, size_t sz, int threads,   AccessMode m ){

    thread_info_t* infos = (thread_info_t*)malloc(threads * sizeof(thread_info_t));
    memset(infos, 0, threads*sizeof(thread_info_t)); 

    size_t partition = sz/threads; 
    access_count = partition/access_size; 
    if(access_count == 0) die("access_count is 0");
    
    cpu_set_t mask;
    for(int i = 0; i < threads; i++){
        if(pthread_attr_init(&(infos[i].attr))){
            die("init pthread attr failed");
        }
        CPU_ZERO(&mask);
        CPU_SET(i, &mask);
        if(pthread_attr_setaffinity_np(&(infos[i].attr),sizeof(cpu_set_t), &mask )){
            die("set CPU affinity mask failed");
        }
        
        if(m == GROUPED){
            infos[i].start = buffer + i*partition;
            infos[i].end = buffer + (i+1)*partition;
            infos[i].stride = access_size;
        }else{
            infos[i].start = buffer + i * access_size;
            infos[i].stride = threads*access_size;
            infos[i].end = buffer + sz + i*access_size;
        }

        infos[i].buffer = (char*)aligned_alloc(4096, access_size);
        memset(infos[i].buffer, 42, access_size);

        init_seed((unsigned)i);
    }
    return infos;

}



void barrier_init(barrier_t* b, int count){
    pthread_cond_init(&(b->cond), nullptr);
    pthread_mutex_init(&(b->mutex), nullptr);

    b->count = count;
    b->crossed = 0;
}
void barrier_cross(barrier_t* b){
    pthread_mutex_lock(&(b->mutex));
    b->crossed++;

    if(b->crossed < b->count){
        pthread_cond_wait(&(b->cond), &(b->mutex));
    }else{
        pthread_cond_broadcast(&(b->cond));
        b->crossed = 0;
    }
    pthread_mutex_unlock(&(b->mutex));

}


void launch(thread_info_t* infos, int threads, func_t func){
    for(int i = 0; i < threads; i++){
        if(pthread_create(&(infos[i].id), &(infos[i].attr), func, (void*)(infos+i))){
            die("create thread failed"); 
        }
    }
}


void read_hub(thread_info_t* infos, int threads,  SRMode srmode){
   
    func_t func = nullptr; 
    if(srmode == SEQ){
        func = read_seq;
    }else{
        func = read_random;
    } 
    launch(infos, threads, func);
}


void* read_seq(void* arg){
    thread_info_t* info = (thread_info_t*)arg;
    char* b = info->buffer;
    char* base = info->start;
    size_t s = info->stride;
    char* memaddr;

    barrier_cross(&barrier);
    rdtscll(info->start_cycle);
    UNROLL(
        for(size_t i = 0; i < nb_accesses; i++){
            memaddr = base + (i&access_count)*s;
            memcpy(b, memaddr, access_size); 
        }
    )
    rdtscll(info->stop_cycle);
    return nullptr; 

}
void* read_random(void* arg){

    thread_info_t* info = (thread_info_t*)arg;
    char* b = info->buffer;
    char* base = info->start;
    size_t s = info->stride;
    char* memaddr;

    barrier_cross(&barrier);
    rdtscll(info->start_cycle);
    UNROLL(
        for(size_t i = 0; i < nb_accesses; i++){
            size_t loc = lehmer64()%access_count;
            memaddr = base + loc*s;
            memcpy(b, memaddr, access_size); 
        }
    )
    rdtscll(info->stop_cycle);
    return nullptr; 

}
void write_hub(thread_info_t* infos, int threads,  SRMode srmode){
   
    func_t func = nullptr; 
    if(srmode == SEQ){
        func = write_seq;
    }else{
        func = write_random;
    } 
    launch(infos, threads, func);
}


void* write_seq(void* arg){
    thread_info_t* info = (thread_info_t*)arg;
    char* b = info->buffer;
    char* base = info->start;
    size_t s = info->stride;
    char* memaddr;

    barrier_cross(&barrier);
    rdtscll(info->start_cycle);
    UNROLL(
        for(size_t i = 0; i < nb_accesses; i++){
            memaddr = base + (i&access_count)*s;
#ifndef PMEM
            memcpy( memaddr,b, access_size); 
#else
            pmem_memcpy_persist(memaddr, b, access_size);
#endif
        }
    )
    rdtscll(info->stop_cycle);
    return nullptr; 

}
void* write_random(void* arg){

    thread_info_t* info = (thread_info_t*)arg;
    char* b = info->buffer;
    char* base = info->start;
    size_t s = info->stride;
    char* memaddr;

    barrier_cross(&barrier);
    rdtscll(info->start_cycle);
    UNROLL(
        for(size_t i = 0; i < nb_accesses; i++){
            size_t loc = lehmer64()%access_count;
            memaddr = base + loc*s;
#ifndef PMEM
            memcpy( memaddr,b, access_size); 
#else
            pmem_memcpy_persist(memaddr, b, access_size);
#endif
        }
    )
    rdtscll(info->stop_cycle);
    return nullptr; 

}
