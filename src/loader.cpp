#include "config.h"
#include <sys/stat.h>
#include <fcntl.h>
#include "util.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "loader.h"
#include "oneapi/tbb.h"
#include <algorithm>

using namespace oneapi::tbb;

size_t NB_EDGES;

Graph* create_graph(const char* binary_file){
    struct stat sb;
    int fd = open(binary_file, O_RDONLY);
    if(fd == -1){
        die("Cannot open graph binary file %s\n", binary_file);
    }
    if(posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL)){
        die("fadvise failed\n");
    }
    fstat(fd, &sb);
    printf("Binary file size: %lu\n", (uint64_t)sb.st_size);

    Graph* graph = (Graph*)graph_malloc(sizeof(Graph));
    graph->nb_nodes = NB_NODES;
    graph->symmetric = SYMMETRIC; 
    assert((size_t)sb.st_size%sizeof(edge_t) == 0);
    graph->nb_edges = (size_t)sb.st_size/sizeof(edge_t);
    NB_EDGES = graph->nb_edges;
    printf("Number of edges: %lu\n", graph->nb_edges);
    edge_t* edges = (edge_t*)graph_malloc(graph->nb_edges*sizeof(edge_t));
    assert(edges != NULL);

    {
        //read in graph->edges
        ssize_t sz;
        size_t count = sb.st_size;
        char* cursor = (char*)edges;
        while((sz = read(fd, (void*)cursor, count)) != count && sz != 0){
            if(sz == -1)
                die("read failed\n");
            cursor += sz;
            count -= sz;
        }
    }
    
    count_degree(graph,edges,1);
    create_edges(graph,edges);
    graph_free(edges);
    return graph;
}


void count_degree(Graph* graph, edge_t* edges, int opt){
    graph->out_edge_offsets = (size_t*)graph_malloc((NB_NODES+1)*sizeof(size_t));
    if(graph->symmetric)
        graph->in_edge_offsets = (size_t*)graph_malloc((NB_NODES+1)*sizeof(size_t));

    switch(opt){
        case 0:
            count_degree_0_sequential(graph, edges);
            break;
        case 1:
            count_degree_1_atomic(graph, edges);
            break;
        default:
            die("how do you want to count?\n");

    }
    sum_degree(graph);
}

void sum_degree(Graph* graph){
    {
        //out edge
        size_t prev = 0;
        size_t tmp;
        for(size_t i= 0; i < NB_NODES; i++ ){
            tmp = graph->out_edge_offsets[i];
            graph->out_edge_offsets[i] = prev;
            prev += tmp;
        }
        assert(prev == NB_EDGES);
        graph->out_edge_offsets[NB_NODES] = prev;
    }

    if(graph->symmetric){
        
        size_t prev = 0;
        size_t tmp;
        for(size_t i= 0; i < NB_NODES; i++ ){
            tmp = graph->in_edge_offsets[i];
            graph->in_edge_offsets[i] = prev;
            prev += tmp;
        }
        assert(prev == NB_EDGES);
        graph->in_edge_offsets[NB_NODES] = prev;
    }
}

void count_degree_0_sequential(Graph* graph, edge_t* edges){

    for(size_t i = 0; i < NB_EDGES; i++){
        graph->out_edge_offsets[edges[i].src]++;
        if(graph->symmetric)
            graph->in_edge_offsets[edges[i].dst]++;
    }


}





void count_degree_1_atomic(Graph* graph, edge_t* edges){
    parallel_for(blocked_range<size_t>(0, NB_EDGES),
        [=](const blocked_range<size_t>& r){
            for(size_t i = r.begin(); i != r.end(); i++){
                __sync_fetch_and_add(&(graph->out_edge_offsets[edges[i].src]),1);
                if(graph->symmetric)
                    __sync_fetch_and_add(&(graph->in_edge_offsets[edges[i].dst]),1);
            }
        }
            );
}


void create_edges(Graph* graph, edge_t* edges){
    uint32_t* out_offsets = (uint32_t*)graph_malloc(NB_NODES*sizeof(uint32_t));
    graph->out_edges = (uint32_t*)graph_malloc(NB_EDGES*sizeof(uint32_t));
    memset((void*)out_offsets,0, NB_NODES*sizeof(uint32_t));
    uint32_t* in_offsets;
    if(graph->symmetric){ 
        in_offsets = (uint32_t*)graph_malloc(NB_NODES*sizeof(uint32_t));
        graph->in_edges = (uint32_t*)graph_malloc(NB_EDGES*sizeof(uint32_t));
        memset((void*)in_offsets,0, NB_NODES*sizeof(uint32_t));
    }

    parallel_for(blocked_range<size_t>(0,NB_EDGES),
        [=](const blocked_range<size_t>& r){
            for(size_t i = r.begin(); i != r.end(); i++){
                uint32_t src = edges[i].src;
                uint32_t o = __atomic_fetch_add(&(out_offsets[src]),1,__ATOMIC_RELAXED);
                graph->out_edges[graph->out_edge_offsets[src] + o] = edges[i].dst;

                if(graph->symmetric){
                    uint32_t dst = edges[i].dst;
                    o = __atomic_fetch_add(&(in_offsets[dst]),1,__ATOMIC_RELAXED);
                    graph->in_edges[graph->in_edge_offsets[dst] + o] = edges[i].src;

                }
            }
        }
            );
}

