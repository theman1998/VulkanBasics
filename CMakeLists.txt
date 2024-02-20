cmake_minimum_required(VERSION 3.5)
set(PROJECT_NAME WindowingApplication)

project(${PROJECT_NAME} VERSION 0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set( OUTPUT_BIN "${CMAKE_BINARY_DIR}/bin")



set(STB_PATH "C:\\Libs\\stb")



set(GLM_PATH "C:\\Libs\\glm")
set(GLFW_PATH "C:\\Libs\\glfw")
set(VULKAN_PATH "C:\\Libs\\VulkanSDK")

set(TINYOBJECTLOADER_PATH "C:\\Libs\\tinyobjloader")

add_subdirectory(Window)
add_subdirectory(FunVulkanGraphicEngine)
