# FindOnnxRuntime.cmake
#
# Locates the official ONNX Runtime C++ package (CPU).
# Expected layout of the extracted GitHub zip (1.17+):
#   <root>/include/onnxruntime_cxx_api.h
#   <root>/lib/onnxruntime.lib   (Windows) or libonnxruntime.so
#   <root>/lib/onnxruntime.dll   or <root>/bin/onnxruntime.dll
#
# Cache/hint:
#   -DOnnxRuntime_DIR=<root>
#   or environment variable OnnxRuntime_DIR

set(OnnxRuntime_DIR "${OnnxRuntime_DIR}" CACHE PATH
    "ONNX Runtime root containing include/ and lib/")

find_path(ONNXRUNTIME_INCLUDE_DIR
    NAMES onnxruntime_cxx_api.h
    HINTS
        ${OnnxRuntime_DIR}
        ENV OnnxRuntime_DIR
    PATH_SUFFIXES include
)

find_library(ONNXRUNTIME_LIB
    NAMES onnxruntime
    HINTS
        ${OnnxRuntime_DIR}
        ENV OnnxRuntime_DIR
    PATH_SUFFIXES lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OnnxRuntime
    REQUIRED_VARS ONNXRUNTIME_INCLUDE_DIR ONNXRUNTIME_LIB
)

if(NOT OnnxRuntime_FOUND)
    return()
endif()

if(NOT TARGET OnnxRuntime::OnnxRuntime)
    add_library(OnnxRuntime::OnnxRuntime SHARED IMPORTED)

    set(_ort_implib "${ONNXRUNTIME_LIB}")
    get_filename_component(_ort_lib_dir "${ONNXRUNTIME_LIB}" DIRECTORY)

    find_file(ONNXRUNTIME_DLL
        NAMES onnxruntime.dll
        HINTS
            "${_ort_lib_dir}"
            "${_ort_lib_dir}/../bin"
            "${OnnxRuntime_DIR}/bin"
            "${OnnxRuntime_DIR}/lib"
        NO_DEFAULT_PATH
    )

    set_target_properties(OnnxRuntime::OnnxRuntime PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${ONNXRUNTIME_INCLUDE_DIR}"
        IMPORTED_IMPLIB "${_ort_implib}"
    )

    if(ONNXRUNTIME_DLL)
        set_target_properties(OnnxRuntime::OnnxRuntime PROPERTIES
            IMPORTED_LOCATION "${ONNXRUNTIME_DLL}")
        get_filename_component(ONNXRUNTIME_BIN_DIR "${ONNXRUNTIME_DLL}" DIRECTORY)
    else()
        set(ONNXRUNTIME_BIN_DIR "${_ort_lib_dir}")
        set_target_properties(OnnxRuntime::OnnxRuntime PROPERTIES
            IMPORTED_LOCATION "${_ort_implib}")
    endif()

    set(ONNXRUNTIME_BIN_DIR "${ONNXRUNTIME_BIN_DIR}" CACHE INTERNAL
        "Directory containing onnxruntime.dll for test PATH")
endif()
