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



set(SOURCES
    src/TraxHost.cpp
    src/Module.cpp
    src/Log.cpp
    src/Hardware.cpp
    src/Display.cpp
)

if(DEFINED ENV{USE_SDL})
    set(USE_SDL on) 
endif()

if(DEFINED ENV{TARGET_XMX} OR TARGET_XMX)
    message("target : XMX, using compact ui, scale 1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTARGET_XMX=1 -DCOMPACT_UI_SCALE=1 -DFORCE_COMPACT_UI=1")
else()
    if (DEFINED ENV{ALWAYS_COMPACT_UI} OR ALWAYS_COMPACT_UI)
        set(ALWAYS_COMPACT_UI 1)
        message("target : SSP, forced compact ui, scale 1")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTARGET_SSP=1 -DCOMPACT_UI_SCALE=1 -DFORCE_COMPACT_UI=1")
    else()
        message("target : SSP, ssp ui and scale")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTARGET_SSP=1 -DCOMPACT_UI_SCALE=2")
    endif() 
endif()


if(TARGET_XMX)
    # set(USE_SDL on)
    if(USE_SDL)
        set(SDL2_INCLUDE_DIRS ${CMAKE_SYSROOT}/usr/include/SDL2)
        set(SDL2_LIBRARIES ${CMAKE_SYSROOT}/usr/lib/libSDL2.so)
        add_definitions(-DUSE_SDL)
    endif()
elseif (TARGET_SSP) 
else()
    set(USE_SDL on)
    if(USE_SDL)
       add_definitions(-DUSE_SDL)
    endif()
endif()




add_executable(TraxHost ${SOURCES})

include_directories(${SDL2_INCLUDE_DIRS} external/rtaudio)
add_subdirectory(external/rtaudio rtaudio)
target_link_libraries(TraxHost ${SDL2_LIBRARIES} ${CMAKE_DL_LIBS} rtaudio)




