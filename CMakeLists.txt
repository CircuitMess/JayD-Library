cmake_minimum_required(VERSION 3.15)
project(JayD-Library)
set(PORT COM19)
set(DEVICE cm:esp32:spencer) # Arduino device FQBN
set(CMAKE_CXX_STANDARD 11)

set(ROOT ${CMAKE_SOURCE_DIR})
if (WIN32 OR CYGWIN)
    set(Arduino15 $ENV{APPDATA}/../Local/Arduino15)
    set(ArduinoLibs $ENV{USERPROFILE}/OneDrive/Documents/Arduino/libraries)

    string(REPLACE "\\" "/" Arduino15 ${Arduino15})
    string(REPLACE "\\" "/" ArduinoLibs ${ArduinoLibs})

    if (CYGWIN)
        string(REGEX REPLACE "^([A-Za-z]):" "/cygdrive/\\1" Arduino15 ${Arduino15})
        string(REGEX REPLACE "^([A-Za-z]):" "/cygdrive/\\1" ArduinoLibs ${ArduinoLibs})
        string(REGEX REPLACE "^/cygdrive/([A-Za-z])" "\\1:/" ROOT ${ROOT})
    endif ()
else ()
    set(Arduino15 ~/.arduino15)
    set(ArduinoLibs ~/Arduino/libraries)
endif ()

# Include directories
message(${ArduinoLibs})
include_directories(${Arduino15}/packages/cm/hardware/esp32/1.1.0/cores/esp32)
include_directories(${Arduino15}/packages/cm/hardware/esp32/1.1.0/tools/sdk)
include_directories(${Arduino15}/packages/cm/hardware/esp32/1.1.0/tools/sdk/include)
include_directories(${Arduino15}/packages/cm/hardware/esp32/1.1.0/tools/sdk/include/driver)
include_directories(${Arduino15}/packages/cm/hardware/esp32/1.1.0/tools/sdk/include/esp32)
include_directories(${Arduino15}/packages/cm/hardware/esp32/1.1.0/libraries/Wire/src)
include_directories(${Arduino15}/packages/cm/hardware/esp32/1.1.0/tools/sdk/include/freertos)
include_directories(${Arduino15}/packages/cm/hardware/esp32/1.1.0/tools/sdk/include/freertos/freertos)
include_directories(${ArduinoLibs}/CircuitOS/src)
#include_directories(${ArduinoLibs}/CircuitOS/src/Devices/LEDmatrix)
include_directories(${ArduinoLibs}/TFT_eSPI)
include_directories(${ArduinoLibs}/TFT_eSPI/Extensions)
include_directories(${ArduinoLibs}/ESP8266Audio/src)




# Source directories
AUX_SOURCE_DIRECTORY(src SOURCES)

# Defines
add_definitions(-DDEBUG)
add_definitions(-DESP32)


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
set(BUILD ${ROOT}/build)

SET_SOURCE_FILES_PROPERTIES(${CMAKE_PROJECT_NAME}.ino PROPERTIES LANGUAGE CXX)

add_custom_command(OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_PROJECT_NAME}.elf ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_PROJECT_NAME}.bin ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_PROJECT_NAME}.partitions.bin
        COMMAND arduino-cli compile --fqbn ${DEVICE} -o ${BUILD}/${CMAKE_PROJECT_NAME}.bin ${CMAKE_PROJECT_NAME}.ino
        DEPENDS ${CMAKE_PROJECT_NAME}.ino ${SOURCES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_custom_target(CMBuild
        SOURCES ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_PROJECT_NAME}.elf)

add_custom_target(CMUpload ALL
        COMMAND arduino-cli upload --fqbn ${DEVICE} -p ${PORT} -i ${BUILD}/${CMAKE_PROJECT_NAME}.bin
        DEPENDS CMBuild
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_executable(${CMAKE_PROJECT_NAME}.bin ${CMAKE_PROJECT_NAME}.ino ${SOURCES} src/InputLib/InputJayD.cpp src/InputLib/InputJayD.h src/MainMenu/UITest.cpp src/MainMenu/UITest.hpp src/EncValue.cpp src/EncValue.hpp src/BtnValue.cpp src/BtnValue.hpp src/ScrollValue.cpp src/ScrollValue.hpp src/MainMenu/AppIcon.cpp src/MainMenu/AppIcon.hpp)