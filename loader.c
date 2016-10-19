#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include <prussdrv.h>
#include <pruss_intc_mapping.h>

#define PRU_NUM 0
#define die(fmt, ...) do { printf(fmt"\n", ##__VA_ARGS__); exit(EXIT_FAILURE); } while(0)

typedef struct {
	uint32_t ddr_address, ddr_len;
	struct {
		uint32_t bits, on_time, off_time;
	} ticks_between;
	uint8_t *other;
} pru_ram_bits_t;

typedef struct {
	pru_ram_bits_t *ram; // the ram on the PRU
	volatile uint8_t *ddr; // shared with the host
} pru_data_t;

pru_data_t *g_pru_data = NULL;

static void init(void) {
	tpruss_intc_initdata pruss_initdata = PRUSS_INTC_INITDATA;

	prussdrv_init();
	if (prussdrv_open(PRU_EVTOUT_0)) {
		die("prussdrv_open failed");
	}
	prussdrv_pruintc_init(&pruss_initdata);
	
}

static void cleanup(void) {

	if (g_pru_data != NULL) {
		free(g_pru_data);
	}

    prussdrv_pru_disable(PRU_NUM);
    prussdrv_exit();
}

static pru_data_t* setup(void) {
	
	pru_data_t *pru_data = calloc(1, sizeof(*pru_data));

	// map the PRU RAM
	prussdrv_map_prumem(PRUSS0_PRU0_DATARAM, (void**) &pru_data->ram);

	// map the DDR
	prussdrv_map_extmem((void**) &pru_data->ddr);
	
	// set the DDR address so the PRU can look for the data in the right place
	pru_data->ram->ddr_address = prussdrv_get_phys_addr((void*) pru_data->ddr);

	return pru_data;
}

static void test() {
	
	pru_data_t *pru_data = setup();

	pru_data->ram->ticks_between.on_time = 1234;
	pru_data->ddr[0] = 66;
	//	((uint32_t*) pru_data->ddr)[0] = 66;

	pru_data->ram->ticks_between.off_time = 0;
	pru_data->ram->ticks_between.bits = 0;

    prussdrv_exec_program(PRU_NUM, "./loader_test.bin");

    prussdrv_pru_wait_event(PRU_EVTOUT_0);
    prussdrv_pru_clear_event(PRU_EVTOUT_0, PRU0_ARM_INTERRUPT);

	// on_time -> off_time
	// ddr[0] -> bits

	printf("1234 = %u\n", pru_data->ram->ticks_between.off_time);
	printf("66 = %u\n", pru_data->ram->ticks_between.bits);

}



int main(void) {

	init();

	pru_data_t *pru_data = setup();
	pru_data->ram->ddr_len = 1;
	pru_data->ddr[0] = 0b11011101;

    prussdrv_exec_program(PRU_NUM, "./transfer.bin");

    prussdrv_pru_wait_event(PRU_EVTOUT_0);
    prussdrv_pru_clear_event(PRU_EVTOUT_0, PRU0_ARM_INTERRUPT);

	//	test();

	cleanup();
	
	return EXIT_SUCCESS;
}
	
