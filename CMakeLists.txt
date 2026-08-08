cmake_minimum_required(VERSION 3.16)

project(VisionLab VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# OpenCV discovery: provide the directory containing OpenCVConfig.cmake via
#   -DOpenCV_DIR=...            (preferred, cached)
#   OpenCV_DIR environment var, or a CMAKE_PREFIX_PATH entry.
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
        detectors/ObjectDetector.h detectors/ObjectDetector.cpp
        detectors/MotionDetector.h detectors/MotionDetector.cpp
        detectors/FaceDetector.h detectors/FaceDetector.cpp

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
