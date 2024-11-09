cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(ObjectStorageProject)


set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(SOURCE_FILES mian.cpp)

find_package(Protobuf REQUIRED)

add_executable(object_storage ${SOURCE_FILES})
add_executable(MemoryTest  Memory.cpp)

add_executable(AnyDataTypeTest AnyDataType.cpp Data.pb.cc)
target_link_libraries(AnyDataTypeTest PRIVATE protobuf::libprotobuf)


find_package(Threads REQUIRED)
target_link_libraries(object_storage Threads::Threads)
