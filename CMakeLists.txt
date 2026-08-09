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
    detectors/IDetector.h
    video/IVideoSource.h
)
target_include_directories(visionlab_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(visionlab_core PUBLIC ${OpenCV_LIBS})
set_target_properties(visionlab_core PROPERTIES LINKER_LANGUAGE CXX)

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
    detectors/SsdDecoding.h
    detectors/SsdDecoding.cpp
    detectors/ObjectDetector.h
    detectors/ObjectDetector.cpp
    detectors/FaceDetector.h
    detectors/FaceDetector.cpp
    detectors/MotionDetector.h
    detectors/MotionDetector.cpp
)
target_link_libraries(visionlab_detectors PUBLIC visionlab_core)

# 视频采集：IVideoSource 实现。不依赖 Qt / QML，不引入线程。
add_library(visionlab_video STATIC
    video/CameraSource.h
    video/CameraSource.cpp
)
target_link_libraries(visionlab_video PUBLIC visionlab_core)

qt_add_executable(appVisionLab
    main.cpp
    app.rc
)

qt_add_qml_module(appVisionLab
    URI VisionLab
    VERSION 1.0

    QML_FILES
        views/Main.qml
        views/TitleBar.qml
        views/ControlButton.qml
        views/CameraView.qml

    SOURCES
        controllers/WindowController.h controllers/WindowController.cpp
        controllers/VisionController.h controllers/VisionController.cpp
        utilities/CameraManager.h utilities/CameraManager.cpp
        utilities/CameraImageProvider.h utilities/CameraImageProvider.cpp

    RESOURCES
        assets/logo.png
        assets/logo.ico
        models/coco.names
        models/yolov4-tiny.cfg
        models/yolov4-tiny.weights
        models/res10_300x300_ssd_iter_140000.caffemodel
        models/deploy.prototxt
)

target_link_libraries(appVisionLab PRIVATE
    visionlab_core
    visionlab_rendering
    visionlab_detectors
    Qt6::Core
    Qt6::Widgets
    Qt6::Quick
    Qt6::Qml
    ${OpenCV_LIBS}
)

set_target_properties(appVisionLab PROPERTIES
    WIN32_EXECUTABLE TRUE
)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
