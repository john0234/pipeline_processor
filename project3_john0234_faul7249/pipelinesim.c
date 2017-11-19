#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUMMEMORY 65536 /* maximum number of data words in memory */
#define NUMREGS 8 /* number of machine registers */

#define ADD 0
#define NAND 1
#define LW 2
#define SW 3
#define BEQ 4
#define JALR 5
#define HALT 6
#define NOOP 7

#define NOOPINSTRUCTION 0x1c00000


typedef struct IFIDstruct{
    int instr;
    int pcPlus1;
} IFIDType;

typedef struct IDEXstruct{
    int instr;
    int pcPlus1;
    int readRegA;
    int readRegB;
    int offset;
} IDEXType;

typedef struct EXMEMstruct{
    int instr;
    int branchTarget;
    int aluResult;
    int readReg;
} EXMEMType;

typedef struct MEMWBstruct{
    int instr;
    int writeData;
} MEMWBType;

typedef struct WBENDstruct{
    int instr;
    int writeData;
}WBENDType;

typedef struct statestruct{
    int pc;
    int instrMem[NUMMEMORY];
    int dataMem[NUMMEMORY];
    int reg[NUMREGS];
    int numMemory;
    IFIDType IFID;
    IDEXType IDEX;
    EXMEMType EXMEM;
    MEMWBType MEMWB;
    WBENDType WBEND;
    int cycles; 		/* Number of cycles run so far */
    int fetched; 		/* Total number of instructions fetched */
    int retired; 		/* Total number of completed instructions */
    int branches; 		/* Total number of branches executed */
    int mispreds; 		/* Number of branch mispredictions*/
    int flushes;        /*Total number of flushes*/
} stateType;

void flush(stateType* newState);
int signExtend(int num);
void EXstage(stateType* state, stateType* newState);
void IFstage(stateType* state, stateType* newState);
void IDstage(stateType* state, stateType* newState);
void MEMstage(stateType* state, stateType* newState);
void WBstage(stateType* state, stateType* newState);

int field0(int instruction){
    return( (instruction>>19) & 0x7);
}

int field1(int instruction){
    return( (instruction>>16) & 0x7);
}

int field2(int instruction){
    return(instruction & 0xFFFF);
}

int opcode(int instruction){
    return(instruction>>22);
}

/*
void printState(stateType *statePtr){
	int i;
	printf("\n@@@\nstate:\n");
	printf("\tpc %d\n", statePtr->pc);
	printf("\tmemory:\n");
	for(i = 0; i < statePtr->numMemory; i++){
		printf("\t\tmem[%d]=%d\n", i, statePtr->mem[i]);
	}
	printf("\tregisters:\n");
	for(i = 0; i < NUMREGS; i++){
		printf("\t\treg[%d]=%d\n", i, statePtr->reg[i]);
	}
	printf("end state\n");
}
*/

void printInstruction(int instr) {
    char opcodeString[10];
    if (opcode(instr) == ADD) {
        strcpy(opcodeString, "add");
    } else if (opcode(instr) == NAND) {
        strcpy(opcodeString, "nand");
    } else if (opcode(instr) == LW) {
        strcpy(opcodeString, "lw");
    } else if (opcode(instr) == SW) {
        strcpy(opcodeString, "sw");
    } else if (opcode(instr) == BEQ) {
        strcpy(opcodeString, "beq");
    } else if (opcode(instr) == JALR) {
        strcpy(opcodeString, "jalr");
    } else if (opcode(instr) == HALT) {
        strcpy(opcodeString, "halt");
    } else if (opcode(instr) == NOOP) {
        strcpy(opcodeString, "noop");

    } else {
        strcpy(opcodeString, "data");
    }

    if(opcode(instr) == ADD || opcode(instr) == NAND){
        printf("%s %d %d %d\n", opcodeString, field2(instr), field0(instr), field1(instr));
    }else if(0 == strcmp(opcodeString, "data")){
        printf("%s %d\n", opcodeString, signExtend(field2(instr)));
    }else{
        printf("%s %d %d %d\n", opcodeString, field0(instr), field1(instr),
               signExtend(field2(instr)));
    }
}//printInstruction

void printState(stateType *statePtr){
    int i;
    printf("\n@@@\nstate before cycle %d starts\n", statePtr->cycles);
    printf("\tpc %d\n", statePtr->pc);
    printf("\tdata memory:\n");

    for (i=0; i<statePtr->numMemory; i++) {
        printf("\t\tdataMem[ %d ] %d\n", i, statePtr->dataMem[i]);
    }

    printf("\tregisters:\n");

    for (i=0; i<NUMREGS; i++) {
        printf("\t\treg[ %d ] %d\n", i, statePtr->reg[i]);
    }

    printf("\tIFID:\n");
    printf("\t\tinstruction ");
    printInstruction(statePtr->IFID.instr);
    printf("\t\tpcPlus1 %d\n", statePtr->IFID.pcPlus1);
    printf("\tIDEX:\n");
    printf("\t\tinstruction ");
    printInstruction(statePtr->IDEX.instr);
    printf("\t\tpcPlus1 %d\n", statePtr->IDEX.pcPlus1);
    printf("\t\treadRegA %d\n", statePtr->IDEX.readRegA);
    printf("\t\treadRegB %d\n", statePtr->IDEX.readRegB);
    printf("\t\toffset %d\n", statePtr->IDEX.offset);
    printf("\tEXMEM:\n");
    printf("\t\tinstruction ");
    printInstruction(statePtr->EXMEM.instr);
    printf("\t\tbranchTarget %d\n", statePtr->EXMEM.branchTarget);
    printf("\t\taluResult %d\n", statePtr->EXMEM.aluResult);
    printf("\t\treadReg %d\n", statePtr->EXMEM.readReg);
    printf("\tMEMWB:\n");
    printf("\t\tinstruction ");
    printInstruction(statePtr->MEMWB.instr);
    printf("\t\twriteData %d\n", statePtr->MEMWB.writeData);
    printf("\tWBEND:\n");
    printf("\t\tinstruction ");
    printInstruction(statePtr->WBEND.instr);
    printf("\t\twriteData %d\n", statePtr->WBEND.writeData);
}//printState

int signExtend(int num){
    // convert a 16-bit number into a 32-bit integer
    if (num & (1<<15) ) {
        num -= (1<<16);
    }
    return num;
}

void print_stats(int n_instrs){
    printf("INSTRUCTIONS: %d\n", n_instrs);
}

void IFstage(stateType* state, stateType* newState) {

    //printInstruction(newState->IFID.instr);
    newState->IFID.instr = state->instrMem[state->pc];

    if(opcode(state->IFID.instr) == LW){
        if(opcode(newState->IFID.instr) == ADD && (field1(newState->IFID.instr) == field0(state->IFID.instr) || field2(newState->IFID.instr) == field0(state->IFID.instr))){
            newState->pc = state->pc;
            newState->IFID.instr = NOOPINSTRUCTION;
            newState->retired--;
        }
        else if(opcode(newState->IFID.instr) == NAND && (field1(newState->IFID.instr) == field0(state->IFID.instr) || field2(newState->IFID.instr) == field0(state->IFID.instr))){
            newState->pc = state->pc;
            newState->IFID.instr = NOOPINSTRUCTION;
            newState->retired--;
        }
        else if(opcode(newState->IFID.instr) == SW && (field0(newState->IFID.instr) == field0(state->IFID.instr))){
            newState->pc = state->pc;
            newState->IFID.instr = NOOPINSTRUCTION;
            newState->retired--;
        }
        else if(opcode(newState->IFID.instr) == BEQ && (field0(newState->IFID.instr) == field0(state->IFID.instr) || field1(newState->IFID.instr) == field0(state->IFID.instr))){
            newState->pc = state->pc;
            newState->IFID.instr = NOOPINSTRUCTION;
            newState->retired--;
        }
        else{
            newState->pc = state->pc + 1;
            newState->IFID.pcPlus1 = state->pc + 1;
        }
    }
    else{
        newState->pc = state->pc + 1;
        newState->IFID.pcPlus1 = state->pc + 1;
    }
    if(newState->IFID.instr != 0){
        ++newState->fetched;
    }
}

void IDstage(stateType* state, stateType* newState)
{
    int instr = state->IFID.instr;
    int pcPlus1 = state->IFID.pcPlus1;
    int offset = 0;
    int readRegA = 0;
    int readRegB = 0;

    if(opcode(instr) == ADD || opcode(instr) == NAND){
        readRegA = field1(instr);
        readRegB = field2(instr);
    }
    else if(opcode(instr) == LW || opcode(instr) == SW || opcode(instr) == BEQ){
        readRegA = field0(instr);
        readRegB = field1(instr);
        offset = signExtend(field2(instr));
    }
    newState->IDEX.readRegA = state->reg[readRegA];
    newState->IDEX.readRegB = state->reg[readRegB];
    newState->IDEX.instr = instr;
    newState->IDEX.pcPlus1 = pcPlus1;
    newState->IDEX.offset = offset;

    /*newState->IDEX.readRegA = state->reg[field0(state->IFID.instr)];
    newState->IDEX.readRegB = state->reg[field1(state->IFID.instr)];
    newState->IDEX.instr = state->IFID.instr;
    newState->IDEX.pcPlus1 = state->IFID.pcPlus1;
    newState->IDEX.offset = signExtend(field2(state->IFID.instr));*/

}//ID stage

void EXstage(stateType *state, stateType *newState) {
    /**
    *
    * Action depends on instruction
     *ALU results
    **/

    int regA = state->IDEX.readRegA;
    int regB = state->IDEX.readRegB;

    newState->EXMEM.aluResult = 0;
    newState->EXMEM.branchTarget = 0;
    newState->EXMEM.instr = state->IDEX.instr;
    newState->EXMEM.readReg = 0;

    // ADD
    if (opcode(state->IDEX.instr) == ADD) {

        if (field1(state->IDEX.instr) == field0(state->EXMEM.instr) && opcode(state->EXMEM.instr) != BEQ) {
            //if regA is the dest of the Mem, then grab that value instead of going to register.
            regA = state->EXMEM.aluResult;
        }
        else if (field1(state->IDEX.instr) == field0(state->MEMWB.instr) &&
            opcode(state->MEMWB.instr) != BEQ) {
            //if regB is the dest of the MEM instruction, grab the correct value.
            regA = state->MEMWB.writeData;
        }
        else if (field1(state->IDEX.instr) == field0(state->WBEND.instr) &&
            opcode(state->WBEND.instr) != BEQ) {
            regA = state->WBEND.writeData;
        }
        if (field2(state->IDEX.instr) == field0(state->EXMEM.instr) &&
            opcode(state->EXMEM.instr) != BEQ) {
            regB = state->EXMEM.aluResult;
        }
        else if (field2(state->IDEX.instr) == field0(state->MEMWB.instr) &&
            opcode(state->MEMWB.instr) != BEQ){
            regB = state->MEMWB.writeData;
        }
        else if (field2(state->IDEX.instr) == field0(state->WBEND.instr) &&
            opcode(state->WBEND.instr) != BEQ) {
            regB = state->WBEND.writeData;
        }
            //do what we would normally do
        newState->EXMEM.aluResult = regA + regB;

    }

    else if (opcode(state->IDEX.instr) == NAND) {

        if (field1(state->IDEX.instr) == field0(state->EXMEM.instr) && opcode(state->EXMEM.instr) != BEQ) {
            //if regA is the dest of the Mem, then grab that value instead of going to register.
            regA = state->EXMEM.aluResult;
        }
        else if (field1(state->IDEX.instr) == field0(state->MEMWB.instr) &&
            opcode(state->MEMWB.instr) != BEQ) {
            //if regB is the dest of the MEM instruction, grab the correct value.
            regB = state->MEMWB.writeData;
        }
        else if (field1(state->IDEX.instr) == field0(state->WBEND.instr) &&
            opcode(state->WBEND.instr) != BEQ) {
            regA = state->WBEND.writeData;
        }
        if (field2(state->IDEX.instr) == field0(state->EXMEM.instr) &&
            opcode(state->EXMEM.instr) != BEQ) {
            regB = state->EXMEM.aluResult;
        }
        else if (field1(state->IDEX.instr) == field0(state->MEMWB.instr) &&
            opcode(state->MEMWB.instr) != BEQ) {
            regA = state->MEMWB.writeData;
        }
        else if (field2(state->IDEX.instr) == field0(state->WBEND.instr) &&
            opcode(state->WBEND.instr) != BEQ) {
            regB = state->WBEND.writeData;
        }
            //do what we would normally do
            newState->EXMEM.aluResult = ~(regA & regB);

    }//NAND

    else if(opcode(state->IDEX.instr) == SW) {

        if(field0(state->IDEX.instr) == field0(state->EXMEM.instr) && opcode(state->EXMEM.instr) != BEQ){
            regA = state->EXMEM.aluResult;
        }
        else if(field0(state->IDEX.instr) == field0(state->MEMWB.instr) && opcode(state->MEMWB.instr) != BEQ){
            regA = state->MEMWB.writeData;
        }
        else if(field0(state->IDEX.instr) == field0(state->WBEND.instr) && opcode(state->WBEND.instr) != BEQ){
            regA = state->WBEND.writeData;
        }
            newState->EXMEM.aluResult = state->IDEX.readRegB + state->IDEX.offset;
           // newState->EXMEM.aluResult = field1(state->IDEX.instr) + signExtend(field2(state->IDEX.instr));
            newState->EXMEM.readReg = regA;

    }
        //LW or SW
    else if (opcode(state->IDEX.instr) == LW) {
        //TODO CHANGE THIS TO JUST FOCUS ON LW
        // Calculate memory address
        newState->EXMEM.aluResult = state->IDEX.readRegB + state->IDEX.offset;

    }

    else if (opcode(state->IDEX.instr) == BEQ) {
        // Calculate condition
        //branch target

        if (field0(state->IDEX.instr) == field0(state->EXMEM.instr) && opcode(state->EXMEM.instr) != BEQ) {
            //if regA is the dest of the Mem, then grab that value instead of going to register.
            regA = state->EXMEM.aluResult;

        }
        else if (field0(state->IDEX.instr) == field0(state->MEMWB.instr) &&
                   opcode(state->MEMWB.instr) != BEQ){
            //if regB is the dest of the MEM instruction, grab the correct value.
            regA = state->MEMWB.writeData;
        }
        else if (field0(state->IDEX.instr) == field0(state->WBEND.instr) &&
                   opcode(state->WBEND.instr) != BEQ) {
            regA = state->WBEND.writeData;
        }
        if (field1(state->IDEX.instr) == field0(state->EXMEM.instr) &&
                   opcode(state->EXMEM.instr) != BEQ) {
            regB = state->EXMEM.aluResult;
        }
        else if (field1(state->IDEX.instr) == field0(state->MEMWB.instr) &&
                   opcode(state->MEMWB.instr) != BEQ){
            regB = state->MEMWB.writeData;
        }
        else if (field1(state->IDEX.instr) == field0(state->WBEND.instr) &&
                   opcode(state->WBEND.instr) != BEQ) {
            regB = state->WBEND.writeData;
        }

        newState->EXMEM.aluResult = regA - regB;
        newState->EXMEM.branchTarget = state->IDEX.pcPlus1 + state->IDEX.offset;


    }

}//EX stage

void MEMstage(stateType *state, stateType *newState) {
    /*
    * Action depends on instruction
     *
    **/
    //int result = 1;
    newState->MEMWB.instr = state->EXMEM.instr;
    //ADD
    if (opcode(state->EXMEM.instr) == ADD) {
        // Add
        // Save result
        //Change the data mem
        newState->MEMWB.writeData = state->EXMEM.aluResult;
    }
        // NAND
    else if (opcode(state->EXMEM.instr) == NAND) {
        // NAND
        // Save result
        newState->MEMWB.writeData = state->EXMEM.aluResult;

    }
        // LW or SW
    else if (opcode(state->EXMEM.instr) == LW || opcode(state->EXMEM.instr) == SW) {
        // Calculate memory address
        if (opcode(state->EXMEM.instr) == LW) {
            // Load
            printf("In LW in Mem \n");
            newState->MEMWB.writeData = state->dataMem[state->EXMEM.aluResult];

        } else if (opcode(state->EXMEM.instr) == SW) {
            // Store
            printf("In SW in Mem");
            newState->dataMem[state->EXMEM.aluResult] = state->EXMEM.readReg;
        }
    }
        // BEQ
    else if (opcode(state->EXMEM.instr) == BEQ) {
        //Do we leave this here? We also have it in the EXE stage.
        newState->branches++;
       if(state->EXMEM.aluResult == 0)
       {
           newState->mispreds++;
           newState->pc = state->EXMEM.branchTarget;
           flush(newState);
       }

    }

}//Mem Stage

void WBStage(stateType *state, stateType *newState) {
    int result = 1;
    //TODO: deal with writeData hazard
    newState->WBEND.instr = state->MEMWB.instr;
    newState->WBEND.writeData = 0;

    // ADD
    if (opcode(state->MEMWB.instr) == ADD) {
        // Add
        newState->reg[field0(state->MEMWB.instr)] = state->MEMWB.writeData;
        newState->WBEND.writeData = state->MEMWB.writeData;
    }
        // NAND
    else if (opcode(state->MEMWB.instr) == NAND) {
        // NAND
        newState->reg[field0(state->MEMWB.instr)] = state->MEMWB.writeData;
        newState->WBEND.writeData = state->MEMWB.writeData;
    }
        // LW or SW
    else if (opcode(state->MEMWB.instr) == LW) {
        // Calculate memory address
            // Load
            //state->reg[field0(instr)] = state->mem[aluResult];
            newState->reg[field0(state->MEMWB.instr)] = state->MEMWB.writeData;
            newState->WBEND.writeData = state->MEMWB.writeData;

    }
    else if (opcode(state->MEMWB.instr) == HALT) {
        printf("machine halted\n");
        result = 0;
        //break;
    }
    newState->retired++;
}//WB stage

void flush(stateType *newState) {
    newState->IFID.instr = NOOPINSTRUCTION;
    newState->EXMEM.instr = NOOPINSTRUCTION;
    newState->IDEX.instr = NOOPINSTRUCTION;
    newState->flushes += 3;
    newState->retired -= 3;

}//Flush


void run(stateType *state, stateType *newState)
 {
    // Primary loop
    newState->retired -= 5;
    while (1) {

        printState(state);

        /* check for halt */
    if(HALT == opcode(state->MEMWB.instr))
    {
        printf("machine halted\n");
        printf("total of %d cycles executed\n", state->cycles);
        printf("total of %d instructions fetched\n", state->fetched);
        printf("total of %d instructions retured\n", state->retired);
        printf("total of %d branches executed\n", state->branches);
        printf("total of %d branch mispredictions\n", state->mispreds);
        exit(0);
    }



    *newState = *state;
    newState->cycles++;


        /*------------------IF stage ---------------------*/
        IFstage(state, newState);


        /*------------------ID stage ---------------------*/

        IDstage(state, newState);

        /*------------------EX stage ---------------------*/
        EXstage(state, newState);

        /*------------------MEM stage --------------------*/
        MEMstage(state, newState);

        /*------------------WB stage ---------------------*/

        //Does this even work?
        WBStage(state, newState);


//	printf("State: \n");
//	printf("New State:\n");
//	printState(newState);
//	printf("\n*******************************************************\n");

        *state = *newState;
    }    //while

}//run

int main(int argc, char **argv) {

    /** Get command line arguments **/
    char *fname;

    if (argc == 1) {
        fname = (char *) malloc(sizeof(char) * 100);
        printf("Enter the name of the machine code file to simulate: ");
        fgets(fname, 100, stdin);
        fname[strlen(fname) - 1] = '\0'; // gobble up the \n with a \0
    } else if (argc == 2) {

        int strsize = strlen(argv[1]);

        fname = (char *) malloc(strsize);
        fname[0] = '\0';

        strcat(fname, argv[1]);
    } else {
        printf("Please run this program correctly\n");
        exit(-1);
    }

    FILE *fp = fopen(fname, "r");
    if (fp == NULL) {
        printf("Cannot open file '%s' : %s\n", fname, strerror(errno));
        return -1;
    }

    /* count the number of lines by counting newline characters */
    int line_count = 0;
    int c;
    while (EOF != (c = getc(fp))) {
        if (c == '\n') {
            line_count++;
        }
    }
    // reset fp to the beginning of the file
    rewind(fp);

    stateType *state = (stateType *) malloc(sizeof(stateType));
    stateType *newState = (stateType *) malloc(sizeof(stateType));

    state->pc = 0;
    memset(state->instrMem, 0, NUMMEMORY * sizeof(int));
    memset(state->dataMem, 0, NUMMEMORY * sizeof(int));
    memset(state->reg, 0, NUMREGS * sizeof(int));
    state->numMemory = line_count;
    state->cycles =0;
    state->fetched=0;
    state->retired =0;
    state->branches =0;
    state->mispreds =0;

    //initialize newState
    newState->pc = 0;
    memset(newState->instrMem, 0, NUMMEMORY * sizeof(int));
    memset(newState->dataMem, 0, NUMMEMORY * sizeof(int));
    memset(newState->reg, 0, NUMREGS * sizeof(int));
    newState->numMemory = line_count;
    newState->cycles =0;
    newState->fetched=0;
    newState->retired =0;
    newState->branches =0;
    newState->mispreds =0;

    char line[256];

    int i = 0;
    while (fgets(line, sizeof(line), fp)) {

        state->instrMem[i] = atoi(line);
        state->dataMem[i] = atoi(line);
        newState->dataMem[i] = atoi(line);
        newState->instrMem[i] = atoi(line);
        i++;
    }
    fclose(fp);
    state->IFID.instr = NOOPINSTRUCTION;
    state->IDEX.instr = NOOPINSTRUCTION;
    state->EXMEM.instr = NOOPINSTRUCTION;
    state->MEMWB.instr = NOOPINSTRUCTION;
    state->WBEND.instr = NOOPINSTRUCTION;

    /** Run the simulation **/
    run(state, newState);
    //int instruction = 8912909;
    //printf("this is regA regB offset: %i, %i, %i\n", field0(instruction),field1(instruction),field2(instruction));

    free(state);
    free(newState);
    free(fname);

    return 1;

}
