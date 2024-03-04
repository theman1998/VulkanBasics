#include "GraphicEngine/GraphicsCore.hpp"

#include <chrono>
#include <iostream>

namespace GE
{

	


	void updateUBOData(std::atomic<bool> *shutdownFlag, GraphicsObjectController *dataObjects, SwapchainHandle * swapchainHandle)
	{
		

		while (!shutdownFlag->load())
		{
			auto swapchainExtent = swapchainHandle->Internals().swapchainExtent;

			auto ids = dataObjects->getIds();
			int xxPlace = 0;
			for (auto id : ids)
			{

				//auto& textureHandle = textureStorage.getTextureHandle(id);
				static auto startTime = std::chrono::high_resolution_clock::now();
				auto currentTime = std::chrono::high_resolution_clock::now();
				float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
				UniformBufferObject ubo{};
				// Rotation around the Z axis
				// glm::mat4(1.0f) return an identity matrix
				// time * radians is 90* a second
				// 0=x 0=y 1=z ?????
				ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
				
				/// Position of camera, position of center vision, Ground trueth for top view
				ubo.view = glm::lookAt(glm::vec3(4.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
				
				int iii = (xxPlace % 2 == 0) ? 1 : -1;
				//ubo.view = glm::translate(ubo.view, glm::vec3(1.5 * xxPlace * iii, 0, 0));
				// 45 degree of field of view.
				// aspect ratio 
				ubo.proj = glm::perspective(glm::radians(45.0f), swapchainExtent.width / (float)swapchainExtent.height, 0.1f, 10.0f);
				
				//ubo.proj = glm::perspective(glm::radians(45.0f), swapchainExtent.width / (float)swapchainExtent.height, 0.1f, 10.0f);
				
				// GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted. 
				// The easiest way to compensate for that is to flip the sign on the scaling factor of the Y axis in the projection matrix. 
				// If you don't do this, then the image will be rendered upside down.
				ubo.proj[1][1] *= -1;

				xxPlace++;

				dataObjects->retrieveObject(id)->setUBO(ubo);
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
		}
	}


	std::vector<ShaderLoadInfo> DefaultShaderInfo() {
		ShaderLoadInfo i1; i1.fileName = "shaders/vert.spv"; i1.name = "main"; i1.type = ShaderType::Vertex;
		ShaderLoadInfo i2; i2.fileName = "shaders/frag.spv"; i2.name = "main"; i2.type = ShaderType::Fragment;
		std::vector<ShaderLoadInfo> shaderInfo;
		shaderInfo.push_back(i1);
		shaderInfo.push_back(i2);
		return shaderInfo;
	}


	GraphicsCore::GraphicsCore()
	{
	}
	GraphicsCore::~GraphicsCore()
	{
		shutdownFlag.store(true);
		threadTest.join();
		graphicObjectController.clear();
		//textureStorage.clear();
		//verticesStorage.clear();
		swapchainHandle.Free();
		graphicPipeline.Free();
	}
	ErrorMessage GraphicsCore::init()
	{
		try {
			deviceGroup = GraphicDeviceGroup::CreateBaseGroup();
		}
		catch (const std::exception& e)
		{
			return e.what();
		}

		GraphicDevice& devices = *(deviceGroup.device.get());

		graphicPipeline.initDevice(devices.device);
		if (!graphicPipeline.initCommandInfo(devices.physicalDevice, devices.surface))
		{
			return graphicPipeline.getError();
		}


		graphicPipeline.setShaders(DefaultShaderInfo());

		if (!swapchainHandle.initSwapchain(deviceGroup.window->getGLFW(), devices.device, devices.physicalDevice, devices.surface))
		{
			return std::string(swapchainHandle.getError());
		}

		if (!graphicPipeline.initRenderPass(devices.physicalDevice, swapchainHandle.Internals().swapchainImageFormat, devices.msaaSamples))
		{
			return graphicPipeline.getError();
		}

		if (!graphicPipeline.initPipeline(swapchainHandle.Internals().swapchainExtent, devices.msaaSamples))
		{
			return graphicPipeline.getError();
		}

		if (!swapchainHandle.initExtras(graphicPipeline.Internals().renderPass, devices.msaaSamples))
		{
			return std::string(swapchainHandle.getError());
		}

		//verticesStorage.init(devices.device, devices.physicalDevice, devices.queues.graphicsQueue, graphicPipeline.Internals().commandPool, graphicPipeline.Internals().descriptorSetLayout);
		//textureStorage.init(devices.device, devices.physicalDevice, devices.queues.graphicsQueue, graphicPipeline.Internals().commandPool, graphicPipeline.Internals().descriptorSetLayout);
		graphicObjectController.init(devices.device, devices.physicalDevice, devices.queues.graphicsQueue, graphicPipeline.Internals().commandPool, graphicPipeline.Internals().descriptorSetLayout);







		shutdownFlag.store(false);
		threadTest = std::thread(updateUBOData , &shutdownFlag, &graphicObjectController, &swapchainHandle);

		return "";
	}









	ErrorMessage GraphicsCore::loadInTexture(std::string_view identifier, std::string_view objectName, std::string_view textureName)
	{
		//verticesStorage.createTexture(std::string(identifier), objectName);
		//textureStorage.createTexture(std::string(identifier), textureName);
		GraphicDevice& devices = *(deviceGroup.device.get());

		graphicObjectController.createObject(std::string(identifier), true);
		auto objectPtr = graphicObjectController.retrieveObject(std::string(identifier));

		{
			bool state = objectPtr->verticesHandle.init(objectName, devices.device, devices.physicalDevice, devices.queues.graphicsQueue, graphicPipeline.Internals().commandPool, graphicPipeline.Internals().descriptorSetLayout);
			if (!state) {
				return "ERROR with creating texture: " + std::string(objectPtr->verticesHandle.getError());
			}
		}
		{
			bool state = objectPtr->textureHandle.init(textureName, devices.device, devices.physicalDevice, devices.queues.graphicsQueue, graphicPipeline.Internals().commandPool, graphicPipeline.Internals().descriptorSetLayout);
			if (!state) {
				return "ERROR with creating texture: " + std::string(objectPtr->textureHandle.getError());
			}
		}


		return "";
	}

	ErrorMessage GraphicsCore::loadInTexture(std::string_view identifier, std::vector<Vertex> vertices, std::vector<uint32_t>indices, std::string_view textureName)
	{
		//verticesStorage.createVertice(std::string(identifier), vertices,indices);
		//textureStorage.createTexture(std::string(identifier), textureName);

		GraphicDevice& devices = *(deviceGroup.device.get());

		graphicObjectController.createObject(std::string(identifier), true);
		auto objectPtr = graphicObjectController.retrieveObject(std::string(identifier));

		{
			bool state = objectPtr->verticesHandle.init(vertices, indices, devices.device, devices.physicalDevice, devices.queues.graphicsQueue, graphicPipeline.Internals().commandPool, graphicPipeline.Internals().descriptorSetLayout);
			if (!state) {
				return "ERROR with creating texture: " + std::string(objectPtr->verticesHandle.getError());
			}
		}
		{
			bool state = objectPtr->textureHandle.init(textureName, devices.device, devices.physicalDevice, devices.queues.graphicsQueue, graphicPipeline.Internals().commandPool, graphicPipeline.Internals().descriptorSetLayout);
			if (!state) {
				return "ERROR with creating texture: " + std::string(objectPtr->textureHandle.getError());
			}
		}

		return "";
	}




	void GraphicsCore::mainLoop()
	{
		//Keeps the application runnig until error or window is closed
		while (!glfwWindowShouldClose(deviceGroup.window->getGLFW())) {
			glfwPollEvents();
			drawFrame();


		}
		vkDeviceWaitIdle(deviceGroup.device->device); // wait for the GPU to finish up. avoid complaints
	}



	void GraphicsCore::recreateSwapChain()
	{
		swapchainHandle.recreateSwapchain();
		graphicPipeline.updateViewPort(currentFrame, swapchainHandle.Internals().swapchainExtent);
	}

	void GraphicsCore::drawFrame()
	{
		
		// VK_True waits for all fances to be complete. Times out after max of int64
		vkWaitForFences(deviceGroup.device->device, 1, &deviceGroup.sync->inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;

		auto& swapchain = swapchainHandle.Internals().swapChain;
		auto& swapchainExtent = swapchainHandle.Internals().swapchainExtent;
		auto& swapchainFramebuffers = swapchainHandle.Internals().swapchainFramebuffers;
		std::vector < VkCommandBuffer >& commandBuffers = graphicPipeline.Internals().commandBuffers;

		// Checking if the state of our window changed.
		VkResult result = vkAcquireNextImageKHR(deviceGroup.device->device, swapchain, UINT64_MAX, deviceGroup.sync->imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		auto ids = graphicObjectController.getIds();

		for (auto& id : ids)
		{
			auto objPtr = graphicObjectController.retrieveObject(id);
			auto uboData = objPtr->getUBO();
			memcpy(objPtr->textureHandle.Internals().ubo.uniformBuffersMapped[currentFrame], &uboData, sizeof(uboData));
		}
		//for (auto& id : ids)
		//{
		//	static int xxPlace = 0;
		//	//auto& textureHandle = textureStorage.getTextureHandle(id);
		//	static auto startTime = std::chrono::high_resolution_clock::now();
		//	auto currentTime = std::chrono::high_resolution_clock::now();
		//	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
		//	UniformBufferObject ubo{};
		//	// Rotation around the Z axis
		//	// glm::mat4(1.0f) return an identity matrix
		//	// time * radians is 90* a second
		//	// 0=x 0=y 1=z ?????
		//	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		//	// Looking at geometry at a 45Degre angle
		//	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		//	int iii = (xxPlace % 2 == 0) ? 1 : -1;
		//	ubo.view = glm::translate(ubo.view, glm::vec3(0.00001 * xxPlace * iii, -0.00001 * xxPlace * iii, 0.00001 * xxPlace * iii));
		//	// 45 degree of field of view.
		//	// aspect ratio 
		//	ubo.proj = glm::perspective(glm::radians(45.0f), swapchainExtent.width / (float)swapchainExtent.height, 0.1f, 10.0f);
		//	// GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted. 
		//	// The easiest way to compensate for that is to flip the sign on the scaling factor of the Y axis in the projection matrix. 
		//	// If you don't do this, then the image will be rendered upside down.
		//	ubo.proj[1][1] *= -1;
		//	// Memory is already mapped due to the technique "persistant mapping"
		//	memcpy(textureHandle.Internals().ubo.uniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
		//	xxPlace++;
		//}



		// Only reset the fence if we are submitting work
		vkResetFences(deviceGroup.device->device, 1, &deviceGroup.sync->inFlightFences[currentFrame]); // reset it after we get the last statement


		vkResetCommandBuffer(commandBuffers[currentFrame], 0); // Do before recording command buffer. If not it will freeze


		graphicPipeline.startPipelinePass(currentFrame, swapchainExtent, swapchainFramebuffers[imageIndex], { {5.0f, 0.0f, 0.0f, 1.0f} });
		for (auto& id : ids)
		{

			//auto& textureHandle = textureStorage.getTextureHandle(id);
			//auto& verticesHandle = verticesStorage.getHandle(id);
			//graphicPipeline.bindAndDrawIndexVertices(currentFrame, verticesHandle.Internals().indexBuffer, verticesHandle.Internals().indices.size(), verticesHandle.Internals().vertexBuffer, textureHandle.Internals().descriptorSets[currentFrame]);
			auto objPtr = graphicObjectController.retrieveObject(id);
			graphicPipeline.bindAndDrawIndexVertices(currentFrame, objPtr->verticesHandle.Internals().indexBuffer, objPtr->verticesHandle.Internals().indices.size(), objPtr->verticesHandle.Internals().vertexBuffer, objPtr->textureHandle.Internals().descriptorSets[currentFrame]);
		}
		graphicPipeline.completePipelinePass(currentFrame);




		// Submitting the command buffer
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { deviceGroup.sync->imageAvailableSemaphores[currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[currentFrame]; // The command buffer to execute


		VkSemaphore signalSemaphores[] = { deviceGroup.sync->renderFinishedSemaphores[currentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;


		if (vkQueueSubmit(deviceGroup.device->queues.graphicsQueue, 1, &submitInfo, deviceGroup.sync->inFlightFences[currentFrame]) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}
		// Presentation
		// Last step in drawing a frame. Submit it to the swap chain
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
		VkSwapchainKHR swapChains[] = { swapchain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr; // Optional
		// At the end because we can still show the visual but it is suboptimal. We will still update the swapchain
		result = vkQueuePresentKHR(deviceGroup.device->queues.presentQueue, &presentInfo);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || deviceGroup.window->getBufferResizeFlag(true)) {
			recreateSwapChain();
		}
		else if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image!");
		}


		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}



}