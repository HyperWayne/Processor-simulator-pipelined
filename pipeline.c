#include <stdlib.h>
#include <stdio.h>

#include "define.h"
#include "globalVariables.h"
#include "Buffer.h"
#include "initialize.h"

int main(){


    i_image = fopen ( "iimage.bin"    , "rb" );
    d_image = fopen ( "dimage.bin"    , "rb" );
    err     = fopen (  "error_dump.rpt", "wb");
    snap    = fopen (  "snapshot.rpt"  , "wb");

    fseek (i_image, 0 , SEEK_END);
    fseek (d_image, 0 , SEEK_END);

    iSize = ftell (i_image);
    dSize = ftell (d_image);

    rewind (i_image);
    rewind (d_image);

    ibuffer = (char*) malloc (sizeof(char)*iSize);
    dbuffer = (char*) malloc (sizeof(char)*dSize);

    iresult = fread (ibuffer,1,iSize,i_image);
    dresult = fread (dbuffer,1,dSize,d_image);

    if(dSize!=0)
    toDmemory();
    toImemory();
    setInstruction();
    pipeline();

    fclose (i_image);
    fclose (d_image);

    free(ibuffer);
    free(dbuffer);
    return 0;


}

void toImemory()
{
    unsigned i, temp = 0, idx = 0;

	for (i = 0; i < 4; i++)
		temp = (temp << 8) + (unsigned char) ibuffer[i];
	PC = temp;

	temp = 0;
	for (i = 4; i < 8; i++)
		num = temp = (temp << 8) + (unsigned char) ibuffer[i];

	for (i = 8; i < 8 + 4 * temp; i++)
		iMemory[idx++] = ibuffer[i];
}

void toDmemory()
{



    unsigned i, temp = 0, idx = 0;
	// Get the value of $sp.
	for (i = 0; i < 4; i++)
		temp = (temp << 8) + (unsigned char) dbuffer[i];
	reg[$sp] = temp;
	// Get the number for D memory.
	temp = 0;
	for (i = 4; i < 8; i++)
		temp = (temp << 8) + (unsigned char) dbuffer[i];
	// Write the value to D memory.
	for (i = 8; i < 8 + 4 * temp; i++)
		dMemory[idx++] = dbuffer[i];

}

void pipeline()
{

    int error = 0;

    while(1){

        if(WB()) break;
        error = DM();
        EX();
        ID();
        IF();
        dumpSnap();
        printError();
        if(error) break;

       //if(cycle > 0) break;
    }
}

void printError()
{
    if (Write_Zero) {
		Write_Zero = 0;
		fprintf(err, "In cycle %d: Write $0 Error\n", cycle);
	}

	if (Address_Overflow) {
		fprintf(err, "In cycle %d: Address Overflow\n", cycle);
	}

	if (Misalignment_Error) {
		fprintf(err, "In cycle %d: Misalignment Error\n", cycle);
	}

	if (Number_Overflow /*&& !Error_Delay*/) {
		Number_Overflow = 0;
		fprintf(err, "In cycle %d: Number Overflow\n", cycle);
	}
}

void dumpSnap()
{

    unsigned i;
	fprintf(snap, "cycle %u\n", cycle++);


	for (i = 0; i < 32; ++i)
    {
        if(i==DM_WB.address){
		fprintf(snap, "$%02u: 0x%08X\n", i, value);
            DM_WB.address = 100;
        }else
        fprintf(snap, "$%02u: 0x%08X\n", i, reg[i]);

	}

	fprintf(snap, "PC: 0x");
	fprintf(snap, "%08X\n", PC + IF_ID.pc );

	//IF
	fprintf(snap, "IF: 0x%08X", IF_ID.instruc);
	if(ID_EX.stall) fprintf(snap, " to_be_stalled");
	if(ID_EX.flush) fprintf(snap, " to_be_flushed");
	fprintf(snap,"\n");

    //ID
	if(ID_EX.opcode==0&&ID_EX.rt==0&&ID_EX.rd==0&&ID_EX.shamt==0&&ID_EX.funct==0) fprintf(snap, "ID: NOP");
	else fprintf(snap, "ID: %s", (ID_EX.opcode==R_TYPE)?sIns[ID_EX.funct]:sIns2[ID_EX.opcode]);
	if(ID_EX.stall) fprintf(snap, " to_be_stalled");
	if(ID_EX.forwarding){

        if(ID_EX.rs==ID_EX.rt && (ID_EX.opcode==BEQ||ID_EX.opcode==BNE) ) {
            fprintf(snap, " fwd_EX-DM_rs_$%d",ID_EX.rs);
            fprintf(snap, " fwd_EX-DM_rt_$%d",ID_EX.rt);
        }else {
             fprintf(snap, " fwd_EX-DM_%s_$%d",Rs_Rt,(Rs_Rt=="rs")?ID_EX.rs:ID_EX.rt);
        }
	}
    fprintf(snap,"\n");

    //EX
	if(EX_DM.opcode==0&&EX_DM.rt==0&&EX_DM.rd==0&&EX_DM.shamt==0&&EX_DM.funct==0) fprintf(snap, "EX: NOP");
	else fprintf(snap, "EX: %s", (EX_DM.opcode==R_TYPE)?sIns[EX_DM.funct]:sIns2[EX_DM.opcode]);
	if(EX_DM.forwarding){

        if(EX_DM.rs==EX_DM.rt && EX_DM.opcode==R_TYPE && EX_DM.funct!=0 && EX_DM.funct!=2 && EX_DM.funct!=3 ) {
            fprintf(snap, " fwd_EX-DM_rs_$%d",EX_DM.rs);
            fprintf(snap, " fwd_EX-DM_rt_$%d",EX_DM.rt);
        }
        else if(EX_DM.rs==EX_DM.rt && (EX_DM.opcode==SB||EX_DM.opcode==SH||EX_DM.opcode==SW)) {
            fprintf(snap, " fwd_EX-DM_rs_$%d",EX_DM.rs);
            fprintf(snap, " fwd_EX-DM_rt_$%d",EX_DM.rt);
        }
        else {
             fprintf(snap, " fwd_EX-DM_%s_$%d",Rs_Rt2,(Rs_Rt2=="rs")?EX_DM.rs:EX_DM.rt);
        }
	}
    fprintf(snap,"\n");

    //DM
	if(DM_WB.opcode==0&&DM_WB.rt==0&&DM_WB.rd==0&&DM_WB.shamt==0&&DM_WB.funct==0) fprintf(snap, "DM: NOP\n"); else
	fprintf(snap, "DM: %s\n", (DM_WB.opcode==R_TYPE)?sIns[DM_WB.funct]:sIns2[DM_WB.opcode]);

    //WB
	if(WB_WB.opcode==0&&WB_WB.rt==0&&WB_WB.rd==0&&WB_WB.shamt==0&&WB_WB.funct==0) fprintf(snap, "WB: NOP\n\n\n"); else
	fprintf(snap, "WB: %s\n\n\n", (WB_WB.opcode==R_TYPE)?sIns[WB_WB.funct]:sIns2[WB_WB.opcode]);
}

void IF()
{
    int i;
    unsigned a=0;

    if( (int)(pc) +4 >= 4)
        for(i=0; i<4; i++) {   a = (a<<8) + (unsigned char) iMemory[i+pc]; }

        IF_ID.instruc = a;

    if(pc +4 >= 0)
        IF_ID.pc = pc;
    else ID_ID.pc = 0;

        if(ID_EX.flush){

            if(ID_EX.opcode==JAL){
                pc = (( pc + 4 ) >> 28 << 28 ) | (ID_EX.jump << 2);
                pc = pc - 4 -PC ;
            }

            else if(ID_EX.opcode==J){

                  pc = (( pc + 4 ) >> 28 << 28 ) | (ID_EX.jump << 2);
                  pc = pc -PC -4;
            }

            else if(ID_EX.opcode==0 && ID_EX.funct==JR ){

                  pc = ID_EX.jump;
                  pc = pc -PC - 4;
            }

            else {

                 pc = pc + ID_EX.address;
                 pc = pc - 4;
            }

        }


    if( (int)pc+PC <= PC)
    {
         pc = pc + 4;
    }

    else
    {
        if(ID_EX.stall) pc = pc;
        else pc = pc + 4 ;
    }

}

void ID()
{

    if(ID_EX.flush) IF_ID.instruc = 0;

        ID_EX.$rt = reg[ID_EX.rt];
        ID_EX.$rs = reg[ID_EX.rs];

    if(!ID_EX.stall && IF_ID.pc>=0 ){

        ID_EX.instruc = IF_ID.instruc;
        ID_EX.opcode = IF_ID.instruc >> 26;
        ID_EX.pc = IF_ID.pc;
        if(ID_EX.opcode==HALT) {
            ID_EX.forwarding=0;  return;
        }
        else if(ID_EX.opcode==J){
            ID_EX.address = IF_ID.instruc << 6 >> 6;
        } else if(ID_EX.opcode==JAL){
            ID_EX.address = IF_ID.instruc << 6 >> 6;
        } else if(ID_EX.opcode==R_TYPE){

            ID_EX.funct = IF_ID.instruc << 26 >> 26;
            ID_EX.shamt = IF_ID.instruc << 21 >> 27;

            ID_EX.rd = IF_ID.instruc << 16 >> 27;
            ID_EX.rt = IF_ID.instruc << 11 >> 27;
            ID_EX.rs = IF_ID.instruc << 6 >> 27;
        }
        else{
            switch(ID_EX.opcode){

            case ANDI:
                ID_EX.immediate = IF_ID.instruc << 16 >> 16;
                ID_EX.rt = IF_ID.instruc << 11 >> 27;
                ID_EX.rs = IF_ID.instruc << 6 >> 27;
                ID_EX.$rt = reg[ID_EX.rt];
                ID_EX.$rs = reg[ID_EX.rs];
                break;
            case ORI:
                ID_EX.immediate = IF_ID.instruc << 16 >> 16;
                ID_EX.rt = IF_ID.instruc << 11 >> 27;
                ID_EX.rs = IF_ID.instruc << 6 >> 27;
                ID_EX.$rt = reg[ID_EX.rt];
                ID_EX.$rs = reg[ID_EX.rs];
                break;
            case NORI:
                ID_EX.immediate = IF_ID.instruc << 16 >> 16;
                ID_EX.rt = IF_ID.instruc << 11 >> 27;
                ID_EX.rs = IF_ID.instruc << 6 >> 27;
                ID_EX.$rt = reg[ID_EX.rt];
                ID_EX.$rs = reg[ID_EX.rs];
                break;
         /*   case LHU:
                ID_EX.immediate = IF_ID.instruc << 16 >> 16;
                ID_EX.rt = IF_ID.instruc << 11 >> 27;
                ID_EX.rs = IF_ID.instruc << 6 >> 27;
                ID_EX.$rt = reg[ID_EX.rt];
                ID_EX.$rs = reg[ID_EX.rs];
                break;
            case LBU:
                ID_EX.immediate = IF_ID.instruc << 16 >> 16;
                ID_EX.rt = IF_ID.instruc << 11 >> 27;
                ID_EX.rs = IF_ID.instruc << 6 >> 27;
                ID_EX.$rt = reg[ID_EX.rt];
                ID_EX.$rs = reg[ID_EX.rs];
                break;*/

            default:
                ID_EX.rt = IF_ID.instruc << 11 >> 27;
                ID_EX.rs = IF_ID.instruc << 6 >> 27;
                ID_EX.$rt = reg[ID_EX.rt];
                ID_EX.$rs = reg[ID_EX.rs];
                int extend = IF_ID.instruc;
                ID_EX.immediate = extend << 16 >> 16;
                break;
            }
        }
    }// If Stall, then we won't decode anything.


        /** Implement Stall and Forward*/

        ID_EX.forwarding = 0;

        /** Set some common condition */
         int EX_DM_I_condition = (EX_DM.opcode==LW || EX_DM.opcode==LH || EX_DM.opcode== LHU || EX_DM.opcode== LB ||EX_DM.opcode==LBU /*|| EX_DM.opcode==SW|| EX_DM.opcode==SB || EX_DM.opcode==SH*/ );

         int EX_DM_I_condition_no_SW = (EX_DM.opcode==LW || EX_DM.opcode==LH || EX_DM.opcode== LHU || EX_DM.opcode== LB ||EX_DM.opcode==LBU );

         int DM_WB_I_condition_no_SW = (DM_WB.opcode==LW || DM_WB.opcode==LH || DM_WB.opcode== LHU || DM_WB.opcode== LB ||DM_WB.opcode==LBU );

         int DM_WB_I_condition = (DM_WB.opcode==LW || DM_WB.opcode==LH || DM_WB.opcode== LHU || DM_WB.opcode== LB || DM_WB.opcode==LBU || DM_WB.opcode==SW|| DM_WB.opcode==SB || DM_WB.opcode==SH  );

         int DM_WB_I_Return_rt = (DM_WB.opcode==ADDI || DM_WB.opcode==ADDIU || DM_WB.opcode==ANDI || DM_WB.opcode==ORI || DM_WB.opcode==NORI || DM_WB.opcode==SLTI || DM_WB.opcode==LUI ||

                                DM_WB.opcode==LW || DM_WB.opcode==LH || DM_WB.opcode== LHU || DM_WB.opcode== LB || DM_WB.opcode==LBU );

         int EX_DM_I_Return_rt = (EX_DM.opcode==ADDI || EX_DM.opcode==ADDIU || EX_DM.opcode==ANDI || EX_DM.opcode==ORI || EX_DM.opcode==NORI || EX_DM.opcode==SLTI || EX_DM.opcode==LUI ||

                                EX_DM.opcode==LW || EX_DM.opcode==LH || EX_DM.opcode== LHU || EX_DM.opcode== LB || EX_DM.opcode==LBU );

         int DM_WB_I_Return_rt_no_LW = (DM_WB.opcode==ADDI || DM_WB.opcode==ADDIU || DM_WB.opcode==ANDI || DM_WB.opcode==ORI || DM_WB.opcode==NORI || DM_WB.opcode==SLTI || DM_WB.opcode==LUI );

         int EX_DM_I_Return_rt_no_LW = (EX_DM.opcode==ADDI || EX_DM.opcode==ADDIU || EX_DM.opcode==ANDI || EX_DM.opcode==ORI || EX_DM.opcode==NORI || EX_DM.opcode==SLTI || EX_DM.opcode==LUI );

         /** rt only */
         if( (ID_EX.opcode==R_TYPE && ( (ID_EX.funct==SLL) || ID_EX.funct==SRL || ID_EX.funct==SRA ))) {

                if(ID_EX.opcode==0&&ID_EX.rt==0&&ID_EX.rd==0&&ID_EX.shamt==0&&ID_EX.funct==0) ID_EX.stall = 0;

                else if( EX_DM.opcode==R_TYPE && ID_EX.rt ==EX_DM.rd  && EX_DM.funct!=0x08 &&EX_DM.rd!=0 ){ ID_EX.stall = 0;}

                else  if( EX_DM_I_Return_rt_no_LW && ID_EX.rt ==EX_DM.rt &&EX_DM.instruc!=0 ){ ID_EX.stall = 0;}

                else if( EX_DM_I_condition && ID_EX.rt==EX_DM.rt && EX_DM.rt!=0 ) ID_EX.stall = 1; // CANNOT forward in EX stage

                else if( DM_WB_I_Return_rt && ID_EX.rt==DM_WB.rt && DM_WB.rt!=0 ) ID_EX.stall = 1; // Must be stalled in DM stage

                else if( DM_WB.opcode==R_TYPE && ID_EX.rt==DM_WB.rd && DM_WB.funct!= 0x08 && DM_WB.rt!=0 ) {

                    if(EX_DM.opcode==R_TYPE  && EX_DM.rd==ID_EX.rt) {ID_EX.stall =0;}
                    else ID_EX.stall = 1;
                }

                else if( DM_WB.opcode == JAL && ( ID_EX.rt==31) ) ID_EX.stall=1;

                else {ID_EX.stall = 0;}

         }

         /** MOST I_TYPE rs only */
        else if(ID_EX.opcode>=8 && ID_EX.opcode <= 0x2B && ID_EX.opcode!=LUI || (ID_EX.opcode==R_TYPE&&ID_EX.funct==0x08)  || ID_EX.opcode==BGTZ){

                /** Predict Forwarding */

                int predict = 0;

                if(ID_EX.opcode==BGTZ && EX_DM.opcode==R_TYPE &&EX_DM.rd!=0 && EX_DM.rd==ID_EX.rs) {ID_EX.stall = 1;predict = 1;}

                else if((ID_EX.opcode==R_TYPE&&ID_EX.funct==0x08) && EX_DM_I_Return_rt_no_LW && EX_DM.rt==ID_EX.rs && EX_DM.rt!=0 ){
                    ID_EX.stall = 1; predict =1;
                }
                else if((ID_EX.opcode==R_TYPE&&ID_EX.funct==0x08) && EX_DM.opcode==R_TYPE && EX_DM.rd==ID_EX.rs && EX_DM.rd!=0 ){
                    ID_EX.stall = 1; predict =1;
                }

                else if(ID_EX.opcode==BGTZ && EX_DM_I_Return_rt_no_LW && EX_DM.rt==ID_EX.rs && EX_DM.rt!=0 ){
                    predict = 1; ID_EX.stall =1;

                }

                else if( EX_DM.opcode==R_TYPE && ID_EX.rs ==EX_DM.rd  && EX_DM.funct!=0x08 &&EX_DM.rd!=0 && ID_EX.opcode!=SW &&ID_EX.opcode!=SB && ID_EX.opcode!=SH){

                        if(DM_WB.opcode==R_TYPE && ID_EX.rs==DM_WB.rd && DM_WB.rd!=0 ) {ID_EX.stall = 0; predict = 1;}

                        else ID_EX.stall = 0;

                }


                else if( ID_EX.opcode==0 && EX_DM_I_Return_rt && ID_EX.rs == EX_DM.rt && EX_DM.rt!=0) {ID_EX.stall = 1; }

                else if( (ID_EX.opcode==SW||ID_EX.opcode==SH||ID_EX.opcode==SB)   && (EX_DM.opcode==0&&EX_DM.rt==0&&EX_DM.rd==0&&EX_DM.shamt==0&&EX_DM.funct==0) ){

                    if((DM_WB.opcode==SW || DM_WB.opcode==SH || DM_WB.opcode==SB))  ID_EX.stall = 0;
                    else if( (DM_WB_I_Return_rt) && (DM_WB.rt==ID_EX.rs||DM_WB.rt==ID_EX.rt))   ID_EX.stall = 1;
                    else if(DM_WB.opcode==R_TYPE && DM_WB.rd!=0 && (DM_WB.rd==ID_EX.rt||DM_WB.rd==ID_EX.rs)) ID_EX.stall = 1;
                    else if(DM_WB.opcode==JAL && (ID_EX.rs==31 ||ID_EX.rt==31) ) ID_EX.stall = 1;
                    else ID_EX.stall = 0;

                }

                else if( (ID_EX.opcode==SW||ID_EX.opcode==SH||ID_EX.opcode==SB) && ( EX_DM.opcode==R_TYPE&&EX_DM.funct!=0x08) && EX_DM.rd!=0 ){

                    if(EX_DM.rd==ID_EX.rt && EX_DM.rd!=0) {

                            if(DM_WB.opcode==R_TYPE && DM_WB.funct!=0x08 && DM_WB.rd==ID_EX.rs &&DM_WB.rd!=0 ) { ID_EX.stall = 1;}
                            else if(DM_WB.opcode==R_TYPE && DM_WB.funct!=0x08 && DM_WB.rd==ID_EX.rt &&DM_WB.rd!=0 && (EX_DM.opcode==SB||EX_DM.opcode==SH||EX_DM.opcode==SW)){ ID_EX.stall = 0;}
                            else if(DM_WB_I_Return_rt && DM_WB.rt!=0 && DM_WB.rt==ID_EX.rs ){ ID_EX.stall = 1;}
                            else if(DM_WB_I_Return_rt && DM_WB.rt!=0 && DM_WB.rt==ID_EX.rt ){ ID_EX.stall = 0;}
                            else if(DM_WB_I_Return_rt && DM_WB.rt!=0 && (DM_WB.rt==ID_EX.rs||DM_WB.rt==ID_EX.rt) && (EX_DM.opcode==SB||EX_DM.opcode==SH||EX_DM.opcode==SW)){   ID_EX.stall = 1;}
                            else ID_EX.stall = 0;


                    }else if(DM_WB_I_Return_rt && (ID_EX.rs==DM_WB.rt || ID_EX.rt==DM_WB.rt) && DM_WB.rt!=0 ){
                          ID_EX.stall = 1;
                    }else if (DM_WB.opcode==R_TYPE && (ID_EX.rs==DM_WB.rd|| ID_EX.rt==DM_WB.rd) && DM_WB.funct!= 0x08 && DM_WB.instruc!=0){
                              ID_EX.stall = 1;
                    }else ID_EX.stall = 0;

                }

                else if( (ID_EX.opcode==SW||ID_EX.opcode==SH||ID_EX.opcode==SB) && (EX_DM_I_Return_rt_no_LW||EX_DM.opcode==SB||EX_DM.opcode==SH||EX_DM.opcode==SW) && EX_DM.rt!=0 ){

                    if(ID_EX.rt==ID_EX.rs && ID_EX.rt==EX_DM.rt && EX_DM_I_Return_rt_no_LW && EX_DM.rt!=0){ ID_EX.stall =0;}

                    else if(EX_DM.rt==ID_EX.rt && EX_DM.rt!=0) {

                            if(DM_WB.opcode==R_TYPE && DM_WB.funct!=0x08 && DM_WB.rd==ID_EX.rs &&DM_WB.rd!=0 && EX_DM.rt!=ID_EX.rs) { ID_EX.stall = 1;}
                            else if(DM_WB.opcode==R_TYPE && DM_WB.funct!=0x08 && DM_WB.rd==ID_EX.rt &&DM_WB.rd!=0 && (EX_DM.opcode==SB||EX_DM.opcode==SH||EX_DM.opcode==SW)){ ID_EX.stall = 1;}
                            else if(DM_WB_I_Return_rt && DM_WB.rt!=0 && DM_WB.rt==ID_EX.rs && EX_DM.rs!=ID_EX.rs){ ID_EX.stall = 1;}
                            else if(DM_WB_I_Return_rt && DM_WB.rt!=0 && DM_WB.rt==ID_EX.rt && EX_DM.rt!=ID_EX.rt){ ID_EX.stall = 1;}
                            else if(DM_WB_I_Return_rt && DM_WB.rt!=0 && (DM_WB.rt==ID_EX.rs||DM_WB.rt==ID_EX.rt) && (EX_DM.opcode==SB||EX_DM.opcode==SH||EX_DM.opcode==SW)){   ID_EX.stall = 1;}
                            else ID_EX.stall = 0;
                    // 還有很多情況沒考慮 加油

                    }else if(DM_WB_I_Return_rt && (ID_EX.rs==DM_WB.rt || ID_EX.rt==DM_WB.rt) && DM_WB.rt!=0 ){
                          ID_EX.stall = 1;
                    }else if (DM_WB.opcode==R_TYPE && (ID_EX.rs==DM_WB.rd|| ID_EX.rt==DM_WB.rd) && DM_WB.funct!= 0x08 && DM_WB.instruc!=0){
                              ID_EX.stall = 1;
                    }else ID_EX.stall = 0;

                }

                else if( EX_DM_I_Return_rt_no_LW && ID_EX.rs == EX_DM.rt && EX_DM.rt!=0) {ID_EX.stall = 0;}


                else if( EX_DM_I_Return_rt_no_LW && ID_EX.rs == EX_DM.rt && EX_DM.rt!=0) {

                        if( DM_WB_I_Return_rt && ( ID_EX.rs==DM_WB.rt ) && DM_WB.rt!=0 ){
                              ID_EX.stall = 1;
                        }else if (DM_WB.opcode==R_TYPE && (ID_EX.rs==DM_WB.rd) && DM_WB.funct!= 0x08 && DM_WB.instruc!=0){
                              ID_EX.stall = 1;
                        }else ID_EX.stall = 0;
                }

                else if( EX_DM_I_condition_no_SW && ID_EX.rs==EX_DM.rt && EX_DM.rt!=0){ID_EX.stall = 1; }// CANNOT forward in EX stage

                else if( EX_DM_I_condition_no_SW && (ID_EX.rt&0x000000FF)==EX_DM.rt && ID_EX.opcode==SB && EX_DM.rt!=0){ID_EX.stall = 1; }// CANNOT forward in EX stage

                else if( EX_DM_I_condition_no_SW && (ID_EX.rt&0x0000FFFF)==EX_DM.rt && ID_EX.opcode==SH && EX_DM.rt!=0){ID_EX.stall = 1; }// CANNOT forward in EX stage

                else if( EX_DM_I_condition_no_SW && (ID_EX.rt&0xFFFFFFFF)==EX_DM.rt && ID_EX.opcode==SW && EX_DM.rt!=0){ID_EX.stall = 1; }// CANNOT forward in EX stage

                else if( DM_WB_I_Return_rt && ID_EX.rs==DM_WB.rt && DM_WB.rt!=0){ID_EX.stall = 1; }// CANNOT forward in EX stage

                else if( DM_WB_I_Return_rt_no_LW && ID_EX.rs==DM_WB.rt && DM_WB.rt!=0){ID_EX.stall = 1; } // Must be stalled in DM stage

                else if( DM_WB_I_condition_no_SW && (ID_EX.rt&0x000000FF)==DM_WB.rt && ID_EX.opcode==SB && DM_WB.rt!=0){ID_EX.stall = 1; }// CANNOT forward in EX stage

                else if( DM_WB_I_Return_rt_no_LW && (ID_EX.rt==DM_WB.rt||ID_EX.rs==DM_WB.rt) && ID_EX.opcode==SH && DM_WB.rt!=0){ID_EX.stall = 1; }// CANNOT forward in EX stage

                else if( DM_WB_I_condition_no_SW && (ID_EX.rt&0xFFFFFFFF)==DM_WB.rt && ID_EX.opcode==SW && DM_WB.rt!=0){ID_EX.stall = 1; }

                else if( DM_WB.opcode==R_TYPE && ID_EX.rs==DM_WB.rd && DM_WB.funct!= 0x08 && DM_WB.rd!=0) {

                        if(EX_DM_I_Return_rt_no_LW&&EX_DM.rt!=0&&EX_DM.rt==ID_EX.rs) ID_EX.stall = 0;
                        else ID_EX.stall = 1;
                }

                else if( DM_WB.opcode == JAL && (ID_EX.rs==31) ) {ID_EX.stall=1;}

                else {ID_EX.stall = 0;}


                if(predict) predict =0;

                else if(ID_EX.funct==JR && ID_EX.opcode==R_TYPE){

                    if( DM_WB.opcode==R_TYPE && DM_WB.funct!=0x08 && DM_WB.rd==ID_EX.rs){
                        ID_EX.forwarding = 1; ID_EX.stall = 0;  ID_EX.$rs  = DM_WB.alu_out; Rs_Rt = "rs";
                    }
                    else if(DM_WB_I_Return_rt_no_LW && DM_WB.instruc!=0 && DM_WB.rt == ID_EX.rs ){
                        ID_EX.forwarding = 1; ID_EX.stall = 0;  ID_EX.$rs  = DM_WB.alu_out; Rs_Rt = "rs";
                    }
                    else if(DM_WB.opcode==JAL && DM_WB.instruc!=0 && 31 == ID_EX.rs ){
                        ID_EX.forwarding = 1; ID_EX.stall = 0;  ID_EX.$rs  = DM_WB.pc + PC + 4 ; Rs_Rt = "rs";
                    }
                }
                 else if(ID_EX.opcode==BGTZ){

                    if( DM_WB.opcode==R_TYPE && DM_WB.funct!=0x08 && DM_WB.rd==ID_EX.rs){
                        ID_EX.forwarding = 1; ID_EX.stall = 0;  ID_EX.$rs  = DM_WB.alu_out; Rs_Rt = "rs";
                    }
                    else if(DM_WB_I_Return_rt_no_LW && DM_WB.instruc!=0 && DM_WB.rt == ID_EX.rs ){
                        ID_EX.forwarding = 1; ID_EX.stall = 0;  ID_EX.$rs  = DM_WB.alu_out; Rs_Rt = "rs";
                    }
                    else if(DM_WB.opcode==JAL && DM_WB.instruc!=0 && 31 == ID_EX.rs ){
                        ID_EX.forwarding = 1; ID_EX.stall = 0;  ID_EX.$rs  = DM_WB.pc + PC + 4; Rs_Rt = "rs";
                    }
                }
        }

        /** BRANCH stall rs rt */
        else if(ID_EX.opcode==BEQ || ID_EX.opcode==BNE){

            int predict = 0;

            if( EX_DM_I_condition && (ID_EX.rs==EX_DM.rt|| ID_EX.rt==EX_DM.rt) && EX_DM.rt!=0) {ID_EX.stall = 1; predict = 1;}// LW CANNOT forward in EX stage, so we stall

            else if( EX_DM_I_Return_rt_no_LW  && (ID_EX.rs==EX_DM.rt||ID_EX.rt==EX_DM.rt) && EX_DM.rt!=0){ID_EX.stall = 1; predict=1;}

            else if( EX_DM.opcode==R_TYPE && (ID_EX.rs==EX_DM.rd || ID_EX.rt==EX_DM.rd) && EX_DM.funct!= 0x08 && EX_DM.rd!=0){predict = 1; ID_EX.stall = 1;}

            else if( DM_WB_I_Return_rt_no_LW && (ID_EX.rs==DM_WB.rt || ID_EX.rt==DM_WB.rt) && DM_WB.rt!=0) ID_EX.stall = 0; // Must be stalled in DM stage

            else if( DM_WB.opcode==R_TYPE && (ID_EX.rs==DM_WB.rd || ID_EX.rt==DM_WB.rd) && DM_WB.funct!= 0x08 && DM_WB.rd!=0) ID_EX.stall = 0;

            else if( DM_WB_I_condition_no_SW && (ID_EX.rs==DM_WB.rt || ID_EX.rt==DM_WB.rt) && DM_WB.funct!= 0x08 && DM_WB.rt!=0) ID_EX.stall = 1;

            else ID_EX.stall = 0;

            /** BRANCH FOrWARDING */

            if(predict==1){ predict =0;}

            else if( (DM_WB.opcode==JAL )&&( ID_EX.rs==31 || ID_EX.rt==31 ) ){

                ID_EX.stall = 0; ID_EX.forwarding = 1;

                Rs_Rt = "rs"; ID_EX.$rs  = DM_WB.pc + PC + 4; ;

            }

            else if( (DM_WB.opcode==R_TYPE )&&( ID_EX.rs==DM_WB.rd && ID_EX.rt==DM_WB.rd ) && DM_WB.funct!= 0x08 && DM_WB.rd!=0 ){

                ID_EX.stall = 0; ID_EX.forwarding = 1; ID_EX.$rt  = DM_WB.alu_out; ID_EX.$rs  = DM_WB.alu_out;
            }

            else if( DM_WB_I_Return_rt_no_LW && (ID_EX.rs==DM_WB.rt && ID_EX.rt==DM_WB.rt) && DM_WB.rt!=0){

                ID_EX.stall = 0; ID_EX.forwarding = 1; ID_EX.$rt  = DM_WB.alu_out; ID_EX.$rs  = DM_WB.alu_out;

            }

            else if( (DM_WB.opcode==R_TYPE )&&( ID_EX.rs==DM_WB.rd || ID_EX.rt==DM_WB.rd ) && DM_WB.funct!= 0x08 && DM_WB.rd!=0 ){

                ID_EX.stall = 0; ID_EX.forwarding = 1;

                Rs_Rt = (DM_WB.rd==ID_EX.rt)?"rt":"rs";

                if(Rs_Rt=="rt") ID_EX.$rt  = DM_WB.alu_out;
                if(Rs_Rt=="rs") ID_EX.$rs  = DM_WB.alu_out;
            }

            else if( DM_WB_I_Return_rt_no_LW && (ID_EX.rs==DM_WB.rt || ID_EX.rt==DM_WB.rt) && DM_WB.rt!=0){

                ID_EX.stall = 0; ID_EX.forwarding = 1;

                Rs_Rt = (DM_WB.rt==ID_EX.rt)?"rt":"rs";

                if(Rs_Rt=="rt") ID_EX.$rt  = DM_WB.alu_out;
                if(Rs_Rt=="rs") ID_EX.$rs  = DM_WB.alu_out;

            }

            else  ID_EX.forwarding = 0;
        }

        /** R_TYPE STALL rs rt */
        else if( ID_EX.opcode==R_TYPE){

            /** predict forwarding */
           if( DM_WB.opcode == 0x03 && (ID_EX.rs == 31 || ID_EX.rt==31 ) ) ID_EX.stall=1;

           else if( EX_DM_I_Return_rt_no_LW && ( ID_EX.rt ==EX_DM.rt && ID_EX.rt==ID_EX.rs) &EX_DM.rt!=0){

                ID_EX.stall = 0;
            }

           else if( EX_DM_I_Return_rt_no_LW && ( ID_EX.rt ==EX_DM.rt) &EX_DM.rt!=0){

                if( DM_WB_I_Return_rt && ( ID_EX.rs==DM_WB.rt ) && DM_WB.rt!=0 ){
                      ID_EX.stall = 1;
                }else if (DM_WB.opcode==R_TYPE && (ID_EX.rs==DM_WB.rd) && DM_WB.funct!= 0x08 && DM_WB.rd!=0){
                      ID_EX.stall = 1;
                }else ID_EX.stall = 0;
            }

            else if( EX_DM_I_Return_rt_no_LW && ( ID_EX.rs ==EX_DM.rt) &EX_DM.rt!=0){

                if( DM_WB_I_Return_rt && ( ID_EX.rt==DM_WB.rt ) && DM_WB.rt!=0 ){
                      ID_EX.stall = 1;
                }else if (DM_WB.opcode==R_TYPE && (ID_EX.rt==DM_WB.rd) && DM_WB.funct!= 0x08 && DM_WB.rd!=0){
                      ID_EX.stall = 1;
                }else ID_EX.stall = 0;
            }

            else if( EX_DM.opcode==R_TYPE && ( ID_EX.rt ==EX_DM.rd && ID_EX.rt==ID_EX.rs) &EX_DM.rd!=0){

                ID_EX.stall = 0;
            }

            else if( EX_DM.opcode==R_TYPE && ( ID_EX.rs ==EX_DM.rd) &EX_DM.rd!=0){

                if( DM_WB_I_Return_rt && ( ID_EX.rt==DM_WB.rt ) && DM_WB.rt!=0 ){
                      ID_EX.stall = 1;
                }else if (DM_WB.opcode==R_TYPE && (ID_EX.rt==DM_WB.rd) && DM_WB.funct!= 0x08 && DM_WB.rd!=0){
                      ID_EX.stall = 1;
                }else ID_EX.stall = 0;
            }

            else if( EX_DM.opcode==R_TYPE && ( ID_EX.rt ==EX_DM.rd) &EX_DM.rd!=0){

                if( DM_WB_I_Return_rt && ( ID_EX.rs==DM_WB.rt ) && DM_WB.rt!=0 ){
                      ID_EX.stall = 1;
                }else if (DM_WB.opcode==R_TYPE && (ID_EX.rs==DM_WB.rd) && DM_WB.funct!= 0x08 && DM_WB.rd!=0){
                      ID_EX.stall = 1;
                }else ID_EX.stall = 0;
            }


            /** end predicting*/

            else if(EX_DM.opcode==R_TYPE && EX_DM.funct!=0x08 && ID_EX.rt ==EX_DM.rd  && ID_EX.rs == ID_EX.rt && EX_DM.rd!=0){
                ID_EX.stall = 0;
            }

            else if( EX_DM_I_Return_rt_no_LW && ( ID_EX.rt == EX_DM.rt && ID_EX.rs ==ID_EX.rt ) &EX_DM.rt!=0){ ID_EX.stall = 1;}

            else if( EX_DM_I_condition && (ID_EX.rs==EX_DM.rt|| ID_EX.rt==EX_DM.rt) && EX_DM.rt!=0 ){  ID_EX.stall = 1;}  // LW CANNOT forward in EX stage, so we stall

            else if( DM_WB_I_Return_rt && (ID_EX.rs==DM_WB.rt || ID_EX.rt==DM_WB.rt) && DM_WB.rt!=0) ID_EX.stall = 1; // Must be stalled in DM stage

            else if( DM_WB.opcode==R_TYPE && (ID_EX.rs==DM_WB.rd || ID_EX.rt==DM_WB.rd) && DM_WB.rd!=0 &&DM_WB.funct!=0x08){ ID_EX.stall = 1;}

            else {ID_EX.stall = 0;}

        }

        else {
                ID_EX.stall=0;  ID_EX.forwarding = 0;
        }

        // If Stall, then we won't implement anything.

        /**Implementing Branch*/

        if(!ID_EX.stall){

            if(ID_EX.opcode==BGTZ){

                int temp = ID_EX.$rs;
                if (temp > 0) {
                ID_EX.immediate = ID_EX.immediate << 2;

                if (ID_EX.immediate >> 15) ID_EX.address = -(~(ID_EX.immediate - 1));
                else ID_EX.address = ID_EX.immediate;
                ID_EX.flush = 1;
                } else ID_EX.flush = 0;

            }else if(ID_EX.opcode==BEQ){

                if (ID_EX.$rs == ID_EX.$rt) {
                    ID_EX.immediate = ID_EX.immediate << 2;
                    if (ID_EX.immediate >> 15) ID_EX.address = -(~(ID_EX.immediate - 1));
                    else ID_EX.address = ID_EX.immediate;
                  //  printf("FLUSH %d\n",cycle);
                    ID_EX.flush = 1;
                } else ID_EX.flush = 0;

            }else if(ID_EX.opcode==BNE){

                if (ID_EX.$rs!=ID_EX.$rt) {
                    ID_EX.immediate = ID_EX.immediate << 2;
                    if (ID_EX.immediate >> 15) ID_EX.address = -(~( ID_EX.immediate - 1));
                    else ID_EX.address = ID_EX.immediate;

                      ID_EX.flush = 1;
                } else ID_EX.flush = 0;

            } else if(ID_EX.opcode==J){

                    ID_EX.jump = ID_EX.instruc << 6 >> 6;

                    ID_EX.flush = 1;

            } else if(ID_EX.opcode==JAL){

                    ID_EX.jump = ID_EX.instruc << 6 >> 6;

                    ID_EX.flush = 1;

            } else if(ID_EX.opcode==0 && ID_EX.funct == JR){

                    if(ID_EX.forwarding) ID_EX.jump = ID_EX.$rs;
                    else ID_EX.jump = reg[ID_EX.rs];
                    ID_EX.flush = 1;

            }  else ID_EX.flush = 0;

        } else ID_EX.flush = 0;


            ID_EX.$rt = reg[ID_EX.rt];
            ID_EX.$rs = reg[ID_EX.rs];


}

void EX()
{

    if(ID_EX.stall){
        EX_DM.instruc = 0;
        EX_DM.opcode = 0;
        EX_DM.funct = 0;
        EX_DM.rd = 0;
        EX_DM.rs = 0;
        EX_DM.rt = 0;
        EX_DM.shamt = 0;
        EX_DM.alu_out = 0;
        ID_EX.$rt = 0;
        ID_EX.$rs = 0;
        EX_DM.$rt = 0;
        EX_DM.$rs = 0;
    }
    else{
        EX_DM.instruc = ID_EX.instruc;
        EX_DM.opcode = ID_EX.opcode;
        EX_DM.funct = ID_EX.funct;
        EX_DM.rd = ID_EX.rd;
        EX_DM.rs = ID_EX.rs;
        EX_DM.rt = ID_EX.rt;
        EX_DM.shamt = ID_EX.shamt;
        EX_DM.pc = ID_EX.pc;
        if(ID_EX.forwarding) {EX_DM.$rt = ID_EX.$rt;EX_DM.$rs = ID_EX.$rs;}
        else {EX_DM.$rt = reg[ID_EX.rt];EX_DM.$rs = reg[ID_EX.rs];}
    }


    /** Implementing Forwarding in EX Stage.*/

    /**  rt only */
    if(EX_DM.opcode==R_TYPE && (EX_DM.funct==SLL||EX_DM.funct==SRL||EX_DM.funct==SRA) && EX_DM.instruc!=0 ){

        if(DM_WB.opcode==R_TYPE && (DM_WB.rd==EX_DM.rt) && DM_WB.rd!=0  && DM_WB.instruc!=0 ){

             EX_DM.forwarding = 1;  Rs_Rt2="rt";  ID_EX.$rt  = DM_WB.alu_out;
        }

        else if(DM_WB.opcode >=8 && DM_WB.opcode <= 0x2B && DM_WB.rt==EX_DM.rt && DM_WB.rt !=0  && DM_WB.instruc!=0 ){

                // If is Load type or Save , do nothing.
                if(DM_WB.opcode==LW||DM_WB.opcode==LH||DM_WB.opcode==LHU||DM_WB.opcode==LB||DM_WB.opcode==LBU||
                   DM_WB.opcode==SW||DM_WB.opcode==SH||DM_WB.opcode==SB) {}
                // Others in I_TYPE like ADDI, forward their alu_out.
                else {
                    EX_DM.forwarding = 1; Rs_Rt2="rt"; ID_EX.$rt = DM_WB.alu_out;
                }
        }

        else EX_DM.forwarding = 0;
    }


    /** Most I_TYPE  rs only */
    else if(EX_DM.opcode>=8 && EX_DM.opcode <= 0x2B && EX_DM.opcode!= LUI && EX_DM.instruc!=0 ){

        if(DM_WB.opcode==R_TYPE && (DM_WB.rd==EX_DM.rs) && DM_WB.rd!=0  && DM_WB.instruc!=0 ){

             EX_DM.forwarding = 1; Rs_Rt2="rs"; ID_EX.$rs  = DM_WB.alu_out;
        }

        else if(DM_WB.opcode==R_TYPE && (DM_WB.rd==EX_DM.rt) && DM_WB.rd!=0  && DM_WB.instruc!=0 && (EX_DM.opcode==SW||EX_DM.opcode==SH||EX_DM.opcode==SB) ){

             EX_DM.forwarding = 1; Rs_Rt2="rt"; ID_EX.$rt  = DM_WB.alu_out;
        }

        else if(DM_WB.opcode >= 0x08 && DM_WB.opcode <= 0x2B && (DM_WB.rt==EX_DM.rt )&& DM_WB.rt!=0 &&(EX_DM.opcode==SW||EX_DM.opcode==SH||EX_DM.opcode==SB)){

              if(DM_WB.opcode==LW||DM_WB.opcode==LH||DM_WB.opcode==LHU||DM_WB.opcode==LB||DM_WB.opcode==LBU
                 ||DM_WB.opcode==SW||DM_WB.opcode==SH||DM_WB.opcode==SB) {}
                // Others in I_TYPE like ADDI, forward their alu_out.
              else if(EX_DM.rt == EX_DM.rs){
                    EX_DM.forwarding = 1;  Rs_Rt2="rs";  ID_EX.$rs = DM_WB.alu_out; Rs_Rt2="rt";  ID_EX.$rt = DM_WB.alu_out;
              }

              else {
                    EX_DM.forwarding = 1;  Rs_Rt2="rt";  ID_EX.$rt = DM_WB.alu_out;
              }

        }

        else if(DM_WB.opcode==R_TYPE && (DM_WB.rd== (EX_DM.rt&0x0000FFFF) ) && DM_WB.rd!=0  && DM_WB.instruc!=0 &&EX_DM.opcode==SH){

             EX_DM.forwarding = 1; Rs_Rt2="rt"; ID_EX.$rt  = DM_WB.alu_out;
        }
        else if(DM_WB.opcode==R_TYPE && (DM_WB.rd== (EX_DM.rt&0x000000FF) ) && DM_WB.rd!=0  && DM_WB.instruc!=0 &&EX_DM.opcode==SB){

             EX_DM.forwarding = 1; Rs_Rt2="rt"; ID_EX.$rt  = DM_WB.alu_out;
        }


        else if(DM_WB.opcode >=8 && DM_WB.opcode <= 0x2B && DM_WB.rt==EX_DM.rs && DM_WB.rt !=0  && DM_WB.instruc!=0 ){

                // If is Load type or Save , do nothing.
                if(DM_WB.opcode==LW||DM_WB.opcode==LH||DM_WB.opcode==LHU||DM_WB.opcode==LB||DM_WB.opcode==LBU||DM_WB.opcode==SW||DM_WB.opcode==SH||DM_WB.opcode==SB) {
                    EX_DM.forwarding = 0;
                }
                // Others in I_TYPE like ADDI, forward their alu_out.
                else {
                    EX_DM.forwarding = 1;  Rs_Rt2="rs";  ID_EX.$rs = DM_WB.alu_out;
                    //printf("~~~ %d %s %d\n",cycle,sIns2[EX_DM.opcode], EX_DM.rs);

                }
        }

        else {EX_DM.forwarding = 0;}
    }


    /** R_TYPE rs, rt   (Not including Branch) */
    else if( EX_DM.opcode==R_TYPE && EX_DM.instruc != 0){
            // R_TYPE forwarding in EX state
            if( DM_WB.opcode==R_TYPE && (DM_WB.rd==EX_DM.rs && DM_WB.rd==EX_DM.rt) && DM_WB.rd!=0  ){

                   EX_DM.forwarding = 1;  ID_EX.$rt = DM_WB.alu_out; ID_EX.$rs = DM_WB.alu_out;

            }else if(DM_WB.opcode >=8 && DM_WB.opcode <= 0x2B && (DM_WB.rt==EX_DM.rs && DM_WB.rt==EX_DM.rt) && DM_WB.rt !=0 ){

                // If is Load type or Save , do nothing.
                if(DM_WB.opcode==LW||DM_WB.opcode==LH||DM_WB.opcode==LHU||DM_WB.opcode==LB||DM_WB.opcode==LBU||
                   DM_WB.opcode==SW||DM_WB.opcode==SH||DM_WB.opcode==SB) { EX_DM.forwarding = 0;}
                // Others in I_TYPE like ADDI, forward their alu_out.
                else {

                    EX_DM.forwarding = 1;  ID_EX.$rt = DM_WB.alu_out;  ID_EX.$rs = DM_WB.alu_out;
                }
            }
            else if( DM_WB.opcode==R_TYPE && (DM_WB.rd==EX_DM.rs||DM_WB.rd==EX_DM.rt) && DM_WB.rd!=0 && DM_WB.instruc!=0 ){

                   EX_DM.forwarding = 1;  Rs_Rt2 = (DM_WB.rd==EX_DM.rt)?"rt":"rs";

                   if(Rs_Rt2=="rt") ID_EX.$rt = DM_WB.alu_out;  else ID_EX.$rs = DM_WB.alu_out;

            }else if(DM_WB.opcode >=8 && DM_WB.opcode <= 0x2B && (DM_WB.rt==EX_DM.rs||DM_WB.rt==EX_DM.rt) && DM_WB.rt !=0 && DM_WB.instruc!=0){

                // If is Load type or Save , do nothing.
                if(DM_WB.opcode==LW||DM_WB.opcode==LH||DM_WB.opcode==LHU||DM_WB.opcode==LB||DM_WB.opcode==LBU||
                   DM_WB.opcode==SW||DM_WB.opcode==SH||DM_WB.opcode==SB) {}
                // Others in I_TYPE like ADDI, forward their alu_out.
                else {

                    EX_DM.forwarding = 1;  Rs_Rt2 = (DM_WB.rt==EX_DM.rt)?"rt":"rs";

                    if(Rs_Rt2=="rt") ID_EX.$rt = DM_WB.alu_out;  else ID_EX.$rs = DM_WB.alu_out;
                }
            }

            else EX_DM.forwarding = 0;
    }


    else EX_DM.forwarding = 0;


    /** Start Implementing EX stage*/

    if(EX_DM.opcode==HALT){
        return;
    }else if(EX_DM.opcode==J){

    }else if(EX_DM.opcode==JAL){

    }else if(EX_DM.opcode==R_TYPE){
          /** Implement R_TYPE */
        switch(ID_EX.funct){

        case ADD:
            {
              unsigned SignBitRs,SignBitRt,SignBitRd;

               SignBitRs = ID_EX.$rs >> 31; SignBitRt = ID_EX.$rt >> 31;

               EX_DM.alu_out = ID_EX.$rs + ID_EX.$rt;

                SignBitRd = EX_DM.alu_out >> 31;

                if( (SignBitRs==SignBitRt) && (SignBitRs != SignBitRd ) )
                            Number_Overflow = 1;

            } break;
        case ADDU :
            {
                EX_DM.alu_out = ID_EX.$rs + ID_EX.$rt;

            } break;
        case SUB:
            {
                unsigned SignBitRs,SignBitRt,SignBitRd;


                SignBitRt  = (-ID_EX.$rt) >> 31; SignBitRs = ID_EX.$rs >> 31;

                EX_DM.alu_out = ID_EX.$rs - ID_EX.$rt;

                SignBitRd = EX_DM.alu_out >> 31;

                if( (SignBitRs==SignBitRt) && (SignBitRs!=SignBitRd))
                                Number_Overflow = 1;

            } break;
        case AND:
                EX_DM.alu_out =  ID_EX.$rs & ID_EX.$rt;
                break;
        case OR:
                EX_DM.alu_out =  ID_EX.$rs | ID_EX.$rt;
                break;
        case XOR:
                EX_DM.alu_out =  ID_EX.$rs ^ ID_EX.$rt;
                break;
        case NOR:
                EX_DM.alu_out = ~(ID_EX.$rs | ID_EX.$rt);
                break;
        case NAND:
                EX_DM.alu_out = ~(ID_EX.$rs & ID_EX.$rt);
                break;
        case SLT:
           {
               int RS = ID_EX.$rs, RT = ID_EX.$rt;

               EX_DM.alu_out = (RS<RT)?1:0;
           }
                break;
        case SLL:
            {
                EX_DM.alu_out = (ID_EX.$rt << ID_EX.shamt);

            }    break;
        case SRL:
            {
                EX_DM.alu_out = ID_EX.$rt >> ID_EX.shamt;
             }   break;
        case SRA:
             {
                int temp;

                temp = ID_EX.$rt;  temp = temp >> ID_EX.shamt;
                EX_DM.alu_out = temp;

             }   break;

        default:
                break;
        }
    }


    else{ /** Implement I_TYPE */

        switch(ID_EX.opcode){

            case ADDI:
            {
                unsigned SignBitRt, SignBitRs, SignBitImm;

                SignBitRs = ID_EX.$rs >> 31; SignBitImm = ID_EX.immediate >> 31;

                EX_DM.alu_out = ID_EX.$rs + ID_EX.immediate;

                SignBitRt = EX_DM.alu_out >> 31;

                if( (SignBitRs==SignBitImm) && (SignBitRs!=SignBitRt) )
                         Number_Overflow = 1;
            }
                break;
            case ADDIU:
            {
                EX_DM.alu_out = ID_EX.$rs + ID_EX.immediate;
            }
                break;
            case LW:
            {
                unsigned Rs, Im, Pos;

                Rs = ID_EX.$rs >> 31; Im = ID_EX.immediate >> 31;

                EX_DM.alu_out = ID_EX.$rs + ID_EX.immediate;

                Pos =  EX_DM.alu_out >> 31;

                if( (Rs==Im) && (Rs!=Pos) ){ Number_Overflow = 1;}

            }
                break;
            case LH:
            {
                unsigned Rs, Im, Pos;

                Rs = ID_EX.$rs >> 31; Im = ID_EX.immediate >> 31;

                EX_DM.alu_out = ID_EX.$rs + ID_EX.immediate;

                Pos =  EX_DM.alu_out >> 31;

                if( (Rs==Im) && (Rs!=Pos) ){ Number_Overflow = 1;}
            }
                break;
            case LHU:
            {
               unsigned Rs, Im, Pos;

                Rs = ID_EX.$rs >> 31; Im = ID_EX.immediate >> 31;

                EX_DM.alu_out = ID_EX.$rs + ID_EX.immediate;

                Pos =  EX_DM.alu_out >> 31;

                if( (Rs==Im) && (Rs!=Pos) ){ Number_Overflow = 1;}
            }
                break;
            case LB:
            {
                unsigned Rs, Im, Pos;

                Rs = ID_EX.$rs >> 31; Im = ID_EX.immediate >> 31;

                EX_DM.alu_out = ID_EX.$rs + ID_EX.immediate;

                Pos =  EX_DM.alu_out >> 31;

                if( (Rs==Im) && (Rs!=Pos) ){ Number_Overflow = 1;}
            }
                break;
            case LBU:
            {
                unsigned Rs, Im, Pos;

                Rs = ID_EX.$rs >> 31; Im = ID_EX.immediate >> 31;

                EX_DM.alu_out = ID_EX.$rs + ID_EX.immediate;

                Pos =  EX_DM.alu_out >> 31;

                if( (Rs==Im) && (Rs!=Pos) ){ Number_Overflow = 1;}
             }
                break;
            case SW:
            {
                 unsigned Rs, Im, Pos;

                Rs = ID_EX.$rs >> 31; Im = ID_EX.immediate >> 31;

                EX_DM.alu_out = ID_EX.$rs + ID_EX.immediate;

                Pos =  EX_DM.alu_out >> 31;

                if( (Rs==Im) && (Rs!=Pos) ){ Number_Overflow = 1;}
            }
                break;
            case SH:
            {
                unsigned Rs, Im, Pos;

                Rs = ID_EX.$rs >> 31; Im = ID_EX.immediate >> 31;

                EX_DM.alu_out = ID_EX.$rs + ID_EX.immediate;

                Pos =  EX_DM.alu_out >> 31;

                if( (Rs==Im) && (Rs!=Pos) ){ Number_Overflow = 1;}
            }
                break;
            case SB:
            {
                unsigned Rs, Im, Pos;

                Rs = ID_EX.$rs >> 31; Im = ID_EX.immediate >> 31;

                EX_DM.alu_out = ID_EX.$rs + ID_EX.immediate;

                Pos =  EX_DM.alu_out >> 31;

                if( (Rs==Im) && (Rs!=Pos) ){ Number_Overflow = 1;}
            }
                break;
            case LUI:
            {
                EX_DM.alu_out = ID_EX.immediate << 16;
            }
                break;
            case ANDI:
            {
                EX_DM.alu_out = ID_EX.$rs & ID_EX.immediate;
             }
                break;
            case ORI:
            {
                EX_DM.alu_out = ID_EX.$rs | ID_EX.immediate;
            }
                break;
            case NORI:
            {
                EX_DM.alu_out = ~(ID_EX.$rs | ID_EX.immediate);
            }
                break;
            case SLTI:
            {
                EX_DM.alu_out = ( (int)ID_EX.$rs< (int)ID_EX.immediate);
            }
                break;

            default:
                break;
        }

    }

    if(EX_DM.forwarding) {EX_DM.$rt = ID_EX.$rt;EX_DM.$rs = ID_EX.$rs;}
    else {EX_DM.$rt = reg[ID_EX.rt];EX_DM.$rs = reg[ID_EX.rs];}

    return;
}

int DM()
{
    DM_WB.instruc = EX_DM.instruc;
    DM_WB.opcode = EX_DM.opcode;
    DM_WB.funct = EX_DM.funct;
    DM_WB.shamt = EX_DM.shamt;
    DM_WB.rd = EX_DM.rd;
    DM_WB.rt = EX_DM.rt;
    DM_WB.rs = EX_DM.rs;
    DM_WB.pc = EX_DM.pc;

    /*  if(EX_DM.forwarding) {EX_DM.$rt = ID_EX.$rt;EX_DM.$rs = ID_EX.$rs;}
      else {EX_DM.$rt = reg[ID_EX.rt];EX_DM.$rs = reg[ID_EX.rs];}*/


    if(Error_Delay) { return 1;}

    if(DM_WB.opcode==HALT) return 0;
    else if(DM_WB.instruc==0){ return 0;}
    else if(DM_WB.opcode==R_TYPE) DM_WB.alu_out = EX_DM.alu_out;
    else if (DM_WB.opcode==LW){

            int h= 0;
            if(  EX_DM.alu_out >= 1021) { Address_Overflow = 1; h=1;}
            if(  EX_DM.alu_out % 4){ Misalignment_Error = 1; h=1;}
            if(h){ Error_Delay = 1; return 1; } else Error_Delay = 0;

            unsigned  First, Second, Third, Fourth;
            First  = dMemory[EX_DM.alu_out  ]; First  = (First << 24);
            Second = dMemory[EX_DM.alu_out+1]; Second = (Second << 24 >> 8);
            Third  = dMemory[EX_DM.alu_out+2]; Third  = (Third << 24 >> 16);
            Fourth = dMemory[EX_DM.alu_out+3]; Fourth = (Fourth << 24 >> 24);

            DM_WB.MDR = First + Second + Third + Fourth;

    }else if (DM_WB.opcode==LH){

            int h = 0;
            if(EX_DM.alu_out >= 1023) { Address_Overflow=1; h=1; }
            if(EX_DM.alu_out % 2){ Misalignment_Error=1; h=1; }
            if(h){ Error_Delay = 1;return 1;} else Error_Delay = 0;

            unsigned  Second;
            int  First;
            First = dMemory[EX_DM.alu_out]; First = First << 24 >> 16;
            Second = dMemory[EX_DM.alu_out + 1]; Second = Second << 24 >> 24;
            DM_WB.MDR = First + Second;


    }else if (DM_WB.opcode==LHU){

        int h = 0;
        if(EX_DM.alu_out >= 1023) { Address_Overflow=1; h=1; }
        if(EX_DM.alu_out % 2){ Misalignment_Error=1; h=1; }
        if(h){ Error_Delay = 1;return 1;} else Error_Delay = 0;

        unsigned First, Second;
        First  = dMemory[EX_DM.alu_out]; First = First << 24 >> 16;
        Second = dMemory[EX_DM.alu_out + 1]; Second = Second << 24 >> 24;
        DM_WB.MDR = First | Second;


    }else if (DM_WB.opcode==LB){

        int h = 0;
        if(EX_DM.alu_out >= 1024) { Address_Overflow=1; h=1; }
        if(EX_DM.alu_out % 1){ Misalignment_Error=1; h=1; }
        if(h){ Error_Delay = 1;return 1;} else Error_Delay = 0;

        DM_WB.MDR = dMemory[EX_DM.alu_out];

    }else if (DM_WB.opcode==LBU){

        int h = 0;
        if(EX_DM.alu_out >= 1024) { Address_Overflow=1; h=1; }
        if(EX_DM.alu_out % 1){ Misalignment_Error=1; h=1; }
        if(h){ Error_Delay = 1; return 1;} else Error_Delay = 0;

        DM_WB.MDR = dMemory[EX_DM.alu_out];
        DM_WB.MDR = DM_WB.MDR << 24 >> 24;

    }else if (DM_WB.opcode==SW){

        int h = 0;

        if(EX_DM.alu_out >= 1021) { Address_Overflow=1; h=1; }
        if(EX_DM.alu_out % 4){ Misalignment_Error=1; h=1; }
        if(h){ Error_Delay = 1;return 1;} else Error_Delay = 0;

        dMemory[EX_DM.alu_out]   = EX_DM.$rt >> 24;
        dMemory[EX_DM.alu_out+1] = EX_DM.$rt << 8 >> 24;
        dMemory[EX_DM.alu_out+2] = EX_DM.$rt << 16 >> 24;
        dMemory[EX_DM.alu_out+3] = EX_DM.$rt << 24 >> 24;

    }else if (DM_WB.opcode==SH){

        int h = 0;

        if(EX_DM.alu_out >= 1023) { Address_Overflow=1; h=1; }
        if(EX_DM.alu_out % 2){ Misalignment_Error=1; h=1; }
        if(h){ Error_Delay = 1;return 1;} else Error_Delay = 0;

        dMemory[EX_DM.alu_out  ] = EX_DM.$rt << 16 >> 24;
        dMemory[EX_DM.alu_out+1] = EX_DM.$rt << 24 >> 24;


    }else if (DM_WB.opcode==SB){


       int h = 0;
        if(EX_DM.alu_out >= 1024) { Address_Overflow=1; h=1; }
        if(EX_DM.alu_out % 1){ Misalignment_Error=1; h=1; }
        if(h){ Error_Delay = 1;return 1;} else Error_Delay = 0;

        dMemory[EX_DM.alu_out] = EX_DM.$rt<< 24 >> 24;

    }
    else DM_WB.alu_out = EX_DM.alu_out;

    return 0;
}

int WB()
{

    WB_WB.rd = DM_WB.rd;
    WB_WB.rs = DM_WB.rs;
    WB_WB.rt = DM_WB.rt;
    WB_WB.MDR = DM_WB.MDR;
    WB_WB.alu_out = DM_WB.alu_out;
    WB_WB.opcode = DM_WB.opcode;
    WB_WB.instruc = DM_WB.instruc;
    WB_WB.funct = DM_WB.funct;
    WB_WB.shamt = DM_WB.shamt;

    if(DM_WB.opcode==R_TYPE && DM_WB.funct!=0x08){

        if(DM_WB.opcode==0&&DM_WB.rt==0&&DM_WB.rd==0&&DM_WB.shamt==0&&DM_WB.funct==0){/* Its a NOP*/}
        else if(DM_WB.rd==0) {  Write_Zero = 1; return 0;}

    }else if(DM_WB.opcode>=8 && DM_WB.opcode<=0x2B &&DM_WB.opcode!=SW &&DM_WB.opcode!=SH &&DM_WB.opcode!=SB){

        if(DM_WB.rt==0) {  Write_Zero = 1; return 0;}

    }else if(DM_WB.opcode==JAL){
        value = reg[31];
        reg[31] = DM_WB.pc + PC + 4;
        DM_WB.address = 31;
    }

    if(COOL.opcode==HALT) {
       return 1;
    }else if(DM_WB.opcode==0&&DM_WB.rt==0&&DM_WB.rd==0&&DM_WB.shamt==0&&DM_WB.funct==0){
       // do nothing
    }else if(DM_WB.opcode==R_TYPE && DM_WB.rd !=0 && DM_WB.funct!= 0x08 ){
        value = reg[DM_WB.rd];
        reg[DM_WB.rd] = DM_WB.alu_out;
        DM_WB.address = DM_WB.rd;
    }else if(DM_WB.opcode==LW && DM_WB.rt !=0){
        value = reg[DM_WB.rt];
        reg[DM_WB.rt] = DM_WB.MDR;
        DM_WB.address = DM_WB.rt;
    }else if(DM_WB.opcode==LH && DM_WB.rt !=0){
        value = reg[DM_WB.rt];
        reg[DM_WB.rt] = DM_WB.MDR;
        DM_WB.address = DM_WB.rt;
    }else if(DM_WB.opcode==LHU && DM_WB.rt !=0){
        value = reg[DM_WB.rt];
        reg[DM_WB.rt] = DM_WB.MDR;
        DM_WB.address = DM_WB.rt;
    }else if(DM_WB.opcode==LB && DM_WB.rt !=0){
        value = reg[DM_WB.rt];
        reg[DM_WB.rt] = DM_WB.MDR;
        DM_WB.address = DM_WB.rt;
    }else if(DM_WB.opcode==LBU && DM_WB.rt !=0){
        value = reg[DM_WB.rt];
        reg[DM_WB.rt] = DM_WB.MDR;
        DM_WB.address = DM_WB.rt;
    }else if(DM_WB.opcode==ADDI||DM_WB.opcode==ADDIU||DM_WB.opcode==LUI||DM_WB.opcode==ANDI||DM_WB.opcode==ORI||DM_WB.opcode==NORI||DM_WB.opcode==SLTI) {
        value = reg[DM_WB.rt];
        reg[DM_WB.rt] = DM_WB.alu_out;
        DM_WB.address = DM_WB.rt;
    }
    else {

    }

    COOL.opcode = DM_WB.opcode;

    return 0;
}
