#include "define.h"
#include "globalVariables.h"

void setInstruction()
{
    sIns[ADD] = "ADD";
    sIns[ADDU] = "ADDU";
    sIns[SUB] = "SUB";
    sIns[AND] = "AND";
    sIns[OR] = "OR";
    sIns[XOR] = "XOR";
    sIns[NOR] = "NOR";
    sIns[NAND] = "NAND";
    sIns[SLT] = "SLT";
    sIns[SLL] = "SLL";
    sIns[SRL] = "SRL";
    sIns[SRA] = "SRA";
    sIns[JR] = "JR";

    sIns2[ADDI] = "ADDI";
    sIns2[ADDIU] = "ADDIU";
    sIns2[LW] = "LW";
    sIns2[LH] = "LH";
    sIns2[LHU] = "LHU";
    sIns2[LB] = "LB";
    sIns2[LBU] = "LBU";
    sIns2[SW] = "SW";
    sIns2[SH] = "SH";
    sIns2[SB] = "SB";
    sIns2[LUI] = "LUI";
    sIns2[ANDI] = "ANDI";
    sIns2[ORI] = "ORI";
    sIns2[NORI] = "NORI";
    sIns2[SLTI] = "SLTI";
    sIns2[BEQ] = "BEQ";
    sIns2[BNE] = "BNE";
    sIns2[BGTZ] = "BGTZ";
    sIns2[J] = "J";
    sIns2[JAL] = "JAL";
    sIns2[HALT] = "HALT";

    pc = 0;
    Error_Delay = 0;
    Write_Delay = 0;
    Write_Zero = 0;
    Number_Overflow = 0;
    Address_Overflow = 0;
    Misalignment_Error = 0;
    EX_DM.forwarding = 0;
    ID_EX.forwarding = 0;
    DM_WB.address = 100;
    value = 100;

}
