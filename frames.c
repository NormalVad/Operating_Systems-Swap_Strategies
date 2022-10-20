#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

int clockHand = 0;

// count1 counts total memory accesses
// count2 counts memory misses
// count3 counts number of writes on the disk
// count4 counts non-dirty drops
void output(int count1, int count2, int count3, int count4){
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
  bool valid;
};

struct frameEntry* frameList;
int* accessList;

int evictIndexRANDOM(int count1, int count2, int count3, int count4, int frameCount){
    return rand() % frameCount;
}

int evictIndexOPT(int count1, int count2, int count3, int count4, int frameCount, char* traceFile){
    int returnIndex = 0;
    int lastFirstAccess = 0;
    for(int i=0; i<frameCount; i++){
        int frameVPN = frameList[i].virtualPageNumber;
        int currFirstAccess = INT32_MAX;
        for(int j=count1; j<1e7; j++){
            if(accessList[j]==-1){
                break;
            }
            if(accessList[j]==frameVPN){
                currFirstAccess = j;
                break;
            }
        }
        if(currFirstAccess>lastFirstAccess){
            returnIndex = i;
            lastFirstAccess = currFirstAccess;
        }
        if(currFirstAccess==INT32_MAX){
            return i;
        }
    }
    return returnIndex;
}

int evictIndexFIFO(int count1, int count2, int count3, int count4, int frameCount){
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

int evictIndexLRU(int count1, int count2, int count3, int count4, int frameCount){
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

int evictIndexCLOCK(int count1, int count2, int count3, int count4, int frameCount){
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
    int count1 = 0;
    int count2 = 0;
    int count3 = 0;
    int count4 = 0;
    char* traceFile;
    int frameCount;
    char* swapStrat;
    bool isVerboseOn;
    isVerboseOn = false;
    srand(5636);
    if(argc==5){
        isVerboseOn = true;
    }
    traceFile = argv[1];
    frameCount = atoi(argv[2]);
    swapStrat = argv[3];
    frameList = malloc(frameCount * sizeof(struct frameEntry ));
    for(int i=0; i<frameCount; i++){
        frameList[i].valid = true;
    }
    unsigned address;
    char readWriteType;
    FILE* tracePointer1 = fopen(traceFile, "r");
    int count = 0;
    accessList = (int*) malloc(1e7 * sizeof(int));
    while(EOF != fscanf(tracePointer1, "%x %c", &address, &readWriteType)) {
        accessList[count] = (address >> 12);
        count++;
    }
    for(int i=count; i<1e7; i++){
        accessList[i]=-1;
    }
    fclose(tracePointer1);
    FILE* tracePointer = fopen(traceFile, "r");
    count = 0;
    while(EOF != fscanf(tracePointer, "%x %c", &address, &readWriteType)) {
        count1++;
        int virtualPageNumber = address >> 12;
        bool frameEmpty = false;
        bool foundMatch = false;
        for(int i=0; i<frameCount; i++){
            if(!frameList[i].valid){
                frameEmpty = true;
                frameList[i].valid = true;
                frameList[i].entryTime = count1;
                frameList[i].lastUsed = count1;
                frameList[i].use = 1;
                frameList[i].virtualPageNumber = virtualPageNumber;
                if(readWriteType == 'W'){
                    frameList[i].isDirty = true;
                }else{
                    frameList[i].isDirty = false;
                }
                break;
            }else if(frameList[i].valid && frameList[i].virtualPageNumber == virtualPageNumber){
                foundMatch = true;
                if(readWriteType == 'W'){
                    frameList[i].isDirty = true;
                    frameList[i].lastUsed = count1;
                    frameList[i].use = 1;
                }else{
                    frameList[i].use = 1;
                    frameList[i].lastUsed = count1;
                }
                break;
            }
        }
        if(!frameEmpty && !foundMatch){
            count2++;
            int remove = -1;
            if(strcmp(swapStrat, "OPT")==0){
                remove = evictIndexOPT(count1, count2, count3, count4, frameCount, traceFile);
            }else if  (strcmp(swapStrat, "RANDOM")==0){
                remove = evictIndexRANDOM(count1, count2, count3, count4, frameCount);
            }else if (strcmp(swapStrat, "FIFO")==0){
                remove = evictIndexFIFO(count1, count2, count3, count4, frameCount);
            }else if (strcmp(swapStrat, "LRU")==0){
                remove = evictIndexLRU(count1, count2, count3, count4, frameCount);
            }else{
                remove = evictIndexCLOCK(count1, count2, count3, count4, frameCount);
            }
            if(frameList[remove].isDirty){
                count3++;
                if(isVerboseOn){
                    verboseOutput(virtualPageNumber,frameList[remove].virtualPageNumber,true);
                }
            }else if (!frameList[remove].isDirty){
                count4++;
                if(isVerboseOn){
                    verboseOutput(virtualPageNumber,frameList[remove].virtualPageNumber,false);
                }
            }
            frameList[remove].virtualPageNumber = virtualPageNumber;
            frameList[remove].entryTime = count1;
            frameList[remove].isDirty = (readWriteType=='W');
            frameList[remove].lastUsed = count1;
            frameList[remove].valid = true;
            frameList[remove].use = 1;
        }
    }
    output(count1, count2, count3, count4-frameCount);
    fclose(tracePointer);
}

