#include <iostream>
#include "config.h"
#include "loader.h"
#include <unistd.h>
#include "bfs.h"

int THREADS;
size_t NB_NODES;
bool SYMMETRIC;
Graph* graph;

int main( int argc, char** argv){

    int c;
    const char* binary;
    bool debug = false;
    int mode;
    SYMMETRIC = false;
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
        THREADS = 4;
        NB_NODES = 16;
        binary = "/home/lsw/graph_data/small_directed_graph_binary";
        SYMMETRIC = true;
    }
    else if(THREADS==0 || NB_NODES == 0){
        printf("./benchmark -t <threads> -v <nb_nodes> -m <bfs_mode> -f <binary> -s<is_symmetric>\n");
        return 0;
    }
    printf("Run with: %d threads, %lu nodes, binary: %s\n", THREADS, NB_NODES, binary);
    
    graph = create_graph(binary); 
    bfs_hub(graph,0,mode);

    return 0;
}
