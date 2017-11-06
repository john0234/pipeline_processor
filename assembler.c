#include <stdio.h>
#include <string.h>
#include "map.c"
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>

bool isValidLabel(char* arr);
bool isOpCode(char* arr);
bool isRType(char *arr, map_int_t opCodeMap);
bool isIType(char *arr, map_int_t opCodeMap);
bool isJType(char *arr, map_int_t opCodeMap);
bool isOType(char *arr, map_int_t opCodeMap);
bool isFill(char *arr, map_int_t opCodeMap);
int getOffset(int lineCount, map_int_t labelMap, char *label);

int main(int argc, char *argv[])
{

    if(argc <= 3){

        //this is the file that we are parsing through
        char *filename = argv[1];
        FILE* file = fopen(filename, "r");

        char *ops[9] = {"add","nand","lw","sw","beq","jalr","halt","noop",".fill"};


        char line[1024];
        char *token;
        int lineCount = 0;

        //initialize the map of the labels that are at the beginning of the line.
        map_int_t labelMap;
        map_init(&labelMap);

        //initialize the map for the opcodes;
        map_int_t opCodeMap;
        map_init(&opCodeMap);
        map_iter_t iter = map_iter(&opCodeMap);
        const char *key;

        for(int i = 0; i < 9; i++){

            map_set(&opCodeMap, ops[i], i);

        }//for

        if(file != NULL){

            while(fgets(line,sizeof(line),file)){

                token = strtok(line," \n");
                int i = 0;
                char *lineToks[sizeof(line)];

                while(token != NULL) {

                    lineToks[i] = token;
                    token = strtok(NULL," \n");
                    i++;
                }


                if(map_get(&opCodeMap,lineToks[0])== NULL && isValidLabel(lineToks[0])){

                    if(map_get(&labelMap,lineToks[0]) == NULL){

                        map_set(&labelMap, lineToks[0], lineCount);
                    }
                    else {

                        fprintf(stderr,"Label %s already Exists!\n",lineToks[0]);
                        return 0;
                    }
                }
                else if(map_get(&opCodeMap,lineToks[0])== NULL && !isValidLabel(lineToks[0])){

                    fprintf(stderr,"%s is not a valid label or opcode!\n", lineToks[0]);
                    return 0;
                }

                lineCount++;

            }//while

            fclose(file);

            FILE *writeToFile;

            if(argc == 3){

                writeToFile = fopen(argv[2],"w");
            }

            FILE* secondRun = fopen(filename, "r");
            lineCount = 0;
            char line2[1024];
            char *token2;

            while(fgets(line2,sizeof(line2),secondRun))
            {
                int l = 0;
                token2 = strtok(line2," \n");
                char *lineToks2[sizeof(line2)];
                int bitNumber = 0;

                while(token2 != NULL) {
                    lineToks2[l] = token2;
                    token2 = strtok(NULL, " \n");
                    l++;
                }


                int argsCount = 4;
                int startVal = 0;
                if(map_get(&labelMap,lineToks2[0]) != NULL){

                    startVal = 1;

                    if(isFill(lineToks2[1],opCodeMap)){

                        argsCount = 2;
                        startVal = 1;
                    }
                    else if(isOType(lineToks2[1],opCodeMap)){

                        argsCount = 1;
                    }
                }//if

                else if(map_get(&opCodeMap,lineToks2[0]) != NULL){

                    if(isFill(lineToks2[0],opCodeMap)){

                        argsCount = 1;
                        startVal = 1;
                    }
                    else if(isOType(lineToks2[0],opCodeMap)){

                        argsCount = 1;
                    }
                }//else if

                else{
                    fprintf(stderr,"Invalid line: %i\n",lineCount);
                    return 0;
                }

                char *builtLine[argsCount];
                int val = 0;

                for(int j = startVal; j < startVal+argsCount; j++){

                    builtLine[val] = lineToks2[j];
                    val++;
                }

                int finalBinary = 0;
                if(isRType(builtLine[0],opCodeMap)){

                    int opCodeBinary = (int)*map_get(&opCodeMap,builtLine[0]);
                    opCodeBinary = opCodeBinary << 22;
                    finalBinary = finalBinary | opCodeBinary;

                    if(map_get(&labelMap,builtLine[1]) == NULL){

                        int dst = atoi(builtLine[1]);
                        if(dst >= 0 && dst <=7){

                            finalBinary = finalBinary | dst;

                            if(map_get(&labelMap,builtLine[2]) == NULL){

                                int regA = atoi(builtLine[2]);

                                if(regA >= 0 && regA <= 7){

                                    regA = regA << 19;
                                    finalBinary = finalBinary | regA;

                                    if(map_get(&labelMap,builtLine[3]) == NULL){

                                        int regB = atoi(builtLine[3]);

                                        if(regB >= 0 && regB <= 7){

                                            regB = regB << 16;
                                            finalBinary = finalBinary | regB;

                                            if(writeToFile != NULL){

                                                fprintf(writeToFile, "%i\n",finalBinary);

                                            }else{

                                                printf("%i\n",finalBinary);
                                            }

                                        }else{

                                            fprintf(stderr,"Invalid Register at line : %i\n",lineCount);
                                            return 0;
                                        }

                                    }else{
                                        //3rd spot is a label
                                        fprintf(stderr,"Invalid label use: line %i\n",lineCount);
                                        return 0;
                                    }

                                }else{

                                    fprintf(stderr,"Invalid Register at line : %i\n",lineCount);
                                    break;
                                }
                            }
                            else{
                                //second spot is label
                                fprintf(stderr,"Invalid label use: line %i",lineCount);
                                break;
                            }
                        }
                        else{
                            fprintf(stderr,"Invalid Register at line : %i\n",lineCount);
                            break;
                        }
                    }else{
                        //first spot is label
                        fprintf(stderr,"Invalid label use: line %i",lineCount);
                        break;
                    }
                }//isRType
                else if(isIType(builtLine[0],opCodeMap)){

                    int opCode = (int)*map_get(&opCodeMap,builtLine[0]);
                    opCode = opCode << 22;
                    finalBinary = finalBinary | opCode;

                    if(map_get(&labelMap,builtLine[1]) == NULL){

                        int regA = atoi(builtLine[1]);
                        if(regA >= 0 && regA <= 7){

                            regA = regA << 19;
                            finalBinary = finalBinary | regA;
                            if(map_get(&labelMap,builtLine[2]) == NULL){
                                int regB = atoi(builtLine[2]);
                                if(regB >= 0 && regB <= 7){

                                    regB = regB << 16;
                                    finalBinary = finalBinary | regB;

                                    if(map_get(&labelMap,builtLine[3]) == NULL){
                                        int immediate = atoi(builtLine[3]);
                                        if(immediate > 32768 || immediate < -32767){
                                            fprintf(stderr,"Immediate value is invalid.\n");
                                            break;
                                        }else{
                                            if(immediate < 0){
                                                int hex = 0x0000FFFF;
                                                immediate = immediate & hex;
                                                finalBinary = finalBinary | immediate;
                                                if(writeToFile == NULL){
                                                    printf("%i\n",finalBinary);
                                                }else{
                                                    fprintf(writeToFile,"%i\n",finalBinary);
                                                }
                                            }else{
                                                finalBinary = finalBinary | immediate;
                                                if(writeToFile != NULL){
                                                    fprintf(writeToFile,"%i\n",finalBinary);
                                                }else{
                                                    printf("%i\n",finalBinary);
                                                }
                                            }
                                        }
                                    }else{
                                        if(*map_get(&opCodeMap,builtLine[0]) == *map_get(&opCodeMap,"beq")){

                                            int offset = getOffset(lineCount+1,labelMap,builtLine[3]);
                                            if(offset <= 32767 && offset >= -32768){
                                                if(offset < 0){
                                                    int hex = 0x0000FFFF;
                                                    offset = offset & hex;
                                                    finalBinary = finalBinary | offset;
                                                    if(writeToFile != NULL){
                                                        fprintf(writeToFile, "%i\n",finalBinary);
                                                    }else{
                                                        printf("%i\n",finalBinary);
                                                    }
                                                }else{
                                                    //greater than 0
                                                    finalBinary = finalBinary | offset;
                                                    if(writeToFile != NULL){
                                                        fprintf(writeToFile, "%i\n",finalBinary);
                                                    }else{
                                                        printf("%i\n",finalBinary);
                                                    }
                                                }
                                            }else{
                                                //not 16bits
                                                fprintf(stderr,"Immediate value is too large - line %i\n",lineCount);
                                                break;
                                            }

                                        }else{
                                            int offset = getOffset(0,labelMap,builtLine[3]);
                                            if(offset <= 32767 && offset >= -32768){
                                                if(offset < 0){
                                                    int hex = 0x0000FFFF;
                                                    offset = offset & hex;
                                                    finalBinary = finalBinary | offset;
                                                    if(writeToFile != NULL){
                                                        fprintf(writeToFile, "%i\n",finalBinary);
                                                    }else{
                                                        printf("%i\n",finalBinary);
                                                    }
                                                }else{
                                                    //greater than 0
                                                    finalBinary = finalBinary | offset;
                                                    if(writeToFile != NULL){
                                                        fprintf(writeToFile, "%i\n",finalBinary);
                                                    }else{
                                                        printf("%i\n",finalBinary);
                                                    }
                                                }
                                            }else{
                                                //not 16bits
                                                fprintf(stderr,"Immediate value is too large - line %i\n",lineCount);
                                                return 0;;
                                            }
                                        }
                                    }//else
                                }else{

                                    fprintf(stderr,"Invalid Register at line : %i\n",lineCount);
                                    return 0;
                                }
                            }else{
                                //its a label
                                fprintf(stderr,"Invalid label use: line %i",lineCount);
                                return 0;
                            }
                        }
                        else{
                            //invalid register
                            fprintf(stderr,"Invalid Register at line : %i\n",lineCount);
                            return 0;
                        }
                    }else{

                        //its a label
                        fprintf(stderr,"Invalid label use: line %i",lineCount);
                        return 0;;
                    }
                }

                else if(isJType(builtLine[0],opCodeMap)){

                    int opCodeBinary = (int)*map_get(&opCodeMap,builtLine[0]);
                    opCodeBinary = opCodeBinary << 22;
                    finalBinary = finalBinary | opCodeBinary;

                    if(map_get(&labelMap,builtLine[1])==NULL){

                        int regA = atoi(builtLine[1]);
                        if(regA >= 0 && regA <= 7){

                            regA = regA << 19;
                            finalBinary = finalBinary | regA;

                            if(map_get(&labelMap,builtLine[2]) == NULL){

                                int regB = atoi(builtLine[2]);

                                if(regB >=0 && regB <= 7){

                                    regB = regB << 16;
                                    finalBinary = finalBinary | regB;

                                    if(writeToFile == NULL){

                                        printf("%i\n",finalBinary);

                                    }else{
                                        fprintf(writeToFile,"%i\n",finalBinary);
                                    }
                                }else{

                                    fprintf(stderr,"Invalid Register at line : %i\n",lineCount);
                                    return 0;
                                }

                            }else{
                                fprintf(stderr,"Invalid label use: line %i",lineCount);
                                return 0;
                            }

                        }else{

                            fprintf(stderr,"Invalid Register at line : %i",lineCount);
                            return 0;
                        }

                    }else{
                        fprintf(stderr,"Invalid label use: line %i",lineCount);
                        return 0;
                    }

                }

                else if(isOType(builtLine[0],opCodeMap)){
                    
                    finalBinary = (int)*map_get(&opCodeMap,builtLine[0]) << 22;

                    if(writeToFile == NULL){

                        printf("%i\n",finalBinary);

                    }else{

                        fprintf(writeToFile,"%i\n",finalBinary);
                    }

                }

                else if(isFill(builtLine[0],opCodeMap)){
                    if(map_get(&labelMap,builtLine[1]) != NULL){

                        finalBinary = (int)*map_get(&labelMap,builtLine[1]);

                        if(writeToFile != NULL){

                            fprintf(writeToFile, "%d\n",finalBinary);

                        }else{

                            printf("%i\n",finalBinary);
                        }

                    }else{

                        finalBinary = atoi(builtLine[1]);
                        if(writeToFile != NULL){

                            fprintf(writeToFile, "%i\n",finalBinary);

                        }else{
                            printf("%i\n",finalBinary);
                        }
                    }//else
                }//isFill

                lineCount++;
            }//while

            if(writeToFile != NULL){

                fclose(writeToFile);
            }
            fclose(secondRun);

        }// <---- IF FILE ISNT NULL

        else{
            //the given FILE is not a valid one
            fprintf(stderr,"The given file is NULL\n");
        }//else

    }//if <- ARGS statement
    else{
        fprintf(stderr,"TOO MANY ARGUMENTS GIVEN\n");
    }//else

    return 0;
}

bool isValidLabel(char* arr){

    int num = 0;
    bool ret = true;

    if(strlen(arr)-1 > 6){
     	fprintf(stderr,"Invalid Label");
    }
    else if(isdigit(arr[0]) != 0){
        num++;
    }

    for(int i = 0; i < strlen(arr)-1; i++){

        if(isalpha(arr[i]) == 0 && isdigit(arr[i]) == 0){

            num++;
        }
    }

    if(num > 0){

        ret = false;
    }

    return ret;

}

bool isRType(char *arr, map_int_t opCodeMap){


    if(map_get(&opCodeMap,arr) != NULL)
    {
        if(*map_get(&opCodeMap,arr) == *map_get(&opCodeMap,"add") || *map_get(&opCodeMap,arr) == *map_get(&opCodeMap, "nand"))
        {
            return true;
        }//if
        else
        {
            return false;
        }


    }//if
    else{
        return false;
    }
}

bool isIType(char *arr, map_int_t opCodeMap){

    if(map_get(&opCodeMap,arr)!= NULL) {

        if(*map_get(&opCodeMap,arr) == *map_get(&opCodeMap,"lw") || *map_get(&opCodeMap,arr) == *map_get(&opCodeMap, "sw") || *map_get(&opCodeMap,arr) == *map_get(&opCodeMap,"beq")) {
            return true;
        }else{
            return false;
        }
    }else{
        return false;
    }
}

bool isJType(char *arr, map_int_t opCodeMap){

    if(map_get(&opCodeMap,arr)!= NULL){

        if(*map_get(&opCodeMap,arr) == *map_get(&opCodeMap,"jalr")){
            return true;
        }else{
            return false;
        }
    }else{
        return false;
    }
}

bool isOType(char *arr, map_int_t opCodeMap)
{
    if(map_get(&opCodeMap,arr)!= NULL){

        if(*map_get(&opCodeMap,arr) == *map_get(&opCodeMap,"halt") || *map_get(&opCodeMap,arr) == *map_get(&opCodeMap, "noop")) {
            return true;
        }else{
            return false;
        }
    }else{
        return false;
    }
}

bool isFill(char *arr, map_int_t opCodeMap){

    if(map_get(&opCodeMap,arr)!= NULL){

        if(*map_get(&opCodeMap,arr) == *map_get(&opCodeMap,".fill")){
            return true;
        }else{
            return false;
        }

    }else{
        return false;
    }
}

int getOffset(int lineCount, map_int_t labelMap, char *label){

    int offset = 0;
    int labelLine = *map_get(&labelMap,label);
    offset = labelLine - lineCount;
    return offset;
}



