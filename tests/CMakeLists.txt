find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(tst_smoke SmokeTest.cpp)
target_link_libraries(tst_smoke PRIVATE Qt6::Core Qt6::Test)
add_test(NAME smoke COMMAND tst_smoke)

add_executable(tst_framepacket FramePacketTest.cpp)
target_link_libraries(tst_framepacket PRIVATE visionlab_core Qt6::Core Qt6::Test)
add_test(NAME framepacket COMMAND tst_framepacket)

add_executable(tst_idetector IDetectorTest.cpp)
target_link_libraries(tst_idetector PRIVATE visionlab_core Qt6::Core Qt6::Test)
target_include_directories(tst_idetector PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME idetector COMMAND tst_idetector)

add_executable(tst_detectionrenderer DetectionRendererTest.cpp)
target_link_libraries(tst_detectionrenderer PRIVATE visionlab_rendering Qt6::Core Qt6::Test)
add_test(NAME detectionrenderer COMMAND tst_detectionrenderer)

add_executable(tst_yolodecoding YoloDecodingTest.cpp)
target_link_libraries(tst_yolodecoding PRIVATE visionlab_detectors Qt6::Core Qt6::Test)
add_test(NAME yolodecoding COMMAND tst_yolodecoding)

add_executable(tst_ssddecoding SsdDecodingTest.cpp)
target_link_libraries(tst_ssddecoding PRIVATE visionlab_detectors Qt6::Core Qt6::Test)
add_test(NAME ssddecoding COMMAND tst_ssddecoding)

add_executable(tst_motiondetector MotionDetectorTest.cpp)
target_link_libraries(tst_motiondetector PRIVATE visionlab_detectors Qt6::Core Qt6::Test)
add_test(NAME motiondetector COMMAND tst_motiondetector)

add_executable(tst_ivideosource IVideoSourceTest.cpp)
target_link_libraries(tst_ivideosource PRIVATE visionlab_core Qt6::Core Qt6::Test)
target_include_directories(tst_ivideosource PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME ivideosource COMMAND tst_ivideosource)

add_executable(tst_camerasource CameraSourceTest.cpp)
target_link_libraries(tst_camerasource PRIVATE visionlab_video Qt6::Core Qt6::Test)
add_test(NAME camerasource COMMAND tst_camerasource)

add_executable(tst_modemapping ModeMappingTest.cpp)
target_link_libraries(tst_modemapping PRIVATE visionlab_core Qt6::Core Qt6::Test)
add_test(NAME modemapping COMMAND tst_modemapping)

add_executable(tst_boundedqueue BoundedQueueTest.cpp)
target_link_libraries(tst_boundedqueue PRIVATE visionlab_core Qt6::Core Qt6::Test)
add_test(NAME boundedqueue COMMAND tst_boundedqueue)

add_executable(tst_latestresult LatestResultTest.cpp)
target_link_libraries(tst_latestresult PRIVATE visionlab_core Qt6::Core Qt6::Test)
add_test(NAME latestresult COMMAND tst_latestresult)

add_executable(tst_captureworker CaptureWorkerTest.cpp)
target_link_libraries(tst_captureworker PRIVATE visionlab_pipeline Qt6::Core Qt6::Test)
target_include_directories(tst_captureworker PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME captureworker COMMAND tst_captureworker)

add_executable(tst_inferenceworker InferenceWorkerTest.cpp)
target_link_libraries(tst_inferenceworker PRIVATE visionlab_pipeline Qt6::Core Qt6::Test)
target_include_directories(tst_inferenceworker PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME inferenceworker COMMAND tst_inferenceworker)

add_executable(tst_latencywindow LatencyWindowTest.cpp)
target_link_libraries(tst_latencywindow PRIVATE visionlab_core Qt6::Core Qt6::Test)
add_test(NAME latencywindow COMMAND tst_latencywindow)

add_executable(tst_statsprobe StatsProbeTest.cpp)
target_link_libraries(tst_statsprobe PRIVATE visionlab_pipeline Qt6::Core Qt6::Test)
add_test(NAME statsprobe COMMAND tst_statsprobe)

add_executable(tst_visionpipeline VisionPipelineTest.cpp)
target_link_libraries(tst_visionpipeline PRIVATE visionlab_pipeline Qt6::Core Qt6::Test)
target_include_directories(tst_visionpipeline PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME visionpipeline COMMAND tst_visionpipeline)

if(WIN32)
    # 让测试可执行文件在 ctest 下能解析 Qt 与 OpenCV 的 DLL。
    # 预编译 OpenCV 标准目录布局：<root>/build/x64/<toolset>/{lib,bin}
    get_filename_component(VISIONLAB_OPENCV_BIN "${OpenCV_DIR}/../bin" ABSOLUTE)
    set(visionlab_tests
        smoke framepacket idetector detectionrenderer
        yolodecoding ssddecoding motiondetector ivideosource
        camerasource modemapping boundedqueue latestresult captureworker
        inferenceworker latencywindow statsprobe visionpipeline)
    set_tests_properties(${visionlab_tests} PROPERTIES
        ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>\;${VISIONLAB_OPENCV_BIN}\;$ENV{PATH}")
endif()
