#ifndef __PEBS_H__
#define __PEBS_H__
#include <cstdint>
#include <linux/perf_event.h>

struct read_group_format {
    uint64_t nr;
    struct {
        uint64_t value;
        uint64_t id;
    }values[1];

};


int perf_count_setup(uint64_t type ,uint64_t config, uint64_t config1, int group_fd);


#endif
