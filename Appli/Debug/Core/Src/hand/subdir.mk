################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Core/Src/hand/hand_bridge.cpp \
../Core/Src/hand/hand_controller.cpp \
../Core/Src/hand/serial_commander.cpp \
../Core/Src/hand/servo.cpp 

OBJS += \
./Core/Src/hand/hand_bridge.o \
./Core/Src/hand/hand_controller.o \
./Core/Src/hand/serial_commander.o \
./Core/Src/hand/servo.o 

CPP_DEPS += \
./Core/Src/hand/hand_bridge.d \
./Core/Src/hand/hand_controller.d \
./Core/Src/hand/serial_commander.d \
./Core/Src/hand/servo.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/hand/%.o Core/Src/hand/%.su Core/Src/hand/%.cyclo: ../Core/Src/hand/%.cpp Core/Src/hand/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m55 -std=gnu++17 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32N657xx -c -I../Core/Inc -I../../Secure_nsclib -I../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../Core/Src/hand -I../Core/Inc/hand -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-hand

clean-Core-2f-Src-2f-hand:
	-$(RM) ./Core/Src/hand/hand_bridge.cyclo ./Core/Src/hand/hand_bridge.d ./Core/Src/hand/hand_bridge.o ./Core/Src/hand/hand_bridge.su ./Core/Src/hand/hand_controller.cyclo ./Core/Src/hand/hand_controller.d ./Core/Src/hand/hand_controller.o ./Core/Src/hand/hand_controller.su ./Core/Src/hand/serial_commander.cyclo ./Core/Src/hand/serial_commander.d ./Core/Src/hand/serial_commander.o ./Core/Src/hand/serial_commander.su ./Core/Src/hand/servo.cyclo ./Core/Src/hand/servo.d ./Core/Src/hand/servo.o ./Core/Src/hand/servo.su

.PHONY: clean-Core-2f-Src-2f-hand

