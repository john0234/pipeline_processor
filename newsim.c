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
} stateType;

void flush(stateType* newState);
int signExtend(int num);

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
    printf("\t\treadRegB %d\n", statePtr->EXMEM.readReg);
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

void IFstage(stateType* state, stateType* newState)
{
    newState->fetched++;
    newState->IFID.instr = state->instrMem[state->pc];
    newState->IFID.pcPlus1 = state->pc + 1;
}

void IDstage(stateType* state, stateType* newState)
{
    newState->IDEX.pcPlus1 = state->IFID.pcPlus1;
    newState->IDEX.instr = state->IFID.instr;
    //gets regA & regB from instruction
    newState->IDEX.readRegA = state->reg[field0(state->IFID.instr)];
    newState->IDEX.readRegB = state->reg[field1(state->IFID.instr)];
    newState->IDEX.offset = state->reg[field2(state->IFID.instr)];

}//ID stage

void EXstage(stateType* state, stateType* newState)
{
    /**
    *
    * Action depends on instruction
     *ALU results
    **/
    newState->EXMEM.aluResult = 0;
    newState->EXMEM.branchTarget = 0;
    newState->EXMEM.instr = state->IDEX.instr;
    newState->EXMEM.readReg = 0;

    // ADD
    if(opcode(state->IDEX.instr) == ADD){
        // Add
        newState->EXMEM.aluResult= state->IDEX.readRegA + state->IDEX.readRegB;


    }
        // NAND
    else if(opcode(state->IDEX.instr) == NAND){
        // NAND
        newState->EXMEM.aluResult = ~(state->IDEX.readRegA & state->IDEX.readRegB);
    }

        //LW or SW
    else if(opcode(state->IDEX.instr) == LW || opcode(state->IDEX.instr) == SW){
        // Calculate memory address
        newState->EXMEM.readReg = state->IDEX.readRegB + state->IDEX.offset;
    }

        /* Not implemented in this simulator
         JALR
        else if(opcode(instr) == JALR){
            // Save pc+1 in regA
            state->reg[field0(instr)] = state->pc;
            //Jump to the address in regB;
            state->pc = state->reg[field1(instr)];
        }*/

        // BEQ
    else if(opcode(state->IDEX.instr) == BEQ){
        // Calculate condition
        //branch target
        newState->branches++;
        // ZD
        if(state->IDEX.readRegA == state->IDEX.readRegB){
            // branch
            newState->mispreds++;
            newState->EXMEM.branchTarget = state->IDEX.pcPlus1 + state->IDEX.offset;
            flush(newState);
        } else{
            newState->EXMEM.branchTarget = state->IDEX.pcPlus1;
        }
    }

}//EX stage

void MEMstage(stateType* state, stateType* newState)
{
    /*
    * Action depends on instruction
     *
    **/

    newState->MEMWB.instr = state->EXMEM.instr;
    //ADD
    if(opcode(state->EXMEM.instr) == ADD){
        // Add
        // Save result
        //Change the data mem
        newState->MEMWB.writeData = state->EXMEM.aluResult;
    }
        // NAND
    else if(opcode(state->EXMEM.instr) == NAND){
        // NAND
        // Save result
        newState->MEMWB.writeData = state->EXMEM.aluResult;

    }
        // LW or SW
    else if(opcode(state->EXMEM.instr) == LW || opcode(state->EXMEM.instr) == SW){
        // Calculate memory address
        if(opcode(state->EXMEM.instr) == LW){
            // Load
            printf("In LW");
            newState->MEMWB.writeData = state->dataMem[state->EXMEM.readReg];

        }else if(opcode(state->EXMEM.instr) == SW){
            // Store
            printf("In SW");
            newState->dataMem[state->EXMEM.readReg] = state->reg[field1(state->EXMEM.instr)];
        }
    }
        /* Not implemented in this simulator
         JALR
        else if(opcode(instr) == JALR){
            // Save pc+1 in regA
            state->reg[field0(instr)] = state->pc;
            //Jump to the address in regB;
            state->pc = state->reg[field1(instr)];
        }*/
        // BEQ
    else if(opcode(state->EXMEM.instr) == BEQ)
    {
        //Do we leave this here? We also have it in the EXE stage.
        newState->pc = state->EXMEM.branchTarget;

    }

}//Mem Stage

int WBStage(stateType* state, stateType* newState)
{
    int result =1;
    //TODO: deal with writeData hazard
    newState->WBEND.instr = state->MEMWB.instr;
    newState->WBEND.writeData = 0;

    // ADD
    if(opcode(state->MEMWB.instr) == ADD){
        // Add
        newState->reg[field2(state->MEMWB.instr)] = state->MEMWB.writeData;
    }
        // NAND
    else if(opcode(state->MEMWB.instr) == NAND){
        // NAND
        newState->reg[field2(state->MEMWB.instr)] = state->MEMWB.writeData;
    }
        // LW or SW
    else if(opcode(state->MEMWB.instr) == LW || opcode(state->MEMWB.instr) == SW){
        // Calculate memory address
        if(opcode(state->MEMWB.instr) == LW){
            // Load
            //state->reg[field0(instr)] = state->mem[aluResult];
            newState->reg[field2(state->MEMWB.instr)] = state->MEMWB.writeData;
        }else if(opcode(state->MEMWB.instr) == SW){
            // Store
            //newState->dataMem[state->MEMWB.writeData] = state->MEMWB.writeData;
            //DO NOTHING?!?!
        }
    }
    else if(opcode(state->MEMWB.instr) == HALT) {
        printf("machine halted\n");
        result =0;
        //break;
    }
    newState->retired++;
    return result;
}//WB stage

void flush(stateType* newState)
{
    newState->IFID.instr = NOOPINSTRUCTION;

    newState->IDEX.instr = NOOPINSTRUCTION;

}//Flush


void run(stateType* state, stateType newState){
    int runner = 1;


	// Primary loop
	while(runner){

		state->cycles++;
		printState(state);

        /*------------------IF stage ---------------------*/

        IFstage(state, newState);
        if(state->cycles < 1){
            continue;
        }

        /*------------------ID stage ---------------------*/

        IDstage(state, newState);
        if(state->cycles < 2){
            continue;
        }
        /*------------------EX stage ---------------------*/

        EXstage(state, newState);
        if(state->cycles < 3){
            continue;
        }
        /*------------------MEM stage --------------------*/

        MEMstage(state, newState);
        if(state->cycles < 4){
            continue;
        }
        /*------------------WB stage ---------------------*/

        //Does this even work?
        runner = WBStage(state, newState);

        state = newState;
    }	//while

}//run

int main(int argc, char** argv){

	/** Get command line arguments **/
	char* fname;

	if(argc == 1){
		fname = (char*)malloc(sizeof(char)*100);
		printf("Enter the name of the machine code file to simulate: ");
		fgets(fname, 100, stdin);
		fname[strlen(fname)-1] = '\0'; // gobble up the \n with a \0
	}
	else if (argc == 2){

		int strsize = strlen(argv[1]);

		fname = (char*)malloc(strsize);
		fname[0] = '\0';

		strcat(fname, argv[1]);
	}else{
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
	while (EOF != (c=getc(fp))) {
		if ( c == '\n' ){
			line_count++;
		}
	}
	// reset fp to the beginning of the file
	rewind(fp);

	stateType* state = (stateType*)malloc(sizeof(stateType));
    stateType* newState = (stateType*)malloc(sizeof(stateType));

	state->pc = 0;
	memset(state->instrMem, 0, NUMMEMORY*sizeof(int));
    memset(state->dataMem, 0, NUMMEMORY*sizeof(int));
	memset(state->reg, 0, NUMREGS*sizeof(int));
	state->numMemory = line_count;

    //initialize newState
    newState->pc = 0;
    memset(newState->instrMem, 0, NUMMEMORY*sizeof(int));
    memset(newState->dataMem, 0, NUMMEMORY*sizeof(int));
    memset(newState->reg, 0, NUMREGS*sizeof(int));
    newState->numMemory = line_count;

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

	/** Run the simulation **/
	run(state, newState);

	free(state);
    free(newState);
	free(fname);

}
