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

#define r_counter r26
	
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

	;; r10 is the data index counter
	;; r11 is the end test	
	MOV	r10, DDR_ADDR_REG
	ADD	r11, r10, DDR_LEN_REG
DATA_LOOP:	
	;; get byte of data
	LBBO	r0, r10, 0, 1

	;; MOV	r0, 215		;
	LDI	r2, 1

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

	MOV	r_counter, DELAY_TIME
DELAY:
	SUB	r_counter, r_counter, 1
	QBNE	DELAY, r_counter, 0

	CLR	r30, r30, 15
	MOV	r_counter, DELAY_TIME
DELAY2:
	SUB	r_counter, r_counter, 1
	QBNE	DELAY2, r_counter, 0
	
	QBGT	BYTE_LOOP, r2, 255

	;; increment data index
	ADD	r10, r10, 1
	QBNE	DATA_LOOP, r10, r11
 
EXIT:
	MOV R31.b0, PRU0_R31_VEC_VALID | PRU_EVTOUT_0
	HALT
