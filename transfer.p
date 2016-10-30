.origin 0
.entrypoint START

;; data on r30, 0 :: pin 45
;; clock on r30, 3 :: pin 44
;; latch on r30, 5 :: pin 42
;; enable on r30, 1 :: pin 46
;; reset on r30, 7 :: pin 40
;; camera on r30, 6 :: pin 39
;; flash on r30, 4 :: pin 41

	
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
#define TB_CAMERA_REG 	r26
#define TB_FLASH_REG	r27
#define USE_TRIGGER_REG	r28
#define PRU_RAM_BITS_NUM_BYTES 36

#define r_counter r18
	
#define CTBIR_0 0x22020

	
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
	SET	r30, r30, 1 	; disable to begin with
	
	QBBC	SLICE_LOOP, USE_TRIGGER_REG.t0

	CLR	r30, r30, 7	; un-reset		
	
WAIT:
	QBBC	WAIT, r31.t9 
	
SLICE_LOOP:
	;; r11 is the slice end test	
	ADD	r11, r10, SLICE_LEN_REG

	;; send latch low
	CLR	r30, r30, 5

DATA_LOOP:	
	;; get byte of data
	LBBO	r0, r10, 0, 1

	;; MOV	r0, 215		;
	LDI	r2, 1		; start with 1

	;; r3 is the bit result from each loop
	;; r2 is bit that's shifted each loop
	;; r0 is where each byte of data lives
BYTE_LOOP:
	CLR	r30, r30, 3	; send clock low
	AND	r3, r0, r2	; grab a bit from the data
	QBEQ	SEND_OFF, r3, 0	; if it's 0 then send off
SEND_ON:
	SET	r30, r30, 0
	QBA	BYTE_LOOP_STEP
SEND_OFF:
	CLR	r30, r30, 0
	QBA	BYTE_LOOP_STEP 	; just to keep the instruction count the the same
BYTE_LOOP_STEP:
	LSL	r2, r2, 1 	; r2 = r2 << 1

	MOV	r_counter, TB_BITS_REG ; start a delay so we can see what we're doing
DELAY:
	SUB	r_counter, r_counter, 1
	QBNE	DELAY, r_counter, 0

	SET	r30, r30, 3 	; send clock high
	
	QBGT	BYTE_LOOP, r2, 255 ; if 255 > r2 do more of the byte

	;; increment data index
	ADD	r10, r10, 1
	QBNE	DATA_LOOP, r10, r11

;; at this point all the data is loaded in we send the latch high
;; to move the data to the output stage
	SET	r30, r30, 5
;;; perhaps enable here?

	CLR	r30, r30, 1 	; enable
	MOV	r_counter, TB_ON_REG
ON_DELAY:
	SUB 	r_counter, r_counter, 1
	QBNE	ON_DELAY, r_counter, 0

	QBEQ	NEXT_SLICE, TB_OFF_REG, 0
	SET	r30, r30, 1	; disable
	MOV	r_counter, TB_OFF_REG
OFF_DELAY:
	SUB 	r_counter, r_counter, 1
	QBNE	OFF_DELAY, r_counter, 0

	
NEXT_SLICE:
;;; move on to next slice
	SUB	NUM_SLICES_REG, NUM_SLICES_REG, 1
	QBNE	SLICE_LOOP, NUM_SLICES_REG, 0


	SET	r30, r30, 7 	; reset to stop everything
	
	MOV	r_counter, TB_CAMERA_REG
CAMERA_DELAY:
	SUB	r_counter, r_counter, 1
	QBNE	CAMERA_DELAY, r_counter, 0

	MOV	r1, 8000000
;;; capture!
	SET	r30, r30, 6
;; ;;; wait for a bit so the trigger registers
;; 	MOV	r_counter, r1
;; CAPTURE_DELAY:
;; 	SUB	r_counter, r_counter, 1
;; 	QBNE	CAPTURE_DELAY, r_counter, 0

;; 	CLR	r30, r30, 2

	
	MOV	r_counter, TB_FLASH_REG
FLASH_DELAY:
	SUB	r_counter, r_counter, 1
	QBNE	FLASH_DELAY, r_counter, 0
	
;;; flash!
	SET	r30, r30, 4
	MOV	r_counter, r1
FLASH_CAPTURE_DELAY:
	SUB	r_counter, r_counter, 1
	QBNE	FLASH_CAPTURE_DELAY, r_counter, 0

	CLR	r30, r30, 6
	CLR	r30, r30, 4

	CLR	r30, r30, 7	; un-reset

	
EXIT:
	MOV R31.b0, PRU0_R31_VEC_VALID | PRU_EVTOUT_0
	HALT
