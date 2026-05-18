cmake_minimum_required(VERSION 3.18)
project(TriDJs_Separador_Stems VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =============================================================================
# LibTorch – caminho de instalação local
# =============================================================================
set(CMAKE_PREFIX_PATH "C:/TridjsStems/libtorch")
find_package(Torch REQUIRED)

# =============================================================================
# JUCE Framework
# =============================================================================
include(FetchContent)
FetchContent_Declare(
    juce
    URL https://github.com/juce-framework/JUCE/archive/refs/tags/7.0.9.zip
)
FetchContent_MakeAvailable(juce)

# =============================================================================
# Alvo principal do aplicativo
# =============================================================================
juce_add_gui_app(TriDJs_Separador_Stems
    PRODUCT_NAME "TriDJs Stems"
    VERSION 1.0.0
    ICON_BIG  "${CMAKE_CURRENT_SOURCE_DIR}/logo.png"
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

# Delay-load das DLLs CUDA para que o app abra normalmente em PCs sem GPU NVIDIA
if(MSVC)
    target_compile_options(TriDJs_Separador_Stems PRIVATE /utf-8)
    target_link_options(TriDJs_Separador_Stems PRIVATE
        /DELAYLOAD:torch_cuda.dll
        /DELAYLOAD:c10_cuda.dll
        /DELAYLOAD:caffe2_nvrtc.dll
        /DELAYLOAD:cudart64_12.dll
        /DELAYLOAD:cublas64_12.dll
        /DELAYLOAD:cublasLt64_12.dll
        /DELAYLOAD:cudnn64_8.dll
        /DELAYLOAD:cudnn_cnn_infer64_8.dll
        /DELAYLOAD:cudnn_ops_infer64_8.dll
        /DELAYLOAD:nvrtc64_120_0.dll
        /DELAYLOAD:nvrtc-builtins64_121.dll
        delayimp.lib)
endif()

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

# =============================================================================
# POST-BUILD: Cópia automática de DLLs para junto do executável
# =============================================================================
# Estratégia de distribuição:
#   GRUPO A – DLLs CPU (obrigatórias em TODOS os clientes)
#             Sem essas DLLs o app não abre de jeito nenhum.
#   GRUPO B – DLLs GPU/CUDA (opcionais – ignoradas silenciosamente em PCs sem NVIDIA)
#             Empacotadas junto para que clientes COM GPU tenham aceleração imediata
#             sem precisar instalar CUDA Toolkit separadamente.
# =============================================================================

set(TORCH_DLL_DIR "C:/TridjsStems/libtorch/lib")
set(OUT_DIR "$<TARGET_FILE_DIR:TriDJs_Separador_Stems>")

# -----------------------------------------------------------------------
# GRUPO A – DLLs CPU obrigatórias (torch_cpu, c10, MKL, OpenMP, etc.)
# -----------------------------------------------------------------------
set(CPU_REQUIRED_DLLS
    "${TORCH_DLL_DIR}/c10.dll"
    "${TORCH_DLL_DIR}/torch.dll"
    "${TORCH_DLL_DIR}/torch_cpu.dll"
    "${TORCH_DLL_DIR}/fbgemm.dll"
    "${TORCH_DLL_DIR}/libiomp5md.dll"
    "${TORCH_DLL_DIR}/libiompstubs5md.dll"
    "${TORCH_DLL_DIR}/mkl_core.1.dll"
    "${TORCH_DLL_DIR}/mkl_intel_thread.1.dll"
    "${TORCH_DLL_DIR}/asmjit.dll"
    "${TORCH_DLL_DIR}/uv.dll"
    "${TORCH_DLL_DIR}/zlibwapi.dll"
    "${TORCH_DLL_DIR}/pytorch_jni.dll"
    "${TORCH_DLL_DIR}/fbjni.dll"
    "${TORCH_DLL_DIR}/torch_global_deps.dll"
)

# -----------------------------------------------------------------------
# GRUPO B – DLLs GPU/CUDA (opcionais – presentes para clientes com NVIDIA)
# O /DELAYLOAD garante que o app não trava ao abrir em PCs sem GPU NVIDIA.
# -----------------------------------------------------------------------
set(GPU_OPTIONAL_DLLS
    "${TORCH_DLL_DIR}/torch_cuda.dll"
    "${TORCH_DLL_DIR}/c10_cuda.dll"
    "${TORCH_DLL_DIR}/caffe2_nvrtc.dll"
    "${TORCH_DLL_DIR}/cudart64_12.dll"
    "${TORCH_DLL_DIR}/cublas64_12.dll"
    "${TORCH_DLL_DIR}/cublasLt64_12.dll"
    "${TORCH_DLL_DIR}/cudnn64_8.dll"
    "${TORCH_DLL_DIR}/cudnn_adv_infer64_8.dll"
    "${TORCH_DLL_DIR}/cudnn_adv_train64_8.dll"
    "${TORCH_DLL_DIR}/cudnn_cnn_infer64_8.dll"
    "${TORCH_DLL_DIR}/cudnn_cnn_train64_8.dll"
    "${TORCH_DLL_DIR}/cudnn_ops_infer64_8.dll"
    "${TORCH_DLL_DIR}/cudnn_ops_train64_8.dll"
    "${TORCH_DLL_DIR}/cufft64_11.dll"
    "${TORCH_DLL_DIR}/cufftw64_11.dll"
    "${TORCH_DLL_DIR}/curand64_10.dll"
    "${TORCH_DLL_DIR}/cusolver64_11.dll"
    "${TORCH_DLL_DIR}/cusolverMg64_11.dll"
    "${TORCH_DLL_DIR}/cusparse64_12.dll"
    "${TORCH_DLL_DIR}/nvrtc64_120_0.dll"
    "${TORCH_DLL_DIR}/nvrtc-builtins64_121.dll"
    "${TORCH_DLL_DIR}/nvJitLink_120_0.dll"
    "${TORCH_DLL_DIR}/nvToolsExt64_1.dll"
    "${TORCH_DLL_DIR}/cupti64_2023.1.1.dll"
)

# -----------------------------------------------------------------------
# Copia GRUPO A (CPU – obrigatórias) – falha o build se não estiverem lá
# -----------------------------------------------------------------------
foreach(DLL ${CPU_REQUIRED_DLLS})
    add_custom_command(TARGET TriDJs_Separador_Stems POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DLL}"
            "${OUT_DIR}"
        COMMENT "  [CPU DLL] Copiando ${DLL}"
    )
endforeach()

# -----------------------------------------------------------------------
# Copia GRUPO B (GPU – opcionais) – usa script que ignora DLLs ausentes
# -----------------------------------------------------------------------
foreach(DLL ${GPU_OPTIONAL_DLLS})
    add_custom_command(TARGET TriDJs_Separador_Stems POST_BUILD
        COMMAND ${CMAKE_COMMAND}
            -DSRC="${DLL}"
            -DDST="${OUT_DIR}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/copy_if_exists.cmake"
        COMMENT "  [GPU DLL] Copiando ${DLL} (opcional)"
    )
endforeach()

message(STATUS "")
message(STATUS "=== TriDJs Stems – Estratégia de DLLs ===")
message(STATUS "  CPU (obrigatórias): ${CPU_REQUIRED_DLLS}")
message(STATUS "  GPU (opcionais):    ${GPU_OPTIONAL_DLLS}")
message(STATUS "==========================================")
