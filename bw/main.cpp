#include <iostream>
#include <unistd.h>
#include "bw.h"
#include "util.h"
#ifdef PMEM
#include "ralloc.hpp"
#endif

barrier_t barrier;
size_t access_size;
size_t nb_accesses;
int main(int argc, char** argv){

#ifdef PMEM
    RP_init("bw", 50*1024*1024*1024ULL);
#endif
    int c;
    const char* path = nullptr;
    size_t size;
    int threads;
    RWMode rwmode;
    SRMode srmode;
    AccessMode accessmode;
    while((c = getopt(argc, argv, "p:s:t:w:r:a:i:n:"))!= -1){
        switch(c){
            case 'p':
                path = optarg; 
                break;
            case 's':
                size = atol(optarg);
                break;
            case 't':
                threads = atoi(optarg);
                break;
            case 'w':
                rwmode = (RWMode)atoi(optarg); 
                break;
            case 'r': 
                srmode = (SRMode)atoi(optarg);
                break;
            case 'a':
                access_size = atol(optarg);
                break;
            case 'i':
                accessmode = (AccessMode)atoi(optarg);
                break;
            case 'n':
                nb_accesses = atol(optarg);
                break;
            default:
                printf("./bw -p<path> -s<size> -t<thread> -w<write> -r<random> -a<access stride> -i<interleaved> -n<number of accesses>\n");
                return 0;
        }
    }
    

    char* addr = create_buffer(path, size);
    thread_info_t* infos = create_thread_info(addr, size, threads, accessmode);
    
    read_hub(infos, threads,  srmode);
    for(int i = 0; i < threads; i++) pthread_join(infos[i].id, nullptr); 

    uint64_t start = find_start(infos, threads);
    uint64_t stop = find_stop(infos, threads);
    uint64_t total_bytes = threads*nb_accesses*access_size;
    float time = (float)(stop-start)/(float)get_cpu_freq(); 
    printf("%s-%s time %lu ( %fs )\n",((srmode==SEQ)?"Sequential":"Random"), ((rwmode == READ)?"read":"write"), stop - start, time);
    printf("bandwidth: %.5f\n", (float)total_bytes/time);
    if(remove(path) == -1){
        die("delete buffer file failed");
    }
    return 0;
}
