.origin 0
.entrypoint START

#define PRU0_R31_VEC_VALID 32
#define PRU_EVTOUT_0	3

#define CONST_PRU_RAM C24
#define DDR_ADDR_REG r0

#define CTBIR_0 0x22020
	
START:
	// enable OCP master port
	LBCO	r0, C4, 4, 4
	CLR	r0, r0, 4
	SBCO	r0, C4, 4, 4

	// configure the RAM for the PRUs
	// don't understand the details of this, but we need it
	MOV	r0, 0
	MOV	r1, CTBIR_0
	SBBO	r0, r1, 0, 4
	
	// get the start address of DDR block
	LBCO	DDR_ADDR_REG, CONST_PRU_RAM, 0, 4
	
TEST:
	// fish out our first test number from DDR
	LBBO	r0, DDR_ADDR_REG, 0, 1
	// and write it out to a bit of the PRU RAM
	;; MOV	r0, 555
	SBCO	r0, CONST_PRU_RAM, 4, 1

	// fish out the next test
	LBCO 	r1, CONST_PRU_RAM, 8, 4
	;; MOV	r1, 444
	SBCO	r1, CONST_PRU_RAM, 12, 4

EXIT:
	MOV R31.b0, PRU0_R31_VEC_VALID | PRU_EVTOUT_0
	HALT
