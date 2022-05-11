#ifndef __PEBS_H__
#define __PEBS_H__
#include <cstdint>
#include <linux/perf_event.h>
#include "config.h"
#include <pthread.h>
#include <vector>

extern bool should_terminate;
extern std::vector<uint64_t> sampled_address;
struct perf_sample{
    struct perf_event_header header;
    uint64_t ip;
    uint64_t addr;
};

struct read_group_format {
    uint64_t nr;
    struct {
        uint64_t value;
        uint64_t id;
    }values[1];

};


int perf_count_setup(uint64_t type ,uint64_t config, uint64_t config1, int group_fd);







int perf_sample_setup(struct perf_event_mmap_page** page, uint64_t type, uint64_t config, uint64_t config1, uint32_t cpu, int group_fd);

void cache_event_setup();


void launch_cache_collect(pthread_t* thread,int core, void* arg);
void terminate_cache_collect(pthread_t* thread);


#endif
