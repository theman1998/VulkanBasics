#pragma once


#include "GraphicEngine/GraphicsDevice.hpp"
#include "GraphicEngine/GraphicsPipeline.hpp"
#include "GraphicEngine/GraphicsSwapchain.hpp"
#include "GraphicEngine/GraphicsVertex.hpp"
#include "GraphicEngine/GraphicsDevice.hpp"
#include "GraphicEngine/GraphicsObjectController.hpp"
#include <map>

#include <thread>

namespace GE 
{
	class GraphicsCore {
	public:
		GraphicsCore();
		~GraphicsCore();


		ErrorMessage init();


		ErrorMessage loadInTexture(std::string_view identifier, std::string_view objectName, std::string_view textureName);

		ErrorMessage loadInTexture(std::string_view identifier, std::vector<Vertex>, std::vector<uint32_t>, std::string_view textureName);


		void mainLoop();

	protected:
		void recreateSwapChain();

		void drawFrame();

	private:
		uint32_t currentFrame = 0;

		GraphicDeviceGroup deviceGroup;

		GraphicPipeline graphicPipeline;
		SwapchainHandle swapchainHandle;

		//GraphicsVerticesStorage verticesStorage;
		//GraphicsTextureStorage textureStorage;

		GraphicsObjectController graphicObjectController;


		std::atomic<bool> shutdownFlag;
		std::thread threadTest;

	};

}