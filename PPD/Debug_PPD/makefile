################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include src/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: PPD

dependents:
	-cd /home/utn_so/Desktop/trabajos/Commons/Debug && $(MAKE) all

# Tool invocations
PPD: $(OBJS) $(USER_OBJS) /home/utn_so/Desktop/trabajos/Commons/Debug/libCommons.so
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc -lpthread -L"/home/utn_so/Desktop/trabajos/Commons/Debug" -o "PPD" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(EXECUTABLES) PPD
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:
/home/utn_so/Desktop/trabajos/Commons/Debug/libCommons.so:

-include ../makefile.targets
