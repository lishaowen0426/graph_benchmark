#include "config.h"
#include <pthread.h>
#include "util.h"

void pinning_observer::on_scheduler_entry(bool worker){
    int s;
    int id = oneapi::tbb::this_task_arena::current_thread_index()%THREADS;
    CPU_ZERO(&mask);
    CPU_SET(id,&mask);
    s = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &mask);
    if(s != 0){
        printf("id: %d\n",id);
        handle_error_en(s,"set affinity failed");
    }
}


void pinning_observer::on_scheduler_exit(bool worker){


}
