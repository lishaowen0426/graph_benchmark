#include "bfs.h"
#include "util.h"
#include "oneapi/tbb/concurrent_vector.h"
#include <string.h>
#include "oneapi/tbb.h"
#include "oneapi/tbb/combinable.h"
#include "oneapi/tbb/scalable_allocator.h"
#include "unistd.h"

using namespace oneapi::tbb;

typedef uint8_t PROP_TY;

concurrent_vector<uint32_t,scalable_allocator<uint32_t>> frontier;
concurrent_vector<uint32_t, scalable_allocator<uint32_t>> frontier_next;
PROP_TY* visited;
bool* in_frontier;
bool* in_frontier_next;
//static_partitioner r;



void bfs_hub(Graph* graph, uint32_t source, int opt){
    
    visited = (PROP_TY*)prop_malloc(graph->nb_nodes*sizeof(PROP_TY));
    memset(visited,0, graph->nb_nodes*sizeof(PROP_TY));

    frontier.reserve(NB_NODES);
    frontier_next.reserve(NB_NODES);
        
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
    //prop_free(visited);
    

}



void bfs_push(Graph* graph, uint32_t start){
    using sz_t =decltype(frontier)::size_type;
    size_t iterations = 0;
    sz_t to_process = 0;
    visited[start] = 1;
    
    thread_buffer frontier;
    frontier.init(NB_NODES);
    frontier.push(start);
    thread_buffer* buffers = (thread_buffer*)malloc(THREADS*sizeof(thread_buffer));
    for(size_t i = 0; i < THREADS; i++) buffers[i].init(NB_NODES);

    uint32_t* const out_edges = graph->out_edges;
    size_t* const out_edge_offsets = graph->out_edge_offsets;

    while( (to_process=frontier.size())!= 0){
        iterations++;
        auto f = [&](){parallel_for(blocked_range<sz_t>(0, to_process ),
        [&](const blocked_range<sz_t>& r){
            thread_buffer& local_buffer = buffers[gettid()%THREADS]; 
            register size_t i = r.begin();
            const size_t stop = r.end(); 
            for(; i <stop; i++){
                uint32_t src = frontier[i];
                register size_t beg = out_edge_offsets[src];
                register const size_t end = out_edge_offsets[src+1];
                for(;beg < end; beg++){
                    uint32_t dst = out_edges[beg];

                    if(visited[dst] == 0){
                        if(__sync_bool_compare_and_swap(&(visited[dst]), 0,1 )){
                            local_buffer.push(dst);
                        } 
                    }

                }
            }
        } );};
        arena.execute(f);
        frontier.clear();
        for(size_t i = 0; i < THREADS; i++) buffers[i].transfer(frontier);
    }
    size_t v =0;
    for(size_t i = 0; i < NB_NODES; i++) v += visited[i];
    printf("visited: %lu\n",v);
}
void bfs_pull(Graph* graph, uint32_t start){
    in_frontier = (bool*)malloc(NB_NODES*sizeof(bool));
    in_frontier_next = (bool*)malloc(NB_NODES*sizeof(bool));
    size_t iterations = 0;
    bool cont;
    in_frontier[start] = true;
    visited[start] = 1;

    do{
        cont = false;
        iterations++;
        auto f = [&](){parallel_for(blocked_range<size_t>(0,NB_NODES),
                [&](const blocked_range<size_t>& r){
                    bool local_cont = false;
                    for(size_t i = r.begin(); i != r.end(); i++){
                        uint32_t dst = i;
                        if(visited[dst] ) continue;
                        size_t beg = graph->in_edge_offsets[dst];
                        size_t end = graph->in_edge_offsets[dst+1];
                        for(;beg < end; beg++){
                            uint32_t src = graph->in_edges[beg];
                            if(in_frontier[src]){
                                local_cont = true;
                                visited[dst] = 1;
                                in_frontier_next[dst] = true;
                                break;
                            }
                        }
                    }
                    if(local_cont) cont = local_cont;
                });};
        arena.execute(f);
        memset(in_frontier, false, NB_NODES*sizeof(bool));
        std::swap(in_frontier, in_frontier_next);
    }while(cont);
    
}
void bfs_pushpull(Graph* graph, uint32_t start){
    using sz_t =decltype(frontier)::size_type;
    combinable<uint64_t> edges_in_frontier;
    {
       // push 
        sz_t to_process = 0;
        frontier.push_back(start);
        visited[start] = 1;
        size_t explored =0;
        while( (to_process=frontier.size())!= 0){
            size_t unexplored = NB_EDGES - explored;
            if(edges_in_frontier.combine([](uint64_t a, uint64_t b){return a+b;}) > unexplored){
                goto PULL;
            }else{
                edges_in_frontier.clear();
            }
            auto f = [&](){parallel_for(blocked_range<sz_t>(0, to_process),
            [&](const blocked_range<sz_t>& r){
                uint64_t local_explored = 0;
                uint64_t local_edges_in_frontier = 0;
                for(size_t i = r.begin(); i != r.end(); i++){
                    uint32_t src = frontier[i];
                    size_t beg = graph->out_edge_offsets[src];
                    size_t end = graph->out_edge_offsets[src+1];
                    local_explored += end-beg;
                    for(;beg < end; beg++){
                        uint32_t dst = graph->out_edges[beg];
                        if(__sync_bool_compare_and_swap(&(visited[dst]), 0,1)){
                            frontier_next.push_back(dst);
                            local_edges_in_frontier += graph->out_edge_offsets[dst+1] - graph->out_edge_offsets[dst];
                        } 
                    }
                }
                edges_in_frontier.local() += local_edges_in_frontier;
                __atomic_add_fetch(&explored, local_explored, __ATOMIC_RELAXED);
            });};
            arena.execute(f);
            frontier.clear();
            frontier.swap(frontier_next);
        }
    }
    {
        //pull
PULL:
        in_frontier = (bool*)malloc(NB_NODES*sizeof(bool));
        in_frontier_next = (bool*)malloc(NB_NODES*sizeof(bool));
        auto f = [&](){parallel_for(blocked_range<sz_t>(0, frontier.size()),
            [&](const blocked_range<sz_t>& r){
                for(sz_t i = r.begin(); i != r.end(); i++) in_frontier[frontier[i]] = true;
            });};
        arena.execute(f);

        bool cont;
        do{
            cont = false;
            auto f =  [&](){parallel_for(blocked_range<size_t>(0,NB_NODES),
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
                                    local_cont = true;
                                    visited[dst] = 1;
                                    in_frontier_next[dst] = true;
                                    break;
                                }
                            }
                        }
                        if(local_cont) cont = local_cont;
                    });};
            arena.execute(f);
            memset(in_frontier, false, NB_NODES*sizeof(bool));
            std::swap(in_frontier, in_frontier_next);
        }while(cont);
    
    }

}
