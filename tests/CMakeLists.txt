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

function(visionlab_copy_ort_dlls target)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:OnnxRuntime::OnnxRuntime>"
            "$<TARGET_FILE_DIR:${target}>")
    if(EXISTS "${ONNXRUNTIME_BIN_DIR}/onnxruntime_providers_shared.dll")
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${ONNXRUNTIME_BIN_DIR}/onnxruntime_providers_shared.dll"
                "$<TARGET_FILE_DIR:${target}>")
    endif()
    if(EXISTS "${ONNXRUNTIME_BIN_DIR}/onnxruntime_providers_cuda.dll")
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${ONNXRUNTIME_BIN_DIR}/onnxruntime_providers_cuda.dll"
                "$<TARGET_FILE_DIR:${target}>")
    endif()
endfunction()

add_executable(tst_onnxruntimeengine OnnxRuntimeEngineTest.cpp)
target_link_libraries(tst_onnxruntimeengine PRIVATE visionlab_inference Qt6::Core Qt6::Test)
target_compile_definitions(tst_onnxruntimeengine PRIVATE
    VISIONLAB_TEST_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
    VISIONLAB_MODELS_DIR="${CMAKE_SOURCE_DIR}/models")
add_test(NAME onnxruntimeengine COMMAND tst_onnxruntimeengine)
# Windows 的 System32 自带旧版 onnxruntime.dll（当前机是 1.17），会抢在 PATH 之前加载。
# 把与头文件匹配的 DLL 拷到测试可执行文件旁，保证加载 1.19+。
visionlab_copy_ort_dlls(tst_onnxruntimeengine)

add_executable(tst_inferenceenginefactory InferenceEngineFactoryTest.cpp)
target_link_libraries(tst_inferenceenginefactory PRIVATE visionlab_inference Qt6::Core Qt6::Test)
target_compile_definitions(tst_inferenceenginefactory PRIVATE
    VISIONLAB_TEST_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")
add_test(NAME inferenceenginefactory COMMAND tst_inferenceenginefactory)
visionlab_copy_ort_dlls(tst_inferenceenginefactory)

add_executable(tst_onnxruntimecudaengine OnnxRuntimeCudaEngineTest.cpp)
target_link_libraries(tst_onnxruntimecudaengine PRIVATE visionlab_inference Qt6::Core Qt6::Test)
target_compile_definitions(tst_onnxruntimecudaengine PRIVATE
    VISIONLAB_TEST_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")
add_test(NAME onnxruntimecudaengine COMMAND tst_onnxruntimecudaengine)
visionlab_copy_ort_dlls(tst_onnxruntimecudaengine)

add_executable(tst_inferenceselection InferenceSelectionTest.cpp)
target_link_libraries(tst_inferenceselection PRIVATE visionlab_inference Qt6::Core Qt6::Test)
add_test(NAME inferenceselection COMMAND tst_inferenceselection)

add_executable(tst_pluginmetadata PluginMetadataTest.cpp)
target_link_libraries(tst_pluginmetadata PRIVATE visionlab_plugin Qt6::Core Qt6::Test)
add_test(NAME pluginmetadata COMMAND tst_pluginmetadata)

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

add_executable(tst_dummydetector DummyDetectorTest.cpp)
target_link_libraries(tst_dummydetector PRIVATE visionlab_detectors Qt6::Core Qt6::Test)
add_test(NAME dummydetector COMMAND tst_dummydetector)

add_executable(tst_dummyplugin DummyPluginTest.cpp)
target_link_libraries(tst_dummyplugin PRIVATE visionlab_plugin Qt6::Core Qt6::Test)
target_compile_definitions(tst_dummyplugin PRIVATE
    VISIONLAB_DUMMY_PLUGIN="$<TARGET_FILE:vision_dummy>")
add_dependencies(tst_dummyplugin vision_dummy)
add_test(NAME dummyplugin COMMAND tst_dummyplugin)
visionlab_copy_ort_dlls(tst_dummyplugin)

add_executable(tst_motionplugin MotionPluginTest.cpp)
target_link_libraries(tst_motionplugin PRIVATE visionlab_plugin Qt6::Core Qt6::Test)
target_compile_definitions(tst_motionplugin PRIVATE
    VISIONLAB_MOTION_PLUGIN="$<TARGET_FILE:vision_motion>")
add_dependencies(tst_motionplugin vision_motion)
add_test(NAME motionplugin COMMAND tst_motionplugin)
visionlab_copy_ort_dlls(tst_motionplugin)

add_executable(tst_faceplugin FacePluginTest.cpp)
target_link_libraries(tst_faceplugin PRIVATE visionlab_plugin Qt6::Core Qt6::Test)
target_compile_definitions(tst_faceplugin PRIVATE
    VISIONLAB_FACE_PLUGIN="$<TARGET_FILE:vision_face>"
    VISIONLAB_MODELS_DIR="${CMAKE_SOURCE_DIR}/models")
add_dependencies(tst_faceplugin vision_face)
add_test(NAME faceplugin COMMAND tst_faceplugin)
visionlab_copy_ort_dlls(tst_faceplugin)

qt_add_plugin(vision_fail_create
    CLASS_NAME FailCreatePlugin
    ${CMAKE_CURRENT_SOURCE_DIR}/plugins/fail_create/FailCreatePlugin.h
    ${CMAKE_CURRENT_SOURCE_DIR}/plugins/fail_create/FailCreatePlugin.cpp
)
target_link_libraries(vision_fail_create PRIVATE visionlab_plugin Qt6::Core)

qt_add_plugin(vision_wrong_iid
    CLASS_NAME WrongIidPlugin
    ${CMAKE_CURRENT_SOURCE_DIR}/plugins/wrong_iid/WrongIidPlugin.h
    ${CMAKE_CURRENT_SOURCE_DIR}/plugins/wrong_iid/WrongIidPlugin.cpp
)
target_link_libraries(vision_wrong_iid PRIVATE Qt6::Core)

add_executable(tst_pluginmanager PluginManagerTest.cpp)
target_link_libraries(tst_pluginmanager PRIVATE visionlab_plugin Qt6::Core Qt6::Test)
target_compile_definitions(tst_pluginmanager PRIVATE
    VISIONLAB_DUMMY_PLUGIN="$<TARGET_FILE:vision_dummy>"
    VISIONLAB_FAIL_CREATE_PLUGIN="$<TARGET_FILE:vision_fail_create>"
    VISIONLAB_WRONG_IID_PLUGIN="$<TARGET_FILE:vision_wrong_iid>")
add_dependencies(tst_pluginmanager vision_dummy vision_fail_create vision_wrong_iid)
add_test(NAME pluginmanager COMMAND tst_pluginmanager)
visionlab_copy_ort_dlls(tst_pluginmanager)

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

if(VISIONLAB_ENABLE_TENSORRT)
    add_executable(tst_tensorrtengine TensorRTEngineTest.cpp)
    target_link_libraries(tst_tensorrtengine PRIVATE
        visionlab_inference
        CUDA::cudart
        Qt6::Core
        Qt6::Test)
    target_compile_definitions(tst_tensorrtengine PRIVATE
        VISIONLAB_TEST_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
        VISIONLAB_MODELS_DIR="${CMAKE_SOURCE_DIR}/models")
    add_test(NAME tensorrtengine COMMAND tst_tensorrtengine)
    visionlab_copy_ort_dlls(tst_tensorrtengine)
    add_custom_command(TARGET tst_tensorrtengine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:CUDA::cudart>"
            "$<TARGET_FILE_DIR:tst_tensorrtengine>")
    if(TARGET TensorRT::nvinfer)
        add_custom_command(TARGET tst_tensorrtengine POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:TensorRT::nvinfer>"
                "$<TARGET_FILE_DIR:tst_tensorrtengine>")
    endif()
    if(TARGET TensorRT::nvonnxparser)
        add_custom_command(TARGET tst_tensorrtengine POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:TensorRT::nvonnxparser>"
                "$<TARGET_FILE_DIR:tst_tensorrtengine>")
    endif()

    add_executable(tst_tensorrtenginecache TensorRTEngineCacheTest.cpp)
    target_link_libraries(tst_tensorrtenginecache PRIVATE
        visionlab_inference
        CUDA::cudart
        Qt6::Core
        Qt6::Test)
    target_compile_definitions(tst_tensorrtenginecache PRIVATE
        VISIONLAB_TEST_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")
    add_test(NAME tensorrtenginecache COMMAND tst_tensorrtenginecache)
    visionlab_copy_ort_dlls(tst_tensorrtenginecache)
    add_custom_command(TARGET tst_tensorrtenginecache POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:CUDA::cudart>"
            "$<TARGET_FILE_DIR:tst_tensorrtenginecache>")
    if(TARGET TensorRT::nvinfer)
        add_custom_command(TARGET tst_tensorrtenginecache POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:TensorRT::nvinfer>"
                "$<TARGET_FILE_DIR:tst_tensorrtenginecache>")
    endif()
    if(TARGET TensorRT::nvonnxparser)
        add_custom_command(TARGET tst_tensorrtenginecache POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:TensorRT::nvonnxparser>"
                "$<TARGET_FILE_DIR:tst_tensorrtenginecache>")
    endif()

    add_executable(tst_cudaraii CudaRaiiTest.cpp)
    target_link_libraries(tst_cudaraii PRIVATE
        visionlab_inference
        CUDA::cudart
        Qt6::Core
        Qt6::Test)
    add_test(NAME cudaraii COMMAND tst_cudaraii)
    visionlab_copy_ort_dlls(tst_cudaraii)
    add_custom_command(TARGET tst_cudaraii POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:CUDA::cudart>"
            "$<TARGET_FILE_DIR:tst_cudaraii>")
    if(TARGET TensorRT::nvinfer)
        add_custom_command(TARGET tst_cudaraii POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:TensorRT::nvinfer>"
                "$<TARGET_FILE_DIR:tst_cudaraii>")
    endif()
    if(TARGET TensorRT::nvonnxparser)
        add_custom_command(TARGET tst_cudaraii POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:TensorRT::nvonnxparser>"
                "$<TARGET_FILE_DIR:tst_cudaraii>")
    endif()
endif()

if(WIN32)
    # 让测试可执行文件在 ctest 下能解析 Qt 与 OpenCV 的 DLL。
    # 预编译 OpenCV 标准目录布局：<root>/build/x64/<toolset>/{lib,bin}
    get_filename_component(VISIONLAB_OPENCV_BIN "${OpenCV_DIR}/../bin" ABSOLUTE)
    set(visionlab_tests
        smoke framepacket modelconfig iinferenceengine onnxruntimeengine inferenceenginefactory onnxruntimecudaengine inferenceselection pluginmetadata idetector detectionrenderer
        yolodecoding yolopreprocess yolodetector detectorfactory ssddecoding motiondetector dummydetector dummyplugin motionplugin faceplugin pluginmanager ivideosource
        camerasource modemapping boundedqueue latestresult captureworker
        inferenceworker latencywindow statsprobe visionpipeline
        pipelineoverload pipelinelifecycle cameramanager localecontroller
        onnxcpubench)
    if(VISIONLAB_ENABLE_TENSORRT)
        list(APPEND visionlab_tests cudaraii tensorrtengine tensorrtenginecache)
    endif()
    set(_visionlab_test_path
        "$<TARGET_FILE_DIR:Qt6::Core>\;${VISIONLAB_OPENCV_BIN}\;${ONNXRUNTIME_BIN_DIR}")
    if(VISIONLAB_ENABLE_TENSORRT)
        if(DEFINED CUDAToolkit_BIN_DIR AND CUDAToolkit_BIN_DIR)
            string(APPEND _visionlab_test_path "\;${CUDAToolkit_BIN_DIR}")
        endif()
        if(DEFINED TENSORRT_BIN_DIR AND TENSORRT_BIN_DIR)
            string(APPEND _visionlab_test_path "\;${TENSORRT_BIN_DIR}")
        endif()
    endif()
    set_tests_properties(${visionlab_tests} PROPERTIES
        ENVIRONMENT "PATH=${_visionlab_test_path}\;$ENV{PATH}")
endif()
