cmake_minimum_required(VERSION 3.15)
project(Syrebas VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Whole-program/link-time optimization lets the linker fold identical CLAP
# vtable trampolines (/OPT:ICF) and drop unreferenced code (/OPT:REF), which
# matters a lot for a plugin this small. Only apply it in Release-type
# builds: it slows down incremental Debug builds for no benefit there.
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
if(ipo_supported AND CMAKE_BUILD_TYPE MATCHES "Release")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
endif()

if(MSVC)
    # Default Release linker behavior already implies /OPT:REF /OPT:ICF
    # (no /DEBUG), but state it explicitly so it doesn't depend on that.
    add_link_options($<$<CONFIG:Release>:/OPT:REF> $<$<CONFIG:Release>:/OPT:ICF>)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/clap/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Source files
set(CORE_SOURCES
    src/core/SynthEngine.cpp
    src/core/Oscillator.cpp
    src/core/Filter.cpp
    src/core/Envelope.cpp
)

set(CLAP_SOURCES
    src/clap/SyrebasClap.cpp
)

set(GUI_SOURCES
    src/gui/Font.cpp
    src/gui/Graphics.cpp
    src/gui/ControlRenderer.cpp
    src/gui/GuiWindow.cpp
)

# Linux X11 library dependency for GUI
if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    set(GUI_LIBS ${X11_LIBRARIES})
elseif(APPLE)
    find_library(COCOA_LIBRARY Cocoa)
    set(GUI_LIBS ${COCOA_LIBRARY})
elseif(WIN32)
    set(GUI_LIBS gdi32 user32)
endif()

# The GUI (60 Hz redraw) and CLAP host-glue code never run in the audio
# callback, so optimizing them for size instead of speed costs nothing where
# performance actually matters, while shrinking the shipped .clap. The DSP
# core keeps the default speed-optimized flags.
if(MSVC)
    set_source_files_properties(${GUI_SOURCES} ${CLAP_SOURCES} PROPERTIES
        COMPILE_OPTIONS "$<$<CONFIG:Release>:/O1>")
else()
    set_source_files_properties(${GUI_SOURCES} ${CLAP_SOURCES} PROPERTIES
        COMPILE_OPTIONS "$<$<CONFIG:Release>:-Os>")
endif()

# Syrebas CLAP plugin library module
add_library(syrebas MODULE
    ${CORE_SOURCES}
    ${CLAP_SOURCES}
    ${GUI_SOURCES}
)

set_target_properties(syrebas PROPERTIES
    PREFIX ""
    SUFFIX ".clap"
    OUTPUT_NAME "syrebas"
)

target_link_libraries(syrebas PRIVATE ${GUI_LIBS})

# Standalone test executable for DSP verification
add_executable(syrebas_dsp_test
    src/core/SynthEngine.cpp
    src/core/Oscillator.cpp
    src/core/Filter.cpp
    src/core/Envelope.cpp
    src/dsp_test_main.cpp
)

# Standalone test executable for filter self-oscillation/stability sweeps
# (see TB303_PARAMETER_CONFIDENCE.md and the header comment in the source
# file for why this exists and how to read its output).
add_executable(syrebas_filter_stability_test
    src/core/Filter.cpp
    src/filter_stability_test_main.cpp
)

# Standalone test executable for GUI frame verification
add_executable(syrebas_gui_test
    ${CORE_SOURCES}
    ${CLAP_SOURCES}
    ${GUI_SOURCES}
    src/gui_test_main.cpp
)
target_link_libraries(syrebas_gui_test PRIVATE ${GUI_LIBS})
