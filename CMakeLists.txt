cmake_minimum_required(VERSION 3.18)
project(TriDJs_Separador_Stems VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Configure LibTorch prefix path
set(CMAKE_PREFIX_PATH "C:/TridjsStems/libtorch")
find_package(Torch REQUIRED)

# Fetch JUCE
include(FetchContent)
FetchContent_Declare(
    juce
    URL https://github.com/juce-framework/JUCE/archive/refs/tags/7.0.9.zip
)
FetchContent_MakeAvailable(juce)

juce_add_gui_app(TriDJs_Separador_Stems
    PRODUCT_NAME "TriDJs Stems"
    VERSION 1.0.0
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/logo.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/logo.png"
)

juce_generate_juce_header(TriDJs_Separador_Stems)

target_sources(TriDJs_Separador_Stems PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/MainComponent.h
    Source/StemEngine.cpp
    Source/StemEngine.h
)

target_compile_options(TriDJs_Separador_Stems PRIVATE "${TORCH_CXX_FLAGS}")

target_compile_definitions(TriDJs_Separador_Stems PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(TriDJs_Separador_Stems PRIVATE
    "${TORCH_LIBRARIES}"
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
)
