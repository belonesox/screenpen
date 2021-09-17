####################################################################################################
# License: GPLv3+
#
# This file is a part of ROSA ScreenPen.
#
# ROSA ScreenPen is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 3,
# or (at your option) any later version.
#
# ROSA ScreenPen is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details
#
# You should have received a copy of the GNU General Public License
# along with ROSA ScreenPen; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
####################################################################################################

cmake_minimum_required(VERSION 2.8.9)

project(screenpen)

# Constants & options ##############################################################################
set(APP_NAME "screenpen" CACHE STRING "Short name of application (ASCII only)")
set(APP_DISP_NAME "ScreenPen" CACHE STRING "Translatable displayable name of application, name for users")
set(APP_VERSION "0.0.4" CACHE STRING "Version of application")
set(ENABLE_DEVEL FALSE CACHE BOOL "Enable build for development")

# Check out of source build ########################################################################
get_filename_component(src_dir_real_path "${CMAKE_SOURCE_DIR}" REALPATH)
get_filename_component(bin_dir_real_path "${CMAKE_BINARY_DIR}" REALPATH)

if(${src_dir_real_path} STREQUAL ${bin_dir_real_path})
    message(FATAL_ERROR "In-source builds are forbidden! Choose separate build directory.")
endif()

# Check required libraries #########################################################################
find_package(Qt5 5.1.0 REQUIRED COMPONENTS Core Gui Widgets OpenGL)

# Settings #########################################################################################
include_directories("${CMAKE_BINARY_DIR}")

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
if(ENABLE_DEVEL)
    message(STATUS "Development build: enabled")
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

# Sources ##########################################################################################
# Compile config.h
include(GNUInstallDirs)
#configure_file("config.in" "config.h")

include(sources.cmake)

# Compile moc files
qt5_wrap_cpp(sources_moc ${sources_h})

# Compile ui files
qt5_wrap_ui(sources_ui ${sources_ui})

# Target ###########################################################################################
add_executable(${PROJECT_NAME} ${sources_cpp} ${sources_moc} ${sources_ui})
set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-std=c++11")
#target_link_libraries(${PROJECT_NAME} X11)
qt5_use_modules(${PROJECT_NAME} Core Gui Widgets OpenGL)  # add Qt5 specific properties

# Install ##########################################################################################
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "bin")
