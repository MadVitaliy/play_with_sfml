cmake_minimum_required (VERSION 3.21)
project ("sfml_test")




# enable FetchContent
include(FetchContent)
# declare SFML
FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.6.x
)
# make SFML available


FetchContent_MakeAvailable(SFML)

FetchContent_GetProperties(SFML)

message(BUILD_SHARED_LIBS: ${BUILD_SHARED_LIBS})

message(SFML_SOURCE_DIR: ${SFML_SOURCE_DIR})
message(SFML_BINARY_DIR: ${SFML_BINARY_DIR})

#if(NOT SFML_POPULATED)
#  # Fetch the content using previously declared details
#  FetchContent_Populate(SFML)
#
#  # Set custom variables, policies, etc.
#  # ...
#
#  # Bring the populated content into the build
#  add_subdirectory(${SFML_SOURCE_DIR} ${SFML_BINARY_DIR})
#endif()

add_executable (sfml_test "sfml_test.cpp" "sfml_test.h")
target_link_libraries(sfml_test PRIVATE sfml-graphics)
target_compile_features(sfml_test PRIVATE cxx_std_17)

if (WIN32 AND BUILD_SHARED_LIBS)
    add_custom_command(TARGET sfml_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:sfml_test> $<TARGET_FILE_DIR:sfml_test> COMMAND_EXPAND_LISTS)
endif()


