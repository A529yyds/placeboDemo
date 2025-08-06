cmake_minimum_required(VERSION 3.22.1 FATAL_ERROR)
project(demo VERSION 1.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra")
set(CMAKE_CXX_EXTENSIONS OFF)

#-flto=auto 链接时优化，提高链接速度，减小程序文件大小，提高运行性能
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto=auto")
set(ENV{PKG_CONFIG_PATH} "/usr/local/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}:/usr/lib/x86_64-linux-gnu:${CMAKE_CURRENT_SOURCE_DIR}/../src")
find_package(PkgConfig REQUIRED)
pkg_check_modules(placebo REQUIRED IMPORTED_TARGET "libplacebo>=4.3.0")
# pkg_check_modules(glfw REQUIRED IMPORTED_TARGET "glfw3>=3.3.6")
# pkg_check_modules(gl REQUIRED IMPORTED_TARGET "gl>=1.2")
# pkg_check_modules(x11 REQUIRED IMPORTED_TARGET "x11>=1.7.5")
pkg_check_modules(egl REQUIRED IMPORTED_TARGET "egl>=1.5")
# pkg_check_modules(ffmpeg REQUIRED IMPORTED_TARGET libavutil=58.29.100 libswresample=4.12.100 libavformat=60.16.100 libavdevice=60.3.100 libavfilter=9.12.100 libswscale=7.5.100 libavcodec=60.31.102)
pkg_check_modules(ffmpeg REQUIRED IMPORTED_TARGET libavutil=60.6.100 libswresample=6.0.100 libavformat=62.1.103 libavdevice=62.0.100 libavfilter=11.3.100 libswscale=9.0.100 libavcodec=62.8.100)


INCLUDE_DIRECTORIES("/usr/include") 
LINK_DIRECTORIES("/usr/lib")
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/head)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/src SOURCES)

add_executable(demo ${SOURCES})
# target_link_libraries(demo PkgConfig::placebo PkgConfig::ffmpeg  PkgConfig::glfw PkgConfig::gl PkgConfig::x11 PkgConfig::egl)
target_link_libraries(demo PkgConfig::placebo PkgConfig::ffmpeg PkgConfig::egl)