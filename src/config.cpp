#include "config.h"
#include <pthread.h>
#include "util.h"
#include <string.h>

void pinning_observer::on_scheduler_entry(bool worker){
    int s;
    int id = oneapi::tbb::this_task_arena::current_thread_index()%THREADS;
    id = base + id;
    cpu_set_t mask;
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


void thread_buffer::init(size_t c){
    next = 0;
    
    buffer = (uint32_t*)malloc(c * sizeof(uint32_t));
    capacity = c;
}



void thread_buffer::push(uint32_t id){
    
    buffer[next] = id;
    next++;
}

void thread_buffer::transfer( thread_buffer& dst){
    if(next == 0) return;
    size_t dst_id = __atomic_fetch_add(&(dst.next),this->next, __ATOMIC_RELAXED);
    memcpy(dst.buffer+dst_id, this->buffer, next*sizeof(uint32_t));

    next = 0;

}
