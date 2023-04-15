cmake_minimum_required (VERSION 3.21)
project ("sfml_test")

add_executable (sfml_test "sfml_test.cpp" "sfml_test.h")
set_property(TARGET sfml_test PROPERTY CXX_STANDARD 17)


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
#target_link_libraries(sfml_test PRIVATE )

message(BUILD_SHARED_LIBS: ${BUILD_SHARED_LIBS})
