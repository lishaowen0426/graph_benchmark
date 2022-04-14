#include "pagerank.h"
#include "config.h"
#include "util.h"
#include <string.h>
#include "oneapi/tbb.h"
using namespace oneapi::tbb;
typedef uint32_t PROP_TY;
PROP_TY* rank;
PROP_TY* prev;

void pr_hub(Graph* graph, int iteration,  int opt){
    assert(iteration != 0);
    
    rank = (PROP_TY*)prop_malloc(NB_NODES*sizeof(PROP_TY));
    prev = (PROP_TY*)prop_malloc(NB_NODES*sizeof(PROP_TY));
    memset(rank, 0, NB_NODES*sizeof(PROP_TY));
    memset(prev, 0, NB_NODES*sizeof(PROP_TY));


    parallel_for(blocked_range<size_t>(0, NB_NODES),
            [&](const blocked_range<size_t>& r){
                for(size_t i = r.begin(); i != r.end(); i++){
                    prev[i] = 4;
                }
            }
            );

    uint64_t start, stop;
    rdtscll(start);
    switch(opt){
        case 0:
            pr_push(graph, iteration);
            break;
        case 1:
            pr_pull(graph, iteration);
            break;
        default:
            die("How to pagerank?\n");
    }
    rdtscll(stop);
    printf("PR-%s time, iteration: %d,  %lu ( %fs )\n",((opt==0)?"push":"pull"),iteration, stop - start, ((float)(stop - start))/(float)get_cpu_freq());


}

void pr_push(Graph* graph, int iteration){
   int it = 0;
   while(it++ < iteration){
        parallel_for(blocked_range<size_t>(0,NB_NODES),
            [&](const blocked_range<size_t>& r){
                for(size_t i = r.begin(); i != r.end(); i++){
                    size_t beg = graph->out_edge_offsets[i];
                    size_t end = graph->out_edge_offsets[i+1];
                    PROP_TY delta = prev[i]/2;
                    for(;beg < end; beg++){
                        size_t dst = graph->out_edges[beg];
                        atomicAdd(&(rank[dst]), delta);
                    }
                } 
            }); 

        parallel_for(blocked_range<size_t>(0,NB_NODES),
            [&](const blocked_range<size_t>& r){
                for(size_t i = r.begin(); i != r.end(); i++){
                    prev[i] = rank[i]/2;
                    rank[i] = 0;
                }
            }); 
   }

}
void pr_pull(Graph* graph, int iteration){}
