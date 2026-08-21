cmake_minimum_required(VERSION 3.16)

project(VisionLab VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 源码统一按 UTF-8 解析（仓库内的简体中文注释依赖此开关）。
if(MSVC)
    add_compile_options(/utf-8)
endif()

# OpenCV 查找：通过以下任一方式提供 OpenCVConfig.cmake 所在目录：
#   -DOpenCV_DIR=...            （推荐，会被缓存）
#   OpenCV_DIR 环境变量，或 CMAKE_PREFIX_PATH 条目。
set(OpenCV_DIR "" CACHE PATH
    "Directory containing OpenCVConfig.cmake (e.g. <opencv>/build/x64/vc16/lib)")

if(WIN32 AND NOT MSVC)
    message(FATAL_ERROR
        "The prebuilt OpenCV for Windows (build/x64/vc16) is an MSVC build and "
        "cannot be linked from MinGW. Use an MSVC kit (e.g. Desktop Qt 6.x "
        "MSVC2022 64bit), or point OpenCV_DIR to a MinGW-built OpenCV and "
        "remove this guard.")
endif()

find_package(OpenCV QUIET)
if(NOT OpenCV_FOUND)
    message(FATAL_ERROR
        "OpenCV was not found.\n"
        "Pass the directory containing OpenCVConfig.cmake, e.g.:\n"
        "  cmake -B build -DOpenCV_DIR=<opencv>/build/x64/vc16/lib")
endif()

# ONNX Runtime CPU：官方 zip 解压后的根目录（含 include/ 与 lib/）。
#   -DOnnxRuntime_DIR=...     （推荐，会被缓存）
#   OnnxRuntime_DIR 环境变量。
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(OnnxRuntime_DIR "" CACHE PATH
    "ONNX Runtime root containing include/ and lib/ (official CPU zip, 1.17+)")
find_package(OnnxRuntime QUIET)
if(NOT OnnxRuntime_FOUND)
    message(FATAL_ERROR
        "ONNX Runtime was not found.\n"
        "Pass the extracted official CPU package root, e.g.:\n"
        "  cmake -B build -DOnnxRuntime_DIR=<onnxruntime-win-x64>")
endif()

# Qt
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Quick
    Qml
)

qt_standard_project_setup(REQUIRES 6.8)

# 纯 C++ 领域层：不依赖 Qt / QML。纯头文件静态库；
# core 不链接 Qt 这一事实本身就是 core/QML 隔离规则的构建期保证。
add_library(visionlab_core STATIC
    core/VisionTypes.h
    core/FramePacket.h
    core/Detection.h
    core/PipelineStats.h
    core/LatencyWindow.h
    core/BoundedQueue.h
    core/LatestResult.h
    core/PresentedFrame.h
    detectors/IDetector.h
    video/IVideoSource.h
    inference/InferenceTypes.h
    inference/ModelConfig.h
    inference/IInferenceEngine.h
)
target_include_directories(visionlab_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(visionlab_core PUBLIC ${OpenCV_LIBS})
set_target_properties(visionlab_core PROPERTIES LINKER_LANGUAGE CXX)

# ONNX Runtime CPU 后端：私有链接 ORT，不链接 Qt。YoloDetector 经工厂注入此引擎。
add_library(visionlab_inference STATIC
    inference/OnnxRuntimeEngine.h
    inference/OnnxRuntimeEngine.cpp
    inference/OnnxRuntimeCudaEngine.h
    inference/OnnxRuntimeCudaEngine.cpp
    inference/InferenceEngineFactory.h
    inference/InferenceEngineFactory.cpp
)
target_include_directories(visionlab_inference PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(visionlab_inference PUBLIC visionlab_core PRIVATE OnnxRuntime::OnnxRuntime)

# Windows System32 自带旧版 onnxruntime.dll，会抢在 PATH 之前加载。
# 拷到构建目录后，测试与 app 可执行文件与 DLL 同目录。
add_custom_command(TARGET visionlab_inference POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:OnnxRuntime::OnnxRuntime>"
        "${CMAKE_BINARY_DIR}")
if(EXISTS "${ONNXRUNTIME_BIN_DIR}/onnxruntime_providers_shared.dll")
    add_custom_command(TARGET visionlab_inference POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ONNXRUNTIME_BIN_DIR}/onnxruntime_providers_shared.dll"
            "${CMAKE_BINARY_DIR}")
endif()
if(EXISTS "${ONNXRUNTIME_BIN_DIR}/onnxruntime_providers_cuda.dll")
    add_custom_command(TARGET visionlab_inference POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ONNXRUNTIME_BIN_DIR}/onnxruntime_providers_cuda.dll"
            "${CMAKE_BINARY_DIR}")
endif()

# 渲染层：Detection 标注的唯一写入方。不依赖 Qt / QML。
add_library(visionlab_rendering STATIC
    rendering/DetectionRenderer.h
    rendering/DetectionRenderer.cpp
)
target_link_libraries(visionlab_rendering PUBLIC visionlab_core)

# 检测器实现：基于 IDetector 的纯 C++ 算法，不依赖 Qt / QML。
add_library(visionlab_detectors STATIC
    detectors/YoloDecoding.h
    detectors/YoloDecoding.cpp
    detectors/YoloPreprocess.h
    detectors/YoloPreprocess.cpp
    detectors/YoloDetector.h
    detectors/YoloDetector.cpp
    detectors/SsdDecoding.h
    detectors/SsdDecoding.cpp
    detectors/FaceDetector.h
    detectors/FaceDetector.cpp
    detectors/MotionDetector.h
    detectors/MotionDetector.cpp
    detectors/DetectorFactory.h
    detectors/DetectorFactory.cpp
)
target_link_libraries(visionlab_detectors PUBLIC visionlab_core visionlab_inference)

# 视频采集：IVideoSource 实现。不依赖 Qt / QML，不引入线程。
add_library(visionlab_video STATIC
    video/CameraSource.h
    video/CameraSource.cpp
)
target_link_libraries(visionlab_video PUBLIC visionlab_core)

# 实时管线工作线程：纯 C++，不链接 Qt。
add_library(visionlab_pipeline STATIC
    pipeline/CaptureWorker.h
    pipeline/CaptureWorker.cpp
    pipeline/InferenceWorker.h
    pipeline/InferenceWorker.cpp
    pipeline/StatsProbe.h
    pipeline/StatsProbe.cpp
    pipeline/VisionPipeline.h
    pipeline/VisionPipeline.cpp
)
target_link_libraries(visionlab_pipeline PUBLIC visionlab_core visionlab_rendering)
target_include_directories(visionlab_pipeline PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_executable(appVisionLab
    main.cpp
    app.rc
)

set(VISIONLAB_YOLO_ONNX_RESOURCE)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/models/yolov4-tiny.onnx")
    set(VISIONLAB_YOLO_ONNX_RESOURCE models/yolov4-tiny.onnx)
endif()

set_source_files_properties(views/I18n.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(appVisionLab
    URI VisionLab
    VERSION 1.0

    QML_FILES
        views/Main.qml
        views/TitleBar.qml
        views/ControlButton.qml
        views/CameraView.qml
        views/I18n.qml

    SOURCES
        controllers/WindowController.h controllers/WindowController.cpp
        controllers/VisionController.h controllers/VisionController.cpp
        controllers/LocaleController.h controllers/LocaleController.cpp
        utilities/CameraManager.h utilities/CameraManager.cpp
        utilities/CameraImageProvider.h utilities/CameraImageProvider.cpp

    RESOURCES
        assets/logo.png
        assets/logo.ico
        models/coco.names
        models/res10_300x300_ssd_iter_140000.caffemodel
        models/deploy.prototxt
        ${VISIONLAB_YOLO_ONNX_RESOURCE}
)

target_link_libraries(appVisionLab PRIVATE
    visionlab_core
    visionlab_rendering
    visionlab_detectors
    visionlab_video
    visionlab_pipeline
    Qt6::Core
    Qt6::Widgets
    Qt6::Quick
    Qt6::Qml
    ${OpenCV_LIBS}
)

set_target_properties(appVisionLab PROPERTIES
    WIN32_EXECUTABLE TRUE
)

# CPU 推理基准：走 OnnxRuntimeEngine::infer，不进默认 ctest 计时。
add_executable(bench_onnx_cpu benchmarks/OnnxCpuBenchmark.cpp)
target_link_libraries(bench_onnx_cpu PRIVATE visionlab_inference)
target_compile_definitions(bench_onnx_cpu PRIVATE
    "VISIONLAB_BUILD_TYPE=\"$<CONFIG>\""
    "VISIONLAB_DEFAULT_MODEL=\"${CMAKE_SOURCE_DIR}/models/yolov4-tiny.onnx\"")
add_custom_command(TARGET bench_onnx_cpu POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:OnnxRuntime::OnnxRuntime>"
        "$<TARGET_FILE_DIR:bench_onnx_cpu>")
if(EXISTS "${ONNXRUNTIME_BIN_DIR}/onnxruntime_providers_shared.dll")
    add_custom_command(TARGET bench_onnx_cpu POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ONNXRUNTIME_BIN_DIR}/onnxruntime_providers_shared.dll"
            "$<TARGET_FILE_DIR:bench_onnx_cpu>")
endif()

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
