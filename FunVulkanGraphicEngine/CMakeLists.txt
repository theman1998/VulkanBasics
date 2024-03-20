set(PROJECT_NAME FunVulkanGraphicEngine)

set(PROJECT_SOURCES
    "main.cpp"
    "GraphicEngine/GraphicsCorePIMPL.cpp"

    "GraphicEngine/GraphicsDevice.cpp"
    "GraphicEngine/GraphicsPipeline.cpp"
    "GraphicEngine/GraphicsPipelineE2E.cpp"
    "GraphicEngine/GraphicsQueue.cpp"
    "GraphicEngine/GraphicsSwapchain.cpp"
    "GraphicEngine/GraphicsObjectController.cpp"
    "GraphicEngine/GraphicsVertex.cpp"
    "GraphicEngine/PipelinesIdMapping.cpp"
    "GraphicEngine/Validation.cpp"

    "GraphicEngine/Utility/DeviceSupport.cpp"
    "GraphicEngine/Utility/MemorySupport.cpp"

    "source/object/ThingBase.cpp"
    "source/object/ThingManager.cpp"
    "source/object/Camera.cpp"

    "source/GraphicsCore.cpp"
    "source/input/InputBase.cpp"

)

set(PROJECT_HEADERS
    "GraphicEngine/ConstDefines.hpp"
    "GraphicEngine/Validation.hpp"
    "GraphicEngine/GraphicsCorePIMPL.hpp"
    "GraphicEngine/GraphicsDevice.hpp"
    "GraphicEngine/GraphicsQueue.hpp"
    "GraphicEngine/GraphicsPipeline.hpp"
    "GraphicEngine/GraphicsPipelineE2E.hpp"
    "GraphicEngine/GraphicsSwapchain.hpp"
    "GraphicEngine/GraphicsObjectController.hpp"
    "GraphicEngine/GraphicsVertex.hpp"
    "GraphicEngine/ThingManagerPIMPL.hpp"
    "GraphicEngine/PipelinesIdMapping.hpp"

    "GraphicEngine/Utility/DeviceSupport.hpp"
    "GraphicEngine/Utility/MemorySupport.hpp"

    "include/StandardInclude.hpp"
    "include/GraphicsCore.hpp"

    "include/object/ThingBase.hpp"
    "include/object/ThingManager.hpp"
    "include/object/Camera.hpp"
    "include/object/UID.hpp"
    "include/input/InputBase.hpp"
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
    #target_compile_options( ${PROJECT_NAME} PUBLIC "/Zc:preprocessor")
endif()



include_directories(
    "."
)

target_include_directories( ${PROJECT_NAME} PUBLIC 
 "${GLM_PATH}"
 "${GLFW_PATH}/include"
 "${VULKAN_PATH}/include"
 "${STB_PATH}"
 "${TINYOBJECTLOADER_PATH}"
)

target_link_directories( ${PROJECT_NAME} PUBLIC 
 "${GLFW_PATH}/lib-vc2019"
 "${VULKAN_PATH}/lib" 
)

target_link_libraries(${PROJECT_NAME} PUBLIC 
	"glfw3"
    "vulkan-1"
)
