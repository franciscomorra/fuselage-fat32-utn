################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/config_manager.c \
../src/log.c \
../src/nipc.c \
../src/tad_sockets.c \
../src/utils.c 

OBJS += \
./src/config_manager.o \
./src/log.o \
./src/nipc.o \
./src/tad_sockets.o \
./src/utils.o 

C_DEPS += \
./src/config_manager.d \
./src/log.d \
./src/nipc.d \
./src/tad_sockets.d \
./src/utils.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utn_so/Desktop/trabajos/Commons/include" -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


