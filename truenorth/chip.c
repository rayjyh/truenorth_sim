#include "core.h"
/*
void read_input_spikes(int** input, char* ch) {
    FILE* fp = open("./test.txt",O_RDONLY, 0);
    if(fp == NULL)
    {
        printf("Cannot open the file.");
        exit(0);
    }
    for(int i = 0; i < TIME_SLOT; i++)
    {
        for(int j = 0; j < PIXEL_NUMBER; j++)
        {
            fscanf(fp,"%d",input[i][j]);
        }
    }

    fclose(fp);

    return;
}
*/

/*
void output_init(output* output) {
    memset ((void*)output->output, 0, sizeof(int) * INPUT_NUMBER * OUTPUT_NEURONS);
}
*/

void chip_init (chip* mychip, char* ch) {
    
    core* mycore;
    int i;
    /*int dest_core_0[3][3] = {{1,1,0},
                             {1,1,0},
                             {1,1,0}};
    int dest_core_1[3][3] = {{2,2,0},
                             {2,2,0},
                             {0,0,0}};
    int dest_core_2[3][3] = {{0,0,0},
                             {0,0,0},
                             {0,0,0}};
    int dest_core_3[3][3] = {{0,0,0},
                             {0,0,0},
                             {0,0,0}};
    int dest_axon_core_0[3][3] = {{1,2,0},
                             {1,2,0},
                             {1,2,0}};
    int dest_axon_core_1[3][3] = {{1,2,0},
                             {1,2,0},
                             {0,0,0}};
    int dest_axon_core_3[3][3] = {{0,0,0},
                             {0,0,0},
                             {0,0,0}};
    int dest_axon_core_4[3][3] = {{0,0,0},
                             {0,0,0},
                             {0,0,0}};*/
    int dest[CHIP_LENGTH*CHIP_LENGTH][NEURONS][MAX_DEST] = {{{1,1,0},{1,1,0},{1,1,0}},
                                                            {{2,2,0},{2,2,0},{0,0,0}},
                                                            {{0,0,0},{0,0,0},{0,0,0}},
                                                            {{0,0,0},{0,0,0}, {0,0,0}}};//4 cores, 3 neurons per core, 3 destinations per neuron
    int dest_axon[CHIP_LENGTH*CHIP_LENGTH][NEURONS][MAX_DEST] = {{{1,2,0},{1,2,0},{1,2,0}},
                                                                 {{1,2,0},{1,2,0},{0,0,0}},
                                                                 {{0,0,0},{0,0,0},{0,0,0}},
                                                                 {{0,0,0},{0,0,0},{0,0,0}}};
    int num_dest[CHIP_LENGTH*CHIP_LENGTH][NEURONS] = {{2,2,2},
                                                      {2,2,0},
                                                      {0,0,0},
                                                      {0,0,0}};
    int ntype[CHIP_LENGTH*CHIP_LENGTH][NEURONS] = {{0,0,0},
                                                   {1,1,0},
                                                   {2,2,0},
                                                   {0,0,0}};
    //output_init(&(mychip->output));
    int* i_ntype;
    //int* i_dest;
    //int* i_dest_axon;
    int* i_num_dest;

    for (i = 0; i < CHIP_LENGTH*CHIP_LENGTH; i++) {
        //i_dest = &dest[i][0][0];
        //i_dest_axon = &dest_axon[i][0][0];
        i_num_dest = &num_dest[i][0];
        i_ntype = &ntype[i][0];
        mycore = &(mychip->cores[i]);

        router_init (&(mycore->rtr));
        scheduler_init (&(mycore->sch));
        token_init (&(mycore->tkn));
        sram_init (&(mycore->srm), ch, i_num_dest, dest, dest_axon, i, i_ntype);
        neuron_init (mycore);
        //output_init(&(mycore->output));
    }

    return;
}

void chip_advance (chip* mychip, int gclk, int* in_spikes) {

    core* mycore;
    int i;

    for (i = 0; i < CHIP_LENGTH*CHIP_LENGTH; i++) {
           
        mycore = &(mychip->cores[i]);

        router_advance (mychip, i, gclk);
        scheduler_advance (mycore);
        token_advance (mycore, gclk, in_spikes, i);
        sram_advance (mycore, i);
        neuron_advance (mycore, i, gclk);
    }

    return;
}