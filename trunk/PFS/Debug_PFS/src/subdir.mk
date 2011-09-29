################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/pfs.c \
../src/pfs_addressing.c \
../src/pfs_comm.c \
../src/pfs_fat32.c \
/home/utn_so/Desktop/trabajos/PPD/src/ppd_io_map.c \
../src/tad_direntry.c \
../src/tad_fat.c 

OBJS += \
./src/pfs.o \
./src/pfs_addressing.o \
./src/pfs_comm.o \
./src/pfs_fat32.o \
./src/ppd_io_map.o \
./src/tad_direntry.o \
./src/tad_fat.o 

C_DEPS += \
./src/pfs.d \
./src/pfs_addressing.d \
./src/pfs_comm.d \
./src/pfs_fat32.d \
./src/ppd_io_map.d \
./src/tad_direntry.d \
./src/tad_fat.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=28 -I"/home/utn_so/Desktop/trabajos/Commons" -I"/home/utn_so/Desktop/trabajos/Commons/include" -I"/home/utn_so/Desktop/trabajos/PFS/include" -I"/home/utn_so/Desktop/trabajos/PPD/include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/ppd_io_map.o: /home/utn_so/Desktop/trabajos/PPD/src/ppd_io_map.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=28 -I"/home/utn_so/Desktop/trabajos/Commons" -I"/home/utn_so/Desktop/trabajos/Commons/include" -I"/home/utn_so/Desktop/trabajos/PFS/include" -I"/home/utn_so/Desktop/trabajos/PPD/include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


