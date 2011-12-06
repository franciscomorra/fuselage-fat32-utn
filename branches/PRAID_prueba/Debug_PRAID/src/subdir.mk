################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/praid.c \
../src/praid_console.c \
../src/praid_pfs_handler.c \
../src/praid_pfslist.c \
../src/praid_ppd_handler.c \
../src/praid_ppdlist.c \
../src/praid_synchronize.c 

OBJS += \
./src/praid.o \
./src/praid_console.o \
./src/praid_pfs_handler.o \
./src/praid_pfslist.o \
./src/praid_ppd_handler.o \
./src/praid_ppdlist.o \
./src/praid_synchronize.o 

C_DEPS += \
./src/praid.d \
./src/praid_console.d \
./src/praid_pfs_handler.d \
./src/praid_pfslist.d \
./src/praid_ppd_handler.d \
./src/praid_ppdlist.d \
./src/praid_synchronize.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utn_so/Escritorio/Workspace/PRAID/include" -I"/home/utn_so/Escritorio/Workspace/Commons/include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


