# FindTensorRT.cmake
#
# Locates TensorRT 10.x (nvinfer + nvonnxparser).
# Expected layout:
#   <root>/include/NvInfer.h
#   <root>/lib/nvinfer.lib or nvinfer_10.lib
#   <root>/lib/nvonnxparser.lib or nvonnxparser_10.lib
#
# Cache/hint:
#   -DTensorRT_DIR=<root>
#   or environment variable TensorRT_DIR

set(TensorRT_DIR "${TensorRT_DIR}" CACHE PATH
    "TensorRT 10 root containing include/ and lib/")

find_path(TENSORRT_INCLUDE_DIR
    NAMES NvInfer.h
    HINTS
        ${TensorRT_DIR}
        ENV TensorRT_DIR
    PATH_SUFFIXES include
)

find_library(TENSORRT_NVINFER_LIB
    NAMES nvinfer_10 nvinfer
    HINTS
        ${TensorRT_DIR}
        ENV TensorRT_DIR
    PATH_SUFFIXES lib lib/x64 lib/x86_64-linux-gnu
)

find_library(TENSORRT_NVONNXPARSER_LIB
    NAMES nvonnxparser_10 nvonnxparser
    HINTS
        ${TensorRT_DIR}
        ENV TensorRT_DIR
    PATH_SUFFIXES lib lib/x64 lib/x86_64-linux-gnu
)

set(TENSORRT_VERSION "")
set(TENSORRT_VERSION_MAJOR "")

# TensorRT 10.16 headers sometimes define NV_TENSORRT_MAJOR as TRT_MAJOR_ENTERPRISE
# instead of a literal integer. Prefer a numeric #define, then the ENTERPRISE alias.
function(_visionlab_trt_read_int header symbol out_var)
    set(_value "")
    if(EXISTS "${header}")
        file(STRINGS "${header}" _line REGEX "^#define[ \t]+${symbol}[ \t]+[0-9]+")
        if(_line)
            list(GET _line 0 _first)
            string(REGEX REPLACE "^#define[ \t]+${symbol}[ \t]+([0-9]+).*" "\\1"
                _value "${_first}")
        endif()
    endif()
    set(${out_var} "${_value}" PARENT_SCOPE)
endfunction()

if(TENSORRT_INCLUDE_DIR AND EXISTS "${TENSORRT_INCLUDE_DIR}/NvInferVersion.h")
    set(_trt_ver_h "${TENSORRT_INCLUDE_DIR}/NvInferVersion.h")
    _visionlab_trt_read_int("${_trt_ver_h}" NV_TENSORRT_MAJOR TENSORRT_VERSION_MAJOR)
    _visionlab_trt_read_int("${_trt_ver_h}" NV_TENSORRT_MINOR _trt_minor)
    _visionlab_trt_read_int("${_trt_ver_h}" NV_TENSORRT_PATCH _trt_patch)
    if(NOT TENSORRT_VERSION_MAJOR MATCHES "^[0-9]+$")
        _visionlab_trt_read_int("${_trt_ver_h}" TRT_MAJOR_ENTERPRISE TENSORRT_VERSION_MAJOR)
        _visionlab_trt_read_int("${_trt_ver_h}" TRT_MINOR_ENTERPRISE _trt_minor)
        _visionlab_trt_read_int("${_trt_ver_h}" TRT_PATCH_ENTERPRISE _trt_patch)
    endif()
    if(NOT _trt_minor MATCHES "^[0-9]+$")
        set(_trt_minor "0")
    endif()
    if(NOT _trt_patch MATCHES "^[0-9]+$")
        set(_trt_patch "0")
    endif()
    if(TENSORRT_VERSION_MAJOR MATCHES "^[0-9]+$")
        set(TENSORRT_VERSION "${TENSORRT_VERSION_MAJOR}.${_trt_minor}.${_trt_patch}")
    endif()
endif()

if(TENSORRT_VERSION_MAJOR AND NOT TENSORRT_VERSION_MAJOR STREQUAL "10")
    message(STATUS
        "TensorRT ${TENSORRT_VERSION} was found, but VisionLab Pro V1 requires 10.x")
    set(TENSORRT_INCLUDE_DIR "TENSORRT_INCLUDE_DIR-NOTFOUND")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TensorRT
    REQUIRED_VARS TENSORRT_INCLUDE_DIR TENSORRT_NVINFER_LIB TENSORRT_NVONNXPARSER_LIB
    VERSION_VAR TENSORRT_VERSION
)

if(NOT TensorRT_FOUND)
    return()
endif()

function(_visionlab_import_trt_lib target_name implib)
    if(TARGET ${target_name})
        return()
    endif()
    add_library(${target_name} SHARED IMPORTED)
    get_filename_component(_lib_dir "${implib}" DIRECTORY)
    set(_dll_hints
        "${_lib_dir}"
        "${_lib_dir}/../bin"
        "${TensorRT_DIR}/bin"
        "${TensorRT_DIR}/lib"
        "${TensorRT_DIR}/tensorrt_libs"
    )
    get_filename_component(_lib_we "${implib}" NAME_WE)
    find_file(_dll
        NAMES "${_lib_we}.dll"
        HINTS ${_dll_hints}
        NO_DEFAULT_PATH
    )
    set_target_properties(${target_name} PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${TENSORRT_INCLUDE_DIR}"
        IMPORTED_IMPLIB "${implib}"
    )
    if(_dll)
        set_target_properties(${target_name} PROPERTIES IMPORTED_LOCATION "${_dll}")
        get_filename_component(TENSORRT_BIN_DIR "${_dll}" DIRECTORY)
        set(TENSORRT_BIN_DIR "${TENSORRT_BIN_DIR}" CACHE INTERNAL
            "Directory containing TensorRT DLLs for test PATH")
    else()
        set_target_properties(${target_name} PROPERTIES IMPORTED_LOCATION "${implib}")
    endif()
    unset(_dll CACHE)
endfunction()

_visionlab_import_trt_lib(TensorRT::nvinfer "${TENSORRT_NVINFER_LIB}")
_visionlab_import_trt_lib(TensorRT::nvonnxparser "${TENSORRT_NVONNXPARSER_LIB}")
