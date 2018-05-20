cmake_minimum_required(VERSION 3.5)

# config project
project(sdgbc CXX)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# enable debug symbols if no build type is specified
if(CMAKE_BUILD_TYPE STREQUAL "")
  set(CMAKE_BUILD_TYPE Debug)
endif()

# define local include dir
include_directories(include)

# define executable sources
file(GLOB sdgbc_SOURCES
                  include/*.h
                  include/audio/*.h
                  include/debug/*.h
                  include/hw/*.h
                  include/hw/apu/*.h
                  include/hw/cart/*.h
                  include/hw/cpu/*.h
                  include/wxui/*.h
                  include/wxui/winrc/*.rc # windows resource files
                  include/wxui/xpm/*.h    # icon xpms
                  include/wxui/xpm/*.xpm

                  src/*.cpp
                  src/audio/*.cpp
                  src/debug/*.cpp
                  src/hw/*.cpp
                  src/hw/apu/*.cpp
                  src/hw/cart/*.cpp
                  src/hw/cpu/*.cpp
                  src/wxui/*.cpp
)

add_executable(sdgbc WIN32 ${sdgbc_SOURCES})

# macro defs for msvc to disable unsafe warnings from wxWidgets headers
if(MSVC)
  target_compile_definitions(sdgbc PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# setup our modules path
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

# if UNIX, find X11 and also find PkgConfig and use it to find GTK
if(UNIX)
  find_package(X11 REQUIRED)
  if(X11_FOUND)
    message(STATUS "X11 found")
    include_directories(${X11_INCLUDE_DIR})
    target_link_libraries(sdgbc ${X11_LIBRARIES})
  endif()

  find_package(PkgConfig REQUIRED)
  if(PkgConfig_FOUND)
    message(STATUS "PkgConfig found")

    pkg_check_modules(GTK REQUIRED gtk+-2.0)
    if(GTK_FOUND)
      message(STATUS "GTK found")
      include_directories(${GTK_INCLUDE_DIRS})
      link_directories(${GTK_LIBRARY_DIRS})
      add_definitions(${GTK_CFLAGS_OTHER})
      target_link_libraries(sdgbc ${GTK_LIBRARIES})
    endif()
  endif()
endif()

# find wxWidgets
find_package(wxWidgets 3.0 REQUIRED core base)
if(wxWidgets_FOUND)
  message(STATUS "wxWidgets found")
  include(${wxWidgets_USE_FILE})
  target_link_libraries(sdgbc ${wxWidgets_LIBRARIES})
endif()

# find SFML
# NOTE: any version above 2.3.1 causes the program to crash due to an X11
# incompatability for some reason
find_package(SFML 2.3.1 EXACT REQUIRED system window graphics audio)
if(SFML_FOUND)
  message(STATUS "SFML found")
  include_directories(${SFML_INCLUDE_DIR})
  target_link_libraries(sdgbc ${SFML_LIBRARIES})
endif()

# install target and extra pre-reqs
install(TARGETS sdgbc DESTINATION bin)
include(InstallRequiredSystemLibraries)
