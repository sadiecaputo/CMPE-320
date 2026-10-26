cmake_minimum_required(VERSION 3.20)
project(cpp_backend)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    Crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG v1.2.1
)

FetchContent_MakeAvailable(Crow)

add_executable(server main.cpp supabase.cpp classes.cpp resources.cpp)
find_package(CURL CONFIG REQUIRED)
target_link_libraries(server PRIVATE Crow::Crow CURL::libcurl)