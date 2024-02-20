#pragma once


// required for Window speciffic systems.
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <vector>

#include "VertexHelpers.h"

namespace V
{

    // For the window size
    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;
    const uint32_t MAX_FRAMES_IN_FLIGHT = 2;



class Window
{

public:
	Window();

	void run();

protected:
    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();
    void cleanupSwapchain();

	void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapChain();
    void createImageViews();

    void createRenderPass(); // specify how the framebuffer will be set up
    void createDescriptorSetLayout();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createTextureImage();
    void createTextureImageView();
    void createTextureSampler();
    void createVertexBuffer();
    void createIndexBuffer();
    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();
    void createCommandBuffer();
    void createSyncObjects();

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo); // required for both debugMessenger and createInstance

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);


    void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

    void createDepthResources();

    void drawFrame();
    void updateUniformBuffer(uint32_t currentImage);
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void recreateSwapChain();

    
    void loadModel();


    // Helpers
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat findDepthFormat();
    bool hasStencilComponent(VkFormat format); // Part of the Depth options. Found in the flag

    void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
    VkSampleCountFlagBits getMaxUsableSampleCount();
    void createColorResources();

private:
    GLFWwindow* window;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    // Used as a logic device
    VkDevice vkDevice;
    VkPhysicalDevice physicalDevice;
    // used to interact with the queues made from the logical devices (vkDevie)
    VkQueue graphicsQueue;

    // Used for presentation
    VkSurfaceKHR surface;
    // Similar to the other queue but we are referecning the surface queue handler with this
    VkQueue presentQueue;

    // swapchain controls the outputting graphics
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages; // once swapchain is setup we can control the images through this member
    VkFormat swapChainImageFormat; //need this in future chapters
    VkExtent2D swapChainExtent; //need this in future chapters

    // Images that wil get render the the pipeline
    std::vector<VkImageView> swapChainImageViews;

    // Used for render passing
    VkRenderPass renderPass;

    // Keeps reference to the Uniform buffers object
    VkDescriptorSetLayout descriptorSetLayout;
    // required for the graphic pipeline
    VkPipelineLayout pipelineLayout;
    // The final piece
    VkPipeline graphicsPipeline;

    // Holds the framebuffers for the rendering pass.
    std::vector<VkFramebuffer> swapChainFramebuffers;

    // Used to draw inderectly
    VkCommandPool commandPool;
    // Used for allocations buffer
    std::vector < VkCommandBuffer > commandBuffers;

    // dynamically controll the vertex input
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory; // allocated memory for gpu
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    // We might have multiple frames in flight at a single time, hence we must use dynamic memory
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;

    // Images pixel are mapped on a 2d plane with colors. They are referered to as "TEXELS"
    uint32_t mipLevels; // Used in Level of design. Halfing the image size each level.
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView; //fpr texture image
    VkSampler textureSampler; // Distink from the image. Can be used to extra pixels from any image

    // Uses same calls as the texture. Allows the rasterizer to do a depth check
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    // Used coasside with the uniform buffers
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    VkImage colorImage; // msaa requires to be processes of screen. So we need a new buffer to store the multisample pixels
    VkDeviceMemory colorImageMemory;
    VkImageView colorImageView;


    // Synchronization Objects
    // Examples: https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples#swapchain-image-acquire-and-present
    std::vector < VkSemaphore > imageAvailableSemaphores;
    std::vector < VkSemaphore > renderFinishedSemaphores;
    std::vector < VkFence > inFlightFences;

    uint32_t currentFrame = 0; // Used with the framebuffers and semaphores

    // variable and callback that will trigger a restart on the swapchain if the size changes
    bool framebufferResized = false;
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);


};
}