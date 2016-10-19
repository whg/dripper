.origin 0
.entrypoint START

#define PRU0_R31_VEC_VALID 32
#define PRU_EVTOUT_0 3

#define CONST_PRU_RAM C24

#define PRU_RAM_BITS_START_REG r20
#define DDR_ADDR_REG 	r20
#define DDR_LEN_REG 	r21
#define TB_BITS_REG 	r22
#define TB_ON_REG 	r23
#define TB_OFF_REG 	r24
#define PRU_RAM_BITS_NUM_BYTES 20
	
#define CTBIR_0 0x22020

#define INS_PER_US 200
#define DELAY_TIME INS_PER_US * 100000

	
START:
	// enable OCP master port
	LBCO	r0, C4, 4, 4
	CLR	r0, r0, 4
	SBCO	r0, C4, 4, 4

	// configure the RAM for the PRUs
	// don't understand the details of this, but we need it
	LDI	r0, 0
	MOV	r1, CTBIR_0
	SBBO	r0, r1, 0, 4
	
	// get the start address of DDR block
	// load all the bits we need, it's a contiguous block of 4 uints
	LBCO	PRU_RAM_BITS_START_REG, CONST_PRU_RAM, 0, PRU_RAM_BITS_NUM_BYTES


	
	;; get byte of data
	LBBO	r0, DDR_ADDR_REG, 0, 1
	;; MOV	r0, 215		;
	LDI	r2, 1

DATA_LOOP:
	;; r3 is the bit result from each loop
	;; r2 is bit that's shifted each loop
	;; r0 is where each byte of data lives
BYTE_LOOP:
	AND	r3, r0, r2
	QBEQ	SEND_OFF, r3, 0
SEND_ON:
	SET	r30, r30, 15
	QBA	BYTE_LOOP_STEP
SEND_OFF:
	CLR	r30, r30, 15
	QBA	BYTE_LOOP_STEP
BYTE_LOOP_STEP:
	LSL	r2, r2, 1

	MOV	r10, DELAY_TIME
DELAY:
	SUB	r10, r10, 1
	QBNE	DELAY, r10, 0

	CLR	r30, r30, 15
	MOV	r10, DELAY_TIME
DELAY2:
	SUB	r10, r10, 1
	QBNE	DELAY2, r10, 0
	
	QBGT	BYTE_LOOP, r2, 255

	
;; 	MOV	r1, 10
;; TEST:
;; 	SET	r30, r30, 15
;; 	MOV	r0, DELAY_TIME
;; DELAY:
;; 	SUB	r0, r0, 1
;; 	QBNE	DELAY, r0, 0
;; 	CLR	r30, r30, 15
;; 	MOV	r0, DELAY_TIME
;; DELAY2:
;; 	SUB	r0, r0, 1
;; 	QBNE	DELAY2, r0, 0
	
;; 	SUB	r1, r1, 1
;; 	QBNE	TEST, r1, 0
	// fish out our first test number from DDR
//	LBBO	r0, DDR_ADDR_REG, 0, 1
	// and write it out to a bit of the PRU RAM
//	SBCO	r0, CONST_PRU_RAM, 4, 1

	// fish out the next test
//	LBCO 	r1, CONST_PRU_RAM, 8, 4
	;; MOV	r1, 444
//	SBCO	r1, CONST_PRU_RAM, 12, 4
 
EXIT:
	MOV R31.b0, PRU0_R31_VEC_VALID | PRU_EVTOUT_0
	HALT
