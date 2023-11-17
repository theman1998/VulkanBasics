#include "graphic/VWindow.h"
#include "configs/VValidation.h"
#include "graphic/GLFWHelpers.h"
#include "graphic/VertexHelpers.h"


#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Only 1 file should define the implemenation
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <chrono>

namespace V
{
	// Callback will get trigger if the window gets resized
	void Window::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
		auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
		app->framebufferResized = true;
	}

	// We need to add this to both the "debugging with Validation" and "VK instance". 
	// vkCreateDebugUtilsMessengerEXT requires an instance to be created first
	// But vkDestroyDebugUtilsMessengerEXT need to be called before the instance is destoyed
	void Window::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT; // Documentation https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/PFN_vkDebugUtilsMessengerCallbackEXT.html
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;//Trigger with problems
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;// Trigger with any types
		createInfo.pfnUserCallback = debugCallback;// Our define callback found in HelloValidation
	}



	Window::Window() = default;

	void Window::run()
	{
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}



	void Window::initWindow()
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // tell glfw to no use the openGL library since we are using vulkan
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // resizing takes special care. Do not do this for now.
		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr); // width height title of window. 4th parameter is for the screen. 5 is for openGL
		glfwSetWindowAttrib(window, GLFW_RESIZABLE, GLFW_TRUE);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	}
	void Window::initVulkan()
	{
		createInstance();
		setupDebugMessenger();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
		createImageViews();
		createRenderPass();
		createDescriptorSetLayout();
		createGraphicsPipeline();
		createFramebuffers();
		createCommandPool();
		createTextureImage();
		createTextureImageView();
		createTextureSampler();
		createVertexBuffer();
		createIndexBuffer();
		createUniformBuffers();
		createDescriptorPool();
		createDescriptorSets();
		createCommandBuffer();
		createSyncObjects();
	}
	void Window::mainLoop()
	{
		//Keeps the application runnig until error or window is closed
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
			drawFrame();
		}
		vkDeviceWaitIdle(vkDevice); // wait for the GPU to finish up. avoid complaints
	}
	void Window::cleanup()
	{

		cleanupSwapchain();

		vkDestroySampler(vkDevice, textureSampler, nullptr);
		vkDestroyImageView(vkDevice, textureImageView, nullptr);
		vkDestroyImage(vkDevice, textureImage, nullptr);
		vkFreeMemory(vkDevice, textureImageMemory, nullptr);

		vkDestroyPipeline(vkDevice, graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(vkDevice, pipelineLayout, nullptr);
		vkDestroyRenderPass(vkDevice, renderPass, nullptr);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroyBuffer(vkDevice, uniformBuffers[i], nullptr);
			vkFreeMemory(vkDevice, uniformBuffersMemory[i], nullptr);
		}

		vkDestroyDescriptorPool(vkDevice, descriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(vkDevice, descriptorSetLayout, nullptr);

		vkDestroyBuffer(vkDevice, indexBuffer, nullptr);
		vkFreeMemory(vkDevice, indexBufferMemory, nullptr);
		vkDestroyBuffer(vkDevice, vertexBuffer, nullptr);
		vkFreeMemory(vkDevice, vertexBufferMemory, nullptr);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroySemaphore(vkDevice, imageAvailableSemaphores[i], nullptr);
			vkDestroySemaphore(vkDevice, renderFinishedSemaphores[i], nullptr);
			vkDestroyFence(vkDevice, inFlightFences[i], nullptr);
		}
		vkDestroyCommandPool(vkDevice, commandPool, nullptr);
		vkDestroySurfaceKHR(instance, surface, nullptr);
		if (enableValidationLayers) {
			DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr); // destroy debug callback
		}
		vkDestroyDevice(vkDevice, nullptr);
		vkDestroyInstance(instance, nullptr);
		glfwDestroyWindow(window);
		glfwTerminate();
	}
	void Window::cleanupSwapchain()
	{
		for (auto framebuffer : swapChainFramebuffers) {
			vkDestroyFramebuffer(vkDevice, framebuffer, nullptr);
		}
		for (auto imageView : swapChainImageViews) {
			vkDestroyImageView(vkDevice, imageView, nullptr);
		}
		vkDestroySwapchainKHR(vkDevice, swapChain, nullptr);
	}


	void Window::createInstance()
	{
		if (enableValidationLayers && !checkValidationLayerSupport()) {
			throw std::runtime_error("validation layers requested, but not available!");
		}
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO; //Must always explicity define this member
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;
		// Another struct that is required to create an instance
		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		// --------------------------------------------------------------------------------------------------
		// Vulkan is a platform agnostic API, which means that you need an extension to interface with the window system.
		// Required to use an extension interface system. GLFW has build in functions for this
		//uint32_t glfwExtensionCount = 0;
		//const char** glfwExtensions;
		//glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount); //Gets the global extensions
		auto glfwVectorList = getRequiredExtensions();
		uint32_t glfwExtensionCount = static_cast<uint32_t>(glfwVectorList.size()); // same as right above but we use our custom function with vector
		const char** glfwExtensions = glfwVectorList.data();
		// -------------------------------------------------------------------------------------------------------
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
		// -------------------------------------------------------------------------------------------------------

		checkUsedInstancesAreValid(glfwExtensions, glfwExtensionCount);

		// Able to call the instance now after all the steps above are executed
		// General pattern vulkan object functions parameters follow. ( struct pointer, allocator callback, variable handle pointer )
		//VkResult result = vkCreateInstance(&createInfo, nullptr, &instance); // one way
		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
			throw std::runtime_error("failed to create instance!");
		}
	}
	void Window::setupDebugMessenger()
	{
		if (!enableValidationLayers) // We want it off
		{
			return;
		}
		VkDebugUtilsMessengerCreateInfoEXT createInfo{}; // Need the VK struct like every other iteraction
		populateDebugMessengerCreateInfo(createInfo);

		// Fill in our internal object debugMessenger with vulkan defination create by this call
		if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
			throw std::runtime_error("failed to set up debug messenger!");
		}
	}

	void Window::createSurface()
	{
		// Now using GLFW which uses an platform agnostic API for window and surfaces
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface!");
		}
	}

	void Window::pickPhysicalDevice()
	{
		physicalDevice = VK_NULL_HANDLE;
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr); // Get number of devices
		std::cout << "device count " << deviceCount << std::endl;
		if (deviceCount == 0) { // no devices, we can't do anything
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}

		// allocate array to hold all device handles
		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		// Find a suitable device meeting our requirments
		for (const auto& device : devices) {
			if (isDeviceSuitable(device, surface)) {
				physicalDevice = device;
				break;
			}
		}

		//Did not find it
		if (physicalDevice == VK_NULL_HANDLE) {
			throw std::runtime_error("failed to find a suitable GPU!");
		}

	}

	void Window::createLogicalDevice()
	{

		QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
		std::cout << "Creating logical device with: " << indices.toString() << std::endl;

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily };

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
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else {
			createInfo.enabledLayerCount = 0;
		}



		// Creation time. 
		// the nullptr arg is an optional callback
		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &vkDevice) != VK_SUCCESS) {
			throw std::runtime_error("failed to create logical device!");
		}

		// fill out our graphics queue witht the newly created logical device
		vkGetDeviceQueue(vkDevice, indices.graphicsFamily, 0, &graphicsQueue);
		vkGetDeviceQueue(vkDevice, indices.presentFamily, 0, &presentQueue);
	}
	void Window::createSwapChain()
	{
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice, surface);

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(window, swapChainSupport.capabilities);

		// recommended to request at least 1 over the min image count, for rendering reasons
		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

		// If out image count goes ovre maximum then we need to set it to max
		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		// Yes now we fill in large struct just like the other devices
		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1; // layers each image contain. use 1 unless you are 3d modleing
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // how images are loaded, we are rendering directly


		// If the phyiscal and logical devices differ we will need to speccify it, if not they both can share the same queue.
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily, indices.presentFamily };
		if (indices.graphicsFamily != indices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; //have to be exclusive as concurrent required 2 distinct queues
			createInfo.queueFamilyIndexCount = 0; // Optional
			createInfo.pQueueFamilyIndices = nullptr; // Optional
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform; // How to rotate images if the desire, but current will leave it as it is
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // If alpha should be used with blending with other windows, Currently being ignored
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE; // Pixels can be blocked by another window.
		createInfo.oldSwapchain = VK_NULL_HANDLE; // Topic for another discussion, but from what I read, if we decide to change swap change, we will need this.
		//createInfo.oldSwapchain = swapChain;

		if (vkCreateSwapchainKHR(vkDevice, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
			throw std::runtime_error("failed to create swap chain!");
		}

		// Like the other get functions found in vk, first to get the size, then second to get the structure
		vkGetSwapchainImagesKHR(vkDevice, swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(vkDevice, swapChain, &imageCount, swapChainImages.data());

		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;
	}


	void Window::createImageViews() {
		swapChainImageViews.resize(swapChainImages.size());

		for (size_t i = 0; i < swapChainImages.size(); i++) {
			swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat);
		}

	}

	VkImageView Window::createImageView(VkImage image, VkFormat format) {
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		VkImageView imageView;
		if (vkCreateImageView(vkDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture image view!");
		}

		return imageView;
	}


	void Window::createRenderPass()
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = swapChainImageFormat; // should match the format of the swap chain
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // no multisampling so keep at 1
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // what to do with attachment before rendering
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // what to do with attachment after rendering. want to see the image so we store it
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // used in the stencil buffer. we arn't using it currently
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		// more discussion on this in the textering chapter
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // specifies which layout the image wil have have before rendering
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // specifies the transition to when render pass finishes

		// Subpasses and attechment references
		// Used to reduce memory bandwidth and increase performance. Imagine filters on a snapchat pic
		// Will only use 1 subpass for the triangle
		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;


		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		// Subpass Dependencies.
		// Subpass it responsible for the image layout transitions
		// Re read this seconds. Blew right through it
		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(vkDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass!");
		}
	}

	void Window::createDescriptorSetLayout()
	{
		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1; // Can be represented as an array. allowing for skeleton movement

		// The type of stages that will be used in this stage. Can OR operation each bit
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		// Used with image sampling descriptors
		uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

		VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding = 1;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		// Typically done in the frag shader. But if dynamically deform a grid of vertices by a heightmap, than we should use the vertex shader
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();


		// Telling vulkan which shader we will be using
		if (vkCreateDescriptorSetLayout(vkDevice, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor set layout!");
		}

	}

	void Window::createGraphicsPipeline()
	{

		std::vector<char> vertShaderCode = readFile("shaders/vert.spv");
		std::vector<char> fragShaderCode = readFile("shaders/frag.spv");

		//Local variables because this happens after the pipeline is created.
		// Only needed within this call stack

		// Created the modules and stages for the vertexes and fragments filters
		VkShaderModule vertShaderModule = createShaderModule(vkDevice, vertShaderCode);
		VkShaderModule fragShaderModule = createShaderModule(vkDevice, fragShaderCode);
		VkPipelineShaderStageCreateInfo vertShaderStageInfo = createShaderStageInfoVertex(vertShaderModule);
		VkPipelineShaderStageCreateInfo fragShaderStageInfo = createShaderStageInfoFragment(fragShaderModule);
		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();


		// This struct describes the format that will be pass to the vertex shader in 2 ways
		// * bindings: space between data. per-vertex or per-instance
		// * atrribute descriptions: type of attriibutes passed to the vertex shader
		// Currently here for filler. Tutorial will go over this in the "vertex buffers" chapter
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription; // meta info for vertex buffering
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());;
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data(); // Meta info for Vertex buffering


		// This struct describes the geometry that will be drawn from the vertices and if primitive restart should be enabled.
		VkPipelineInputAssemblyStateCreateInfo inputAssembly = createBasicAssemblyInfo();


		//Viewport and scissors
		std::shared_ptr < ViewportContainer> viewportStateContainer = createBasicViewportState(&swapChainExtent);
		VkPipelineViewportStateCreateInfo& viewportState = viewportStateContainer->viewportState;


		// Takes geometry that is shaped by the vertices from the vertex shader and turns it into graments to be colored by the fragment shader
		VkPipelineRasterizationStateCreateInfo rasterizer = createBasicRasterizeInfo();
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // due to the flip we are doing within the y position in the matrixs

		// one way to perform anti-aliasing when combining the fragment shader and the rasterizer fragment polygon output.
		VkPipelineMultisampleStateCreateInfo multisampling = createBasicMultisamplingInfo();


		// Depth and stencil testing
		// Not needed. comeback to another chapter.
		// struct is VkPipelineDepthStencilStateCreateInfo


		// After fragment shader returns the color, it needs to be combine with the color that is already in the framebuffer.
		std::shared_ptr < ColorBlendContainer > colorBlendContainer = createBasicColorBlendState();
		VkPipelineColorBlendStateCreateInfo colorBlending = colorBlendContainer->info;


		// Dynamic state
		// limited amount of the states created above can't be changes without recreating the pipeline
		// Not sure if this is required as it is used to change some features during run time
		std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_LINE_WIDTH
		};
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();


		// represented in the global space
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = createBasicPipelineLayoutInfo();
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO; // Tell the pipeline about out shaders fd
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
		if (vkCreatePipelineLayout(vkDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}



		// Finaly. We can create the Graphic Pipeline !!!!!
		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = nullptr; // Optional
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = nullptr; // Optional
		pipelineInfo.layout = pipelineLayout; // fixed function stage
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0; // The index of the subpass
		// The following 2 will derive from an existing pipeline. save time.
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional


		if (vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline!");
		}

		// Will clean up at the end of the pipeline
		vkDestroyShaderModule(vkDevice, fragShaderModule, nullptr);
		vkDestroyShaderModule(vkDevice, vertShaderModule, nullptr);
	}
	void Window::createFramebuffers()
	{
		swapChainFramebuffers.resize(swapChainImageViews.size()); // The framebuffers will be from the swap chain the we prevously set up

		for (size_t i = 0; i < swapChainImageViews.size(); i++) {
			VkImageView attachments[] = {
				swapChainImageViews[i]
			};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(vkDevice, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create framebuffer!");
			}
		}
	}
	void Window::createCommandPool()
	{
		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Allow the buffer to be rerecoreded individually. Other commands can be found here
		// https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Command_buffers
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;

		if (vkCreateCommandPool(vkDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create command pool!");
		}
	}

	void Window::createTextureImage()
	{
		// Will be using command buffer to store our image object. The images can be found in shader/texures

		int texWidth, texHeight, texChannels;
		// Force the load to create alpha, so it consistant with all other texures
		stbi_uc* pixels = stbi_load("textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		// X by 4 because of pixel colors
		VkDeviceSize imageSize = texWidth * texHeight * 4;

		if (!pixels) {
			throw std::runtime_error("failed to load texture image!");
		}
		// Copying image data to our vk buffer
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
		void* data;
		vkMapMemory(vkDevice, stagingBufferMemory, 0, imageSize, 0, &data); // Going to allocate memory to persistant memory
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(vkDevice, stagingBufferMemory);

		stbi_image_free(pixels);

		
		createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, 
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

		// VK_IMAGE_LAYOUT_UNDEFINED used before that how we initilized the image. Don't care about contents tell we perform copy operation
		transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));// Moving the data down the pipeline
		transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(vkDevice, stagingBuffer, nullptr);
		vkFreeMemory(vkDevice, stagingBufferMemory, nullptr);

	}

	void Window::createTextureImageView() {

		textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB);
	}

	void Window::createTextureSampler() {
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		// specify how to interpolate texels that are magnified or minified
		// Magnified helps with the oversampling problem
		// Minufucation helps with the undersampling
		samplerInfo.magFilter = VK_FILTER_LINEAR; 
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		/// axes are called U,V,W instead of X,Y,Z
		// This field only relevant when masking outside of the image
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		// NEED TO Read More About this
		// anisotropic filtering should be used most of the time. Unless performance is an issue
		// Lower value is lower quality. Should get maxium value from our device
		// If device doesn't contain the capability, than we should make this false
		samplerInfo.anisotropyEnable = VK_TRUE;
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);
		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		// Returns the color if clamp to border addressing mode is active
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		// If true, than values will be in range of [0,texWidth]
		// If false, than values will range in [9,1]
		// Most application are in normalized coordinates. Reason can be textures of different resolutions.
		samplerInfo.unnormalizedCoordinates = VK_FALSE;

		// Enables filtering on shadow maps.
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

		// READ More about this. mipmapping
		// Another filter that can be applied. 
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		if (vkCreateSampler(vkDevice, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture sampler!");
		}
	}

	void Window::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
		
		// Extent defines the dimensinal the image is
		// 1D images store an array of data or gradient
		// 2D images usefull for texures
		// 3D store voxels volumes
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width; 
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1; // 1d as we our image uses array
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format; // Should be the same as the texels as with buffer
		imageInfo.tiling = tiling; // How the pixels are ordered. Use linear if going into buffer manually is desired. not in our case. The shader will be doing it.
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Not usable by GPU.
		imageInfo.usage = usage;// The image is copy is going directly to its desination
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT; //Used with Multi Sampling
		imageInfo.flags = 0; // Optional. Used with "Sparse Images"
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; //Only going to one queue family

		if (vkCreateImage(vkDevice, &imageInfo, nullptr, &image) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image!");
		}

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(vkDevice, image, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties); // ERROR MAYBE?
		// Image allocation is same as buffer allocation
		if (vkAllocateMemory(vkDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate image memory!");
		}

		vkBindImageMemory(vkDevice, image, imageMemory, 0);
	}

	void Window::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		// Could copy image data into bkCmdCopyBufferToImage. but we need to have image in the right layout
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		// Barries are used to synchronize access to resouces, like images. 
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout; // Can use VK_IMAGE_LAYOUT_UNDEFINED if we don't care about existing contents of image
		barrier.newLayout = newLayout;

		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; //used if we are transforing ownership of queues
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		// More information on the aforementioned table
		// https://registry.khronos.org/vulkan/specs/1.3-extensions/html/chap7.html#VkPipelineStageFlagBits
		// First stage within the transition
		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0; // Don't need to wait on anything
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; // Going to copy image data over to shader

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;//Not real graphic pipeline stage. Its a pseudo stage where the transfer is happings.
		}
		// Second stage of transision
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else {
			throw std::invalid_argument("unsupported layout transition!");
		}

		// 2nd param - defines pipeline stage the operation occurs before the barrier
		// 3rd param - pipeline stage in which to wait for barrier
		// 4th - 0 or VK_DEPENDENCY_BY_REGION_BIT. Allow to read part that were written.
		// 5th - reference to memory barriers
		// 6th - buffer memory barriers
		// 7th - image memory barriers
		vkCmdPipelineBarrier(
			commandBuffer,
			sourceStage, destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		endSingleTimeCommands(commandBuffer);

	}

	void Window::createVertexBuffer()
	{
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(vkDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), (size_t)bufferSize);
		vkUnmapMemory(vkDevice, stagingBufferMemory);

		// Reason for having this extra buffer, is so that we can load our vertex in more performant memory
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
		copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
		vkDestroyBuffer(vkDevice, stagingBuffer, nullptr);
		vkFreeMemory(vkDevice, stagingBufferMemory, nullptr);
	}
	void Window::createIndexBuffer()
	{
		VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(vkDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices.data(), (size_t)bufferSize);
		vkUnmapMemory(vkDevice, stagingBufferMemory);
		// Bit different from vertex buffer. Using VK_BUFFER_USAGE_INDEX_BUFFER_BIT instead of VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

		copyBuffer(stagingBuffer, indexBuffer, bufferSize);

		vkDestroyBuffer(vkDevice, stagingBuffer, nullptr);
		vkFreeMemory(vkDevice, stagingBufferMemory, nullptr);
	}

	void Window::createUniformBuffers()
	{
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);

		uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);//memory void here, so using vulkan memory mappping, we can make it dynamic

		// Technique "Persistent Mapping" which maps the buffer to specific pointer during the application lifetime
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);

			vkMapMemory(vkDevice, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
		}
	}

	void Window::createDescriptorPool()
	{
		// These descriptors are created every frame
		std::array<VkDescriptorPoolSize, 2> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);// No reason to allow create more than whats needed
		//VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT is an option to create the descriptors every frame

		if (vkCreateDescriptorPool(vkDevice, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
		}


	}

	void Window::createDescriptorSets()
	{
		// Creating only 1 descriptor set for each descrptor pool
		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocInfo.pSetLayouts = layouts.data();

		descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		if (vkAllocateDescriptorSets(vkDevice, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}


		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			// Defining out uniform buffer object with the descriptor
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = textureImageView;
			imageInfo.sampler = textureSampler;

			std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;// define configuration
			descriptorWrites[0].dstSet = descriptorSets[i];// address to descriptor
			descriptorWrites[0].dstBinding = 0;// Define first index of array. They can be arrays
			descriptorWrites[0].dstArrayElement = 0;// Not using an array so 0 will make it not treat is as suc
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;// Buffer data

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = descriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = &imageInfo;// Optional Image data

			vkUpdateDescriptorSets(vkDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}


	}

	void Window::createCommandBuffer()
	{
		commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // specifies if it's primary or secondary command buffers
		allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

		if (vkAllocateCommandBuffers(vkDevice, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}

	}
	void Window::createSyncObjects()
	{
		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // requires so we don't block indefinitely on vkWaitForFences for first instance

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			if (vkCreateSemaphore(vkDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(vkDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(vkDevice, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create semaphores!");
			}
		}
	}






	void Window::drawFrame() {
		// VK_True waits for all fances to be complete. Times out after max of int64
		vkWaitForFences(vkDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;

		// Checking if the state of our window changed.
		VkResult result = vkAcquireNextImageKHR(vkDevice, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		updateUniformBuffer(currentFrame);


		// Only reset the fence if we are submitting work
		vkResetFences(vkDevice, 1, &inFlightFences[currentFrame]); // reset it after we get the last statement


		vkResetCommandBuffer(commandBuffers[currentFrame], 0); // Do before recording command buffer. If not it will freeze
		recordCommandBuffer(commandBuffers[currentFrame], imageIndex);


		// Submitting the command buffer
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[currentFrame]; // The command buffer to execute


		VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;


		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}


		// Presentation
		// Last step in drawing a frame. Submit it to the swap chain
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr; // Optional

		// At the end because we can still show the visual but it is suboptimal. We will still update the swapchain
		result = vkQueuePresentKHR(presentQueue, &presentInfo);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
			framebufferResized = false;
			recreateSwapChain();
		}
		else if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image!");
		}

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void Window::updateUniformBuffer(uint32_t currentImage)
	{
		// Typicall time interpolation
		static auto startTime = std::chrono::high_resolution_clock::now();
		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		UniformBufferObject ubo{};
		// Rotation around the Z axis
		// glm::mat4(1.0f) return an identity matrix
		// time * radians is 90* a second
		// 0=x 0=y 1=z ?????
		ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

		// Looking at geometry at a 45Degre angle
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

		// 45 degree of field of view.
		// aspect ratio 
		ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);

		// GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted. 
		// The easiest way to compensate for that is to flip the sign on the scaling factor of the Y axis in the projection matrix. 
		// If you don't do this, then the image will be rendered upside down.
		ubo.proj[1][1] *= -1;

		// Memory is already mapped due to the technique "persistant mapping"
		memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
	}



	void Window::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0; // Optional
		beginInfo.pInheritanceInfo = nullptr; // Optional. Used for secondary command buffers

		// If the command buffer was already recorded once, then vkBeginCommandBuffer will implicity reset it. 
		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}


		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = (swapChainFramebuffers)[imageIndex]; // create a framebuffer for each swap chain image where it is specified as a color attachment

		renderPassInfo.renderArea.offset = { 0, 0 }; // Size for render area. Starting position
		renderPassInfo.renderArea.extent = swapChainExtent; // size

		VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} }; // Black with 100% opacity
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		// Render pass can now begin. All function that record commands can be recongnized by their vkCmd
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);


		// 2nd arg: pipline object is either graphics or compute pipeline. 
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

		// This is what is what uploading the input to the shader of the program
		VkBuffer vertexBuffers[] = { vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

		// Binding our descriptor sets to the frame. Specifing that its for the graphics over the compute pipeline. 
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

		//vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
		// buffer, vertexCount, instanceCount, firstVertex, firstInstance
		//vkCmdDraw(commandBuffer, 3, 1, 0, 0);


		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
		// Ends the render pass
		vkCmdEndRenderPass(commandBuffer);
		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}

	// Used when the window changes
	// It works but requires us to wait for all old resources to finish before updating.
	// Wy around this is to by passing the old swap chain in the oldSwapChain in the VKSwapchainCreateInfoKHR
	void Window::recreateSwapChain()
	{
		int width = 0, height = 0;
		glfwGetFramebufferSize(window, &width, &height);
		while (width == 0 || height == 0) {
			glfwGetFramebufferSize(window, &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(vkDevice); // wait for shared resources to be freed

		cleanupSwapchain();

		createSwapChain();
		createImageViews();
		createRenderPass();

		// There are ways around rebuilding the pipeline. One is using the dynamic state that was created during construction.
		// Viewport and scissoring could be alter this way
		createGraphicsPipeline();
		createFramebuffers(); // depend on swap chain images
	}









	void Window::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(vkDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create buffer!");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(vkDevice, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(vkDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate buffer memory!");
		}

		vkBindBufferMemory(vkDevice, buffer, bufferMemory, 0);
	}

	void Window::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
	{
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkBufferCopy copyRegion{};
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		endSingleTimeCommands(commandBuffer);
	}







	VkCommandBuffer Window::beginSingleTimeCommands() {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;// VK_COMMAND_POOL_CREATE_TRANSIENT_BIT  if we want to create a command pool for this
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(vkDevice, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;// Good practice to tell driver the intent usage

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		return commandBuffer;
	}

	void Window::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
		vkEndCommandBuffer(commandBuffer);// Contains only copy command, need to stop recording

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		// There are 2 things we could do to wait for the transmission to complete
		// 1) Add a fence and wait for it to be free.
		// 2) wait tell the queue become idle.
		vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue);

		vkFreeCommandBuffers(vkDevice, commandPool, 1, &commandBuffer);
	}

	void Window::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = {
			width,
			height,
			1
		};

		vkCmdCopyBufferToImage(
			commandBuffer,
			buffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,// Need to match up with the operation that was defined when allocating the image
			1,
			&region
		);

		endSingleTimeCommands(commandBuffer);
	}






}