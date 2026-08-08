find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(tst_smoke SmokeTest.cpp)
target_link_libraries(tst_smoke PRIVATE Qt6::Core Qt6::Test)
add_test(NAME smoke COMMAND tst_smoke)

add_executable(tst_framepacket FramePacketTest.cpp)
target_link_libraries(tst_framepacket PRIVATE visionlab_core Qt6::Core Qt6::Test)
add_test(NAME framepacket COMMAND tst_framepacket)

if(WIN32)
    # Let test executables resolve Qt and OpenCV DLLs when run via ctest.
    # Standard prebuilt OpenCV layout: <root>/build/x64/<toolset>/{lib,bin}.
    get_filename_component(VISIONLAB_OPENCV_BIN "${OpenCV_DIR}/../bin" ABSOLUTE)
    set_tests_properties(smoke framepacket PROPERTIES
        ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>\;${VISIONLAB_OPENCV_BIN}\;$ENV{PATH}")
endif()
