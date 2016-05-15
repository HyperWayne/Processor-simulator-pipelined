#include <stdlib.h>
#include <stdio.h>
#include "define.h"
int num;
int Error_Delay;
int Write_Delay;
int value;
int ZZ;

char * Instruction;
char * ibuffer, *dbuffer;
char *Rs_Rt;
char *Rs_Rt2;
char *sIns[1024], *sIns2[1024];
char iMemory[1024], dMemory[1024];
unsigned PC, reg[32], cycle, pc;
unsigned iSize, dSize;
unsigned iresult, dresult;

int Write_Zero;
int Number_Overflow;
int Address_Overflow;
int Misalignment_Error;


FILE * i_image, *d_image, *err, *snap;

void toImemory();
void toDmemory();
void decode();
void pipeline();
void printError();


int WriteToZero(int number);
void RsRdRt(unsigned *rs, unsigned *rd, unsigned *rt);
void ShiftAmount(unsigned *shamt);
void Immediate(unsigned *immediate);
int offset(unsigned *p,unsigned short *imm, unsigned *rs);
void dumpSnap();
int WriteToZero();

int WB();
int DM();
void EX();
void ID();
void IF();
