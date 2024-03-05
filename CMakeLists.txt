cmake_minimum_required(VERSION 3.28)
project(
        SpineSimServer
        LANGUAGES CXX C
        DESCRIPTION "OpenGL Server"
        VERSION 0.0.1
)

set(CMAKE_CXX_STANDARD 14)

aux_source_directory(./src server_src)

file(GLOB server_header_files ./include/*.h)
file(GLOB server_proto_files ./protobuf/*.h)
file(GLOB ImGui_files_cpp ./include/ImGui/*.cpp)
file(GLOB ImGui_files_h ./include./ImGui/*.h)
file(GLOB pb_files ./protobuf/*.cc)

add_executable(SpineSimServer
        src/glad.c
        ${server_src}
        ${server_header_files}
        ${server_proto_files}
        ${ImGui_files_cpp}
        ${ImGui_files_h}
        ${pb_files}
)

link_directories(${CMAKE_SOURCE_DIR}/libs)

target_link_libraries(SpineSimServer
        "${CMAKE_SOURCE_DIR}/libs/assimp-vc143-mt.lib"
        "${CMAKE_SOURCE_DIR}/libs/ecal_core.lib"
        "${CMAKE_SOURCE_DIR}/libs/ecal_proto.lib"
        "${CMAKE_SOURCE_DIR}/libs/glfw3.lib"
        "${CMAKE_SOURCE_DIR}/libs/libprotobuf.lib"
)

target_include_directories(SpineSimServer PRIVATE include)
target_include_directories(SpineSimServer PRIVATE protobuf)
target_include_directories(SpineSimServer PRIVATE include/ImGui)
target_precompile_headers(SpineSimServer PUBLIC ./include/PCH.h)

