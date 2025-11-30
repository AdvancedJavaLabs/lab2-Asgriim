cmake_minimum_required(VERSION 3.29)

project(parl-lab2)

set(CMAKE_CXX_STANDARD 23)

include(FetchContent)

FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 12.0.0
)
FetchContent_MakeAvailable(fmt)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)

set(AMQP-CPP_LINUX_TCP ON)
FetchContent_Declare(
        amqpcpp
        GIT_REPOSITORY https://github.com/CopernicaMarketingSoftware/AMQP-CPP.git
        GIT_TAG v4.3.27
        PATCH_COMMAND ${amqpcpp_patch}
)
FetchContent_MakeAvailable(amqpcpp)

file(GLOB_RECURSE UTIL_HEADER "include/*.hpp")
file(GLOB_RECURSE UTIL_SOURCE "src/*.cpp")

add_library(lab2_util STATIC ${UTIL_HEADER} ${UTIL_SOURCE})
target_include_directories(lab2_util PUBLIC include/)
target_link_libraries(lab2_util PUBLIC fmt::fmt nlohmann_json::nlohmann_json)

add_subdirectory(bins)