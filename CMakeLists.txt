cmake_minimum_required(VERSION 3.16)

project(VisionLab VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# OpenCV
set(OpenCV_DIR "C:/opencv/build/x64/vc16/lib")
find_package(OpenCV REQUIRED)

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

target_include_directories(appVisionLab PRIVATE ${OpenCV_INCLUDE_DIRS})

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
