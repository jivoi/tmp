cmake_minimum_required(VERSION 2.8)


# Define project name as the folder name.
project(final)


if (UNIX)
    message(STATUS "UNIX")

    # Use C++ 11 standart.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

    # Use C 99 standart.
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

endif()


set (LIBS "-pthread")


aux_source_directory(src SRCS)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fpermissive")


add_executable(${PROJECT_NAME} ${SRCS})

target_link_libraries(${PROJECT_NAME} ${LIBS})

set_property(TARGET ${ProjectId} PROPERTY FOLDER "unit_final")
