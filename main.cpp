#include <iostream>
#include "config.h"
#include "loader.h"
#include <unistd.h>
#include "bfs.h"
#include "util.h"
#include "pebs.h"
#include "pagerank.h"
#include "oneapi/tbb.h"
#include <vector>



using namespace oneapi::tbb;

int THREADS;
size_t NB_NODES;
bool SYMMETRIC;
Graph* graph;
task_arena arena;

int main( int argc, char** argv){

#ifdef PMEM
    RP_init("graph",150*1024*1024*1024ULL);
#endif
    int c;
    const char* binary;
    bool debug = false;
    int mode;
    int iterations;
    SYMMETRIC = false;
    THREADS = 0;
    const char* output = nullptr;
    while((c = getopt(argc, argv, "t:v:f:m:i:o:ds"))!= -1){
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
            case 'i':
                iterations = atoi(optarg);
                break;
            case 'o':
                output = optarg;
                break;
            default:
                printf("./benchmark -t <threads> -v <nb_nodes> -m <bfs_mode> -f <binary> -i <iterations> -s<is_symmetric>\n");
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
        printf("./benchmark -t <threads> -v <nb_nodes> -m <bfs_mode> -f <binary> -i <iterations> -s<is_symmetric>\n");
        return 0;
    }

    if(THREADS == 0) THREADS = 1;
    {
        //initialize arena
        std::vector<oneapi::tbb::numa_node_id> numa_nodes = oneapi::tbb::info::numa_nodes();
        arena.initialize(task_arena::constraints(numa_nodes[0], THREADS));
    }
    pinning_observer obs(arena,0); //bind threads

    printf("Run with: %u worker threads, %lu nodes, binary: %s\n", arena.max_concurrency(), NB_NODES, binary);


    uint64_t start, stop;
    rdtscll(start); 
    graph = create_graph(binary); 
    rdtscll(stop);
    printf("Create graph: %fs\n",((float)(stop-start))/(float)(get_cpu_freq()));
    if(output != nullptr){
        rdtscll(start);
        write_to_file(graph,output);
        rdtscll(stop);
        printf("output graph: %fs\n",((float)(stop-start))/(float)(get_cpu_freq()));
        return 0;
    }
    /*
    int perf_fd = perf_count_setup(PERF_TYPE_HARDWARE,PERF_COUNT_HW_CACHE_MISSES,0,-1 );
    struct read_group_format perf_data;
    uint64_t cache_misses_start, cache_misses_end;
    read(perf_fd, (void*)&perf_data, sizeof(struct read_group_format));
    assert(perf_data.nr == 1);
    cache_misses_start = perf_data.values[0].value;
    */
#ifdef LABOS
    //if(system("/home/blepers/linux/tools/perf/perf  record -e cycles:pp -F 9997  -a  2>&1 &")){}
#ifdef PMEM
    printf("PMEM\n");
    if(system("/home/blepers/linux/tools/perf/perf  stat -e mem_inst_retired.lock_loads,MEM_LOAD_RETIRED.LOCAL_PMM,OCR.ALL_PF_RFO.PMM_HIT_LOCAL_PMM.SNOOP_NONE,OCR.DEMAND_RFO.PMM_HIT_LOCAL_PMM.SNOOP_NONE  -a  2>&1 &")){}
#else
    printf("DRAM\n");
    if(system("/home/blepers/linux/tools/perf/perf  stat -e mem_inst_retired.lock_loads,MEM_LOAD_L3_MISS_RETIRED.LOCAL_DRAM,OCR.DEMAND_RFO.L3_MISS_LOCAL_DRAM.SNOOP_NONE,OCR.ALL_PF_RFO.L3_MISS_LOCAL_DRAM.SNOOP_NONE   -a  2>&1 &")){}
#endif
#else
    if(system("perf record -e cycles:pp -F 997  -a  2>&1 &")){}
#endif
    sleep(2);
    bfs_hub(graph,0,mode);
    //pr_hub(graph,iterations,mode);
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
