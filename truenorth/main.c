//#include <GL/glut.h>
#include <pthread.h>
#include "core.h"

#define SIMTIME     30000
#define FILE_NAME   256

chip mychip;

extern int router_drops, neuron_drops, gens;
int clear_time = 0;

// functions for notifying informations to users
void help ();
void simulation_report ();

// GUI functions
void gui_init (int* argc, char* argv[]);
void display();
void* gui_thread ();

int main (int argc, char* argv[]) {

    int opt, i;
    int gui_on = 0;
    pthread_t thread;
    char filename[FILE_NAME];
    memcpy (filename, "./test.txt", 256);
    
    // for check packet clear time
    int j, pckt_chk;
    router* rtr = NULL;

    // -h: help, -g: GUI ON, -f: filename
    while ((opt = getopt (argc, argv, "hgf:")) != -1) {
        switch (opt) {
            case 'h':
                help ();
                break;
            case 'g':
                gui_on = 1;
                break;
            case 'f':
                memcpy (filename, optarg, 256);
                break;
        }
    }

    // initiate TrueNorth chip
    printf ("now initiate a chip...\n");
    chip_init (&mychip, filename);
    printf ("complete!\n");

    // GUI init
    /*if (gui_on == 1) {
        printf ("now initiate GUI...\n");
        gui_init (&argc, argv);
        pthread_create (&thread, NULL, &gui_thread, NULL);
        printf ("complete!\n");
    }*/

    //read input spikes
    int in_spikes[SIMTIME/GTICK_INTERVAL][PIXEL_NUMBER];

    FILE* fp = fopen("/Users/jingyu/Desktop/neuromorphic_computing/sw:hw codesign/simulators/truenorth_sim/truenorth/test.txt", "r");
    if(fp == NULL)
    {
        printf("Cannot open the file.");
        exit(0);
    }
    for(int i = 0; i < SIMTIME/GTICK_INTERVAL; i++)
    {
        for(int j = 0; j < PIXEL_NUMBER; j++)
        {
            fscanf(fp,"%d",&in_spikes[i][j]);
        }
    }

    fclose(fp);
    // simulate TrueNorth for 'SIMTIME' tick
    int* input;
    int idx;
    printf ("simulate TrueNorth Chip for %dms...\n", SIMTIME/GTICK_INTERVAL);
    for (i = 0; i < SIMTIME; i++) {
        idx = i/GTICK_INTERVAL;
        input = &in_spikes[idx][0];
        printf ("global clock: %d\n", i);
        chip_advance (&mychip, i, input);
        if (i%GTICK_INTERVAL == 0) {
            printf ("global clock: %d\n", i/GTICK_INTERVAL);
            clear_time = 0;
        }
        // calculate packet clear time
        if (i%GTICK_INTERVAL > 100 && !clear_time) {
            pckt_chk = 0;
            for (j = 0; j < CHIP_LENGTH*CHIP_LENGTH; j++) {
                rtr = &(mychip.cores[j].rtr);
                pckt_chk += isempty(&rtr->leftq);
                pckt_chk += isempty(&rtr->rightq);
                pckt_chk += isempty(&rtr->upperq);
                pckt_chk += isempty(&rtr->downq);
                pckt_chk += isempty(&rtr->inq);
            }
            if (pckt_chk == 4096*5)
                clear_time = i%GTICK_INTERVAL;
        }
    }
    printf ("complete!\n");

    // write output trace
    FILE* fp1;
    FILE* fp2;
    if ((fp1 = fopen("/Users/jingyu/Desktop/neuromorphic_computing/sw:hw codesign/simulators/truenorth_sim/truenorth/trace.txt","w"))==NULL)
    {
        printf("Can not open the file..");
        exit(0);
    }

    if ((fp2 = fopen("/Users/jingyu/Desktop/neuromorphic_computing/sw:hw codesign/simulators/truenorth_sim/truenorth/output.txt","w"))==NULL)
    {
        printf("Can not open the file..");
        exit(0);
    }

    core* mycore;
    trace* trace_ptr;
    output* output_ptr;
    for (int k = 0; k < CHIP_LENGTH*CHIP_LENGTH; ++k) {
        mycore = &(mychip.cores[k]);
        // write trace
        fprintf(fp1,"The trace of core %d: \n", k);
        printf("Issue time  dest\n");
        while(isempty(&(mycore->nrn.trq)) == 0) {
           trace_ptr = (trace*) dequeue (&(mycore->nrn.trq));
           fprintf(fp1,"%d  %d \n", trace_ptr->gclk, trace_ptr->dest);
           free(trace_ptr);
        }
        // write output
        fprintf(fp2,"The output of core %d: \n", k);
        printf("input index  neuron id\n");
        while(isempty(&(mycore->nrn.oq)) == 0) {
            output_ptr = (output*) dequeue (&(mycore->nrn.oq));
            fprintf(fp2,"%d  %d \n", output_ptr->input_idx, output_ptr->neuron_id);
            free(output_ptr);
        }
    }

    fclose(fp1);
    fclose(fp2);

    // print out final simulation result
    simulation_report ();
    // print out packet clear time
    printf ("packet clear time:\t\t%d tick\n", clear_time);
    return 0;
}


/********************************************************************************/
/******************************* report functions *******************************/
/********************************************************************************/

// help massage for user
void help () {

    printf ("Usage: ./truenorth [OPTION] [FILE]\n");
    printf ("-h\t\thelp\n");
    printf ("-g\t\tGUI ON\n");
    printf ("-f [FILE]\tsync neuron_info data\n");

    exit (0);
}

// notifying simulation result to user
void simulation_report () {
    
    float rtr_act = 0,  sch_act = 0, tkn_act = 0, nrn_act = 0, srm_act = 0;
    core* mycore = NULL;
    int i;
    printf ("\n\n********************simulation result********************\n");

    // generated packets and drops and drop rate
    printf ("\n1. drop rate\n");
    printf ("generated packets:\t\t%d\n", gens);
    printf ("dropped packets:\t\t%d\n", router_drops + neuron_drops);
    printf ("drop rate:\t\t\t%f%%\n", (float)(router_drops + neuron_drops)/gens * 100);
    
    // activation rate of each core modules
    for (i = 0; i < CHIP_LENGTH*CHIP_LENGTH; i++) {
        mycore = &(mychip.cores[i]);
        rtr_act += mycore->rtr.router_activate;
        sch_act += mycore->sch.sch_activate;
        tkn_act += mycore->tkn.token_activate;
        nrn_act += mycore->nrn.neuron_activate;
        srm_act += mycore->srm.sram_activate;
    }
    printf ("\n2. activation rate\n");
    printf ("router activation rate:\t\t%f%%\n", rtr_act/(SIMTIME*CHIP_LENGTH*CHIP_LENGTH)*100);
    printf ("scheduler activation rate:\t%f%%\n", sch_act/(SIMTIME*CHIP_LENGTH*CHIP_LENGTH)*100);
    printf ("token activation rate:\t\t%f%%\n", tkn_act/(SIMTIME*CHIP_LENGTH*CHIP_LENGTH)*100);
    printf ("neuronBlock activation rate:\t%f%%\n", nrn_act/(SIMTIME*CHIP_LENGTH*CHIP_LENGTH)*100);
    printf ("coreSRAM activation rate:\t%f%%\n", srm_act/(SIMTIME*CHIP_LENGTH*CHIP_LENGTH)*100);
}

/********************************************************************************/
/******************************** GUI functions *********************************/
/********************************************************************************/
/*
// initiate graphic user interface
void gui_init (int* argc, char* argv[]) {
    glutInit(argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(500, 500);
    glutCreateWindow("Router Activation level");
    glClearColor(0.0, 0.0, 0, 0.0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
    return;
}

// draw Router Activation level Image periodically
void dotimer(int value) {

    glutPostRedisplay();
    glutTimerFunc(50, dotimer, 1);
}

void* gui_thread () {

    glutDisplayFunc(display);
    glutTimerFunc(10, &dotimer, 1); 
    glutMainLoop();
    pthread_exit((void *) 0);

}

void display() {
    
    float activate;
    router* rtr;

    glClear(GL_COLOR_BUFFER_BIT);
    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < 64; j++) {
            rtr = &(mychip.cores[i+j*CHIP_LENGTH].rtr);
            activate = ((float)(rtr->leftq.size + rtr->rightq.size + rtr->upperq.size + rtr->downq.size))/(3*ROUTERQUEUE_SIZE);
            glColor3f(1, 1-activate, 1-activate);
            glRectf(0.015625*(i+1), 0.015625*(j+1), 0.015625*i, 0.015625*j);       
        }
    }
    glutSwapBuffers();
}
*/
