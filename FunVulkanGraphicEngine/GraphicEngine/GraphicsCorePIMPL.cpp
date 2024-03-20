#include "GraphicEngine/GraphicsCorePIMPL.hpp"

#include <iostream>

namespace GE
{
	PipelinesIdMapping& pipelineMappingsController = PipelinesIdMapping::getInstance();

	std::vector<ShaderLoadInfo> DefaultShaderInfo2() {
		ShaderLoadInfo i1; i1.fileName = "shaders/vert.spv"; i1.name = "main"; i1.type = ShaderType::Vertex;
		ShaderLoadInfo i2; i2.fileName = "shaders/frag.spv"; i2.name = "main"; i2.type = ShaderType::Fragment;
		//ShaderLoadInfo i2; i2.fileName = "shaders/fragAlter.spv"; i2.name = "main"; i2.type = ShaderType::Fragment;
		std::vector<ShaderLoadInfo> shaderInfo;
		shaderInfo.push_back(i1);
		shaderInfo.push_back(i2);
		return shaderInfo;
	}




	GraphicsCorePIMPL::GraphicsCorePIMPL(){}
	GraphicsCorePIMPL::~GraphicsCorePIMPL() {
		graphicObjectController.clear();
		swapchainHandle.Free();
		for(auto & graphicPipeline : graphicPipelines) graphicPipeline->Free();
		commandPool.Free();

		if (shutdownFlag != nullptr) { shutdownFlag->store(true); }
	}
	void GraphicsCorePIMPL::setShutdownFlag(std::atomic<bool>* flag)
	{
		shutdownFlag = flag;
	}


	ErrorMessage GraphicsCorePIMPL::init()
	{
		try {
			deviceGroup = GraphicDeviceGroup::CreateBaseGroup();
		}
		catch (const std::exception& e)
		{
			return e.what();
		}

		GraphicDevice& devices = *(deviceGroup.device.get());
		commandPool.setDevice(deviceGroup.device->device);
		if (!commandPool.init(deviceGroup.device->physicalDevice, deviceGroup.device->surface))
		{
			return commandPool.currentError;
		}

		if (!swapchainHandle.initSwapchain(deviceGroup.window->getGLFW(), devices.device, devices.physicalDevice, devices.surface,devices.msaaSamples))
		{
			return std::string(swapchainHandle.getError());
		}
		swapchainHandle.setCommandBuffer(commandPool.getCommandBuffers());


		graphicObjectController.init(devices.device, devices.physicalDevice, devices.queues.graphicsQueue);
		
		for (auto& pipelineData : pipelineMappingsController.getMetadataList())
		{
			graphicPipelines.push_back(pipelineMappingsController.getPipeline(pipelineData.first));
			GraphicPipeline* graphicPipeline = graphicPipelines.back();
			graphicPipeline->setShaders(pipelineData.second.shaders);
			graphicPipeline->setCommandBuffers(commandPool.getCommandBuffers());

			graphicPipeline->setDevice(devices.device);


			if (!graphicPipeline->initPipeline(devices.physicalDevice, swapchainHandle.Internals().swapchainImageFormat, swapchainHandle.Internals().swapchainExtent, devices.msaaSamples, swapchainHandle.Internals().renderPass))
			{
				return graphicPipeline->getError();
			}	


			graphicObjectController.initPipelineMeta(graphicPipeline->pipelineId, commandPool.getCommandPool(), graphicPipeline->Internals().descriptorSetLayout);
		}



		deviceGroup.window->ownerReferencePointer = this;
		glfwSetKeyCallback(deviceGroup.window->getGLFW(), keyCallbackHandler);
		glfwSetMouseButtonCallback(deviceGroup.window->getGLFW(), mouseClickCallbackHandler);
		glfwSetCursorPosCallback(deviceGroup.window->getGLFW(), cursorPositionCallbackHandler);
		glfwSetScrollCallback(deviceGroup.window->getGLFW(), scrollCallbackHandler);

		return "";
	}


	void GraphicsCorePIMPL::registerForInput(MGE::InputCallback callback, uint16_t inputTypes, uint16_t inputActions)
	{
		if (inputActions == (uint16_t)MGE::InputAction::Unknown || inputTypes == (uint16_t)MGE::InputType::Unknown)return;

		registeredCallbacks.push_back({ std::move(callback),inputTypes,inputActions });
	}


	ErrorMessage GraphicsCorePIMPL::loadInTexture(std::string_view identifier, std::string_view objectName, std::string_view textureName)
	{
		GraphicDevice& devices = *(deviceGroup.device.get());

		uint64_t thisId = graphicObjectController.createObject(0);
		auto objectPtr = graphicObjectController.retrieveObject(thisId);

		{
			//bool state = objectPtr->verticesHandle.init(objectName, devices.device, devices.physicalDevice, devices.queues.graphicsQueue, graphicPipelines.front()->Internals().commandPool, graphicPipelines.front()->Internals().descriptorSetLayout);
			bool state = objectPtr->verticesHandle.init(objectName, devices.device, devices.physicalDevice, devices.queues.graphicsQueue, commandPool.getCommandPool(), graphicPipelines.front()->Internals().descriptorSetLayout);
			if (!state) {
				return "ERROR with creating texture: " + std::string(objectPtr->verticesHandle.getError());
			}
		}
		{
			//bool state = objectPtr->textureHandle.init(textureName, devices.device, devices.physicalDevice, devices.queues.graphicsQueue, graphicPipelines.front()->Internals().commandPool, graphicPipelines.front()->Internals().descriptorSetLayout);
			bool state = objectPtr->textureHandle.init(textureName, devices.device, devices.physicalDevice, devices.queues.graphicsQueue, commandPool.getCommandPool(), graphicPipelines.front()->Internals().descriptorSetLayout);
			if (!state) {
				return "ERROR with creating texture: " + std::string(objectPtr->textureHandle.getError());
			}
		}


		UniformBufferObject ubo{};
		ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(glm::vec3(4.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f), 1.5f, 0.1f, 10.0f);
		ubo.proj[1][1] *= -1;
		objectPtr->setUBO({ ubo });

		return "";
	}





	void GraphicsCorePIMPL::mainLoop()
	{
		//Keeps the application runnig until error or window is closed
		while (!glfwWindowShouldClose(deviceGroup.window->getGLFW())) {
			glfwPollEvents();

			dispatchInputs();
			drawFrame();

			if (shutdownFlag != nullptr && shutdownFlag->load() == true) { break; }

			if (viewPortDirty)
			{
				viewPortDirty = false;
				graphicObjectController.resetPipelineMeta();
				
				GraphicDevice& devices = *(deviceGroup.device.get());
				for (auto& pipeline : graphicPipelines)
				{
					pipeline->Free();
					if (!pipeline->initPipeline(devices.physicalDevice, swapchainHandle.Internals().swapchainImageFormat, swapchainHandle.Internals().swapchainExtent, devices.msaaSamples,swapchainHandle.Internals().renderPass))
					{
						std::cout <<  pipeline->getError() << std::endl;
						continue;
					}

					graphicObjectController.initPipelineMeta(pipeline->pipelineId, commandPool.getCommandPool(), pipeline->Internals().descriptorSetLayout);
				}

			}

		}
		vkDeviceWaitIdle(deviceGroup.device->device); // wait for the GPU to finish up. avoid complaints
		if (shutdownFlag != nullptr) { shutdownFlag->store(true); }
	}



	void GraphicsCorePIMPL::recreateSwapChain()
	{
		swapchainHandle.recreateSwapchain();
		viewPortDirty = true;
	}


	void GraphicsCorePIMPL::readKeys()
	{
		GLFWwindow*  window = deviceGroup.window->getGLFW();
		if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
			// The E key is pressed, handle the event here
			// For example, print a message
			printf("E key is pressed\n");

		}
	
	}

	void GraphicsCorePIMPL::drawFrame()
	{
		auto& swapchain = swapchainHandle.Internals().swapChain;
		auto& swapchainExtent = swapchainHandle.Internals().swapchainExtent;
	
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSemaphore signalSemaphores[] = { deviceGroup.sync->renderFinishedSemaphores[currentFrame] };
		VkSemaphore waitSemaphores[] = { deviceGroup.sync->imageAvailableSemaphores[currentFrame] };



		size_t counter = 0;
		{
			auto& graphicPipeline = graphicPipelines.at(0);
			uint32_t imageIndex;
			//vkWaitForFences(deviceGroup.device->device, 1, &deviceGroup.sync->inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
			vkWaitForFences(deviceGroup.device->device, 1, &deviceGroup.sync->inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
			VkResult result = vkAcquireNextImageKHR(deviceGroup.device->device, swapchain, UINT64_MAX, deviceGroup.sync->imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
			if (result == VK_ERROR_OUT_OF_DATE_KHR) {
				recreateSwapChain();
				return;
			}
			else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
				throw std::runtime_error("failed to acquire swap chain image!");
			}
			//vkResetFences(deviceGroup.device->device, 1, &deviceGroup.sync->inFlightFences[currentFrame]); // reset it after we get the last statement
			vkResetFences(deviceGroup.device->device, 1, &deviceGroup.sync->inFlightFences[currentFrame]); // reset it after we get the last statement


			auto& swapchainExtra = swapchainHandle.InternalsBuffers();
			auto& swapchainFramebuffers = swapchainExtra.swapchainFramebuffers;
			CommandBuffers& commandBuffers = commandPool.getCommandBuffers();


			VkClearColorValue background = { 1.0f, 0.0f, 0.0f, 1.0f };


			{
				swapchainHandle.beginRenderPass(currentFrame, imageIndex, background);
				for (auto& pipe : graphicPipelines)
				{
					auto ids = graphicObjectController.getIds(pipe->pipelineId);
					for (auto& id : ids)
					{
						auto objPtr = graphicObjectController.retrieveObject(id);
						auto uboData = objPtr->getUBO();
						if (!objPtr->textureHandle.Internals().ubo.uniformBuffersMapped.empty())
							memcpy(objPtr->textureHandle.Internals().ubo.uniformBuffersMapped[currentFrame], &uboData, sizeof(uboData));
					}
					swapchainHandle.bindPipeline(currentFrame, pipe->Internals().graphicsPipeline);
					for (auto& id : ids)
					{
						auto objPtr = graphicObjectController.retrieveObject(id);
						swapchainHandle.drawVertices(currentFrame, pipe->Internals().pipelineLayout, objPtr->verticesHandle.Internals().indexBuffer, (uint32_t)objPtr->verticesHandle.Internals().indices.size(), objPtr->verticesHandle.Internals().vertexBuffer, objPtr->textureHandle.Internals().descriptorSets[currentFrame]);
					}
				}
				swapchainHandle.endRenderPass(currentFrame);
			}


			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.waitSemaphoreCount = 1;

			submitInfo.pWaitSemaphores = waitSemaphores;
			submitInfo.pWaitDstStageMask = waitStages;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &commandBuffers[currentFrame]; // The command buffer to execute
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = signalSemaphores;
			

			if (vkQueueSubmit(deviceGroup.device->queues.graphicsQueue, 1,&submitInfo, deviceGroup.sync->inFlightFences[currentFrame]) != VK_SUCCESS) {
				throw std::runtime_error("failed to submit draw command buffer!");
			}

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

			counter++;
		}

	}

	void GraphicsCorePIMPL::dispatchInputs()
	{
		while (!queuedInputs.empty())
		{
			MGE::Input& input = queuedInputs.front();
			
			for (CallbackMetaData & item : registeredCallbacks)
			{
				if (!(item.actionTypes & (uint16_t)input.getAction()) || !(item.inputTypes & (uint16_t)input.getType()))continue;

				item.callback(input);
			}

			queuedInputs.pop();
		}
	}



	MGE::InputAction getGlfwAction(int action)
	{
		MGE::InputAction inputAction;
		switch (action)
		{
		case 0: inputAction = MGE::InputAction::Up; break;
		case 1: inputAction = MGE::InputAction::Down; break;
		case 2: inputAction = MGE::InputAction::Hold; break;
		default:
			inputAction = MGE::InputAction::Unknown;
		}
		return inputAction;
	}


	void GraphicsCorePIMPL::keyCallbackHandler(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		auto myWindow = reinterpret_cast<GE::DeviceWindow*>(glfwGetWindowUserPointer(window));
		auto app = reinterpret_cast<GE::GraphicsCorePIMPL*>(myWindow->ownerReferencePointer);




		MGE::InputAction inputAction = getGlfwAction(action);
		auto input = MGE::Input::CreateKeyInput(key, inputAction, (mods & GLFW_MOD_SHIFT), (mods & GLFW_MOD_CONTROL), (mods & GLFW_MOD_ALT));
		app->queuedInputs.push(input);
	}

	void GraphicsCorePIMPL::mouseClickCallbackHandler(GLFWwindow* window, int button, int action, int mods)
	{
		auto myWindow = reinterpret_cast<GE::DeviceWindow*>(glfwGetWindowUserPointer(window));
		auto app = reinterpret_cast<GE::GraphicsCorePIMPL*>(myWindow->ownerReferencePointer);



		MGE::InputAction inputAction = getGlfwAction(action);
		auto input = MGE::Input::CreateKeyInput(button, inputAction, (mods & GLFW_MOD_SHIFT), (mods & GLFW_MOD_CONTROL), (mods & GLFW_MOD_ALT));
		input.setType(MGE::InputType::Mouse);
		input.setXY(app->lastXYPos[0], app->lastXYPos[1]);
		app->queuedInputs.push(input);
	}

	void GraphicsCorePIMPL::cursorPositionCallbackHandler(GLFWwindow* window, double xpos, double ypos)
	{
		auto myWindow = reinterpret_cast<GE::DeviceWindow*>(glfwGetWindowUserPointer(window));
		auto app = reinterpret_cast<GE::GraphicsCorePIMPL*>(myWindow->ownerReferencePointer);
		app->lastXYPos[0] = xpos;
		app->lastXYPos[1] = ypos;

		app->queuedInputs.push(MGE::Input::CreateMouseInput(xpos, ypos, MGE::InputAction::Move));
	}

	void GraphicsCorePIMPL::scrollCallbackHandler(GLFWwindow* window, double xOffset, double yOffset)
	{
		auto myWindow = reinterpret_cast<GE::DeviceWindow*>(glfwGetWindowUserPointer(window));
		auto app = reinterpret_cast<GE::GraphicsCorePIMPL*>(myWindow->ownerReferencePointer);

		auto input = MGE::Input::CreateScrollInput(xOffset, yOffset);
		app->queuedInputs.push(input);
	}


}