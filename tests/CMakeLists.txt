find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(tst_smoke SmokeTest.cpp)
target_link_libraries(tst_smoke PRIVATE Qt6::Core Qt6::Test)
add_test(NAME smoke COMMAND tst_smoke)

add_executable(tst_framepacket FramePacketTest.cpp)
target_link_libraries(tst_framepacket PRIVATE visionlab_core Qt6::Core Qt6::Test)
add_test(NAME framepacket COMMAND tst_framepacket)

add_executable(tst_idetector IDetectorTest.cpp)
target_link_libraries(tst_idetector PRIVATE visionlab_core Qt6::Core Qt6::Test)
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
add_test(NAME ivideosource COMMAND tst_ivideosource)

add_executable(tst_camerasource CameraSourceTest.cpp)
target_link_libraries(tst_camerasource PRIVATE visionlab_video Qt6::Core Qt6::Test)
add_test(NAME camerasource COMMAND tst_camerasource)

add_executable(tst_modemapping ModeMappingTest.cpp)
target_link_libraries(tst_modemapping PRIVATE visionlab_core Qt6::Core Qt6::Test)
add_test(NAME modemapping COMMAND tst_modemapping)

if(WIN32)
    # 让测试可执行文件在 ctest 下能解析 Qt 与 OpenCV 的 DLL。
    # 预编译 OpenCV 标准目录布局：<root>/build/x64/<toolset>/{lib,bin}
    get_filename_component(VISIONLAB_OPENCV_BIN "${OpenCV_DIR}/../bin" ABSOLUTE)
    set(visionlab_tests
        smoke framepacket idetector detectionrenderer
        yolodecoding ssddecoding motiondetector ivideosource
        camerasource modemapping)
    set_tests_properties(${visionlab_tests} PROPERTIES
        ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>\;${VISIONLAB_OPENCV_BIN}\;$ENV{PATH}")
endif()
