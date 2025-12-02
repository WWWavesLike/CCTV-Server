# 최소 CMake 버전 설정
cmake_minimum_required(VERSION 3.25)

# 컴파일러 지정 (project() 호출 전에 설정)
# 원하는 컴파일러 경로로 수정하세요.
set(CMAKE_C_COMPILER "/usr/bin/clang")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
#set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 프로젝트 이름 및 버전 설정
project(receiver VERSION 1.0)

# C++ 표준 설정
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# C 표준 설정
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED True)

# 소스 파일 목록 설정 (예: main.cpp, main.c)
set(SOURCES
    main.cpp
    socket/communication.cpp
    socket/sql.cpp
    filehandler/filehandler.cpp
    gst/gst.cpp
)

find_path(MARIADB_INCLUDE_DIR
  NAMES conncpp.hpp
  HINTS /usr/include/mariadb
)

find_library(MARIADB_CPP_LIB
  NAMES mariadbcpp
  HINTS /usr/lib
)

find_package(PkgConfig REQUIRED)
find_package(Boost 1.66 REQUIRED COMPONENTS system)

pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(GST REQUIRED
    gstreamer-1.0
    gstreamer-base-1.0
    gstreamer-app-1.0
    gstreamer-video-1.0
    glib-2.0
)

# Boost의 include 디렉토리 추가
include_directories(
    ${GLIB_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
    ${GST_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include
)

link_directories(
    ${GLIB_LIBRARY_DIRS}
    ${GST_LIBRARY_DIRS}
)

# 실행 파일 생성
add_executable(${PROJECT_NAME} ${SOURCES})

add_definitions(
    ${GLIB_CFLAGS_OTHER}
    ${GST_CFLAGS_OTHER}
)
# 컴파일 옵션 추가 (PRIVATE 옵션은 해당 타겟에만 적용)
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall -Wextra -Werror
)

# 추가 include 디렉토리 설정 (필요시)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${GST_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    ${MARIADB_INCLUDE_DIRS}
)

# 라이브러리 연결 (필요시)
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${GLIB_LIBRARIES}
    ${Boost_LIBRARIES}
    ${MARIADB_CPP_LIB}
    ${GST_LIBRARIES}
)
message(STATUS "GLIB_INCLUDE_DIRS: ${GLIB_INCLUDE_DIRS}")
message(STATUS "GSTREAMER_INCLUDE_DIRS: ${GST_INCLUDE_DIRS}")
message(STATUS "GLIB_LIBRARY_DIRS: ${GLIB_LIBRARY_DIRS}")
message(STATUS "GST_LIBRARY_DIRS: ${GST_LIBRARY_DIRS}")
