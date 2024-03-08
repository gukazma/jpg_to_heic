set(TARGET_NAME "JPGToHeic")

file(GLOB_RECURSE HEADER_FILES "*.h")
file(GLOB_RECURSE HPP_FILES "*.hpp")
file(GLOB_RECURSE CPP_FILES "*.cpp")

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${HEADER_FILES} ${HPP_FILES} ${CPP_FILES})

add_library(${TARGET_NAME} ${HEADER_FILES} ${CPP_FILES} ${HEADER_FILES} ${HPP_FILES})
add_library(JPGToHeic::JPGToHeic ALIAS ${TARGET_NAME})

set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD 17)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER Modules)

# being a cross-platform target, we enforce standards conformance on MSVC
target_compile_options(${TARGET_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive>")
target_compile_options(${TARGET_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/WX->")

# Link dependencies    
target_link_libraries(${TARGET_NAME} 
    PUBLIC
    libheif::heif
    libjpeg-turbo::jpeg-static
    libsvtav1::libsvtav1
    libx265::libx265
          )

target_include_directories(
  ${TARGET_NAME} 
  PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

set_target_properties(${TARGET_NAME}
    PROPERTIES
        FOLDER Modules/Common
)