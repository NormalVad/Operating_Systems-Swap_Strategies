#include <stdio.h>
#include <string.h>
#include <stdbool.h>

int count1 = 0;
int count2 = 0;
int count3 = 0;
int count4 = 0;
char* traceFile;
int frameCount;
char* swapStrat;
bool isVerboseOn;
int clockHand = 0;

// count1 counts total memory accesses
// count2 counts memory misses
// count3 counts number of writes on the disk
// count4 counts non-dirty drops
void output(){
  printf("Number of memory accesses: %d\n", count1);
  printf("Number of misses: %d\n", count2);
  printf("Number of writes: %d\n", count3);
  printf("Number of drops: %d\n", count4);
}

void verboseOutput(int readAddr, int dropAddr, bool isDirty){
  if(isDirty){
    printf("Page 0x%05x was read from disk, page 0x%05x was written to the disk. \n", readAddr, dropAddr);
  }else{
    printf("Page 0x%05x was read from disk, page 0x%05x was dropped (it was not dirty). \n", readAddr, dropAddr);
  }
}

struct frameEntry {
  int virtualPageNumber;
  bool isDirty;
  bool use;    
  int lastUsed; 
  int entryTime;
};

struct frameEntry* frameList;

int evictIndexRANDOM(){
    return rand() % frameCount;
}

int evictIndexOPT(){
    int returnIndex = 0;
    int lastFirstAccess = frameList[0].lastUsed;
    for(int i=1; i<frameCount; i++){
        //
    }
    return returnIndex;
}

int evictIndexFIFO(){
    int returnIndex = 0;
    int firstEntry = frameList[0].entryTime;
    for(int i=1; i<frameCount; i++){
        if(frameList[i].entryTime < firstEntry){
            firstEntry = frameList[i].entryTime;
            returnIndex = i;
        }
    }
    return returnIndex;
}

int evictIndexLRU(){
    int returnIndex = 0;
    int lastUse = frameList[0].lastUsed;
    for(int i=1; i<frameCount; i++){
        if(frameList[i].lastUsed < lastUse){
            lastUse = frameList[i].lastUsed;
            returnIndex = i;
        }
    }
    return returnIndex;
}

int evictIndexCLOCK(){
    int start = clockHand;
    int returnIndex = -1;
    do {
        if (frameList[clockHand].use) {
        frameList[clockHand].use = false;
        clockHand = (clockHand + 1) % frameCount;
        continue;
        }
        returnIndex = clockHand;
        clockHand = (clockHand + 1) % frameCount;
        return returnIndex;
    } while (start != clockHand);
    returnIndex = clockHand;
    clockHand = (clockHand + 1) % frameCount;
    return returnIndex;
}

int main(int argc, char *argv[]){
    isVerboseOn = false;
    srand(5636);
    if(argc==5){
        isVerboseOn = true;
    }
    traceFile = argv[1];
    frameCount = atoi(argv[2]);
    swapStrat = argv[3];
    frameList = malloc(frameCount * sizeof(struct frameEntry *));
    FILE* tracePointer = fopen(traceFile, "r");
    //
    output();
    fclose(traceFile);
}

