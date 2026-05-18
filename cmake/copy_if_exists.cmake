# copy_if_exists.cmake
# Uso: cmake -DSRC="caminho/da/dll" -DDST="pasta/destino" -P copy_if_exists.cmake
#
# Copia SRC para DST somente se o arquivo existir.
# NÃO falha o build se a DLL não for encontrada (útil para DLLs GPU opcionais).

if(EXISTS "${SRC}")
    get_filename_component(FILENAME "${SRC}" NAME)
    set(DEST_FILE "${DST}/${FILENAME}")

    # Só copia se o destino não existir ou for mais antigo que a origem
    if(NOT EXISTS "${DEST_FILE}")
        file(COPY_FILE "${SRC}" "${DEST_FILE}" RESULT COPY_RESULT)
        if(COPY_RESULT EQUAL 0)
            message(STATUS "  [GPU-OK ] ${FILENAME} copiada para ${DST}")
        else()
            message(STATUS "  [GPU-ERR] Falha ao copiar ${FILENAME}: ${COPY_RESULT}")
        endif()
    else()
        message(STATUS "  [GPU-SKP] ${FILENAME} já existe no destino (skip)")
    endif()
else()
    message(STATUS "  [GPU-N/A] ${SRC} não encontrada (cliente sem GPU NVIDIA – OK)")
endif()
