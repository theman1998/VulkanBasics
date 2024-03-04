#include "GraphicEngine/GraphicsDevice.hpp"
#include "GraphicEngine/Validation.hpp"
#include "GraphicEngine/Utility/DeviceSupport.hpp"

#include <stdexcept>
#include <iostream>
#include <set>

namespace {
	// Callback will get trigger if the window gets resized
	void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
		auto app = reinterpret_cast<GE::DeviceWindow*>(glfwGetWindowUserPointer(window));
		app->triggerBufferResizeFlag();
	}
	// We need to add this to both the "debugging with Validation" and "VK instance". 
	// vkCreateDebugUtilsMessengerEXT requires an instance to be created first
	// But vkDestroyDebugUtilsMessengerEXT need to be called before the instance is destoyed
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT; // Documentation https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/PFN_vkDebugUtilsMessengerCallbackEXT.html
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;//Trigger with problems
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;// Trigger with any types
		createInfo.pfnUserCallback = GE::debugCallback;
	}
}

namespace GE {
	DeviceWindow::Area::Area(int w, int h) :width(w),height(h){}
	DeviceWindow::DeviceWindow():window(nullptr), type(TYPE::Undefined), frameBufferResizeFlag(false){}
	DeviceWindow::~DeviceWindow() { 
		if (window == nullptr)return;

		if (type == TYPE::GLFW)
		{
			glfwDestroyWindow(static_cast<GLFWwindow*>(window));
			glfwTerminate();
		}
	}
	DeviceWindow::TYPE DeviceWindow::getWindowType()const
	{
		return type;
	}
	void* DeviceWindow::operator()() { return window; }

	std::unique_ptr<DeviceWindow> DeviceWindow::CreateDeviceWindow(TYPE typeOfWindow, Area area)
	{
		if (typeOfWindow == TYPE::Undefined) { return std::unique_ptr<DeviceWindow>(nullptr); }
		std::unique_ptr<DeviceWindow> res = std::make_unique<DeviceWindow>();
		res->type = typeOfWindow;

		if (typeOfWindow == TYPE::GLFW)
		{
			glfwInit();
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // tell glfw to no use the openGL library since we are using vulkan
			glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // resizing takes special care. Do not do this for now.
			res->window = glfwCreateWindow(area.width, area.height, "Vulkan", nullptr, nullptr); // width height title of window. 4th parameter is for the screen. 5 is for openGL
			glfwSetWindowAttrib(static_cast<GLFWwindow*>(res->window), GLFW_RESIZABLE, GLFW_TRUE);
			glfwSetWindowUserPointer(static_cast<GLFWwindow*>(res->window), res.get());
			glfwSetFramebufferSizeCallback(static_cast<GLFWwindow*>(res->window), framebufferResizeCallback);
		}

		return res;
	}
	bool DeviceWindow::getBufferResizeFlag(bool doReset) 
	{ 
		bool state = frameBufferResizeFlag.load(); 
		if (doReset && state) { frameBufferResizeFlag.store(false); }
		return this->frameBufferResizeFlag; 
	}
	void DeviceWindow::triggerBufferResizeFlag(bool state) { frameBufferResizeFlag.store(state); }
	GLFWwindow* DeviceWindow::getGLFW() { return static_cast<GLFWwindow*>(window); }


	std::unique_ptr<GraphicDevice> GraphicDevice::CreateDevice(DeviceWindow& deviceWindow, const VkApplicationInfo& appInfo, std::vector<const char*> requiredExtensions)
	{
		if (deviceWindow.getWindowType() != DeviceWindow::TYPE::GLFW) { throw std::runtime_error("ERROR Only can handle GLFW WINDOW"); }

		std::unique_ptr<GraphicDevice> instance(new GraphicDevice);
		{
			VkInstanceCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			createInfo.pApplicationInfo = &appInfo;

			uint32_t glfwExtensionCount = static_cast<uint32_t>(requiredExtensions.size()); // same as right above but we use our custom function with vector
			const char** glfwExtensions = requiredExtensions.data();

			VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{}; //outside of if statement so it doesn't get destroyed
			// Update the vulkan create info structure
			createInfo.enabledExtensionCount = glfwExtensionCount;
			createInfo.ppEnabledExtensionNames = glfwExtensions;
			// Validation Layers -------------------------------------------------------------------------------------------------------
			createInfo.enabledLayerCount = 0; //validation layer
			if (enableValidationLayers) {

				createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
				createInfo.ppEnabledLayerNames = validationLayers.data();
				populateDebugMessengerCreateInfo(debugCreateInfo);
				createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
			}


			if (!Util::checkUsedInstancesAreValid(glfwExtensions, glfwExtensionCount))
			{
				throw std::runtime_error("glfwExtensions are not valid");
			}

			// Able to call the instance now after all the steps above are executed
			// General pattern vulkan object functions parameters follow. ( struct pointer, allocator callback, variable handle pointer )
			if (vkCreateInstance(&createInfo, nullptr, &instance->instance) != VK_SUCCESS) {
				throw std::runtime_error("failed to create instance!");
			}

			if (enableValidationLayers)
			{
				VkDebugUtilsMessengerCreateInfoEXT createInfo{}; // Need the VK struct like every other iteraction
				populateDebugMessengerCreateInfo(createInfo);

				// Fill in our internal object debugMessenger with vulkan defination create by this call
				if (CreateDebugUtilsMessengerEXT(instance->instance, &createInfo, nullptr, &instance->debugMessenger) != VK_SUCCESS) {
					throw std::runtime_error("failed to set up debug messenger!");
				}
			}
		}


		auto window = static_cast<GLFWwindow*>(deviceWindow());

		// Now using GLFW which uses an platform agnostic API for window and surfaces
		if (glfwCreateWindowSurface(instance->instance, window, nullptr, &instance->surface) != VK_SUCCESS) { throw std::runtime_error("failed to create window surface!"); }

		instance->physicalDevice = VK_NULL_HANDLE;
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance->instance, &deviceCount, nullptr); // Get number of devices
		std::cout << "device count " << deviceCount << std::endl;
		if (deviceCount == 0) { // no devices, we can't do anything
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}


		//--------------------------------------------------
		// Phyiscal Device Set up

		// allocate array to hold all device handles
		std::vector<VkPhysicalDevice> physicalDevices(deviceCount);//phiscal fda 
		vkEnumeratePhysicalDevices(instance->instance, &deviceCount, physicalDevices.data());

		// Find a suitable device meeting our requirments
		for (const auto& device : physicalDevices) {
			if (Util::isDeviceSuitable(device, instance->surface)) {
				instance->physicalDevice = device;
				instance->msaaSamples = [&](auto physicalDevice) {
					VkPhysicalDeviceProperties physicalDeviceProperties;
					vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
					VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
					if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
					if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
					if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
					if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
					if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
					if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }
					return VK_SAMPLE_COUNT_1_BIT;
					}(device);
					break;
			}
		}
		if (instance->physicalDevice == VK_NULL_HANDLE) { throw std::runtime_error("failed to find a suitable GPU!"); }




		Util::QueueFamilyIndices indices = Util::findQueueFamilies(instance->physicalDevice, instance->surface);
		if (!indices.isComplete()) { throw std::runtime_error( "Fail to find queue family for the current device"); }
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { *indices.graphicsFamily, *indices.presentFamily };

		// Queue priority (0.0 - 1.0 ) based on a float value system
		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}


		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.samplerAnisotropy = VK_TRUE;

		// CreatInfo requires the 2 structures above.
		VkDeviceCreateInfo createInfo{};
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

		createInfo.pEnabledFeatures = &deviceFeatures;

		// This extension resembles vkInstanceCreateInfo and is device specific. It requires extenstins and validation layers.
		// It posssible if a extension if activated that it might not be support for anothre device.
		// Do more research into this
		createInfo.enabledExtensionCount = static_cast<uint32_t>(Util::deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = Util::deviceExtensions.data();
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else {
			createInfo.enabledLayerCount = 0;
		}



		// Creation time. 
		// the nullptr arg is an optional callback
		if (vkCreateDevice(instance->physicalDevice, &createInfo, nullptr, &instance->device) != VK_SUCCESS) {
			throw std::runtime_error("failed to create logical device!");
		}

		// fill out our graphics queue witht the newly created logical device
		vkGetDeviceQueue(instance->device, indices.graphicsFamily.value(), 0, &instance->queues.graphicsQueue);
		vkGetDeviceQueue(instance->device, indices.presentFamily.value(), 0, &instance->queues.presentQueue);






		return instance;
	}

	VkApplicationInfo GraphicDevice::DefaultAppInfo()
	{
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO; //Must always explicity define this member
		appInfo.pApplicationName = "ApplicationName";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;
		return appInfo;
	}
	const std::vector<const char*>& GraphicDevice::DefaultExtensions()
	{
		static std::vector<const char*> list = Util::getRequiredExtensions();
		return list;
	}


	GraphicDevice::GraphicDevice() : instance(nullptr), debugMessenger(nullptr), physicalDevice(nullptr), device(device), surface(surface), msaaSamples(VK_SAMPLE_COUNT_1_BIT),queues(){}
	GraphicDevice::~GraphicDevice() {
		if (instance != nullptr && surface != nullptr) { vkDestroySurfaceKHR(instance, surface, nullptr); }
		if (device != nullptr) { vkDestroyDevice(device, nullptr); }
		if (debugMessenger != nullptr) { DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr); }
		if (instance != nullptr) { vkDestroyInstance(instance, nullptr); }
	}


	VkDevice GraphicDevice::operator()() const { return device; }



	Sync::~Sync()
	{
		if (device == nullptr)return;

		for (auto obj : imageAvailableSemaphores)vkDestroySemaphore(device, obj, nullptr);
		for (auto obj : renderFinishedSemaphores)vkDestroySemaphore(device, obj, nullptr);
		for (auto obj : inFlightFences)vkDestroyFence(device, obj, nullptr);
	}

	std::unique_ptr<Sync> Sync::CreateSync(VkDevice device)
	{
		if (device == nullptr) { throw std::runtime_error("Must enter a valid device object"); }
		std::unique_ptr<Sync> syncPtr(new Sync);
		auto& sync = *(syncPtr.get());
		sync.device = device;


		sync.imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		sync.renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		sync.inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // requires so we don't block indefinitely on vkWaitForFences for first instance

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &sync.imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(device, &semaphoreInfo, nullptr, &sync.renderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(device, &fenceInfo, nullptr, &sync.inFlightFences[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create semaphores!");
			}
		}

		return syncPtr;
	}


	GraphicDeviceGroup::GraphicDeviceGroup() : window{nullptr}, device(nullptr), sync(nullptr) {}
	GraphicDeviceGroup::~GraphicDeviceGroup() = default;
	GraphicDeviceGroup::GraphicDeviceGroup(GraphicDeviceGroup&&) = default;
	GraphicDeviceGroup& GraphicDeviceGroup::operator=(GraphicDeviceGroup&&) = default;


	GraphicDeviceGroup GraphicDeviceGroup::CreateBaseGroup()
	{
		GraphicDeviceGroup res;

		res.window = DeviceWindow::CreateDeviceWindow();
		res.device = GraphicDevice::CreateDevice(*res.window.get());

		res.sync = Sync::CreateSync(res.device->device);


		return res;
	}

}