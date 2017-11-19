Josh Johnson and Kelsey Faulise
CISC 340
Project 3 ReadMe

There are may files in this folder.
The pipelinesim.c is the code for the simulator that we created for this project. It
will run any of the testcases when they are put into machine code.

The next file is the Makefile. This will compile and create the executable and .o file
for the simulator in order for it to run. 

The next file is the oveview document. This document describes in detail how to simulator
works. It will go through each function to explain how they work and it also talks about the issues that ran into during the development process. 

Then there is a folder that contains all of the test cases called testcases. 

testcase1: This is a test case that will address the lw bubble, to where it needs to stall in order to gather the correct value it also deals with data forwarding. It loads four values into the registers and then adds registers together and where the next instruction uses what is being stores in the instruction prior. The addition should look like: reg 5 = 10 + 1, reg 6 = 11 + 6, reg 7 = ~(4 & 17)  

testcase2: This is a test case that will deal strictly with data forwarding. This will load in three values and then add registers together.It will then save the result of the addition to memory. The results of the addition are: reg 4 = 4 + 6, reg 6 = 10 + 5, reg 5 = 6 + 15. It then stores 21 into memory address 4.  

testcase3: This is a test case that has no hazards. This is done by loading in five values and then inserting a bunch of NOOPs into the code so that the lws have time to complete the whole cycle and a properly loaded into the registers because the adds start. The results of the addition is: reg 6 = 2 + 4, reg 7 = 7 + 12, reg 8 = 15 + 2. 

testcase4: This is a test case that deals with the beq hazard and flushing. It will load in four values. It will then check to see if registers 1 and 2 are equal (o and 10) since they are not it will increment registers 2 by one and increment register 4 by itself and then continue to loop until registers 1 equals 10 and then halt. 

testcase5: This is a test case that will again deal with the beq/flushing hazards. It will load in 3 values, 1, 100, and 0. It will then check to see if registers 2 and 3 are equal (100 and 0) If it is not it will increment registers 3. This loop will continue and should run 100 times until registers 3 equals 100 and then it should halt. 

testcase6: This is a test case again deals with the beq/flusing hazard. It will load in three values and then check to see if registers 2 is equal to register 0. It will then decement the value of registers 2. This will continue, and run 50 times until register 2 euqals 0. 

testcase7: This is a test case that was given to us by Dr. Myre. It has a load word that takes ina value from a .fill and then halts.     
