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
    Sql
)

qt_standard_project_setup(REQUIRES 6.8)

# 纯 C++ 领域层：不依赖 Qt / QML。纯头文件静态库；
# core 不链接 Qt 这一事实本身就是 core/QML 隔离规则的构建期保证。
add_library(visionlab_core STATIC
    core/VisionTypes.h
    core/FramePacket.h
    core/Detection.h
    core/Track.h
    core/VisionEvent.h
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

# 跟踪契约：纯 C++，不链接 Qt。算法实现在后续任务加入本目标。
add_library(visionlab_tracking STATIC
    tracking/ITracker.h
    tracking/IouMatching.h
    tracking/IouMatching.cpp
    tracking/KalmanBoxFilter.h
    tracking/KalmanBoxFilter.cpp
    tracking/ByteTrackTracker.h
    tracking/ByteTrackTracker.cpp
)
target_include_directories(visionlab_tracking PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(visionlab_tracking PUBLIC visionlab_core)
set_target_properties(visionlab_tracking PROPERTIES LINKER_LANGUAGE CXX)

# 规则契约：纯 C++，不链接 Qt。算法实现在后续任务加入本目标。
add_library(visionlab_analytics STATIC
    analytics/IRule.h
    analytics/RuleGeometry.h
    analytics/RuleGeometry.cpp
    analytics/RoiIntrusionRule.h
    analytics/RoiIntrusionRule.cpp
    analytics/LineCrossingRule.h
    analytics/LineCrossingRule.cpp
    analytics/LoiteringRule.h
    analytics/LoiteringRule.cpp
    analytics/CountingRule.h
    analytics/CountingRule.cpp
    analytics/RuleSpec.h
    analytics/RuleFactory.h
    analytics/RuleFactory.cpp
    analytics/RuleEngine.h
    analytics/RuleEngine.cpp
)
target_include_directories(visionlab_analytics PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(visionlab_analytics PUBLIC visionlab_core)
set_target_properties(visionlab_analytics PROPERTIES LINKER_LANGUAGE CXX)

# 事件仓储：Qt Sql，不链 QML。连接只允许在拥有该实例的线程上使用。
add_library(visionlab_storage STATIC
    storage/EventQuery.h
    storage/IEventRepository.h
    storage/SqliteEventRepository.h
    storage/SqliteEventRepository.cpp
    storage/EventWriter.h
    storage/EventWriter.cpp
)
target_include_directories(visionlab_storage PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(visionlab_storage PUBLIC visionlab_core Qt6::Sql)
set_target_properties(visionlab_storage PROPERTIES LINKER_LANGUAGE CXX)

# 检测器插件契约：Qt Core 仅出现在 plugin/ 层，core 仍然不链接 Qt。
add_library(visionlab_plugin STATIC
    plugin/PluginMetadata.h
    plugin/DetectorCreateRequest.h
    plugin/IVisionPlugin.h
    plugin/PluginManager.h
    plugin/PluginManager.cpp
)
target_include_directories(visionlab_plugin PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(visionlab_plugin PUBLIC visionlab_core Qt6::Core)
set_target_properties(visionlab_plugin PROPERTIES LINKER_LANGUAGE CXX)

# TensorRT 10 + CUDA RAII：默认关闭，CPU 构建不需要 NVIDIA 工具链。
# 打开后必须同时找到 CUDA Toolkit 与 TensorRT 10，找不到就 configure 失败。
option(VISIONLAB_ENABLE_TENSORRT
    "Build CUDA RAII helpers and link TensorRT 10 (requires CUDA Toolkit + TensorRT)"
    OFF)
set(TensorRT_DIR "" CACHE PATH
    "TensorRT 10 root containing include/ and lib/ (or extracted wheel layout)")
set(CUDAToolkit_ROOT "" CACHE PATH
    "CUDA Toolkit root (include/cuda_runtime.h, lib, bin)")

if(VISIONLAB_ENABLE_TENSORRT)
    if(CUDAToolkit_ROOT)
        list(PREPEND CMAKE_PREFIX_PATH "${CUDAToolkit_ROOT}")
    endif()
    find_package(CUDAToolkit QUIET)
    if(NOT CUDAToolkit_FOUND)
        message(FATAL_ERROR
            "VISIONLAB_ENABLE_TENSORRT=ON but CUDA Toolkit was not found.\n"
            "Pass -DCUDAToolkit_ROOT=<toolkit-root> (include/cuda_runtime.h, lib, bin).\n"
            "Do not leave the switch ON without a complete toolchain.")
    endif()
    find_package(TensorRT QUIET)
    if(NOT TensorRT_FOUND)
        message(FATAL_ERROR
            "VISIONLAB_ENABLE_TENSORRT=ON but TensorRT 10 was not found.\n"
            "Pass -DTensorRT_DIR=<TensorRT-10-root> containing include/NvInfer.h and lib/ "
            "(nvinfer / nvonnxparser import libraries).\n"
            "Do not leave the switch ON without a complete toolchain.")
    endif()
    if(NOT TENSORRT_VERSION_MAJOR STREQUAL "10")
        message(FATAL_ERROR
            "VISIONLAB_ENABLE_TENSORRT requires TensorRT 10.x, found '${TENSORRT_VERSION}'.\n"
            "Pass -DTensorRT_DIR=<TensorRT-10-root>")
    endif()
endif()

# ONNX Runtime CPU 后端：私有链接 ORT，不链接 Qt。YoloDetector 经工厂注入此引擎。
add_library(visionlab_inference STATIC
    inference/OnnxRuntimeEngine.h
    inference/OnnxRuntimeEngine.cpp
    inference/OnnxRuntimeCudaEngine.h
    inference/OnnxRuntimeCudaEngine.cpp
    inference/InferenceEngineFactory.h
    inference/InferenceEngineFactory.cpp
    inference/InferenceSelection.h
)
target_include_directories(visionlab_inference PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(visionlab_inference PUBLIC visionlab_core PRIVATE OnnxRuntime::OnnxRuntime)

if(VISIONLAB_ENABLE_TENSORRT)
    target_sources(visionlab_inference PRIVATE
        inference/cuda/CudaStream.h
        inference/cuda/CudaStream.cpp
        inference/cuda/CudaDeviceBuffer.h
        inference/cuda/CudaDeviceBuffer.cpp
        inference/TensorRTEngine.h
        inference/TensorRTEngine.cpp
    )
    target_link_libraries(visionlab_inference PRIVATE
        CUDA::cudart
        TensorRT::nvinfer
        TensorRT::nvonnxparser
    )
    target_compile_definitions(visionlab_inference PUBLIC VISIONLAB_HAS_TENSORRT)
endif()

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
if(VISIONLAB_ENABLE_TENSORRT)
    add_custom_command(TARGET visionlab_inference POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:CUDA::cudart>"
            "${CMAKE_BINARY_DIR}")
    if(TARGET TensorRT::nvinfer)
        add_custom_command(TARGET visionlab_inference POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:TensorRT::nvinfer>"
                "${CMAKE_BINARY_DIR}")
    endif()
    if(TARGET TensorRT::nvonnxparser)
        add_custom_command(TARGET visionlab_inference POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:TensorRT::nvonnxparser>"
                "${CMAKE_BINARY_DIR}")
    endif()
endif()

# 渲染层：Detection / Track 标注的写入方。不依赖 Qt / QML。
add_library(visionlab_rendering STATIC
    rendering/DetectionRenderer.h
    rendering/DetectionRenderer.cpp
    rendering/TrackRenderer.h
    rendering/TrackRenderer.cpp
    rendering/Letterbox.h
    rendering/Letterbox.cpp
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
    detectors/DummyDetector.h
    detectors/DummyDetector.cpp
)
target_link_libraries(visionlab_detectors PUBLIC visionlab_core visionlab_inference)

function(visionlab_set_plugin_output target)
    set(_dir "${CMAKE_BINARY_DIR}/plugins")
    set_target_properties(${target} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${_dir}"
        RUNTIME_OUTPUT_DIRECTORY "${_dir}"
    )
    foreach(_cfg IN ITEMS DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
        set_target_properties(${target} PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY_${_cfg} "${_dir}"
            RUNTIME_OUTPUT_DIRECTORY_${_cfg} "${_dir}"
        )
    endforeach()
endfunction()

# 运行时检测器插件：MODULE，不链进 app。由 PluginManager 扫描加载。
qt_add_plugin(vision_dummy
    CLASS_NAME DummyVisionPlugin
    ${CMAKE_CURRENT_SOURCE_DIR}/plugins/dummy/DummyVisionPlugin.h
    ${CMAKE_CURRENT_SOURCE_DIR}/plugins/dummy/DummyVisionPlugin.cpp
)
visionlab_set_plugin_output(vision_dummy)
target_link_libraries(vision_dummy PRIVATE visionlab_detectors visionlab_plugin Qt6::Core)

qt_add_plugin(vision_motion
    CLASS_NAME MotionVisionPlugin
    ${CMAKE_CURRENT_SOURCE_DIR}/plugins/motion/MotionVisionPlugin.h
    ${CMAKE_CURRENT_SOURCE_DIR}/plugins/motion/MotionVisionPlugin.cpp
)
visionlab_set_plugin_output(vision_motion)
target_link_libraries(vision_motion PRIVATE visionlab_detectors visionlab_plugin Qt6::Core)

qt_add_plugin(vision_face
    CLASS_NAME FaceVisionPlugin
    ${CMAKE_CURRENT_SOURCE_DIR}/plugins/face/FaceVisionPlugin.h
    ${CMAKE_CURRENT_SOURCE_DIR}/plugins/face/FaceVisionPlugin.cpp
)
visionlab_set_plugin_output(vision_face)
target_link_libraries(vision_face PRIVATE visionlab_detectors visionlab_plugin Qt6::Core)

qt_add_plugin(vision_yolo
    CLASS_NAME YoloVisionPlugin
    ${CMAKE_CURRENT_SOURCE_DIR}/plugins/yolo/YoloVisionPlugin.h
    ${CMAKE_CURRENT_SOURCE_DIR}/plugins/yolo/YoloVisionPlugin.cpp
)
visionlab_set_plugin_output(vision_yolo)
target_link_libraries(vision_yolo PRIVATE visionlab_detectors visionlab_plugin Qt6::Core)

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
    pipeline/EventLog.h
    pipeline/EventLog.cpp
    pipeline/InferenceWorker.h
    pipeline/InferenceWorker.cpp
    pipeline/StatsProbe.h
    pipeline/StatsProbe.cpp
    pipeline/VisionPipeline.h
    pipeline/VisionPipeline.cpp
)
target_link_libraries(visionlab_pipeline PUBLIC
    visionlab_core
    visionlab_rendering
    visionlab_tracking
    visionlab_analytics
)
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
        views/SideNav.qml
        views/MonitorPage.qml
        views/EventsPage.qml
        views/PerformancePage.qml
        views/SettingsPage.qml
        views/RuleOverlay.qml

    SOURCES
        controllers/WindowController.h controllers/WindowController.cpp
        controllers/VisionController.h controllers/VisionController.cpp
        controllers/LocaleController.h controllers/LocaleController.cpp
        utilities/CameraManager.h utilities/CameraManager.cpp
        utilities/CameraImageProvider.h utilities/CameraImageProvider.cpp
        models/DetectionModel.h models/DetectionModel.cpp
        models/TrackModel.h models/TrackModel.cpp
        models/EventModel.h models/EventModel.cpp
        models/EventTypeFilterModel.h models/EventTypeFilterModel.cpp
        models/PerformanceModel.h models/PerformanceModel.cpp
        models/PluginModel.h models/PluginModel.cpp
        models/RuleModel.h models/RuleModel.cpp

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
    visionlab_plugin
    visionlab_inference
    visionlab_video
    visionlab_pipeline
    visionlab_storage
    visionlab_analytics
    Qt6::Core
    Qt6::Widgets
    Qt6::Quick
    Qt6::Qml
    ${OpenCV_LIBS}
)

set_target_properties(appVisionLab PROPERTIES
    WIN32_EXECUTABLE TRUE
)
add_dependencies(appVisionLab vision_dummy vision_motion vision_face vision_yolo)
add_custom_command(TARGET appVisionLab POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:appVisionLab>/plugins"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:vision_dummy>"
        "$<TARGET_FILE:vision_motion>"
        "$<TARGET_FILE:vision_face>"
        "$<TARGET_FILE:vision_yolo>"
        "$<TARGET_FILE_DIR:appVisionLab>/plugins"
)

# 推理基准：经 createInferenceEngine，ORT CPU / ORT CUDA / TensorRT。
# 四次独立进程对比，勿在单进程连跑四种后端。不进默认 ctest 计时。
add_executable(bench_onnx_cpu benchmarks/OnnxCpuBenchmark.cpp)
add_executable(bench_inference ALIAS bench_onnx_cpu)
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
if(EXISTS "${ONNXRUNTIME_BIN_DIR}/onnxruntime_providers_cuda.dll")
    add_custom_command(TARGET bench_onnx_cpu POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ONNXRUNTIME_BIN_DIR}/onnxruntime_providers_cuda.dll"
            "$<TARGET_FILE_DIR:bench_onnx_cpu>")
endif()
if(VISIONLAB_ENABLE_TENSORRT)
    target_link_libraries(bench_onnx_cpu PRIVATE CUDA::cudart)
    add_custom_command(TARGET bench_onnx_cpu POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:CUDA::cudart>"
            "$<TARGET_FILE_DIR:bench_onnx_cpu>")
    if(TARGET TensorRT::nvinfer)
        add_custom_command(TARGET bench_onnx_cpu POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:TensorRT::nvinfer>"
                "$<TARGET_FILE_DIR:bench_onnx_cpu>")
    endif()
    if(TARGET TensorRT::nvonnxparser)
        add_custom_command(TARGET bench_onnx_cpu POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:TensorRT::nvonnxparser>"
                "$<TARGET_FILE_DIR:bench_onnx_cpu>")
    endif()
endif()

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
