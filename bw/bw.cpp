#include "bw.h"
#include "util.h"
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>

typedef void* (*func_t)(void* arg);

char* create_buffer(const char* path, size_t& sz){
    //check if the file exists
    std::ifstream ifs(path);
    if(ifs.good()){
      die("the buffer file exists!"); 
    }
    
    sz = ( sz + PAGESIZE -1 )/PAGESIZE*PAGESIZE; // align size to pagesize

    int fd = open(path,O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
    if(fd == -1) {
        die("open file failed\n");
    }
    off_t offt = lseek(fd, sz-1, SEEK_SET);
    if(offt == -1) die("lseek failed");

    int result = write(fd, "", 1);
    if(result == -1) die("write failed");


    void* addr = mmap(0, sz, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if(addr == MAP_FAILED) die("mmap failed");
    
    return (char*)addr;

}



thread_info_t* create_thread_info(char* buffer, size_t sz, int threads, size_t stride,  AccessMode m ){

    thread_info_t* infos = (thread_info_t*)malloc(threads * sizeof(thread_info_t));
    memset(infos, 0, threads*sizeof(thread_info_t)); 

    size_t access_sz = sz/threads; 
    size_t access_count = access_sz/stride; 
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
            infos[i].start = buffer + i*access_sz;
            infos[i].end = buffer + (i+1)*access_sz;
            infos[i].stride = stride;
        }else{
            infos[i].start = buffer + i * stride;
            infos[i].stride = threads*stride;
            infos[i].end = buffer + sz + i*stride;
        }

        //printf("i: %d, start: %lu, end: %lu, stride: %lu\n", i, (long)(infos[i].start - buffer),(long)(infos[i].end - buffer), infos[i].stride);

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


void read_hub(thread_info_t* infos, int threads, size_t stride, SRMode srmode){
   
    func_t func = nullptr; 
    if(srmode == SEQ){
       switch(stride){
            case 64:
                func = read_nt_64;
              break;
            case 128:
                func = read_nt_128;
              break;
            case 256:
                func = read_nt_256;
              break;
            case 512:
                func = read_nt_512;
              break;
            default:
              die("wrong read stride");
       }
    }else{
        die("random read has not been implemented");
    } 
    launch(infos, threads, func);
}


void* read_nt_64(void* arg){
    thread_info_t* info = (thread_info_t*)arg;

    barrier_cross(&barrier);
    rdtscll(info->stop_cycle);
    UNROLL(
        for(char* t = info->start; t < info->end; t += info->stride){
            char* memaddr = t;
    
        }
    )
    return nullptr; 

}
void* read_nt_128(void* arg){}
void* read_nt_256(void* arg){}
void* read_nt_512(void* arg){}
