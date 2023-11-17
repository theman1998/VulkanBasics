cmake_minimum_required(VERSION 3.5)
set(PROJECT_NAME WindowingApplication)

project(${PROJECT_NAME} VERSION 0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set( OUTPUT_BIN "${CMAKE_BINARY_DIR}/bin")

set(SFML_PATH "C:\\Code_Libraries\\SFML-2.6.0")
set(SFML_LIBS "sfml-system"  "sfml-window" "sfml-main" "sfml-graphics")
#set(SFML_LIBS "sfml-system-d"  "sfml-window-d" "sfml-main-d" "sfml-graphics-d")
#"sfml-window"


set(STB_PATH "C:\\Code_Libraries\\stb-master")



set(GLM_PATH "C:\\Code_Libraries\\glm-master")
set(GLFW_PATH "C:\\Code_Libraries\\glfw-3.3.7.WIN64")
set(VULKAN_PATH "C:\\Code_Libraries\\vulkan")


add_subdirectory(Window)
