#include <stdio.h>
#include <stdlib.h>

typedef struct IF_to_ID{

	unsigned pc;
	unsigned instruc;

}IF2ID;

IF2ID IF_ID;

typedef struct ID_to_EX{

	unsigned opcode;
	unsigned funct;
	unsigned shamt;
	unsigned rt;
	unsigned rd;
	unsigned rs;
	unsigned address;
	unsigned immediate;
	unsigned instruc;
	unsigned $rs;
	unsigned $rt;
	unsigned pc;
	unsigned stall;
	unsigned forwarding;
	unsigned flush;
	unsigned predict;
	unsigned jump;
	unsigned signal;


}ID2EX;

ID2EX ID_EX;
ID2EX ID_ID;

typedef struct EX_to_DM{

    unsigned funct;
    unsigned instruc;
    unsigned opcode;
    unsigned rd;
    unsigned rs;
    unsigned rt;
    unsigned $rt;
    unsigned $rs;
    unsigned MDR;
    unsigned alu_out;
    unsigned pc;
    unsigned forwarding;
    unsigned shamt;
    unsigned predict;
    unsigned signal;

}EX2DM;

EX2DM EX_DM;

typedef struct DM_to_WB{

    unsigned funct;
    unsigned instruc;
    unsigned opcode;
    unsigned $rt;
    unsigned $rs;
    unsigned rt;
    unsigned rd;
    unsigned rs;
    unsigned MDR;
    unsigned alu_out;
    unsigned writeError;
    unsigned shamt;
    unsigned address;
    unsigned pc;

}DM2WB;

DM2WB DM_WB;
DM2WB WB_WB;
DM2WB COOL;

