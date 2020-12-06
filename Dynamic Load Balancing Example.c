#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <pthread.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define NUM_NODES 4
#define STEP_COUNT 400

int nodeStepTime[NUM_NODES];
int nodeStepTime2[NUM_NODES];

int workload[NUM_NODES];
int workloadUpdates[NUM_NODES];
int balancedCounter[NUM_NODES];

int globalBalancedCounter = 0;

int caZells[500][1000];

int globalStepTime = 0;

int upperBound[NUM_NODES];

void printBoard(){
    for(int x = 0; x < 10; x++){
        for(int y = 0; y < 100; y++){
            if(y * 10 == upperBound[0]) printf("|");
            else if(y * 10 == upperBound[1]) printf("|");
            else if(y * 10 == upperBound[2]) printf("|");
            else if(y * 10 == upperBound[3]) printf("|");
            else if(y * 10 == upperBound[4]) printf("|");

            if(caZells[x * 50][y * 10] == 1) printf("!");
            else printf(".");
        }
        printf("|\n");
    }
    printf("\n");
}

int isLoadBalancingPhase(){
    if((globalStepTime % 100) == 0 && globalStepTime > 0)
        return 1;
    else 
        return 0;
}

int transitionFunktion(int leftNeighbor){
    if(leftNeighbor == 1){
        usleep(1);
        return 1;
    } 
    else {
        return 0; 
        }
}

//funktion for all nodes
void* nodes(void* vargp){
    int myCaZells[500][1000];//save map for yourselve
    
    int *myid = (int*) vargp;
    printf("Node %d: Has Started.\n", *myid);
    int myStepTime = 0;
    int myWorkload = 0;
    int myBalanceCounter = 0;
    while(myStepTime < STEP_COUNT){
        while(myStepTime >= globalStepTime){
            //wait for new Time Step
        }
        for(int x = 0; x < 500; x++){
            for(int y = 0; y < 1000; y++){
                myCaZells[x][y] = caZells[x][y];
            }
        }
        myStepTime++;
        if(isLoadBalancingPhase() == 1){
            workload[*myid] = myWorkload;
            myBalanceCounter++;
            workloadUpdates[*myid] = myBalanceCounter;
            while(globalBalancedCounter < myBalanceCounter){}
        }

        //compute Zells
        myWorkload = 0;
        for(int x = 0; x < 500; x++){//go throw all cells in this colum
            for(int y = upperBound[*myid]; y < upperBound[*myid + 1]; y++){//colum of cells for this node
                if(y <= 0) {myCaZells[x][y] = transitionFunktion(caZells[x][y]);}
                else myCaZells[x][y] = transitionFunktion(caZells[x][y-1]);
                if(myCaZells[x][y] == 1) myWorkload++;
            }
        }
        //wait for other nodes to finish
        nodeStepTime2[*myid] = myStepTime;
        for(int y = 0; y < NUM_NODES; y++){
            while(nodeStepTime2[y] < myStepTime){}
        }

        int counter = 0;
        for(int x = 0; x < 500; x++){//go throw all cells in this colum
            for(int y = upperBound[*myid]; y < upperBound[*myid + 1]; y++){//colum of cells for this node
                caZells[x][y] = myCaZells[x][y];
                if(myCaZells[x][y] == 1) counter++;
            }
        }
        
        //tell main time step has been done
        nodeStepTime[*myid] = myStepTime;
    }
    
    
    pthread_exit(0);
}



void initActiveCells(){
    int center_x = 125;
    int center_y = 250;

    for(int x = 0; x < 500; x++){
        for(int y = 0; y < 1000; y++){
            int d = 75 * 75 - ((center_x - x) * (center_x - x) + (center_y - y) * (center_y - y));

            if(d >= 0)caZells[x][y] = 1;
            else caZells[x][y] = 0;
        }
    }
    int counter = 0;
    for(int x = 0; x < 500; x++){
        for(int y = 0; y < 1000; y++){
            if(caZells[x][y] == 1) counter++;
        }
    }
    printf("Counter Init: %d\n", counter);
}

void initBounderys(){
    upperBound[0] = 0;
    upperBound[1] = 250;
    upperBound[2] = 500;
    upperBound[3] = 750;
    upperBound[4] = 1000;
}

void balanceWork(){
    //finde who has work
    //set boundarys new
    int haveLoad = 0;
    int maxLoad = workload[0];
    int maxLoadAt = 0;
    int minLoad = 0;
    for(int x = 0; x < NUM_NODES; x++){
        if(workload[x] > 0) haveLoad++;
        if(maxLoad < workload[x]) {
            maxLoad = workload[x];
            maxLoadAt = x;
        }
        if(minLoad > workload[x]) {
            minLoad = workload[x];
        }
    }

    printf("######Balancing######\n");
    printf("Max Load: %d\n", maxLoad);
    printf("Max Load at: %d\n", maxLoadAt);
    printf("Have Load at: %d\n", haveLoad);

    printf("blond 1 %d\n", upperBound[1]);
    printf("blond 2 %d\n", upperBound[2]);
    printf("blond 3 %d\n", upperBound[3]);
    //wenn workload nur in einem ist
    if(haveLoad == 1 || (haveLoad == 2 && maxLoadAt > 50 * minLoad)){
        upperBound[0] = 0;
        upperBound[4] = 1000;

        int share_space = (upperBound[maxLoadAt + 1] - upperBound[maxLoadAt]) / 4;
        upperBound[1] = upperBound[maxLoadAt] + 1 * share_space;
        upperBound[2] = upperBound[maxLoadAt] + 2 * share_space;
        upperBound[3] = upperBound[maxLoadAt] + 3 * share_space;

        upperBound[1] -= upperBound[1] % 10;
        upperBound[2] -= upperBound[2] % 10;
        upperBound[3] -= upperBound[3] % 10;
    }

    else if(haveLoad == 2){
        if(maxLoadAt < NUM_NODES){
            if(workload[maxLoadAt + 1] > 0){
                upperBound[0] = 0;
                upperBound[4] = 1000;

                int share_space = (upperBound[maxLoadAt + 2] - upperBound[maxLoadAt]) / 4;
                upperBound[1] = upperBound[maxLoadAt] + share_space;
                upperBound[2] = upperBound[maxLoadAt] + 2 * share_space;
                upperBound[3] = upperBound[maxLoadAt] + 3 * share_space;
            }
        }

        if(maxLoadAt > 0){
            if(workload[maxLoadAt - 1] > 0){
                upperBound[0] = 0;
                upperBound[4] = 1000;

                int share_space = (upperBound[maxLoadAt + 1] - upperBound[maxLoadAt - 1]) / 4;
                upperBound[1] = upperBound[maxLoadAt - 1] + share_space;
                upperBound[2] = upperBound[maxLoadAt - 1] + 2 * share_space;
                upperBound[3] = upperBound[maxLoadAt - 1] + 3 * share_space;
            }
        }
        upperBound[1] -= upperBound[1] % 10;
        upperBound[2] -= upperBound[2] % 10;
        upperBound[3] -= upperBound[3] % 10;
    }
    printf("blond 1 %d\n", upperBound[1]);
    printf("blond 2 %d\n", upperBound[2]);
    printf("blond 3 %d\n", upperBound[3]);
    
}

int main(){
    //create NUM_NODES threades#
    clock_t begin = clock();
    initActiveCells();
    initBounderys();
    //printBoard();
    pthread_t threads[NUM_NODES];
    int id[NUM_NODES];
    for(int x = 0; x < NUM_NODES; x++){
        id[x] = x;
        pthread_create(&threads[x], NULL, &nodes, (void*)&id[x]);
    }
    
    globalStepTime = 0;
    for(int x = 0; x < STEP_COUNT; x++){
        //wait for all nodes to get to same step
        for(int y = 0; y < NUM_NODES; y++){
            while(nodeStepTime[y] < globalStepTime){}
        }
        globalStepTime++;
        if(isLoadBalancingPhase() == 1){
            printf("Main: Time Step %d\n", globalStepTime);
            printBoard();
            printf("Main: Waiting for workloads\n");
            for(int x = 0; x < NUM_NODES; x++){
                while(globalBalancedCounter >= workloadUpdates[x]){}
            }
            balanceWork();
            printBoard();
            globalBalancedCounter++;
        }
    }
    
    //wait for all threads
    for(int x = 0; x < NUM_NODES; x++){
        pthread_join(threads[x], NULL);
    }
    clock_t end = clock();
    printf("Execution time: %f\n", (double)(end-begin)/CLOCKS_PER_SEC);
    printf("Main: Program has ended!\n");
    exit(0);
}