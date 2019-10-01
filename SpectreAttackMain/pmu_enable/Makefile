PWD	:= $(shell pwd)
obj-m	:= enable_arm_pmu.o
#KDIR	:= /home/rasitmete/Downloads/petalinuxsource/xlnx-4.9
KDIR 	:= /MUST/BE/EXACT/KERNEL/BUILD/DIRECTORY"
CROSS	:= /usr/bin/arm-linux-gnueabihf-

	
all:
	make ARCH=arm CROSS_COMPILE=$(CROSS) -C $(KDIR) SUBDIRS=$(PWD) modules
clean:
	make -C $(KERNEL) SUBDIRS=$(PWD) clean
	
	

	
### Simple sudo make should run within /ko directory