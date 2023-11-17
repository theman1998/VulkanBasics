set(PROJECT_NAME VWindow)

set(PROJECT_SOURCES
    "main.cpp"
    "configs/VValidation.cpp"
    "graphic/VWindow.cpp"
    "graphic/GLFWHelpers.cpp"
    "graphic/VQueues.cpp"
    "graphic/VSwapChain.cpp"
    "graphic/VertexHelpers.cpp"
)

set(PROJECT_HEADERS
    "configs/VValidation.h"
    "graphic/VWindow.h"
    "graphic/GLFWHelpers.h"
    "graphic/VQueues.h"
    "graphic/VSwapChain.h"
    "graphic/VertexHelpers.h"
)


add_executable(${PROJECT_NAME}
    ${PROJECT_SOURCES}
    ${PROJECT_HEADERS}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_BIN}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_BIN}"
)


if ((MSVC) AND (MSVC_VERSION GREATER_EQUAL 1914))
    #target_compile_options( ${PROJECT_NAME} PUBLIC "/Zc:__cplusplus")
    #target_compile_options( ${PROJECT_NAME} PUBLIC /permissive-)
endif()



include_directories(
    "."
)

target_include_directories( ${PROJECT_NAME} PUBLIC 
 "${GLM_PATH}"
 "${GLFW_PATH}/include"
 "${VULKAN_PATH}/include"
 "${STB_PATH}"
)

target_link_directories( ${PROJECT_NAME} PUBLIC 
 "${GLFW_PATH}/lib-vc2019"
 "${VULKAN_PATH}/lib" 
)

target_link_libraries(${PROJECT_NAME} PUBLIC 
	"glfw3"
    "vulkan-1"
)
