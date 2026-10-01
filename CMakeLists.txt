cmake_minimum_required(VERSION 3.15)
project(Acidus VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Calibration build: exposes every circuit-topology constant (ladder coupling
# poles, VCA saturation drive, etc.) as an automatable CLAP parameter, under
# "Experimental/..." module paths, for an external optimizer/fitting tool to
# drive. Off by default: a Release build only exposes the 7 front-panel
# controls, matching the real hardware's user-facing surface.
option(ACIDUS_CALIBRATION_BUILD "Expose hidden circuit-calibration parameters as CLAP params" OFF)
if(ACIDUS_CALIBRATION_BUILD)
    add_compile_definitions(ACIDUS_CALIBRATION_BUILD=1)
endif()

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
    src/core/Distortion.cpp
)

set(CLAP_SOURCES
    src/clap/AcidusClap.cpp
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

# Acidus CLAP plugin library module
add_library(acidus MODULE
    ${CORE_SOURCES}
    ${CLAP_SOURCES}
    ${GUI_SOURCES}
)

set_target_properties(acidus PROPERTIES
    PREFIX ""
    SUFFIX ".clap"
    OUTPUT_NAME "acidus"
)

target_link_libraries(acidus PRIVATE ${GUI_LIBS})

# Build the plugin and copy the .clap to the folder in the CLAPTEST environment
# variable. Not part of ALL: a host that has the plugin loaded locks the file,
# so a plain build should not fail on the copy.
add_custom_target(deploy_clap
    COMMAND ${CMAKE_COMMAND} -DCLAP_FILE=$<TARGET_FILE:acidus>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DeployClap.cmake
    COMMENT "Deploying acidus.clap to %CLAPTEST%"
    VERBATIM
)
add_dependencies(deploy_clap acidus)

# Standalone test executable for DSP verification
add_executable(acidus_dsp_test
    src/core/SynthEngine.cpp
    src/core/Oscillator.cpp
    src/core/Filter.cpp
    src/core/Envelope.cpp
    src/core/Distortion.cpp
    src/dsp_test_main.cpp
)

# Standalone test executable for filter self-oscillation/stability sweeps
add_executable(acidus_filter_stability_test
    src/core/Filter.cpp
    src/filter_stability_test_main.cpp
)

# Reference-conformance tests: checks the DSP core against the sourced
# claims in docs/TB-303 Reference/TB303_REFERENCE.md (frequency response,
# loop stability margin, envelope/slide time constants, control laws).
add_executable(acidus_reference_test
    ${CORE_SOURCES}
    src/reference_conformance_test_main.cpp
)

# Standalone offline optimizer: fits Filter.hpp's calibration constants
# (ladder pole spread, feedback gain ceiling, input drive) against target
# (frequency, relative-dB) points read off a reference hardware spectrum.
# See src/filter_resonance_fit_main.cpp's header comment for usage.
add_executable(acidus_filter_resonance_fit
    src/core/Filter.cpp
    src/filter_resonance_fit_main.cpp
)

# Standalone test executable for the MXR Distortion+ emulation
add_executable(acidus_distortion_test
    src/core/Distortion.cpp
    src/distortion_test_main.cpp
)

# Standalone test executable for GUI frame verification
add_executable(acidus_gui_test
    ${CORE_SOURCES}
    ${CLAP_SOURCES}
    ${GUI_SOURCES}
    src/gui_test_main.cpp
)
target_link_libraries(acidus_gui_test PRIVATE ${GUI_LIBS})

# Robustness tests: hostile parameters/events/state must never crash or
# produce non-finite audio.
add_executable(acidus_robustness_test
    ${CORE_SOURCES}
    ${CLAP_SOURCES}
    ${GUI_SOURCES}
    src/robustness_test_main.cpp
)
target_link_libraries(acidus_robustness_test PRIVATE ${GUI_LIBS})

# Performance benchmark (DSP, GUI rendering, optional real X11 window)
add_executable(acidus_perf_bench
    ${CORE_SOURCES}
    ${CLAP_SOURCES}
    ${GUI_SOURCES}
    src/perf_bench_main.cpp
)
target_link_libraries(acidus_perf_bench PRIVATE ${GUI_LIBS})

# Shared library with a C ABI that tools/calibrate_reference.py loads through
# ctypes to render candidate notes while fitting the model to hardware
# reference samples (test/resources). Same DSP core as the plugin.
add_library(acidus_calibration_render SHARED
    ${CORE_SOURCES}
    src/calibration/CalibrationRender.cpp
)
set_target_properties(acidus_calibration_render PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    PREFIX ""
)

# Burette: a separate TB-303 style pattern sequencer plugin
# (burette.clap) that outputs notes for Acidus. Its own target: nothing
# here is linked into acidus.clap. It reuses the GUI's Graphics/Font code.
set(SEQ_SOURCES
    src/sequencer/Pattern.cpp
    src/sequencer/SequencerEngine.cpp
    src/sequencer/MidiExport.cpp
)
set(SEQ_PLUGIN_SOURCES
    src/sequencer/SequencerClap.cpp
    src/sequencer/SequencerGui.cpp
    src/gui/Font.cpp
    src/gui/Graphics.cpp
)
if(MSVC)
    set_source_files_properties(${SEQ_SOURCES} ${SEQ_PLUGIN_SOURCES} PROPERTIES
        COMPILE_OPTIONS "$<$<CONFIG:Release>:/O1>")
else()
    set_source_files_properties(${SEQ_SOURCES} ${SEQ_PLUGIN_SOURCES} PROPERTIES
        COMPILE_OPTIONS "$<$<CONFIG:Release>:-Os>")
endif()

add_library(burette MODULE
    ${SEQ_SOURCES}
    ${SEQ_PLUGIN_SOURCES}
)
set_target_properties(burette PROPERTIES
    PREFIX ""
    SUFFIX ".clap"
    OUTPUT_NAME "burette"
)
# Windows: file dialogs (comdlg32) and dragging MIDI files out (ole32, shell32).
if(WIN32)
    set(SEQ_GUI_LIBS ${GUI_LIBS} comdlg32 ole32 shell32 uuid)
else()
    set(SEQ_GUI_LIBS ${GUI_LIBS})
endif()
target_link_libraries(burette PRIVATE ${SEQ_GUI_LIBS})

add_custom_target(deploy_burette_clap
    COMMAND ${CMAKE_COMMAND} -DCLAP_FILE=$<TARGET_FILE:burette>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DeployClap.cmake
    COMMENT "Deploying burette.clap to %CLAPTEST%"
    VERBATIM
)
add_dependencies(deploy_burette_clap burette)

# Sequencer tests: step timing (gate, slide, tie, accent), host-position
# sync, trigger handling, state round trip and GUI editing.
add_executable(burette_test
    ${SEQ_SOURCES}
    ${SEQ_PLUGIN_SOURCES}
    src/sequencer_test_main.cpp
)
target_link_libraries(burette_test PRIVATE ${SEQ_GUI_LIBS})
