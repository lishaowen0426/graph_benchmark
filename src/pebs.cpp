#include "pebs.h"
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "util.h"
#include <string.h>
static int perf_event_open(struct perf_event_attr *attr, pid_t pid, int cpu, int group_fd, unsigned long flags){
    
    int ret;
    ret = syscall(__NR_perf_event_open, attr, pid, cpu, group_fd, flags);
    return ret;

}

 int perf_count_setup(uint64_t type ,uint64_t config, uint64_t config1, int group_fd){

     struct perf_event_attr attr;
     memset(&attr, 0, sizeof(struct perf_event_attr));

     attr.type = type;
     attr.size = sizeof(struct perf_event_attr);
     attr.config = config;
     attr.config1 = config1;
     attr.disabled = 0;
     attr.exclude_kernel = 1;
     attr.exclude_hv = 1;
     attr.exclude_callchain_kernel = 1;
     attr.exclude_callchain_user = 1;

     attr.read_format = PERF_FORMAT_GROUP;
     
     int fd = perf_event_open(&attr, 0, -1, group_fd, 0);
     if(fd == -1)
         die("open perf event failed\n");
     return fd;
 
 }
