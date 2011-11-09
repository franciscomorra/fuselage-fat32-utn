################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/ppd.c \
../src/ppd_SSTF.c \
../src/ppd_comm.c \
../src/ppd_common.c \
../src/ppd_io_map.c \
../src/ppd_qManager.c \
../src/ppd_taker.c \
../src/ppd_translate.c 

OBJS += \
./src/ppd.o \
./src/ppd_SSTF.o \
./src/ppd_comm.o \
./src/ppd_common.o \
./src/ppd_io_map.o \
./src/ppd_qManager.o \
./src/ppd_taker.o \
./src/ppd_translate.o 

C_DEPS += \
./src/ppd.d \
./src/ppd_SSTF.d \
./src/ppd_comm.d \
./src/ppd_common.d \
./src/ppd_io_map.d \
./src/ppd_qManager.d \
./src/ppd_taker.d \
./src/ppd_translate.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utn_so/Desktop/trabajos/Commons" -I"/home/utn_so/Desktop/trabajos/PPD/include" -I"/home/utn_so/Desktop/trabajos/Commons/include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


