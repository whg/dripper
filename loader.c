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

// if you change this, don't forget to change
// arg for prussdrv_map_prumem()
#define PRU_NUM 1

#define die(fmt, ...) do { printf(fmt"\n", ##__VA_ARGS__); exit(EXIT_FAILURE); } while(0)

typedef struct {
    uint32_t ddr_address;
    volatile uint32_t ddr_len;
    struct {
	uint32_t bits, on_time, off_time;
    } ticks_between;
} __attribute__((__packed__)) pru_ram_bits_t;

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
    pru_data->ram->ddr_len = 55;
    //	((uint32_t*) pru_data->ddr)[0] = 66;

    prussdrv_exec_program(PRU_NUM, "./loader_test.bin");

    prussdrv_pru_wait_event(PRU_EVTOUT_0);
    prussdrv_pru_clear_event(PRU_EVTOUT_0, PRU0_ARM_INTERRUPT);

    // on_time -> off_time
    // ddr[0] -> bits

    printf("%u = %u\n", pru_data->ram->ticks_between.on_time, pru_data->ram->ticks_between.off_time);
    printf("%u = %u\n", pru_data->ddr[0], pru_data->ram->ticks_between.bits);
    printf("%u = %u\n", pru_data->ram->ddr_len, pru_data->ram->ddr_len);
    
}



int main(void) {

    init();

    pru_data_t *pru_data = setup();
    pru_data->ram->ddr_len = 2;
    pru_data->ddr[0] = 0b00011100;
    pru_data->ddr[1] = 255; //0b11011101;

    prussdrv_exec_program(PRU_NUM, "./transfer.bin");

    prussdrv_pru_wait_event(PRU_EVTOUT_0);
    prussdrv_pru_clear_event(PRU_EVTOUT_0, PRU1_ARM_INTERRUPT);
    //    printf("%u\n", pru_data->ram->ddr_len);

    /* test(); */


    cleanup();
	
    return EXIT_SUCCESS;
}
	
