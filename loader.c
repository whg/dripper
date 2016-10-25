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

#include "vol/vol.h"

// if you change this, don't forget to change
// arg for prussdrv_map_prumem()
#define PRU_NUM 1

#define die(fmt, ...) do { printf(fmt"\n", ##__VA_ARGS__); exit(EXIT_FAILURE); } while(0)

typedef struct {
    uint32_t ddr_address;
    uint32_t slice_len, num_slices;
    struct {
	uint32_t bits, on_time, off_time;
	uint32_t camera;
    } ticks_between;
} __attribute__((__packed__)) pru_ram_bits_t;

typedef struct {
    volatile pru_ram_bits_t *ram; // the ram on the PRU
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
    prussdrv_map_prumem(PRUSS0_PRU1_DATARAM, (void**) &pru_data->ram);

    // map the DDR
    prussdrv_map_extmem((void**) &pru_data->ddr);
	
    // set the DDR address so the PRU can look for the data in the right place
    pru_data->ram->ddr_address = prussdrv_get_phys_addr((void*) pru_data->ddr);

    return pru_data;
}

static void test() {
	
    pru_data_t *pru_data = setup();

    pru_data->ram->ticks_between.on_time = 2345;
    pru_data->ddr[0] = 77;
    pru_data->ram->slice_len = 55;
    //	((uint32_t*) pru_data->ddr)[0] = 66;

    prussdrv_exec_program(PRU_NUM, "./loader_test.bin");

    prussdrv_pru_wait_event(PRU_EVTOUT_0);
    prussdrv_pru_clear_event(PRU_EVTOUT_0, PRU0_ARM_INTERRUPT);

    // on_time -> off_time
    // ddr[0] -> bits
    printf("%u = %u\n", pru_data->ram->ticks_between.on_time, pru_data->ram->ticks_between.off_time);
    printf("%u = %u\n", pru_data->ddr[0], pru_data->ram->ticks_between.bits);
    printf("%u = %u\n", pru_data->ram->slice_len, pru_data->ram->slice_len);


    printf("%u\n", pru_data->ram->slice_len);
    printf("%u\n", pru_data->ram->num_slices);
    printf("%u\n", pru_data->ram->ticks_between.bits);
    printf("%u\n", pru_data->ram->ticks_between.on_time);
    printf("%u\n", pru_data->ram->ticks_between.off_time);

}

uint32_t us_to_ticks(uint32_t microseconds) {
    // PRU runs at 200MHz, so 5ns per instruction
    // and all delay loops in are 2 instrutions long
    // == return microseconds * 200 / 2
    return microseconds * 100;
}


int main(void) {

    vol_t vol;
    uint32_t res = vol_read( "./testf.vol", &vol );
    if ( res != VOL_READ_SUCCESS ) {
	printf("can't read vol file %d\n", res);
	return EXIT_FAILURE;
    }

    printf("dim = %u, %u, %u\n", vol.dim[0], vol.dim[1], vol.dim[2]);

    init();

    pru_data_t *pru_data = setup();
    /* pru_data->ram->slice_len = vol_get_slice_len(&vol); */
    /* pru_data->ram->num_slices = vol_get_num_slices(&vol); */
    pru_data->ram->slice_len = 1;
    pru_data->ram->num_slices = 4;
    
    pru_data->ddr[0] = 255;//0b10101010;////0b01011100;
    pru_data->ddr[1] = 255;//0b10101010; //255; //0b11011101;
    pru_data->ddr[2] = 255;//0b01011010;
    pru_data->ddr[3] = 255;//0b11011011;
    pru_data->ram->ticks_between.bits = us_to_ticks(1000);
    pru_data->ram->ticks_between.on_time = us_to_ticks(1000000);
    pru_data->ram->ticks_between.off_time = us_to_ticks(1000000);
    pru_data->ram->ticks_between.camera = us_to_ticks(10);


    prussdrv_exec_program(PRU_NUM, "./transfer.bin");

    prussdrv_pru_wait_event(PRU_EVTOUT_0);
    prussdrv_pru_clear_event(PRU_EVTOUT_0, PRU1_ARM_INTERRUPT);
    //    printf("%u\n", pru_data->ram->ddr_len);

    /* test(); */

    vol_free_data( &vol );
    free(pru_data);
    cleanup();

    return EXIT_SUCCESS;
}
	
