
.origin 0
.entrypoint MEMACCESSPRUDATARAM

#include "PRU_memAccessPRUDataRam.hp"

#define PRU0_R31_VEC_VALID 32
#define PRU_EVTOUT_0	3

MEMACCESSPRUDATARAM:
    // Enable OCP master port
    LBCO      r0, C4, 4, 4
    CLR     r0, r0, 4         // Clear SYSCFG[STANDBY_INIT] to enable OCP master port
    SBCO      r0, C4, 4, 4

    // Configure the programmable pointer register for PRU0 by setting c31_pointer[15:0]
    // field to 0x0010.  This will make C31 point to 0x80001000 (DDR memory).
    //MOV     r0, 0x00100000
  //  MOV       r1, CTPPR_1
//    ST32      r0, r1

       // Configure the block index register for PRU0 by setting c24_blk_index[7:0] and
    // c25_blk_index[7:0] field to 0x00 and 0x00, respectively.  This will make C24 point
    // to 0x00000000 (PRU0 DRAM) and C25 point to 0x00002000 (PRU1 DRAM).
    //MOV       r0, 0x00000000
    //MOV       r1, CTBIR_0
    //ST32      r0, r1


    //Load values from external DDR Memory into Registers R0/R1/R2
//    LBCO      r0, CONST_DDR, 0, 12 
//  LBCO r0, CONST_DDR, 0, 4
   MOV r9, 0x9e700000
   LBBO r0, r9, 4, 4
//   MOV r0, 1235
  SBCO r0, CONST_PRUDRAM, 12, 4

//Store values from read from the DDR memory into PRU shared RAM
  //  SBCO      r0, CONST_PRUSHAREDRAM, 0, 12


    //Load 32 bit value in r1
//    MOV       r1, 0x0010f012
  LBCO r1, CONST_PRUDRAM, 4, 4
       
    //Load address of PRU data memory in r2
    MOV       r2, 0x0004

    // Move value from register to the PRU local data memory using registers
//    ST32      r1,r2
//       SBBO r1, r2, 0x0, 4
   SBCO r1, CONST_PRUDRAM, 4, 4

    // Load 32 bit value into r3
    MOV       r3, 0x0000567A

    LBCO      r4, CONST_PRUDRAM, 4, 4 //Load 4 bytes from memory location c3(PRU0/1 Local Data)+4 into r4 using constant table

    // Add r3 and r4
    ADD       r3, r3, r4

    //Store result in into memory location c3(PRU0/1 Local Data)+8 using constant table
    SBCO      r3, CONST_PRUDRAM, 8, 4
   

    MOV r1, 0x27
    SBCO r1, CONST_PRUDRAM, 0, 4

    MOV R31.b0, PRU0_R31_VEC_VALID | PRU_EVTOUT_0
    HALT
