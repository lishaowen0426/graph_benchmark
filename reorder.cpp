#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <algorithm>
#include <stdio.h>
#include <assert.h>
#include <fstream>
#include "oneapi/tbb.h"


using namespace oneapi::tbb;

typedef uint32_t EdgeIndexType;
#define die(msg, args...) \
do {                         \
            fprintf(stderr,"(%s,%d) " msg "\n", __FUNCTION__ , __LINE__, ##args); \
            assert(0);                 \
         } while(0)


typedef struct edge{ 
    EdgeIndexType src, dst;
}edge_t;

size_t NB_NODES;
size_t NB_EDGES;
uint32_t* out_edge_offsets;
uint32_t* out_edges;
uint32_t* mapping;
uint32_t* ordered;
bool ascending;
bool maintain;


void  sort_by_degree(const char* input, const char* output){
    struct stat sb;
    int fd = open(input, O_RDONLY);
    if(fd == -1){
        die("Cannot open graph binary file %s\n", input);
    }
    if(posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL)){
        die("fadvise failed\n");
    }
    fstat(fd, &sb);
    printf("Binary file size: %lu\n", (uint64_t)sb.st_size);
    NB_EDGES = (size_t)sb.st_size/sizeof(edge_t);
    printf("Number of edges: %lu\n", NB_EDGES);
    edge_t* edges = (edge_t*)malloc(NB_EDGES*sizeof(edge_t));
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

    {
        out_edge_offsets = (uint32_t*)malloc(NB_NODES*sizeof(uint32_t));
        //count degree 
        parallel_for(blocked_range<size_t>(0, NB_EDGES),
            [=](const blocked_range<size_t>& r){
                for(size_t i = r.begin(); i != r.end(); i++){
                    __sync_fetch_and_add(&(out_edge_offsets[edges[i].src]),1);
                }
            });

    }
    //for(size_t i = 0; i < NB_NODES; i++) printf("%lu: %u\n", i, out_edge_offsets[i]);
    {
        //sort
        ordered = (uint32_t*)malloc(NB_NODES*sizeof(uint32_t));
        parallel_for(blocked_range<size_t>(0,NB_NODES),
            [&](const blocked_range<size_t>& r){
               for(size_t i = r.begin(); i != r.end(); i++){
                    ordered[i] = i;
               }
            });

        std::sort(ordered, ordered+NB_NODES, 
            [&](uint32_t a, uint32_t b){
                if(ascending) return out_edge_offsets[a] < out_edge_offsets[b];
                else return out_edge_offsets[a] > out_edge_offsets[b];
            }
                );
        
        mapping = (uint32_t*)malloc(NB_NODES*sizeof(uint32_t));
        size_t count = maintain? 1: 0;
        for(size_t i = 0; i < NB_NODES; i++){
            if(maintain && ordered[i] == 0) mapping[0] = 0;
            else {
                mapping[ordered[i]] = count;
                count++;
            } 
        }
    }
    /*
    for(size_t i = 0; i < NB_NODES; i++) printf("%u, ", ordered[i]);
    printf("\n");
    for(size_t i = 0; i < NB_NODES; i++) printf("%u, ", mapping[i]);
    printf("\n");

    for(size_t i = 0; i < NB_EDGES; i++) printf("( %u -> %u)\n", edges[i].src, edges[i].dst);
    */
    {
        //re-write
        parallel_for(blocked_range<size_t>(0, NB_EDGES),
            [&](const blocked_range<size_t>& r){
                for(size_t i = r.begin(); i != r.end(); i++){
                    edges[i].src = mapping[edges[i].src];
                    edges[i].dst = mapping[edges[i].dst];
                }
            });

    }
    /*
    printf("rewrite\n");
    for(size_t i = 0; i < NB_EDGES; i++) printf("( %u -> %u)\n", edges[i].src, edges[i].dst);
    */
    {
        printf("output\n");
        std::ofstream ofs (output, std::ofstream::out|std::ofstream::binary);
        ofs.write((const char*)edges, NB_EDGES*sizeof(edge_t));
        ofs.flush();
        ofs.close();
        
        std::ofstream ofs_info (std::string(output)+std::string(".ini"), std::ofstream::out);
        ofs_info<<"Nodes: " << NB_NODES<<"\n"<<"Edges: "<<NB_EDGES<<"\n";
        ofs_info.flush();
        ofs_info.close();

    }

}

int main(int argc, char** argv){
    
    const char* input;
    const char* output;
    bool debug = false;
    int c;

    ascending = false;
    maintain = false;
    while((c = getopt(argc, argv, "i:o:v:adm"))!=-1){
        switch(c){
            case 'i':
                input = optarg;
                break;
            case 'o':
                output = optarg;
                break;
            case 'a':
                ascending = true;
                break;
            case 'v':
                NB_NODES = atol(optarg);
                break;
            case 'd':
                debug = true;
                break;
            case 'm':
                maintain = true;
                break;
            default:
                printf("./reorder -i <input> -o <output> -v <nodes> -a<ascending> -m<maintain 0>");
                return 0;
        }
    }

    if(debug){
        input = "/home/lsw/graph_data/small_directed_graph_binary";
        output = "output";
        NB_NODES = 16;
    }

    assert(input != NULL && output != NULL&& NB_NODES !=0);
    sort_by_degree(input,output);

}
