#include "include/GraphicsCore.hpp"
#include "include/object/ThingManager.hpp"

#include "GraphicEngine/GraphicsCorePIMPL.hpp"
#include "GraphicEngine/ThingManagerPIMPL.hpp"

#include <iostream>

namespace
{
	void RunGraphicsEngine(GE::GraphicsCorePIMPL * graphicsCore)
	{
		graphicsCore->mainLoop();
	}
}


namespace MGE
{






	GraphicsCore::GraphicsCore(int argc, char** argv)
	{
	}
	GraphicsCore::~GraphicsCore()
	{
		windowShutdownFlag.store(true);
	}


	std::string GraphicsCore::init()
	{
		core.reset(new GE::GraphicsCorePIMPL());
		if (auto message = core->init(); !message.empty())
		{
			return message;
		}
		core->setShutdownFlag(&windowShutdownFlag);
		return "";
	}

	int GraphicsCore::run()
	{
		if (core.get() == nullptr) { return -1; }

		windowShutdownFlag.store(false);
		core->mainLoop();

		return 0;
	}


	std::unique_ptr<ThingManager> GraphicsCore::getThingManager()
	{
		if (core.get() == nullptr) {
			return std::unique_ptr<ThingManager>();
		}
		auto item = std::make_unique<ThingManager>();
		item->impl.reset(new GE::ThingManagerPIMPL());
		item->impl->device = core->deviceGroup.device->device;
		item->impl->physicalDevice = core->deviceGroup.device->physicalDevice;
		item->impl->queue = core->deviceGroup.device->queues.graphicsQueue;
		item->impl->commandPool = core->commandPool.getCommandPool();
		item->impl->descriptorSetLayout = core->graphicPipelines.front()->Internals().descriptorSetLayout;
		item->impl->controller = &core->graphicObjectController;

		return item;
	}
	std::atomic<bool>* GraphicsCore::getShutdownFlag()
	{
		return &windowShutdownFlag;
	}


	void GraphicsCore::registerForKeyPress(MGE::InputCallback c) { core->registerForInput(std::move(c), (uint16_t)MGE::InputType::Key, (uint16_t)MGE::InputAction::Down | (uint16_t)MGE::InputAction::Up | (uint16_t)MGE::InputAction::Hold); }
	void GraphicsCore::registerForMouseClick(MGE::InputCallback c) { core->registerForInput(std::move(c), (uint16_t)MGE::InputType::Mouse, (uint16_t)MGE::InputAction::Down | (uint16_t)MGE::InputAction::Up); }
	void GraphicsCore::registerForMouseMovement(MGE::InputCallback c) { core->registerForInput(std::move(c), (uint16_t)MGE::InputType::Mouse, (uint16_t)MGE::InputAction::Down | (uint16_t)MGE::InputAction::Up | (uint16_t)MGE::InputAction::Move); }
	void GraphicsCore::registerForScroll(MGE::InputCallback c) { core->registerForInput(std::move(c), (uint16_t)MGE::InputType::Scroll, (uint16_t)0x1F); }

}