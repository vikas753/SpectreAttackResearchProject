/*
 * Enable user-mode ARM performance counter access.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/smp.h>

#define DRVR_NAME "enable_arm_pmu"


static void
enable_cpu_counters(void* data)
{

	/* Enable user-mode access to counters. */
	asm volatile("mcr p15, 0, %0, c9, c14, 0" :: "r"(1));
	/* Program PMU and enable all counters */
	asm volatile("mcr p15, 0, %0, c9, c12, 0" :: "r"(0x00000017));
	asm volatile("mcr p15, 0, %0, c9, c12, 1" :: "r"(0x8000000f));


}


static int __init
init(void)
{
	on_each_cpu(enable_cpu_counters, NULL, 1);
	return 0;
}

module_init(init);

