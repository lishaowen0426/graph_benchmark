#ifndef __BFS_H_
#define __BFS_H_

#include "config.h"




void bfs_hub(Graph* graph, uint32_t start, int opt);

void bfs_push(Graph* graph, uint32_t start);
void bfs_pull(Graph* graph, uint32_t start);
void bfs_pushpull(Graph* graph, uint32_t start);





#endif
