cmake_minimum_required(VERSION 3.14)
project(assignment)

set(CMAKE_CXX_STANDARD 14)

find_package(DevIL REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(GLUT REQUIRED)

add_executable(cosc422-assignment-1-mjs351-bezier
        model.h shader.h util.h

        bezier.cpp)

add_executable(cosc422-assignment-1-mjs351-terrain
        model.h shader.h texture.h util.h

        terrain.cpp)

target_link_libraries(cosc422-assignment-1-mjs351-bezier ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES} ${GLUT_LIBRARIES} ${IL_LIBRARIES})

target_link_libraries(cosc422-assignment-1-mjs351-terrain  ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES} ${GLUT_LIBRARIES} ${IL_LIBRARIES})