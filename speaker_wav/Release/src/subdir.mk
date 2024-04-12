################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/cr_startup_lpc17.c \
../src/main.c \
../src/startupSound_8k.c 

OBJS += \
./src/cr_startup_lpc17.o \
./src/main.o \
./src/startupSound_8k.o 

C_DEPS += \
./src/cr_startup_lpc17.d \
./src/main.d \
./src/startupSound_8k.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -Os -g -Wall -c -fmessage-length=0 -fmacro-prefix-map="../$(@D)/"=. -mcpu=cortex-m7 -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


