################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/ppd.c \
../src/ppd_comm.c \
../src/ppd_io_map.c 

OBJS += \
./src/ppd.o \
./src/ppd_comm.o \
./src/ppd_io_map.o 

C_DEPS += \
./src/ppd.d \
./src/ppd_comm.d \
./src/ppd_io_map.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utn_so/Desktop/trabajos/PPD/include" -I"/home/utn_so/Desktop/trabajos/Commons/include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


