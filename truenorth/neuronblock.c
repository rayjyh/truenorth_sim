#include "core.h"

#define NRNQUEUE_SIZE       256
#define NRNCALCULATE_DELAY  1//30
#define NRNCOMPUTE_DELAY    1//3
#define NRNPSEND_DELAY      1//1
#define NRNNINFO_DELAY      1//2

#define THRESHOLD_VOLT      30
#define BOTTOM_VOLT         0

int gens = 0;   // generated packets
int neuron_drops = 0;   // drops when neuron transports packet to router

void neuron_init (core* mycore) {

    neuron* nrn = &(mycore->nrn);
    memset(&(nrn->timer), 0, sizeof(token_t));
    queue_init (&(nrn->crq), NRNQUEUE_SIZE);
    queue_init (&(nrn->prq), NRNQUEUE_SIZE);
    queue_init (&(nrn->nrq), NRNQUEUE_SIZE);
    nrn->neuron_activate = 0;

    return;
}

void dxdy_compute (int coreno, int des, int* dx, int* dy) {
    *dx = (des%CHIP_LENGTH) - (coreno%CHIP_LENGTH);
    *dy = (des/CHIP_LENGTH) - (coreno/CHIP_LENGTH);
    return;
}

int neuron_compute (core* mycore, int coreno, int gclk, output* output) {

    neuron* nrn = &(mycore->nrn);
    int* timer = &(nrn->timer.in_timer);
    compute_info* cinfo = NULL;
    queue* crq = &(nrn->crq);
    queue* prq = &(nrn->prq);
    queue* nrq = &(nrn->nrq);
    int i;
    packet* pkt = NULL;

    // if queue is empty, return
    if (isempty (crq)) {
        return 0;
    }
    nrn->neuron_activate++;
    // waiting start
    if (*timer == 0) {
        cinfo = (compute_info*) dequeue (crq);
        if (cinfo->iscompute == 1)
            *timer = NRNCALCULATE_DELAY;
        else *timer = NRNCOMPUTE_DELAY;
        enqueue (crq, (void*)cinfo);
        return 0;
    }
    (*timer) -= 1;
    // if compute block have to wait more time, return
    if (*timer != 0) {
        return 0;
    }

    cinfo = (compute_info*) dequeue (crq);
    // when input spike location and synapse is equal, compute
    if (cinfo->iscompute == 1) {
        /*if (cinfo->ninfo.ntype == 0){
            for (i = 0; i < PIXEL_NUMBER; i++) {
                cinfo->ninfo.potential += cinfo->ninfo.synapse[i] * in_spikes[i];
            }
        }else {//if (cinfo->ninfo.ntype == 1){*/
            for (i = 0; i < AXON_NUMBER; i++) {
                cinfo->ninfo.potential += cinfo->ninfo.weight[i] * cinfo->ninfo.synapse[i] * cinfo->spike.spike[i];
            }
        //}
    }
    // if potential is over the threshold voltage, or the neuron is a spike generator,
    // send a packet to router
    if (cinfo->ninfo.potential >= THRESHOLD_VOLT || cinfo->ninfo.nopt == 1) {
        
        cinfo->ninfo.potential = BOTTOM_VOLT;
        for (int j = 0; j < cinfo->ninfo.num_dest; ++j) { //add multicasting
            pkt = (packet*) malloc (sizeof(packet));
            dxdy_compute (coreno, cinfo->ninfo.dest[j], &(pkt->dx), &(pkt->dy));
            pkt->spk.axonno = cinfo->ninfo.des_axon[j];
            pkt->spk.tick = cinfo->ninfo.tick;
            pkt->spk.input_idx = cinfo->input_idx;
            enqueue (prq, (void*)pkt);
        }
        //collect output if this is a output neuron
        if (cinfo->ninfo.ntype == 2){
            output->output[cinfo->input_idx][cinfo->ninfo.neuron_id] = 1;
        }
    }
    // if potential is lower than bottom voltage,
    cinfo->ninfo.potential -= cinfo->ninfo.leak;
    if (cinfo->ninfo.potential < BOTTOM_VOLT) {
        cinfo->ninfo.potential = BOTTOM_VOLT;
    }
    // send updated neuron_info to nrq
    enqueue (nrq, (void*)cinfo);
    return 0;
}

int send_packet_nrn_to_rtr (core* mycore) {

    neuron* nrn = &(mycore->nrn);
    int* timer = &(nrn->timer.nr_timer);
    queue* prq = &(nrn->prq);
    packet* pkt = NULL;

    // if queue is empty, return
    if (isempty (prq)) {
        return 0;
    }
    // waiting start
    if (*timer == 0) {
        *timer = NRNPSEND_DELAY;
        return 0;
    }
    (*timer) -= 1;
    // if packet sending block need more time, return;
    if (*timer != 0) {
        return 0;
    }
    // send packet to router
    pkt = (packet*) dequeue (prq);
    if (recieve_packet (&(mycore->rtr), pkt) == -1) {
        neuron_drops += 1;
    }
    gens++;
    return 0;
}

int send_ninfo_to_sram (core* mycore) {

    neuron* nrn = &(mycore->nrn);
    int* timer = &(nrn->timer.ns_timer);
    compute_info* cinfo = NULL;
    queue* nrq = &(nrn->nrq);

    // if queue is empty, return
    if (isempty (nrq)) {
        return 0;
    }
    // waiting start
    if (*timer == 0) {
        *timer = NRNNINFO_DELAY;
        return 0;
    }
    (*timer) -= 1;
    // if sram saving block have to wait more time, return
    if (*timer != 0) {
        return 0;
    }

    cinfo = (compute_info*) dequeue (nrq);
    memcpy ((void*)&(mycore->srm.ninfo[cinfo->neuron_no]), (void*)&(cinfo->ninfo), sizeof(neuron_info));
    free ((void*)cinfo);
    return 0;
}

void neuron_advance (core* mycore, int coreno, int gclk, output* output) {
    
    neuron_compute (mycore, coreno, gclk, output);
    send_packet_nrn_to_rtr (mycore);
    send_ninfo_to_sram (mycore);
    
    return;
}
