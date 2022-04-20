#ifndef __CONFIG_H__
#define __CONFIG_H__
#include <sched.h>
#include <cstdint>
#include <cstdlib>
#include "util.h"
#include "oneapi/tbb/task_arena.h"
#include "oneapi/tbb/partitioner.h"
#include "oneapi/tbb/task_scheduler_observer.h"
#ifdef PMEM
#include "ralloc.hpp"
#endif


#define graph_malloc __malloc
#define graph_free __free
#define prop_malloc __malloc
#define prop_free __free

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

class pinning_observer : public oneapi::tbb::task_scheduler_observer {
public:

    pinning_observer( oneapi::tbb::task_arena &a )
        : oneapi::tbb::task_scheduler_observer(a) {
        observe(true); // activate the observer
    }
    void on_scheduler_entry( bool worker ) override;
    void on_scheduler_exit( bool worker ) override ;
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
extern oneapi::tbb::task_arena arena;
#endif

