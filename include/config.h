#ifndef __CONFIG_H__
#define __CONFIG_H__
#include <cstdint>
#include <cstdlib>
#include "util.h"


typedef uint32_t EdgeIndexType;

typedef struct edge{ 
    EdgeIndexType src, dst;
}edge_t;


class Graph{
public:
    bool symmetric;
    size_t nb_nodes;
    size_t nb_edges;
    uint32_t* out_edges;
    size_t* out_edge_offsets; //prefix sum of node degree with length (nb_nodes+1)
    uint32_t* in_edges;
    size_t* in_edge_offsets; //prefix sum of node degree with length (nb_nodes+1)

    ~Graph(){
        free(out_edges);
        free(out_edge_offsets);
    }
};



/*
 * global variables
 *
 */

extern int THREADS;
extern uint64_t NB_NODES;
extern uint64_t NB_EDGES;
extern Graph* graph;
extern bool SYMMETRIC;
#endif

