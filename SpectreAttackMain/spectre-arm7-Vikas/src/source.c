

#define _GNU_SOURCE

#include <linux/perf_event.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <pthread.h>
#include <signal.h>

asm(".include \"monitor.S\"");

signed int *exp_sign;

static inline uint32_t _read_cpu_counter(int r) {
  // Read PMXEVCNTR #r
  // This is done by first writing the counter number to PMSELR and then reading PMXEVCNTR
  uint32_t ret;
  asm volatile ("MCR p15, 0, %0, c9, c12, 5\t\n" :: "r"(r));      // Select event counter in PMSELR
  asm volatile ("MRC p15, 0, %0, c9, c13, 2\t\n" : "=r"(ret));    // Read from PMXEVCNTR
  return ret;
}

// Very simple , switch to secure world api 
static inline void _secure_world_switch()
{
  printf(" 	asm volatile(CPS     0x16); \n");
  asm volatile("CPS     0x16");  // Change the processor state to MONITOR mode .
  uint32_t scr_reg_value = 0x0;  
  printf(" 	asm volatile(MRC     p15, 0, %0, c1, c1, 0); \n");
  asm volatile("MRC     p15, 0, %0, c1, c1, 0" : "=r"(scr_reg_value));    
}

static inline void _setup_cpu_counter(uint32_t r, uint32_t event, const char* name) {
	//  cpu_name[r] = name;

  // Write PMXEVTYPER #r
  // This is done by first writing the counter number to PMSELR and then writing PMXEVTYPER
  asm volatile ("MCR p15, 0, %0, c9, c12, 5\t\n" :: "r"(r));        // Select event counter in PMSELR
  asm volatile ("MCR p15, 0, %0, c9, c13, 1\t\n" :: "r"(event));    // Set the event number in PMXEVTYPER
}

static void init_cpu_perf() {

  // Disable all counters for configuration (PCMCNTENCLR)
  asm volatile ("MCR p15, 0, %0, c9, c12, 2\t\n" :: "r"(0x8000003f));

  // disable counter overflow interrupts
  // asm volatile ("MCR p15, 0, %0, c9, c14, 2\n\t" :: "r"(0x8000003f));


  // Select which events to count in the 6 configurable counters
  // All of these events come from the list of required events.

//  _setup_cpu_counter(0, 0x00, "SW_INC");
//  _setup_cpu_counter(1, 0x03, "L1DFILL");
//  _setup_cpu_counter(2, 0x04, "L1DACC");
  _setup_cpu_counter(1, 0x10, "BR_MIS_PRED");
//  _setup_cpu_counter(4, 0x11, "CYCLE");
  _setup_cpu_counter(0, 0x12, "SP_EXEC");

}

static inline void reset_cpu_perf() {
  // Disable all counters (PMCNTENCLR):
  asm volatile ("MCR p15, 0, %0, c9, c12, 2\t\n" :: "r"(0x8000003f));

  uint32_t pmcr  = 0x1    // enable counters
            | 0x2    // reset all other counters
            | 0x4    // reset cycle counter
//            | 0x8    // enable "by 64" divider for CCNT.
            | 0x10;  // Export events to external monitoring

  // program the performance-counter control-register (PMCR):
  asm volatile ("MCR p15, 0, %0, c9, c12, 0\t\n" :: "r"(pmcr));

  // clear overflows (PMOVSR):
    asm volatile ("MCR p15, 0, %0, c9, c12, 3\t\n" :: "r"(0x8000003f));

  // Enable all counters (PMCNTENSET):
  asm volatile ("MCR p15, 0, %0, c9, c12, 1\t\n" :: "r"(0x8000003f));
}

static inline uint64_t read_perf_counter(void){
	uint32_t r = 0;
	asm volatile("mrc p15, 0, %0, c9, c12, 1" : "=r"(r) );
	return r;
}

static inline void barriers(void){
	__asm__ __volatile__ ("dsb sy"  : : : "memory");
}

static inline void ins_barriers(void){
	__asm__ __volatile__ ("isb" : : : "memory");
}

static inline void dmb_barriers(void){
	__asm__ __volatile__ ("dmb" : : : "memory");
}

static uint32_t get_mispred(void){
	uint32_t r;

	ins_barriers();
	dmb_barriers();

	r = _read_cpu_counter(1);


	ins_barriers();
	dmb_barriers();
	return r;
}

static uint32_t get_pred(void){
	uint32_t r;

	ins_barriers();
	dmb_barriers();

	r = _read_cpu_counter(0);

	ins_barriers();
	dmb_barriers();
	return r;
}

void print_hist(int* hist, int total, const char* title) {
	for(int i = 0; i < total; i++)
		printf("%s %i\n", title, hist[i]);
}

uint32_t mispred_rev[10000];
uint32_t    pred_rev[10000];

uint32_t mispred_1;
uint32_t mispred_2;

uint32_t pred_1;
uint32_t pred_2;

int* dummmmy;
int array[1024];
int dummy_index = 0;
int multiplyOrNot(signed char e, int a){
	if ((signed char)e < 0){ // *exp_sign
		a *= 37;
		*dummmmy += 13;
		array[(dummy_index++)%1024] = *dummmmy;
	}
	*dummmmy -= 11;

	return a;

}

void trainVulnBranch_n(){ /* n of them will be run below */
	signed int e = -1;
	int a = 2;

	multiplyOrNot(e,a); //1
	multiplyOrNot(e,a); //2
	multiplyOrNot(e,a); //3
	multiplyOrNot(e,a); //4
	multiplyOrNot(e,a); //5
	multiplyOrNot(e,a); //6
	multiplyOrNot(e,a); //7
	multiplyOrNot(e,a); //8
//	multiplyOrNot(e,a); //9
//	multiplyOrNot(e,a); //10
//	multiplyOrNot(e,a); //11
//	multiplyOrNot(e,a); //12
//	multiplyOrNot(e,a); //13
//	multiplyOrNot(e,a); //14
//	multiplyOrNot(e,a); //15
//	multiplyOrNot(e,a); //16

}

void trainVulnBranch_n4(){ /* n+4 of them will be run below*/
	signed int e = -1;
	int a = 2;

	multiplyOrNot(e,a); //1
	multiplyOrNot(e,a); //2
	multiplyOrNot(e,a); //3
	multiplyOrNot(e,a); //4
	multiplyOrNot(e,a); //5
	multiplyOrNot(e,a); //6
	multiplyOrNot(e,a); //7
	multiplyOrNot(e,a); //8
	multiplyOrNot(e,a); //9
	multiplyOrNot(e,a); //10
	multiplyOrNot(e,a); //11
	multiplyOrNot(e,a); //12
//	multiplyOrNot(e,a); //13
//	multiplyOrNot(e,a); //14
//	multiplyOrNot(e,a); //15
//	multiplyOrNot(e,a); //16
//	multiplyOrNot(e,a); //17
//	multiplyOrNot(e,a); //18
//	multiplyOrNot(e,a); //19
//	multiplyOrNot(e,a); //20

}

int cpu = 0;

void setcpuaffinity(int cpu) {
    cpu_set_t cs;
    CPU_ZERO(&cs);
    CPU_SET(cpu, &cs);
    if (sched_setaffinity(0, sizeof(cs), &cs) < 0) {
        perror("migrate");
        exit(1);
    }
}


int eData = 100;

static inline void resultProcess(int try)
{
  int a=2;
  int mispred_1_l = get_mispred();
  int pred_1_l    = get_pred();

  multiplyOrNot(eData, a); // taken
  int mispred_2_l = get_mispred();
  int pred_2_l    = get_pred();
  mispred_rev[try] = mispred_2_l - mispred_1_l;
  pred_rev[try] = pred_2_l - pred_1_l;
}

static inline void trainProcess(int try)
{
  int a = 2;
/*1*/ trainVulnBranch_n4(); // non taken
  barriers();
  ins_barriers();
  dmb_barriers();
/*2*/ multiplyOrNot(1, a); // taken
  barriers();
  ins_barriers();
  dmb_barriers();
/*3*/ trainVulnBranch_n(); // non taken
  barriers();
  ins_barriers();
  dmb_barriers();
/*4*/ multiplyOrNot(1, a); // taken
  barriers();
  ins_barriers();
  dmb_barriers();
/*5*/ trainVulnBranch_n(); // non taken
  barriers();
  ins_barriers();
  dmb_barriers();

}

#define PROCESS_FORK

void ReverseEng(int try){

#ifdef PROCESS_FORK
	pid_t pid = fork();
	if(pid == 0)
	{
	  printf(" Cross Process Training e \n");
#endif
	  setcpuaffinity(cpu);
      trainProcess(try);
#ifdef PROCESS_FORK
	  exit(0);
	}
	else
	{
	  // CheckPoint
	  wait(0);
	  resultProcess(try);
      //_secure_world_switch();
#endif
	  setcpuaffinity(cpu);
      resultProcess(try);

#ifdef PROCESS_FORK
	}
#endif
}

int mispred[1000];
int bit_no = 0;
int bits[8];


void printText()
{
  for(int i=0;i<1;i++)
  {
    printf("%s \r\t","VV----------------");
    printf("%s \r\t","VV----------------");
    printf("%s \r\t","VV----------------");
    printf("%s \r\t","VV----------------");
    printf("%s \r\t","VV----------------");
  }
}


#define PAGE_SIZE 0x2000


static inline void* alloc_page(void* function_ptr , int bitmask)
{
    int addr = (int)function_ptr & bitmask;
    void* page_ptr = mmap((void*)addr, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	printf(" alloc_page :: addr : %x , function_ptr : %p , bitmask : %x , page_ptr : %p \n " , addr , function_ptr , bitmask , page_ptr);
    return page_ptr;
}

static inline void* find_function_address_page(void* function_ptr , int bitmask , void* page_ptr)
{
    int page_base_addr = (int)page_ptr;
    int page_top_addr  = page_base_addr + PAGE_SIZE;
    int function_ptr_bmsk_val = (int)function_ptr & bitmask;

    for(int i=page_base_addr;i<page_top_addr;i++)
    {
      int bitmask_value = i & bitmask;
      if(bitmask_value == function_ptr_bmsk_val)
      {
    	printf(" find_function_address_page : page_addr : %x , bmsk_val : %x , function_ptr_bmsk_val : %x \n" , i , bitmask_value , function_ptr_bmsk_val);
        return (void*)i;
      }
    }

    return NULL;
}

// Function size is 132 bytes , it is scaled by 3
#define BUMP_UP_FUNC_SIZE 1024

void copy_function_data(void * dest , void * src , int size_in_bytes)
{
  memcpy(dest,src,size_in_bytes);
  printf(" copy_function_data :: dest : %p , src : %p , size_in_bytes : %d \n" , dest , src , size_in_bytes);

}

int main(int argc, char **argv)
{

    /*Shared Memeories*/
    exp_sign = mmap(NULL, sizeof(*exp_sign), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *exp_sign = 0;

    init_cpu_perf();
    reset_cpu_perf();

    void * function_page_ptr = find_function_address_page(&multiplyOrNot , 0xfff,alloc_page(&multiplyOrNot , 0xfff));
    copy_function_data(function_page_ptr , &multiplyOrNot , BUMP_UP_FUNC_SIZE);
    void (*func_ptr)(signed char,int) = &multiplyOrNot;
    //func_ptr(-1,2);
    printf(" Function pointer works ! \n ");

    dummmmy = (int*)malloc(sizeof(int));
    *dummmmy = 5;


    for (int i=0; i<1000; i++)
    {
      printText();
      ReverseEng(i);
    }

    printf("\n");

    int num_mis_preds = 0;
    int num_preds     = 0;

    for(int i=0; i<1000; i++)
    {
      printf("(%d , %d) ", mispred_rev[i] , pred_rev[i]);
      if(mispred_rev[i] == 1)
      {
        num_mis_preds++;
      }
      num_preds++;
    }

    printf(" \n");

    printf("stats , num_mis_preds : %d ,  num_preds : %d " , num_mis_preds , num_preds);
    printf("\n--DONE--\n");

    return (0);

}




