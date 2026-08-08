find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(tst_smoke SmokeTest.cpp)

target_link_libraries(tst_smoke PRIVATE Qt6::Core Qt6::Test)

add_test(NAME smoke COMMAND tst_smoke)

if(WIN32)
    # Let the test executable resolve Qt DLLs when run via ctest.
    set_tests_properties(smoke PROPERTIES
        ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>\;$ENV{PATH}")
endif()
