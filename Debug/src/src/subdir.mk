################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/src/Directory.cpp \
../src/src/DirectoryNode.cpp \
../src/src/DirectoryTree.cpp \
../src/src/FreeBlockList.cpp \
../src/src/HDD.cpp \
../src/src/INodeList.cpp \
../src/src/SuperBlock.cpp \
../src/src/UnitTest.cpp \
../src/src/bitFunctions.cpp \
../src/src/debugFunctions.cpp 

OBJS += \
./src/src/Directory.o \
./src/src/DirectoryNode.o \
./src/src/DirectoryTree.o \
./src/src/FreeBlockList.o \
./src/src/HDD.o \
./src/src/INodeList.o \
./src/src/SuperBlock.o \
./src/src/UnitTest.o \
./src/src/bitFunctions.o \
./src/src/debugFunctions.o 

CPP_DEPS += \
./src/src/Directory.d \
./src/src/DirectoryNode.d \
./src/src/DirectoryTree.d \
./src/src/FreeBlockList.d \
./src/src/HDD.d \
./src/src/INodeList.d \
./src/src/SuperBlock.d \
./src/src/UnitTest.d \
./src/src/bitFunctions.d \
./src/src/debugFunctions.d 


# Each subdirectory must supply rules for building sources it contributes
src/src/%.o: ../src/src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I../__GXX_EXPERIMENTAL_CXX0X__ -O0 -g3 -Wall -c -fmessage-length=0 -std=c++0x -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


