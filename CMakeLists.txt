cmake_minimum_required(VERSION 3.15)
project(TraxHost)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    message("apple config")
    find_package(SDL2 REQUIRED)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if (CMAKE_CROSSCOMPILING)
        message("cross compile config")
    else ()
            message("no cross compile config")
    endif ()
endif ()


include_directories(${SDL2_INCLUDE_DIRS})


set(SOURCES
    src/main.cpp
    src/Module.cpp
    src/Log.cpp
    src/Hardware.cpp
)

add_executable(TraxHost ${SOURCES})

target_link_libraries(TraxHost ${SDL2_LIBRARIES} ${CMAKE_DL_LIBS})