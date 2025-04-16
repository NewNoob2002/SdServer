set(SDfat ${CMAKE_CURRENT_LIST_DIR})

if(DEFINED ESP_PLATFORM)
    set(COMPONENT_ADD_INCLUDEDIRS
        ${SDfat}/src
    )
    file(GLOB SRCS
        ${SDfat}/src/common/*.cpp
        ${SDfat}/src/ExFatLib/*.cpp
        ${SDfat}/src/FatLib/*.cpp
        ${SDfat}/src/FsLib/*.cpp
        ${SDfat}/src/iostream/*.cpp
        ${SDfat}/src/SdCard/*.cpp
        ${SDfat}/src/SdCard/TeensySdio/*.cpp
        ${SDfat}/src/SpiDriver/*.cpp
        ${SDfat}/src/*.cpp
    )

    set(COMPONENT_SRCS ${SRCS})


    set(COMPONENT_REQUIRES arduino-esp32)


    ### If you use arduino-esp32 components, please activate next comment line.
    # list(APPEND COMPONENT_REQUIRES arduino-esp32)


    message(STATUS "SdFat use components = ${COMPONENT_REQUIRES}")

    register_component()
else()
    message("Error: No platform specified")
endif()
