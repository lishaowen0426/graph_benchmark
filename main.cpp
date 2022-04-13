#include <iostream>
#include "config.h"
#include "loader.h"
#include <unistd.h>
#include "bfs.h"
#include "pebs.h"
#include "oneapi/tbb/global_control.h"



using namespace oneapi::tbb;

int THREADS;
size_t NB_NODES;
bool SYMMETRIC;
Graph* graph;

int main( int argc, char** argv){

#ifdef PMEM
    RP_init("graph",30*1024*1024*1024ULL);
#endif
    int c;
    const char* binary;
    bool debug = false;
    int mode;
    SYMMETRIC = false;
    THREADS = 0;
    while((c = getopt(argc, argv, "t:v:f:m:ds"))!= -1){
        switch(c){
            case 't':
                THREADS = atoi(optarg);
                break;
            case 'v':
                NB_NODES = atol(optarg);
                break;
            case 'f':
                binary = optarg;
                break;
            case 'd':
                debug = true;
                break;
            case 's':
                SYMMETRIC = true; 
                break;
            case 'm':
                mode = atoi(optarg);
                break;
            default:
                printf("./benchmark -t <threads> -v <nb_nodes> -m <bfs_mode> -f <binary> -s<is_symmetric>\n");
                return 0;
        }
    }
    if(debug){
        THREADS = 8;
        NB_NODES = 65536;
        binary = "/home/lsw/graph_data/equal_rmat_binary_32";
        SYMMETRIC = true;
    }
    else if(  NB_NODES == 0){
        printf("./benchmark -t <threads> -v <nb_nodes> -m <bfs_mode> -f <binary> -s<is_symmetric>\n");
        return 0;
    }

    if(THREADS == 0) THREADS = 1;

    global_control thread_limit(global_control::parameter::max_allowed_parallelism, THREADS);
    printf("Run with: %lu worker threads, %lu nodes, binary: %s\n", global_control::active_value(global_control::parameter::max_allowed_parallelism), NB_NODES, binary);

    
    graph = create_graph(binary); 
    /*
    int perf_fd = perf_count_setup(PERF_TYPE_HARDWARE,PERF_COUNT_HW_CACHE_MISSES,0,-1 );
    struct read_group_format perf_data;
    uint64_t cache_misses_start, cache_misses_end;
    read(perf_fd, (void*)&perf_data, sizeof(struct read_group_format));
    assert(perf_data.nr == 1);
    cache_misses_start = perf_data.values[0].value;
    */

    if(system("perf record -F 997 -e instructions:pp -a  2>&1 &")){}
    sleep(2);
    bfs_hub(graph,0,mode);
    if(system("echo pmem | sudo -S killall -INT -w perf")) {};
    /*
    read(perf_fd, (void*)&perf_data, sizeof(struct read_group_format));
    assert(perf_data.nr == 1);
    cache_misses_end = perf_data.values[0].value;
    printf("cache misses: %lu\n", cache_misses_end - cache_misses_start);
    */
#ifdef PMEM
    RP_close();
#endif
    return 0;
}
