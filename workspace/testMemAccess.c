/*****************************************************************************
* PRU_memAccessPRUDataRam.c
*
* The PRU reads and stores values into the PRU Data RAM memory. PRU Data RAM
* memory has an address in both the local data memory map and global memory
* map. The example accesses the local Data RAM using both the local address
* through a register pointed base address and the global address pointed by
* entries in the constant table.
*
******************************************************************************/


/*****************************************************************************
* Include Files                                                              *
*****************************************************************************/

#include <stdio.h>

// Driver header file
#include <prussdrv.h>
#include <pruss_intc_mapping.h>

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>

#define die(fmt, ...) do { printf(fmt"\n", ##__VA_ARGS__); exit(EXIT_FAILURE); } while(0)

int mem_fd;
uint32_t *ddr_memory;


#define PRU_NUM 	0
#define ADDEND1		0x0010F012u
#define ADDEND2		0x0000567Au

static int LOCAL_exampleInit ( unsigned short pruNum );
static unsigned short LOCAL_examplePassed ( unsigned short pruNum );

static void *pruDataMem;
static unsigned int *pruDataMem_int;

int main (void)
{
    unsigned int ret;
    tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;

    printf("\nINFO: Starting %s example.\r\n", "PRU_memAccessPRUDataRam");
    prussdrv_init ();

    ret = prussdrv_open(PRU_EVTOUT_0);
    if (ret)
    {
        printf("prussdrv_open open failed\n");
        return (ret);
    }

    prussdrv_pruintc_init(&pruss_intc_initdata);

    printf("\tINFO: Initializing example.\r\n");
    LOCAL_exampleInit(PRU_NUM);

    printf("\tINFO: Executing example.\r\n");
    prussdrv_exec_program (PRU_NUM, "./PRU_memAccessPRUDataRam.bin");

    printf("\tINFO: Waiting for HALT command.\r\n");
    prussdrv_pru_wait_event (PRU_EVTOUT_0);
    printf("\tINFO: PRU completed transfer.\r\n");
    prussdrv_pru_clear_event (PRU_EVTOUT_0, PRU0_ARM_INTERRUPT);

    if ( LOCAL_examplePassed(PRU_NUM) )
    {
        printf("INFO: Example executed succesfully.\r\n");
    }
    else
    {
        printf("INFO: Example failed.\r\n");

    }

    prussdrv_pru_disable (PRU_NUM);
    prussdrv_exit ();

    munmap(ddr_memory, 0x0FFFFFFF);

    return(0);

}


inline uint32_t readValue(const char *filename) {
  FILE *f = fopen(filename, "r");
  if (!f) {
    die("unable to open %s, (%s)", filename, strerror(errno));
  }
  uint32_t v;
  fscanf(f, "%x", &v);
  fclose(f);
  return v;
}

static int LOCAL_exampleInit ( unsigned short pruNum )
{

  // ok, here are the findings
  // going down the memory mapped route, the addr returned from mmap
  // is off by the size, you need to add the size to the address to get to the
  // right place.

  // a better and easier way of doing it is to use prussdrv_map_extmem()fg

  const uint32_t ddr_addr = readValue("/sys/class/uio/uio0/maps/map1/addr");
  const uint32_t ddr_size = readValue("/sys/class/uio/uio0/maps/map1/size");

  mem_fd = open("/dev/mem", O_RDWR);

  if(mem_fd < 0) {
    die("can't open /dev/mem (%s)", strerror(errno));
  }

  ddr_memory = mmap(0, ddr_size, PROT_WRITE | PROT_READ, MAP_SHARED, mem_fd, ddr_addr);

  if (ddr_memory == MAP_FAILED) {
    die("mmap failed (%s)", strerror(errno));
  }

  close(mem_fd);

  uint8_t *usable_ddr = (uint8_t*) ddr_memory;
  usable_ddr = usable_ddr + ddr_size;

  usable_ddr[1] = 123;

    //Initialize pointer to PRU data memory
    if (pruNum == 0)
    {
      prussdrv_map_prumem (PRUSS0_PRU0_DATARAM, &pruDataMem);
    }
    else if (pruNum == 1)
    {
      prussdrv_map_prumem (PRUSS0_PRU1_DATARAM, &pruDataMem);
    }
    pruDataMem_int = (unsigned int*) pruDataMem;

    // Flush the values in the PRU data memory locations
    /* pruDataMem_int[1] = 0x00; */
    /* pruDataMem_int[2] = 0x00; */

    pruDataMem_int[1] = ADDEND1;
    pruDataMem_int[2] = 0x10;


    // Pointer into the DDR RAM mapped by the uio_pruss kernel module.  
    volatile void *shared_ddr = NULL;  
    prussdrv_map_extmem((void**) &shared_ddr);  
    unsigned int shared_ddr_len = prussdrv_extmem_size();  
    unsigned int physical_address = prussdrv_get_phys_addr((void *) shared_ddr);  
   
    printf("%u bytes of shared DDR available.\n Physical (PRU-side) address:%xfg\n",  
	   shared_ddr_len, physical_address);  
    printf("Virtual (linux-side) address: %p\n\n", shared_ddr);  
   
    *((uint32_t*)shared_ddr) = 1020304;
    //    *(((uint32_t*)shared_ddr)+1) = 666;



    printf("sddr[0] = %u\n", ((uint32_t*)shared_ddr)[0]);
    printf("%p\n", shared_ddr);
    printf("%p\n", (void*) ddr_memory);
    printf("%p\n", (void*) usable_ddr);
    return(0);
}

static unsigned short LOCAL_examplePassed ( unsigned short pruNum )
{
  printf("index 0 is good: %s\n", pruDataMem_int[0] == 0x27 ? "yes" : "no");
  printf("index 1 is good: %s\n", pruDataMem_int[1] == ADDEND1 ? "yes" : "no");
  printf("index 2 is good: %s\n", pruDataMem_int[2] == ADDEND1 + ADDEND2 ? "yes" : "no");
  printf("index 3 is: %u\n", pruDataMem_int[3]);

  return ((pruDataMem_int[1] ==  ADDEND1) & (pruDataMem_int[2] ==  ADDEND1 + ADDEND2));
}
