#pragma once



#include "GraphicEngine/GraphicsDevice.hpp"
#include "GraphicEngine/GraphicsPipeline.hpp"
#include "GraphicEngine/GraphicsSwapchain.hpp"
#include "GraphicEngine/GraphicsVertex.hpp"
#include "GraphicEngine/GraphicsDevice.hpp"
#include "GraphicEngine/GraphicsObjectController.hpp"
#include "GraphicEngine/PipelinesIdMapping.hpp"

#include "include/input/InputBase.hpp"

#include <array>
#include <functional>
#include <queue>

namespace MGE {
	class GraphicsCore;
}

namespace GE {
	class GraphicsCorePIMPL
	{
		friend MGE::GraphicsCore;
	public:
		GraphicsCorePIMPL();
		~GraphicsCorePIMPL();

		void setShutdownFlag(std::atomic<bool>*);

		ErrorMessage init();


		ErrorMessage loadInTexture(std::string_view identifier, std::string_view objectName, std::string_view textureName);


		void mainLoop();

		void registerForInput(MGE::InputCallback callback, uint16_t inputTypes, uint16_t inputActions);

	protected:
		void recreateSwapChain();

		void readKeys();

		void drawFrame();

		void dispatchInputs();


	private:
		uint32_t currentFrame = 0;

		GraphicDeviceGroup deviceGroup;
		
		GraphicsCommandPool commandPool;
		std::vector<GraphicPipeline*> graphicPipelines;

		SwapchainHandle swapchainHandle;




		bool viewPortDirty{false};
		GraphicsObjectController graphicObjectController;


		std::atomic<bool>* shutdownFlag{nullptr};


		static void keyCallbackHandler(GLFWwindow* window, int key, int scancode, int action, int mods);
		static void mouseClickCallbackHandler(GLFWwindow* window, int button, int action, int mods);
		static void cursorPositionCallbackHandler(GLFWwindow* window, double xpos, double ypos);
		static void scrollCallbackHandler(GLFWwindow* window, double xoffset, double yoffset);


		std::queue<MGE::Input> queuedInputs;
		

		struct CallbackMetaData{
			MGE::InputCallback callback;
			uint16_t inputTypes{0};
			uint16_t actionTypes{0};
		};
		std::vector<CallbackMetaData> registeredCallbacks;


		std::array<double, 2> lastXYPos{0,0};
	};

}