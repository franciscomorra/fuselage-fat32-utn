################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../ppd_FSCAN.c 

OBJS += \
./ppd_FSCAN.o 

C_DEPS += \
./ppd_FSCAN.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utn_so/Desktop/trabajos/Commons" -I"/home/utn_so/Desktop/trabajos/PPD/include" -I"/home/utn_so/Desktop/trabajos/Commons/include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


