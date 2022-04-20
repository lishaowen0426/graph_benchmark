#include "iostream"
#include "pointer_chasing.h"


#define PAD_SIZE 8
#define BUF_SIZE 1024*1024*1024ULL

int main(int argc, char** argv){

#ifdef PMEM
    RP_init("chasing",50*1024*1024*1024ULL);
#endif

    size_t len;
    void* buffer = create_random<PAD_SIZE>(BUF_SIZE,len);
    uint64_t start, stop;
    rdtscll(start);
    cas_chase_fail(buffer, len);
    rdtscll(stop);
    printf("Chasing time: %fs\n", CYCLES_TO_SEC(start,stop, get_cpu_freq()));


#ifdef PMEM
    RP_close();
#endif
}
