set(TARGET_NAME jpg2heic)

file(GLOB_RECURSE HEADER_FILES "*.h")
file(GLOB_RECURSE HPP_FILES "*.hpp")
file(GLOB_RECURSE CPP_FILES "*.cpp")


source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${CPP_HEAD_FILES} ${CPP_SOUCE_FILES})

add_executable(${TARGET_NAME} ${HEADER_FILES} ${HPP_FILES} ${CPP_FILES})
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER Apps)
target_link_libraries(${TARGET_NAME} PRIVATE
JPGToHeic::JPGToHeic
)
target_include_directories(
${TARGET_NAME}
PUBLIC
${CMAKE_SOURCE_DIR}/Modules
)
