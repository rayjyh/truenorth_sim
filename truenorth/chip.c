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

void chip_init (chip* mychip, char* ch) {
    
    core* mycore;
    //int i;
    int in_spikes[TIME_SLOT][PIXEL_NUMBER];

    FILE* fp = fopen("test.txt", "r");
    if(fp == NULL)
    {
        printf("Cannot open the file.");
        exit(0);
    }
    for(int i = 0; i < TIME_SLOT; i++)
    {
        for(int j = 0; j < PIXEL_NUMBER; j++)
        {
            fscanf(fp,"%d",&in_spikes[i][j]);
        }
    }

    fclose(fp);

    for(int i = 0; i < TIME_SLOT; i++)
    {
        for(int j = 0; j < PIXEL_NUMBER; j++)
        {
            printf("%d",in_spikes[i][j]);
        }
        printf("\n");
    }

    for (i = 0; i < CHIP_LENGTH*CHIP_LENGTH; i++) {

        mycore = &(mychip->cores[i]);

        router_init (&(mycore->rtr));
        scheduler_init (&(mycore->sch));
        token_init (&(mycore->tkn));
        sram_init (&(mycore->srm), ch);
        neuron_init (mycore);
    }

    return;
}

void chip_advance (chip* mychip, int gclk) {

    core* mycore;
    int i;

    for (i = 0; i < CHIP_LENGTH*CHIP_LENGTH; i++) {
           
        mycore = &(mychip->cores[i]);

        router_advance (mychip, i, gclk);
        scheduler_advance (mycore);
        token_advance (mycore, gclk);
        sram_advance (mycore);
        neuron_advance (mycore, i);
    }

    return;
}