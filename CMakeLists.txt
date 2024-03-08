cmake_minimum_required(VERSION 3.16)

# 设置基础环境
set(CMAKE_INCLUDE_CURRENT_DIR ON)
cmake_policy(SET CMP0100 NEW)

# 设置默认的编译配置为 Debug，conan.cmake 需要确定你当前正在使用哪种编译配置以决定如何下载和编译三方库
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
    message(STATUS "No CMAKE_BUILD_TYPE selected, defaulting to ${CMAKE_BUILD_TYPE}")
endif ()

project(JPGToHeic VERSION 0.1 LANGUAGES CXX)

include(CMakeModules/conan.cmake)
if (CONAN_PROFILE_BUILD AND CONAN_PROFILE_HOST)
conan_cmake_install(PATH_OR_REFERENCE .. BUILD missing
                    SETTINGS_HOST build_type=${CMAKE_BUILD_TYPE}
                    SETTINGS_BUILD build_type=${CMAKE_BUILD_TYPE}
                    PROFILE_BUILD ${CONAN_PROFILE_BUILD}
                    PROFILE_HOST ${CONAN_PROFILE_HOST})
                    # ENV_HOST CONAN_CMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
else ()
    conan_cmake_autodetect(settings)
    if(CONAN_REMOTE)
      conan_cmake_install(PATH_OR_REFERENCE .. BUILD missing SETTINGS ${settings} REMOTE ${CONAN_REMOTE})
    else()
      conan_cmake_install(PATH_OR_REFERENCE .. BUILD missing SETTINGS ${settings})
    endif()
endif ()

# 在 conan_cmake_install 执行后，会在 CMAKE_BINARY_DIR 目录下生成 cmake 的配置信息和 Qt 的依赖库路径信息，我们将其引入到 CMake 工程中，并调用 conan_basic_setup 来自动配置各个环境变量信息。
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
include(${CMAKE_BINARY_DIR}/conan_paths.cmake)
conan_basic_setup(TARGETS)


option(ENABLE_DOCS "Enable doxygen docs" ON)
option(ENABLE_DEBUG "Enable debug" OFF)
# just for qt
# set(CMAKE_AUTOUIC ON)
# set(CMAKE_AUTOMOC ON)
# set(CMAKE_AUTORCC ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_POLICY_DEFAULT_CMP0091 NEW)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_DEBUG_POSTFIX "d")
set(CMAKE_RELEASE_POSTFIX "") 
# your thirdparty cmake scripts
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)

include(CMakeModules/doxygen.cmake)
include(CMakeModules/dependency-graph.cmake)
include(CMakeModules/formatting.cmake)
# ---- Include guards ----
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
      "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

if(ENABLE_DEBUG)
  if(MSVC)
  # 使用 /O2 开启优化, /Zi 开启调试信息
  # 使用 /O2 开启优化, /Zi 开启调试信息
  set(CMAKE_CXX_FLAGS_RELEASE "/O2 /Zi /Ob1")

  # 在链接阶段生成调试信息
  set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG")
  else()
  # 对于其他编译器, 使用 -O2 开启优化, -g 开启调试信息
  set(CMAKE_CXX_FLAGS_RELEASE "-O2 -g")
  endif()
endif()


find_package(libheif REQUIRED CONFIG)
find_package(libjpeg-turbo REQUIRED CONFIG)
find_package(libsvtav1 REQUIRED CONFIG)
find_package(libx265 REQUIRED CONFIG)

add_subdirectory(Modules)
add_subdirectory(Apps)
add_subdirectory(Tests)

if(ENABLE_DOCS)
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../Docs)
  build_docs(PROCESS_DOXYFILE TRUE DOXYFILE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Docs/Doxyfile.in)
endif()
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT jpg2heic)

if(CLANG_FORMAT_EXE)
  message(STATUS "Found clang format ............ Configuring ..............")
  file(GLOB_RECURSE FFILES ${CMAKE_SOURCE_DIR}/Modules/*.[hc] ${CMAKE_SOURCE_DIR}/Modules/*.[hc]pp ${CMAKE_SOURCE_DIR}/Apps/*.[hc] ${CMAKE_SOURCE_DIR}/Apps/*.[hc]pp)
  clang_format(${PROJECT_NAME}Format ${FFILES})
  set_target_properties(${PROJECT_NAME}Format PROPERTIES FOLDER Tools)
endif()

if(DOT_EXE)
  message(STATUS "Found Graphviz ............ Configuring ..............")
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../Docs/DependencyGraph)
  gen_dep_graph(
    png 
    OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/../Docs/DependencyGraph
    TARGET_NAME ${PROJECT_NAME}DependencyGraph
  )
  set_target_properties(${PROJECT_NAME}DependencyGraph PROPERTIES FOLDER Tools)
endif()