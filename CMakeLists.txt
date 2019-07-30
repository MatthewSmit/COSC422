cmake_minimum_required(VERSION 3.14)
project(assignment)

set(CMAKE_CXX_STANDARD 14)

add_executable(assignment
        main.cpp)

find_package(DevIL REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(GLUT REQUIRED)

target_link_libraries(assignment ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES} ${GLUT_LIBRARIES} ${IL_LIBRARIES})