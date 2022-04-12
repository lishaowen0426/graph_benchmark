#ifndef __LOADER_H__
#define __LOADER_H__

#include "config.h"



Graph* create_graph(const char* binary_file);


void count_degree(Graph* graph, edge_t* edges, int opt);

void sum_degree(Graph* graph);

void count_degree_0_sequential(Graph* graph, edge_t* edges);





void count_degree_1_atomic(Graph* graph, edge_t* edges);


void create_edges(Graph* graph, edge_t* edges);

#endif
