#include <stdio.h>
#include <stdlib.h>

#define NUMMEMORY 65536 /* maximum number of data words in memory */
#define NUMREGS 8 /* number of machine registers */
#define ADD 0
#define NAND 1
#define LW 2
#define SW 3
#define BEQ 4
#define JALR 5 /* JALR – not implemented in this project */
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

void printState(stateType *statePtr);
int signExtend(int num);
void print_stats(int n_instrs);

int main(int argc, char** argv){

	if(argc <= 2){
	//if there is only one given argument(should be a valid file)
		FILE *file = fopen(argv[1],"r");
		if(file != NULL){

			//file does exist.
			//todo: set all registers and program counter to 0.
			//then go until program hits a halt.

			stateType state;
			//int lineVal[1024];
			for(int i = 0; i < NUMREGS; i++){
				state.reg[i] = 0;
			}
			state.pc = 0;
			state.instructionCount = 0;
			state.numMemory = 0;
			//read in the file line by line:

			char line[1024];
			//int lineNumber = 0;
			while(fgets(line,sizeof(line),file)){
				//lineVal[lineNumber]=atoi(line);
				//printf("%d at : %i\n",lineVal[lineNumber],lineNumber);
				//lineNumber++;
				state.mem[state.numMemory] = atoi(line);
				state.numMemory++;
			}

			//for(int i = 0; i < lineNumber; i++){
			int run = 1;
			int opcodeMask = 7 << 22;//0x0FF00000;//change this
			int regAMask = 0x003F0000;
			int regBMask = 0x00070000;
			int destMask = 0x00000007;
			int immediateMask = 0x0000FFFF;

			while(run){

				int line = state.mem[state.pc];
				int lineCopy = line;
				printState(&state);
				state.pc++;
				if(((lineCopy & opcodeMask) >> 22) == 0){				
					int regA=0;
					int regB=0;
					int dest=0;
					regA = (lineCopy & regAMask) >> 19;
					regB = (lineCopy & regBMask) >> 16;
					dest = (lineCopy & destMask);
					if(dest == 0){
						fprintf(stderr,"Value cannot be stored at Reg0 -- line %i\n",state.pc);
						run = 0;
					}
					regA = state.reg[regA];
					regB = state.reg[regB];
					state.reg[dest] = regA + regB;
					state.instructionCount++;
				}//ADD
				else if(((lineCopy&opcodeMask) >> 22) == 1){
					int regA=0;
					int regB=0;
					int dest=0;
					regA = (lineCopy & regAMask) >> 19;
					regB = (lineCopy & regBMask) >> 16;
					dest = (lineCopy & destMask);

					if(dest == 0){
						fprintf(stderr,"Value cannot be stored at Reg0 -- line %i\n",state.pc);
						run = 0;
					}
					regA = state.reg[regA];
					regB = state.reg[regB];
					state.reg[dest] = ~(regA & regB);
					state.instructionCount++;
				}//NAND
				else if(((lineCopy&opcodeMask) >> 22) == 2){
					int regA = 0;
					int regB = 0;
					int immediate = 0;

					regA = (lineCopy & regAMask) >> 19;
					regB = (lineCopy & regBMask) >> 16;
					immediate = (lineCopy & immediateMask);
					immediate = signExtend(immediate);
					//sign extend here if negative
					immediate = state.reg[regB] + immediate;
					state.reg[regA] = state.mem[immediate];
					state.instructionCount++;
				}//LW
				else if(((lineCopy&opcodeMask) >> 22) == 3){
					int regA = (lineCopy & regAMask) >> 19;
					int regB = (lineCopy & regBMask) >> 16;
					int immediate = (lineCopy & immediateMask);
					//check if immediate is negative
					immediate = signExtend(immediate);
					immediate = state.reg[regB] + immediate;
					state.mem[immediate] = state.reg[regA];
					state.instructionCount++;

				}
				else if(((lineCopy&opcodeMask) >> 22) == 4){
					state.instructionCount++;
					int regA = (lineCopy & regAMask) >> 19;
					int regB = (lineCopy & regBMask) >> 16;
					int immediate = (lineCopy & immediateMask);
					immediate = signExtend(immediate);
					//check if immediate is negative
					//sign extend here
					if(state.reg[regA] == state.reg[regB]){
						state.pc = state.pc + immediate;
					}
					

				}

				/* JALR (Not Needed)
				else if(((lineCopy&opcodeMask) >> 22) == 5){
					int regA=0;
					int regB=0;
					regA = (lineCopy & regAMask) >> 19;
					regB = (lineCopy & regBMask) >> 16;
					state.reg[regA] = state.pc + 1;
					regB = state.reg[regB];
					state.pc = regB;
					state.instructionCount++;

				}	
				*/

				else if(((lineCopy&opcodeMask) >> 22) == 6){
					state.instructionCount++;

					run = 0;
				}
				else if(((lineCopy&opcodeMask) >> 22) == 7){
					state.instructionCount++;
				}
			}
	
			print_stats(state.instructionCount);
		}else{
			//file does not exist
			fprintf(stderr, "The given file does not exist.\n");
			return 0;
		}


	}else{
	//if there are too many arguments
		fprintf(stderr,"Too many given arguments.\n");
		return 0;
	}

	return 0;
}

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

int signExtend(int num){
	num  = (int16_t)num;
	int32_t holder = (int32_t)num;
	return holder;
}

void print_stats(int n_instrs){
	printf("INSTRUCTIONS: %d\n", n_instrs); // total executed instructions
}


//********************* ALL PRINT STUFF HERE ******************************* 

/*

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
	printf("\t\treadRegB %d\n", statePtr->EXMEM.readRegB);
 	printf("\tMEMWB:\n");
	printf("\t\tinstruction ");
	printInstruction(statePtr->MEMWB.instr);
	printf("\t\twriteData %d\n", statePtr->MEMWB.writeData);
 	printf("\tWBEND:\n");
	printf("\t\tinstruction ");
printInstruction(statePtr->WBEND.instr);
	printf("\t\twriteData %d\n", statePtr->WBEND.writeData);
}

*/

