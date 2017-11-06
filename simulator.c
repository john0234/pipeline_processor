#include <stdio.h>
#include <stdlib.h>


#define NUMMEMORY 65536
#define NUMREGS 8


typedef struct stateStruct{
	int pc;
	int reg[NUMREGS];
	int numMemory;
	int mem[NUMMEMORY];
	int instructionCount;
}stateType;

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
				//	printf("This is regA -- %i\n",regA);
				//	printf("This is regB -- %i\n",regB);
				//	printf("This is dest -- %i\n",dest);
					regA = state.reg[regA];
					regB = state.reg[regB];
					state.reg[dest] = regA + regB;
					state.instructionCount++;
				}//ADD
				else if(((lineCopy&opcodeMask) >> 22) == 1){
				//	printf("NAND instruction\n");
					//state.pc++;
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
				//	printf("This is regA -- %i\n",regA);
				//	printf("This is regB -- %i\n",regB);
				//	printf("This is dest -- %i\n",dest);
					regA = state.reg[regA];
					regB = state.reg[regB];
					state.reg[dest] = ~(regA & regB);
					state.instructionCount++;
				}//NAND
				else if(((lineCopy&opcodeMask) >> 22) == 2){
				//	printf("LW instruction\n");
					//state.pc++;
					int regA = 0;
					int regB = 0;
					int immediate = 0;

					regA = (lineCopy & regAMask) >> 19;
					regB = (lineCopy & regBMask) >> 16;
					immediate = (lineCopy & immediateMask);
					immediate = signExtend(immediate);
					//sign extend here if negative
				//	printf("this is regA -- %i\n",regA);
				//	printf("this is regB -- %i\n", regB);
				//	printf("this is immediate val -- %i\n",immediate);

					immediate = state.reg[regB] + immediate;
					state.reg[regA] = state.mem[immediate];
				//	printf("this is state.reg[regA] -- %i\n",state.reg[regA]);
					state.instructionCount++;
				}//LW
				else if(((lineCopy&opcodeMask) >> 22) == 3){
				//	printf("SW instruction\n");
					//state.pc++;
					int regA = (lineCopy & regAMask) >> 19;
					int regB = (lineCopy & regBMask) >> 16;
					int immediate = (lineCopy & immediateMask);
					//check if immediate is negative
					immediate = signExtend(immediate);
					immediate = state.reg[regB] + immediate;
					state.mem[immediate] = state.reg[regA];
					
				//	printf("this is where we store %i\n",immediate);
				//	printf("this is what we are storing %i\n",state.reg[regA]);
					state.instructionCount++;

				}
				else if(((lineCopy&opcodeMask) >> 22) == 4){
				//	printf("BEQ instruction\n");
					//state.pc++;
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
				else if(((lineCopy&opcodeMask) >> 22) == 5){
					//state.pc++;
				//	printf("JALR instruction\n");
					int regA=0;
					int regB=0;
					regA = (lineCopy & regAMask) >> 19;
					regB = (lineCopy & regBMask) >> 16;
					state.reg[regA] = state.pc + 1;
					regB = state.reg[regB];
					state.pc = regB;
					state.instructionCount++;

				}
				else if(((lineCopy&opcodeMask) >> 22) == 6){
				//	printf("HALT instruction\n");
					//state.pc++;
					state.instructionCount++;

					run = 0;
				}
				else if(((lineCopy&opcodeMask) >> 22) == 7){
				//	printf("NOOP instruction\n");
					///state.pc++;
					state.instructionCount++;

				}

				//state.pc++;
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
