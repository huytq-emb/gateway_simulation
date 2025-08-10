################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/can_drv.c \
../Core/Src/can_test_generator.c \
../Core/Src/main_loopback.c \
../Core/Src/pdu_router.c \
../Core/Src/stm32f4xx_hal_msp.c \
../Core/Src/stm32f4xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_config.c \
../Core/Src/system_stm32f4xx.c \
../Core/Src/uart_drv.c 

OBJS += \
./Core/Src/can_drv.o \
./Core/Src/can_test_generator.o \
./Core/Src/main_loopback.o \
./Core/Src/pdu_router.o \
./Core/Src/stm32f4xx_hal_msp.o \
./Core/Src/stm32f4xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_config.o \
./Core/Src/system_stm32f4xx.o \
./Core/Src/uart_drv.o 

C_DEPS += \
./Core/Src/can_drv.d \
./Core/Src/can_test_generator.d \
./Core/Src/main_loopback.d \
./Core/Src/pdu_router.d \
./Core/Src/stm32f4xx_hal_msp.d \
./Core/Src/stm32f4xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_config.d \
./Core/Src/system_stm32f4xx.d \
./Core/Src/uart_drv.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/can_drv.cyclo ./Core/Src/can_drv.d ./Core/Src/can_drv.o ./Core/Src/can_drv.su ./Core/Src/can_test_generator.cyclo ./Core/Src/can_test_generator.d ./Core/Src/can_test_generator.o ./Core/Src/can_test_generator.su ./Core/Src/main_loopback.cyclo ./Core/Src/main_loopback.d ./Core/Src/main_loopback.o ./Core/Src/main_loopback.su ./Core/Src/pdu_router.cyclo ./Core/Src/pdu_router.d ./Core/Src/pdu_router.o ./Core/Src/pdu_router.su ./Core/Src/stm32f4xx_hal_msp.cyclo ./Core/Src/stm32f4xx_hal_msp.d ./Core/Src/stm32f4xx_hal_msp.o ./Core/Src/stm32f4xx_hal_msp.su ./Core/Src/stm32f4xx_it.cyclo ./Core/Src/stm32f4xx_it.d ./Core/Src/stm32f4xx_it.o ./Core/Src/stm32f4xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_config.cyclo ./Core/Src/system_config.d ./Core/Src/system_config.o ./Core/Src/system_config.su ./Core/Src/system_stm32f4xx.cyclo ./Core/Src/system_stm32f4xx.d ./Core/Src/system_stm32f4xx.o ./Core/Src/system_stm32f4xx.su ./Core/Src/uart_drv.cyclo ./Core/Src/uart_drv.d ./Core/Src/uart_drv.o ./Core/Src/uart_drv.su

.PHONY: clean-Core-2f-Src

