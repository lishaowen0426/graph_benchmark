#include <iostream>
#include "pointer_chasing.h"
#include <stdlib.h>


#define PAD_SIZE 0 
#define BUF_SIZE 1024*1024*1024ULL

int main(int argc, char** argv){

    if(argc < 4){
        printf("./chasing [buffer size in bytes] [count] [cas_success]");
        exit(0);
    }
#ifdef PMEM
    RP_init("chasing",50*1024*1024*1024ULL);
#endif
    
    size_t len;
    size_t buffer_sz;
    buffer_sz = atol(argv[1]); 
    size_t count = atol(argv[2]);
    //count = (count > (1<<30))? count: (1<<30);
    bool cas_success = atoi(argv[3]);
    bool shuffle = !cas_success;
    if(cas_success){
        printf("Benchmark cas_success\n");
    }else{
        printf("Benchmark cas_fail\n");
    }

    void* buffer = create_random<PAD_SIZE>( buffer_sz, len, shuffle);
    if(buffer_sz < (1024*1024))
        printf(" buffer size: %.2fkb\n",  ((float)buffer_sz/(1024.0)));
    else
        printf(" buffer size: %.2fmb\n",  ((float)buffer_sz/(1024.0*1024.0)));
    uint64_t start, stop;
    rdtscll(start);
    cas_hub(buffer, len, count, cas_success, PAD_SIZE+sizeof(void*));
    rdtscll(stop);
    printf("Chasing time: %.4f ns\n", 1000000000.0*CYCLES_TO_SEC(start,stop, get_cpu_freq())/(float)count);


#ifdef PMEM
    RP_close();
#endif
}
