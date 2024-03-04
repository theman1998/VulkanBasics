#pragma once

#include <atomic>
#include <array>
#include <memory>
#include <vector>
#include "GraphicEngine/GraphicsQueue.hpp"



namespace GE
{

	class DeviceWindow{
	public:
		struct Area {
			int width{ 0 };
			int height{ 0 };
			Area(int w, int h);
		};

		enum TYPE{
			GLFW,
			Undefined
		};
		DeviceWindow();
		~DeviceWindow();
		TYPE getWindowType()const;
		void* operator()();
		GLFWwindow* getGLFW();


		bool getBufferResizeFlag(bool doReset = false);
		void triggerBufferResizeFlag(bool state = true);

		static std::unique_ptr<DeviceWindow> CreateDeviceWindow(TYPE typeOfWindow = TYPE::GLFW, Area = Area(1200,800));
	private:
		void* window;
		TYPE type;
		std::atomic<bool> frameBufferResizeFlag;
	};


	class GraphicDevice {
		GraphicDevice();
	public:
		static std::unique_ptr<GraphicDevice> CreateDevice(DeviceWindow & deviceWindow, const VkApplicationInfo& appInfo = DefaultAppInfo(), std::vector<const char*> requiredExtensions = DefaultExtensions());

		static VkApplicationInfo DefaultAppInfo();
		static const std::vector<const char*>& DefaultExtensions();

		~GraphicDevice();
		VkDevice operator()() const;



	//private:
		VkInstance instance;
		VkDebugUtilsMessengerEXT debugMessenger;
		VkPhysicalDevice physicalDevice;
		VkDevice device;
		VkSurfaceKHR surface;
		VkSampleCountFlagBits msaaSamples;
		Queues queues;
	};


	struct Sync {
		~Sync();

		static std::unique_ptr<Sync> CreateSync(VkDevice);

		// Examples: https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples#swapchain-image-acquire-and-present
		std::vector < VkSemaphore > imageAvailableSemaphores;
		std::vector < VkSemaphore > renderFinishedSemaphores;
		std::vector < VkFence > inFlightFences;

		VkDevice device{ nullptr };
	};

	

	class GraphicDeviceGroup {
		GraphicDeviceGroup(const GraphicDeviceGroup&) = delete;
		GraphicDeviceGroup& operator=(const GraphicDeviceGroup&) = delete;
	public:
		GraphicDeviceGroup();
		~GraphicDeviceGroup();
		GraphicDeviceGroup(GraphicDeviceGroup&&);
		GraphicDeviceGroup& operator=(GraphicDeviceGroup&&);


		std::unique_ptr<DeviceWindow> window;
		std::unique_ptr<GraphicDevice> device;
		std::unique_ptr<Sync> sync;


		static GraphicDeviceGroup CreateBaseGroup();
	};

	

}