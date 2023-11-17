#pragma once


#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>


#include <set>
#include <string>

#include "configs/VValidation.h"
#include "graphic/VQueues.h"
#include "graphic/VSwapChain.h"

namespace V
{


const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME // used to enable the swap chain
};


bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface );
bool checkDeviceExtensionSupport(VkPhysicalDevice device);


std::vector<VkExtensionProperties> getExtensionPropertiesList(); //Gets the vector list of all usable extension properties

bool checkUsedInstancesAreValid(const char** glfwExtensions, uint32_t count); // Checks our grab properies and compares it to the all extensionPropertiesList

std::vector<const char*> getRequiredExtensions();


std::vector<char> readFile(const std::string& filename);













	
}