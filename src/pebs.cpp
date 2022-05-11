#include "pebs.h"
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "util.h"
#include <string.h>
#include <sys/mman.h>

#define SAMPLE_PERIOD 99
#define PERF_PAGES  (1+ (1<<16))
#define PAGE_SIZE 4096

std::vector<uint64_t> sampled_address;
struct perf_event_mmap_page** cache_event_pages;
int* cache_event_fd;

static const int CACHE_EVENTS = 1;

#ifdef PMEM
static uint64_t cache_configs[CACHE_EVENTS] = {0x80d1}; //MEM_LOAD_RETIRED.LOCAL_PMM
#else
static uint64_t cache_configs[CACHE_EVENTS] = {0x20d1}; //MEM_LOAD_L3_MISS_RETIRED.LOCAL_DRAM
#endif

auto loc = [](int core, int event){return core*CACHE_EVENTS + event;};
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


int perf_sample_setup(struct perf_event_mmap_page** page, uint64_t type, uint64_t config, uint64_t config1, uint32_t cpu, int group_fd){
    
    struct perf_event_attr attr;
    memset(&attr, 0, sizeof(struct perf_event_attr));

    attr.type = type;
    attr.size = sizeof(struct perf_event_attr);
    attr.config = config;
    attr.config1 = config1;
    attr.sample_period = SAMPLE_PERIOD;

    attr.sample_type = PERF_SAMPLE_IP | PERF_SAMPLE_ADDR;
    attr.disabled = 0;
    attr.exclude_kernel = 1;
    attr.exclude_callchain_kernel = 1;
    attr.exclude_callchain_user = 1;
    attr.precise_ip = 1;

    int fd = perf_event_open(&attr, -1/*pid*/, cpu, group_fd, 0/*flags*/ );
    if(fd == -1){
        perror("open failed");
        die("Open sample event failed\n"); 
    }

    void* addr = mmap(NULL, PAGE_SIZE*PERF_PAGES, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if(addr == nullptr){
        die("mmap sample page failed\n");
    }
    *page = (struct perf_event_mmap_page*)addr;
    return fd;

}


// setup event to sample LLC read, write, prefetch misses
void cache_event_setup(){
    const int NCORES = THREADS;

    cache_event_pages = (struct perf_event_mmap_page**)malloc(CACHE_EVENTS*NCORES*sizeof(void*));
    cache_event_fd = (int*)malloc(CACHE_EVENTS*NCORES*sizeof(int));
    

    
    for(int i = 0; i < NCORES; i++){
        for(int j = 0; j < CACHE_EVENTS; j++){
                cache_event_fd[loc(i,j)] = perf_sample_setup(&(cache_event_pages[loc(i,j)]),PERF_TYPE_RAW,cache_configs[j],0/*config1*/, i/*cpu*/, -1/*group_fd*/);
        }
    }    
    

}



void* cache_collect_sample(void* arg){
   const int NCORES = THREADS;
   while(!should_terminate){
        for(int i = 0; i < NCORES; i++){
            for(int j = 0; j < CACHE_EVENTS; j++){
                struct perf_event_mmap_page* p = cache_event_pages[loc(i,j)]; 
                char* pbuf = (char*)p + p->data_offset;
                __sync_synchronize();
                if(p->data_head == p->data_tail){
                    continue;
                }

                struct perf_event_header* ph = (struct perf_event_header*)(pbuf+(p->data_tail%p->data_size));
                struct perf_sample* ps;
                uint64_t vaddr;
                uint64_t ip;

                switch(ph->type){
                    case PERF_RECORD_SAMPLE:
                        ps = (struct perf_sample*)ph;
                        vaddr = ps->addr;
                        ip = ps->ip;
                        sampled_address.push_back(vaddr);
                        //printf("%p\n", ip);
                        break;
                    default:
                        break;
                }
                p->data_tail += ph->size;

            }
        }
   }
   return nullptr;
}



void launch_cache_collect(pthread_t* thread, int core, void* arg){
    pthread_attr_t attr;
    if(pthread_attr_init(&attr)){
        die("Init pthread attr failed\n");
    }

    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(core, &mask);
    if(pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &mask)){
        die("set CPU affinity mask failed\n");
    }
    sampled_address.reserve(10000);
    should_terminate = false;
    if(pthread_create(thread, &attr, cache_collect_sample, arg)){
        die("create cache collect thread failed\n");
    }
    return;
}
void terminate_cache_collect(pthread_t* thread){
    printf("%lu addresses\n", sampled_address.size());
    should_terminate = true;
    if(pthread_join(*thread,nullptr)){
        die("join failed\n");
    }
}
