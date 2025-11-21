add_subdirectory(lib)

add_executable(queue_test tests/queue_test.cpp)
target_link_libraries(queue_test PRIVATE queueLib memoryLib gtest gtest_main)

add_executable(memory_test tests/memory_test.cpp)
target_link_libraries(memory_test PRIVATE memoryLib gtest gtest_main)
