cmake_minimum_required(VERSION 3.10)

project(wasapi-cli-player)

set(CMAKE_CXX_STANDARD 17)

# Add the executable
add_executable(${PROJECT_NAME} src/main.cpp)

# Link necessary libraries (if any)
# target_link_libraries(${PROJECT_NAME} <libraries>)