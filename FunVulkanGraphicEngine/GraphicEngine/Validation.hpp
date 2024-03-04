#pragma once


#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "public/StandardInclude.hpp"


namespace GE{

// For using the validation layer
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

void outputAllValidationSupport();
bool checkValidationLayerSupport();

// Callbacks
// VKAPI_ATTR & VKAPI_CALL required as to match vulkan signatures. 
VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, // Specifies servirity of the message. Find them here https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Validation_layers
    VkDebugUtilsMessageTypeFlagsEXT messageType, // if something wrong, violation, or non-optimal use
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, // contains pMessage, pObjects, & objectCount. pMessage=nullEndString, pObject=VulkanObjectHandles
    void* pUserData); // User define callback to pass own data to it

// used to look up the address of the VkDebugUtilsMessengerEXT object
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);


}