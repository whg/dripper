#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <argp.h>

#include <prussdrv.h>
#include <pruss_intc_mapping.h>

#include "vol/vol.h"

#define PRU_NUM 1

#define die(fmt, ...) do { printf(fmt"\n", ##__VA_ARGS__); exit(EXIT_FAILURE); } while(0)
#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

///////////////////////////////////////////////////////////////////
// data structures

typedef struct {
    uint32_t ddr_address;
    uint32_t slice_len, num_slices;
    struct {
	uint32_t bits, on_time, off_time;
	uint32_t camera, flash;
    } ticks_between;
    uint32_t other;
} __attribute__((__packed__)) pru_ram_bits_t;

typedef struct {
    volatile pru_ram_bits_t *ram; // the ram on the PRU
    volatile uint8_t *ddr; // shared with the host
} pru_data_t;

typedef struct {
    char *vol_file;
    float on_time, off_time, bit_time, camera, flash;
    uint32_t other;
    int quiet;
} args_n_opts_t;

///////////////////////////////////////////////////////////////////
// variables

static pru_data_t *g_pru_data = NULL;
static vol_t *g_vol = NULL;

struct argp_option argp_options[] = {
    { "on-time", 'n', "ms", 0, "Slice on time (ms)" },
    { "off-time", 'o', "ms", 0, "Slice off time (ms)" },
    { "bit-time", 'b', "ms", 0, "Time between bits (ms)" },
    { "camera-time", 'c', "ms", 0, "Delay before camera (ms)" },
    { "flash-time", 'f', "ms", 0, "Delay before flash (ms)" },
    { "use-trigger", 't', 0, 0, "Use trigger to start" },
    { "quiet", 'q', 0, 0, "Stop being so chatty" },
    { 0 }
};

static char args_doc[] = "VOL_FILE";
const char *program_version ="0.1";

///////////////////////////////////////////////////////////////////
// functions

uint32_t ms_to_ticks(float milliseconds) {
    // PRU runs at 200MHz, so 5ns per instruction
    // and all delay loops in are 2 instrutions long
    // == return microseconds * 200 / 2
    return (uint32_t) (milliseconds * 100000);
}

#define CASE_ARG_FLOAT(param, chr) case chr: ano->param = atof(arg); break;

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    args_n_opts_t *ano = state->input;

    switch (key) {
	CASE_ARG_FLOAT(on_time, 'n');
	CASE_ARG_FLOAT(off_time, 'o');
	CASE_ARG_FLOAT(bit_time, 'b');
	CASE_ARG_FLOAT(camera, 'c');
	CASE_ARG_FLOAT(flash, 'f');

    case 't':
	ano->other |= 1;
	break;

    case 'q':
	ano->quiet = 1;
	break;

    case ARGP_KEY_ARG:
	if (state->arg_num >= 1) {
	    /* Too many arguments. */
	    argp_usage(state);
	}
	ano->vol_file = arg;
	break;

    case ARGP_KEY_END:
	if (state->arg_num < 1) {
	    /* Not enough arguments. */
	    argp_usage(state);
	}
	break;

    default:
	return ARGP_ERR_UNKNOWN;
    }  
    return 0;
} 


static void init(void) {
    tpruss_intc_initdata pruss_initdata = PRUSS_INTC_INITDATA;

    prussdrv_init();
    if (prussdrv_open(PRU_EVTOUT_0)) {
	die("prussdrv_open failed");
    }
    prussdrv_pruintc_init(&pruss_initdata);	

    g_pru_data = calloc(1, sizeof(*g_pru_data));
    g_vol = malloc(sizeof(*g_vol));
}

static void cleanup(void) {

    if (g_pru_data != NULL) {
	free(g_pru_data);
    }
    if (g_vol != NULL) {
	vol_free_data(g_vol);
    }

    prussdrv_pru_disable(PRU_NUM);
    prussdrv_exit();
}

static uint32_t setup(const args_n_opts_t *ano) {	

    // map the PRU RAM
    prussdrv_map_prumem(PRU_NUM == 0 ? PRUSS0_PRU0_DATARAM : PRUSS0_PRU1_DATARAM, 
			(void**) &g_pru_data->ram);

    // map the DDR
    prussdrv_map_extmem((void**) &g_pru_data->ddr);
	
    // set the DDR address so the PRU can look for the data in the right place
    g_pru_data->ram->ddr_address = prussdrv_get_phys_addr((void*) g_pru_data->ddr);
    
    g_pru_data->ram->ticks_between.bits = max(1, ms_to_ticks(ano->bit_time));
    g_pru_data->ram->ticks_between.on_time = max(1, ms_to_ticks(ano->on_time));
    g_pru_data->ram->ticks_between.off_time = ms_to_ticks(ano->off_time);
    g_pru_data->ram->ticks_between.camera = max(1, ms_to_ticks(ano->camera));
    g_pru_data->ram->ticks_between.flash = max(1, ms_to_ticks(ano->flash));
    g_pru_data->ram->other = ano->other;
    
    uint32_t res = vol_read(ano->vol_file, g_vol);
    if (res != VOL_READ_SUCCESS) {
    	printf("can't read vol file %d\n", res);
	return res;
    }

    uint32_t slice_len = vol_get_slice_len(g_vol);
    uint32_t num_slices = vol_get_num_slices(g_vol);
    g_pru_data->ram->slice_len = slice_len;
    g_pru_data->ram->num_slices = num_slices;
    memcpy((void*)g_pru_data->ddr, g_vol->data, num_slices * slice_len);

    return 0;
}

void print_settings(const args_n_opts_t *ano) {
    printf("Solenoids on for %.3fms (%u ticks)\n", ano->on_time, g_pru_data->ram->ticks_between.on_time);
    if (ano->off_time == 0) {
	printf("There is no off between layers\n");
    } else {
	printf("Solenoids off for %.3fms (%u ticks)\n", ano->off_time, g_pru_data->ram->ticks_between.off_time);
    }
    printf("%.4fms (%u ticks) between sending bits\n", ano->bit_time, g_pru_data->ram->ticks_between.bits);
    printf("%.3fms wait before camera trigger (%u ticks)\n", ano->camera, g_pru_data->ram->ticks_between.camera);
    printf("%.3fms wait before flash trigger (%u ticks)\n", ano->flash, g_pru_data->ram->ticks_between.flash);
    printf("vol file has %u slices with %u on each slice\n", g_pru_data->ram->num_slices, g_pru_data->ram->slice_len * 8);
}

int main(int argc, char *argv[]) {

    args_n_opts_t ano;

    ano.bit_time = 0.0001;
    ano.on_time = 1;
    ano.off_time = 2;
    ano.flash = 1;
    ano.camera = 1;
    ano.other = 0;
    ano.quiet = 0;

    struct argp argp = { argp_options, parse_opt, args_doc, 0 };
    argp_parse(&argp, argc, argv, 0, 0, &ano); 

    init();
    uint32_t setup_failed = setup(&ano);

    if (!setup_failed) {	
	if (!ano.quiet) {
	    print_settings(&ano);
	}
	/* printf("%u, %u, %u, %u, %s, %u, %u\n", g_pru_data->ram->ticks_between.bits, */
	/*        g_pru_data->ram->ticks_between.on_time, */
	/*        g_pru_data->ram->ticks_between.off_time, */
	/*        g_pru_data->ram->ticks_between.camera, */
	/*        ano.vol_file,  */
	/*        g_pru_data->ram->slice_len, */
	/*        g_pru_data->ram->num_slices); */

	prussdrv_exec_program(PRU_NUM, "./transfer.bin");

	prussdrv_pru_wait_event(PRU_EVTOUT_0);
	prussdrv_pru_clear_event(PRU_EVTOUT_0,
				 PRU_NUM == 0 ? PRU0_ARM_INTERRUPT : PRU1_ARM_INTERRUPT);
    }

    cleanup();

    return EXIT_SUCCESS;
}
