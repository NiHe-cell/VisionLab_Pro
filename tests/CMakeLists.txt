find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(tst_smoke SmokeTest.cpp)
target_link_libraries(tst_smoke PRIVATE Qt6::Core Qt6::Test)
add_test(NAME smoke COMMAND tst_smoke)

add_executable(tst_framepacket FramePacketTest.cpp)
target_link_libraries(tst_framepacket PRIVATE visionlab_core Qt6::Core Qt6::Test)
add_test(NAME framepacket COMMAND tst_framepacket)

add_executable(tst_modelconfig ModelConfigTest.cpp)
target_link_libraries(tst_modelconfig PRIVATE visionlab_core Qt6::Core Qt6::Test)
add_test(NAME modelconfig COMMAND tst_modelconfig)

add_executable(tst_iinferenceengine IInferenceEngineTest.cpp)
target_link_libraries(tst_iinferenceengine PRIVATE visionlab_core Qt6::Core Qt6::Test)
target_include_directories(tst_iinferenceengine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME iinferenceengine COMMAND tst_iinferenceengine)

add_executable(tst_onnxruntimeengine OnnxRuntimeEngineTest.cpp)
target_link_libraries(tst_onnxruntimeengine PRIVATE visionlab_inference Qt6::Core Qt6::Test)
target_compile_definitions(tst_onnxruntimeengine PRIVATE
    VISIONLAB_TEST_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
    VISIONLAB_MODELS_DIR="${CMAKE_SOURCE_DIR}/models")
add_test(NAME onnxruntimeengine COMMAND tst_onnxruntimeengine)
# Windows 的 System32 自带旧版 onnxruntime.dll（当前机是 1.17），会抢在 PATH 之前加载。
# 把与头文件匹配的 DLL 拷到测试可执行文件旁，保证加载 1.19+。
add_custom_command(TARGET tst_onnxruntimeengine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:OnnxRuntime::OnnxRuntime>"
        "$<TARGET_FILE_DIR:tst_onnxruntimeengine>")
if(EXISTS "${ONNXRUNTIME_BIN_DIR}/onnxruntime_providers_shared.dll")
    add_custom_command(TARGET tst_onnxruntimeengine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ONNXRUNTIME_BIN_DIR}/onnxruntime_providers_shared.dll"
            "$<TARGET_FILE_DIR:tst_onnxruntimeengine>")
endif()

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

add_executable(tst_yolopreprocess YoloPreprocessTest.cpp)
target_link_libraries(tst_yolopreprocess PRIVATE visionlab_detectors Qt6::Core Qt6::Test)
add_test(NAME yolopreprocess COMMAND tst_yolopreprocess)

add_executable(tst_yolodetector YoloDetectorTest.cpp)
target_link_libraries(tst_yolodetector PRIVATE visionlab_detectors Qt6::Core Qt6::Test)
target_include_directories(tst_yolodetector PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME yolodetector COMMAND tst_yolodetector)

add_executable(tst_detectorfactory DetectorFactoryTest.cpp)
target_link_libraries(tst_detectorfactory PRIVATE visionlab_detectors Qt6::Core Qt6::Test)
target_compile_definitions(tst_detectorfactory PRIVATE
    VISIONLAB_TEST_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")
add_test(NAME detectorfactory COMMAND tst_detectorfactory)
add_custom_command(TARGET tst_detectorfactory POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:OnnxRuntime::OnnxRuntime>"
        "$<TARGET_FILE_DIR:tst_detectorfactory>")
if(EXISTS "${ONNXRUNTIME_BIN_DIR}/onnxruntime_providers_shared.dll")
    add_custom_command(TARGET tst_detectorfactory POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ONNXRUNTIME_BIN_DIR}/onnxruntime_providers_shared.dll"
            "$<TARGET_FILE_DIR:tst_detectorfactory>")
endif()

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

add_executable(tst_pipelineoverload PipelineOverloadTest.cpp)
target_link_libraries(tst_pipelineoverload PRIVATE visionlab_pipeline Qt6::Core Qt6::Test)
target_include_directories(tst_pipelineoverload PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME pipelineoverload COMMAND tst_pipelineoverload)

add_executable(tst_pipelinelifecycle PipelineLifecycleTest.cpp)
target_link_libraries(tst_pipelinelifecycle PRIVATE visionlab_pipeline Qt6::Core Qt6::Test)
target_include_directories(tst_pipelinelifecycle PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME pipelinelifecycle COMMAND tst_pipelinelifecycle)

add_executable(tst_cameramanager
    CameraManagerTest.cpp
    ${CMAKE_SOURCE_DIR}/utilities/CameraManager.cpp
    ${CMAKE_SOURCE_DIR}/utilities/CameraManager.h)
target_link_libraries(tst_cameramanager PRIVATE
    visionlab_pipeline
    visionlab_detectors
    visionlab_video
    Qt6::Core
    Qt6::Gui
    Qt6::Test)
target_include_directories(tst_cameramanager PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})
add_test(NAME cameramanager COMMAND tst_cameramanager)

add_executable(tst_localecontroller
    LocaleControllerTest.cpp
    ${CMAKE_SOURCE_DIR}/controllers/LocaleController.cpp
    ${CMAKE_SOURCE_DIR}/controllers/LocaleController.h)
target_link_libraries(tst_localecontroller PRIVATE Qt6::Core Qt6::Test)
target_include_directories(tst_localecontroller PRIVATE ${CMAKE_SOURCE_DIR})
add_test(NAME localecontroller COMMAND tst_localecontroller)

# 只断言缺模型退出码与 identity 夹具输出字段，不把真实耗时当门槛。
file(GENERATE
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/OnnxCpuBenchmarkPaths.h"
    CONTENT "#pragma once
#define BENCH_ONNX_CPU R\"path($<TARGET_FILE:bench_onnx_cpu>)path\"
#define VISIONLAB_IDENTITY_ONNX R\"path(${CMAKE_CURRENT_SOURCE_DIR}/fixtures/identity_f32_1x3x2x2.onnx)path\"
")
add_executable(tst_onnxcpubench OnnxCpuBenchmarkTest.cpp)
add_dependencies(tst_onnxcpubench bench_onnx_cpu)
target_link_libraries(tst_onnxcpubench PRIVATE Qt6::Core Qt6::Test)
target_include_directories(tst_onnxcpubench PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME onnxcpubench COMMAND tst_onnxcpubench)

if(WIN32)
    # 让测试可执行文件在 ctest 下能解析 Qt 与 OpenCV 的 DLL。
    # 预编译 OpenCV 标准目录布局：<root>/build/x64/<toolset>/{lib,bin}
    get_filename_component(VISIONLAB_OPENCV_BIN "${OpenCV_DIR}/../bin" ABSOLUTE)
    set(visionlab_tests
        smoke framepacket modelconfig iinferenceengine onnxruntimeengine idetector detectionrenderer
        yolodecoding yolopreprocess yolodetector detectorfactory ssddecoding motiondetector ivideosource
        camerasource modemapping boundedqueue latestresult captureworker
        inferenceworker latencywindow statsprobe visionpipeline
        pipelineoverload pipelinelifecycle cameramanager localecontroller
        onnxcpubench)
    set_tests_properties(${visionlab_tests} PROPERTIES
        ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>\;${VISIONLAB_OPENCV_BIN}\;${ONNXRUNTIME_BIN_DIR}\;$ENV{PATH}")
endif()
