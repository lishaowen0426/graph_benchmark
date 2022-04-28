#include <iostream>
#include <unistd.h>
#include "bw.h"
#include "util.h"
#ifdef PMEM
#include "ralloc.hpp"
#endif

barrier_t barrier;

int main(int argc, char** argv){

#ifdef PMEM
    RP_init("bw", 50*1024*1024*1024ULL);
#endif
    int c;
    const char* path = nullptr;
    size_t size;
    int threads;
    size_t stride;
    RWMode rwmode;
    SRMode srmode;
    AccessMode accessmode;
    while((c = getopt(argc, argv, "p:s:t:w:r:a:i:"))!= -1){
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
                stride = atol(optarg);
                break;
            case 'i':
                accessmode = (AccessMode)atoi(optarg);
                break;
            default:
                printf("./bw -p<path> -s<size> -t<thread> -w<write> -r<random> -a<access stride> -i<interleaved>\n");
                return 0;
        }
    }
    

    char* addr = create_buffer(path, size);
    thread_info_t* infos = create_thread_info(addr, size, threads, stride, accessmode);
    
    read_hub(infos, threads, stride, srmode);
    
    if(remove(path) == -1){
        die("delete buffer file failed");
    }
    return 0;
}
