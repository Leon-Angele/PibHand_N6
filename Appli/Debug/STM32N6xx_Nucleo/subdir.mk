################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_DEPS += \
./STM32N6xx_Nucleo/stm32n6xx_nucleo.d 

OBJS += \
./STM32N6xx_Nucleo/stm32n6xx_nucleo.o 


# Each subdirectory must supply rules for building sources it contributes
STM32N6xx_Nucleo/stm32n6xx_nucleo.o: C:/Users/leon.angele/Desktop/fuckAround/PibHand_N6/Drivers/BSP/STM32N6xx_Nucleo/stm32n6xx_nucleo.c STM32N6xx_Nucleo/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu17 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32N657xx -DUSE_NUCLEO_64 -c -I../Core/Inc -I../../Secure_nsclib -I../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../Core/Src/hand -I../Core/Inc/hand -I../../Drivers/BSP/STM32N6xx_Nucleo -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -mcmse -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-STM32N6xx_Nucleo

clean-STM32N6xx_Nucleo:
	-$(RM) ./STM32N6xx_Nucleo/stm32n6xx_nucleo.cyclo ./STM32N6xx_Nucleo/stm32n6xx_nucleo.d ./STM32N6xx_Nucleo/stm32n6xx_nucleo.o ./STM32N6xx_Nucleo/stm32n6xx_nucleo.su

.PHONY: clean-STM32N6xx_Nucleo

