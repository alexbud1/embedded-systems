################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: spi_flash.axf

# Tool invocations
spi_flash.axf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -L"C:\Users\student\Documents\f07\embedded-systems\Lib_CMSISv1p30_LPC17xx\Debug" -L"C:\Users\student\Documents\f07\embedded-systems\Lib_EaBaseBoard\Debug" -L"C:\Users\student\Documents\f07\embedded-systems\Lib_MCU\Debug" -Xlinker --gc-sections -Xlinker -Map=spi_flash.map -mcpu=cortex-m3 -mthumb -T "rdb1768cmsis_uart_Debug.ld" -o "spi_flash.axf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) spi_flash.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size spi_flash.axf; # arm-none-eabi-objdump -h -S spi_flash.axf >spi_flash.lss
	-@echo ' '

.PHONY: all clean dependents post-build

-include ../makefile.targets
