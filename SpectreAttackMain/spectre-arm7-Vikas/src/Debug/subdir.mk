################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source.c \
../vlist.c 

OBJS += \
./source.o \
./vlist.o 

C_DEPS += \
./source.d \
./vlist.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 Linux gcc compiler'
	arm-linux-gnueabihf-gcc -Wall -O0 -g3 -I"C:\XilinxProjects\SpectreAttackResearchProject\SpectreAttackMain\Debug\armageddon\libflush\build\armv7\release" -I"C:\XilinxProjects\SpectreAttackResearchProject\SpectreAttackMain\Debug\armageddon\libflush\libflush" -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


