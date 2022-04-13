#include "bfs.h"
#include "util.h"
#include "oneapi/tbb/concurrent_vector.h"
#include <string.h>
#include "oneapi/tbb.h"
#include "oneapi/tbb/combinable.h"
using namespace oneapi::tbb;

concurrent_vector<uint32_t> frontier;
concurrent_vector<uint32_t> frontier_next;
bool* visited;
bool* in_frontier;
bool* in_frontier_next;


void bfs_hub(Graph* graph, uint32_t source, int opt){
    
    visited = (bool*)prop_malloc(graph->nb_nodes*sizeof(bool));
    memset(visited,0, graph->nb_nodes*sizeof(bool));
        
    uint64_t start, stop;
    rdtscll(start);
    switch(opt){
        case 0:
            bfs_push(graph, source);
            break;
        case 1:
            bfs_pull(graph,source);
            break;
        case 2:
            bfs_pushpull(graph, source);
            break;
        default:
            die("How do you want to bfs?\n");
    }
    rdtscll(stop);
    printf("BFS-%s time %lu ( %fs )\n",((opt==0)?"push":((opt==1)?"pull":"push-pull")), stop - start, ((float)(stop - start))/(float)get_cpu_freq());
    free(visited);
    

}



void bfs_push(Graph* graph, uint32_t start){
    using sz_t =decltype(frontier)::size_type;
    size_t iterations = 0;
    sz_t to_process = 0;
    frontier.push_back(start);
    __atomic_test_and_set(&(visited[start]), __ATOMIC_RELAXED);

    combinable<uint64_t> connected;

    while( (to_process=frontier.size())!= 0){
        iterations++;
        //printf("Iterations: %lu, to_process: %lu\n",iterations, to_process);
        parallel_for(blocked_range<sz_t>(0, to_process),
        [&](const blocked_range<sz_t>& r){
            uint64_t local_connected = 0;
            for(size_t i = r.begin(); i != r.end(); i++){
                uint32_t src = frontier[i];
                size_t beg = graph->out_edge_offsets[src];
                size_t end = graph->out_edge_offsets[src+1];
                for(;beg < end; beg++){
                    uint32_t dst = graph->out_edges[beg];
                    if(!__atomic_test_and_set(&(visited[dst]), __ATOMIC_RELAXED)){
                        frontier_next.push_back(dst);
                        local_connected++;
                    } 
                }
            }
            connected.local() += local_connected;
        });
        frontier.clear();
        frontier.swap(frontier_next);
    }
    uint64_t total_connected = connected.combine(
        [](uint64_t a, uint64_t b){
            return a+b;
        });

    printf("push: %lu\n", total_connected);
}
void bfs_pull(Graph* graph, uint32_t start){
    in_frontier = (bool*)malloc(NB_NODES*sizeof(bool));
    in_frontier_next = (bool*)malloc(NB_NODES*sizeof(bool));
    size_t iterations = 0;
    bool cont;
    in_frontier[start] = true;
    visited[start] = true;

    combinable<uint64_t> connected;
    do{
        cont = false;
        iterations++;
        parallel_for(blocked_range<size_t>(0,NB_NODES),
                [&](const blocked_range<size_t>& r){
                    bool local_cont = false;
                    uint64_t local_connected = 0;
                    for(size_t i = r.begin(); i != r.end(); i++){
                        uint32_t dst = i;
                        if(visited[dst]) continue;
                        size_t beg = graph->in_edge_offsets[dst];
                        size_t end = graph->in_edge_offsets[dst+1];
                        for(;beg < end; beg++){
                            uint32_t src = graph->in_edges[beg];
                            if(in_frontier[src]){
                                local_connected += 1;
                                local_cont = true;
                                visited[dst] = true;
                                in_frontier_next[dst] = true;
                                break;
                            }
                        }
                    }
                    if(local_cont) cont = local_cont;
                    connected.local() += local_connected;
                });
        memset(in_frontier, false, NB_NODES*sizeof(bool));
        std::swap(in_frontier, in_frontier_next);
    }while(cont);
    
    uint64_t total_connected = connected.combine(
        [](uint64_t a, uint64_t b){
            return a+b;
        });
    printf("pull: %lu\n",total_connected);
}
void bfs_pushpull(Graph* graph, uint32_t start){
    using sz_t =decltype(frontier)::size_type;
    combinable<uint64_t> connected;
    combinable<uint64_t> edges_in_frontier;
    {
       // push 
        sz_t to_process = 0;
        frontier.push_back(start);
        __atomic_test_and_set(&(visited[start]), __ATOMIC_RELAXED);

        size_t explored =0;
        while( (to_process=frontier.size())!= 0){
            size_t unexplored = NB_EDGES - explored;
            if(edges_in_frontier.combine([](uint64_t a, uint64_t b){return a+b;}) > unexplored){
                goto PULL;
            }else{
                edges_in_frontier.clear();
            }
            parallel_for(blocked_range<sz_t>(0, to_process),
            [&](const blocked_range<sz_t>& r){
                uint64_t local_connected = 0;
                uint64_t local_explored = 0;
                uint64_t local_edges_in_frontier = 0;
                for(size_t i = r.begin(); i != r.end(); i++){
                    uint32_t src = frontier[i];
                    size_t beg = graph->out_edge_offsets[src];
                    size_t end = graph->out_edge_offsets[src+1];
                    local_explored += end-beg;
                    for(;beg < end; beg++){
                        uint32_t dst = graph->out_edges[beg];
                        if(!__atomic_test_and_set(&(visited[dst]), __ATOMIC_RELAXED)){
                            frontier_next.push_back(dst);
                            local_edges_in_frontier += graph->out_edge_offsets[dst+1] - graph->out_edge_offsets[dst];
                            local_connected++;
                        } 
                    }
                }
                connected.local() += local_connected;
                edges_in_frontier.local() += local_edges_in_frontier;
                __atomic_add_fetch(&explored, local_explored, __ATOMIC_RELAXED);
            });
            frontier.clear();
            frontier.swap(frontier_next);
        }
        goto OUT;
    }
    {
        //pull
PULL:
        in_frontier = (bool*)malloc(NB_NODES*sizeof(bool));
        in_frontier_next = (bool*)malloc(NB_NODES*sizeof(bool));
        parallel_for(blocked_range<sz_t>(0, frontier.size()),
            [&](const blocked_range<sz_t>& r){
                for(sz_t i = r.begin(); i != r.end(); i++) in_frontier[frontier[i]] = true;
            });


        bool cont;
        do{
            cont = false;
            parallel_for(blocked_range<size_t>(0,NB_NODES),
                    [&](const blocked_range<size_t>& r){
                        bool local_cont = false;
                        uint64_t local_connected = 0;
                        for(size_t i = r.begin(); i != r.end(); i++){
                            uint32_t dst = i;
                            if(visited[dst]) continue;
                            size_t beg = graph->in_edge_offsets[dst];
                            size_t end = graph->in_edge_offsets[dst+1];
                            for(;beg < end; beg++){
                                uint32_t src = graph->in_edges[beg];
                                if(in_frontier[src]){
                                    local_connected += 1;
                                    local_cont = true;
                                    visited[dst] = true;
                                    in_frontier_next[dst] = true;
                                    break;
                                }
                            }
                        }
                        if(local_cont) cont = local_cont;
                        connected.local() += local_connected;
                    });
            memset(in_frontier, false, NB_NODES*sizeof(bool));
            std::swap(in_frontier, in_frontier_next);
        }while(cont);
    
    }
OUT:
    uint64_t total_connected = connected.combine([](uint64_t a, uint64_t b){return a+b;});
    printf("push-pull: %lu\n",total_connected);

}
