################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/source.c \
../src/vlist.c 

S_UPPER_SRCS += \
../src/monitor.S 

OBJS += \
./src/monitor.o \
./src/source.o \
./src/vlist.o 

S_UPPER_DEPS += \
./src/monitor.d 

C_DEPS += \
./src/source.d \
./src/vlist.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.S
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 Linux gcc compiler'
	arm-linux-gnueabihf-gcc -Wall -O3 -g3 -I"C:\XilinxProjects\SpectreAttackResearchProject\SpectreAttackMain\Debug\armageddon\libflush\libflush\armv7" -I"C:\XilinxProjects\SpectreAttackResearchProject\SpectreAttackMain\spectre-arm7-Vikas\src" -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 Linux gcc compiler'
	arm-linux-gnueabihf-gcc -Wall -O3 -g3 -I"C:\XilinxProjects\SpectreAttackResearchProject\SpectreAttackMain\Debug\armageddon\libflush\libflush\armv7" -I"C:\XilinxProjects\SpectreAttackResearchProject\SpectreAttackMain\spectre-arm7-Vikas\src" -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


