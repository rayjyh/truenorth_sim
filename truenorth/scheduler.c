#include "core.h"

#define AXONSEND_DELAY  1
#define SCHQUEUE_SIZE   1

static int sch_checker = 0; // variable for preventing dual activation checking of scheduler

void scheduler_init (scheduler* sch) {
   
    memset ((void*)&(sch->timer), 0, sizeof(scheduler_t));
    memset ((void*)sch->axons, 0, sizeof(axon) * AXON_NUMBER);
    memset ((void*)sch->input_idx, 0, sizeof(int) * TICK_NUMBER);
    queue_init (&(sch->rq), SCHQUEUE_SIZE);
    queue_init (&(sch->tq), SCHQUEUE_SIZE);
    
    sch->sch_activate = 0;

    return;
}

int save_spike_info (scheduler* sch) {
    
    spike_info* ptr;

    // if queue is empty, return
    if (isempty (&(sch->rq))){
        return 0;
    }
    if (!sch_checker) {
        sch->sch_activate++;
        sch_checker = 1;
    }
    // save spike_info into local sram(axons)
    ptr = (spike_info*) dequeue (&(sch->rq));
    sch->axons[ptr->tick].spike[ptr->axonno] = 1;
    sch->input_idx[ptr->tick] = ptr->input_idx;
    free (ptr);
    return 0;
}

int send_axon_to_token (core* mycore) {
    
    scheduler* sch = &(mycore->sch);
    queue* myqueue = &(sch->tq);
    int* timer = &(sch->timer.st_timer);
    sch_request* rqst;
    axon* ptr;
    axon* src;

    // if there is no request from router, return
    if (isempty (myqueue)) {
        return 0;
    }
    if (!sch_checker) {
        sch->sch_activate++;
        sch_checker = 1;
    }
    // waiting start
    if (*timer == 0) {
        *timer = AXONSEND_DELAY;
        return 0;
    }
    (*timer) -= 1;
    // if scheduler have to wait more time, return
    if (*timer != 0) {
        return 0;
    }
    // send axon to local TokenController
    rqst = (sch_request*) dequeue (myqueue);
    ptr = (axon*) malloc (sizeof(axon));
    src = &(sch->axons[rqst->tick]);
    memcpy ((void*)ptr, (void*)src, sizeof(axon));
    memset ((void*)src, 0, sizeof(axon));
    free (rqst);
    if (mycore->tkn.input != NULL) {
        return -1;
    }
    mycore->tkn.input = ptr;
    mycore->tkn.input_idx = sch->input_idx[rqst->tick];
    //(mycore->tkn.state) += 1;
    return 0;
}

void scheduler_advance (core* mycore) {
    
    scheduler* sch = &(mycore->sch);

    sch_checker = 0;
    save_spike_info (sch);
    send_axon_to_token (mycore);

    return;
}