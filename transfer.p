.origin 0
.entrypoint START

#define PRU0_R31_VEC_VALID 32
#define PRU_EVTOUT_0 3

#define CONST_PRU_RAM C24

#define PRU_RAM_BITS_START_REG r20
#define DDR_ADDR_REG 	r20
#define SLICE_LEN_REG 	r21
#define NUM_SLICES_REG 	r22
#define TB_BITS_REG 	r23
#define TB_ON_REG 	r24
#define TB_OFF_REG 	r25
#define PRU_RAM_BITS_NUM_BYTES 24

#define r_counter r18
	
#define CTBIR_0 0x22020

#define INS_PER_US 200
#define DELAY_TIME INS_PER_US * 100000

	
START:
	// enable OCP master port in the SYSCFG register
	LBCO	r0, C4, 4, 4
	CLR	r0, r0, 4
	SBCO	r0, C4, 4, 4

	// configure the RAM for the PRUs
	// don't understand the details of this, but we need it (?)
	LDI	r0, 0
	MOV	r1, CTBIR_0
	SBBO	r0, r1, 0, 4
	
	// get the start address of DDR block
	// load all the bits we need, it's a contiguous block
	LBCO	PRU_RAM_BITS_START_REG, CONST_PRU_RAM, 0, PRU_RAM_BITS_NUM_BYTES

	;; r10 is the data index counter
	MOV	r10, DDR_ADDR_REG
	
SLICE_LOOP:

	;; r11 is the slice end test	
	ADD	r11, r10, SLICE_LEN_REG

	;; send latch low
	CLR	r30, r30, 4

DATA_LOOP:	
	;; get byte of data
	LBBO	r0, r10, 0, 1

	;; MOV	r0, 215		;
	LDI	r2, 1		; start with 1

	;; r3 is the bit result from each loop
	;; r2 is bit that's shifted each loop
	;; r0 is where each byte of data lives
BYTE_LOOP:
	CLR	r30, r30, 7	; send clock low
	AND	r3, r0, r2	; grab a bit from the data
	QBEQ	SEND_OFF, r3, 0	; if it's 0 then send off
SEND_ON:
	SET	r30, r30, 6
	QBA	BYTE_LOOP_STEP
SEND_OFF:
	CLR	r30, r30, 6
BYTE_LOOP_STEP:
	LSL	r2, r2, 1 	; r2 = r2 << 1

	MOV	r_counter, DELAY_TIME ; start a delay so we can see what we're doing
DELAY:
	SUB	r_counter, r_counter, 1
	QBNE	DELAY, r_counter, 0

	SET	r30, r30, 7 	; send clock high
	
	CLR	r30, r30, 6 	; turn off LED
	MOV	r_counter, DELAY_TIME
DELAY2:
	SUB	r_counter, r_counter, 1
	QBNE	DELAY2, r_counter, 0
	
	QBGT	BYTE_LOOP, r2, 255 ; if 255 > r2 do more of the byte

	;; increment data index
	ADD	r10, r10, 1
	QBNE	DATA_LOOP, r10, r11

;; at this point all the data is loaded in we send the latch high
;; to move the data to the output stage
	SET	r30, r30, 4
;;; perhaps enable here?

	MOV	r_counter, TB_ON_REG
ON_DELAY:
	SUB 	r_counter, r_counter, 1
	QBNE	ON_DELAY, r_counter, 0

;;; reset or disable here
	SET	r30, r30, 5

	MOV	r_counter, TB_OFF_REG
OFF_DELAY:
	SUB 	r_counter, r_counter, 1
	QBNE	OFF_DELAY, r_counter, 0

;;; move on to next slice
	SUB	NUM_SLICES_REG, NUM_SLICES_REG, 1
	QBNE	SLICE_LOOP, NUM_SLICES_REG, 0
	
EXIT:
	MOV R31.b0, PRU0_R31_VEC_VALID | PRU_EVTOUT_0
	HALT
