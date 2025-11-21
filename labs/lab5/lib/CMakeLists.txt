include_directories(memory_resource queue)
add_library(queueLib INTERFACE)
target_include_directories(queueLib INTERFACE queue)

add_library(memoryLib INTERFACE)
target_include_directories(memoryLib INTERFACE memory_resource)
