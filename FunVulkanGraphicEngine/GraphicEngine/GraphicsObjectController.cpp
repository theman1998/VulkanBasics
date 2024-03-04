#include "GraphicEngine/GraphicsObjectController.hpp"


namespace GE
{
	UniformBufferObject GraphicObject::getUBO() const
	{
		std::lock_guard lock(mutex);
		return uniformObject;
	}
	void GraphicObject::setUBO(const UniformBufferObject& o)
	{
		std::lock_guard lock(mutex);
		uniformObject = o;
	}

	void GraphicsObjectController::init(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue, VkCommandPool commandPool, VkDescriptorSetLayout descriptorSetLayout)
	{
		this->device = device;
		this->physicalDevice = physicalDevice;
		this->graphicsQueue = graphicsQueue;
		this->commandPool = commandPool;
		this->descriptorSetLayout = descriptorSetLayout;
	}

	void GraphicsObjectController::clear()
	{
		for (auto& [_,obj] : objectList)
		{
			obj->textureHandle.Free();
			obj->verticesHandle.Free();
		}
	}
	void GraphicsObjectController::remove(const std::string& id)
	{
		if (auto it = objectList.find(id); it != objectList.end()) {
			it->second->textureHandle.Free();
			it->second->verticesHandle.Free();
		}
	}
	bool GraphicsObjectController::contains(const std::string& id)const
	{
		return objectList.find(id) != objectList.end();
	}
	GraphObjPtr GraphicsObjectController::retrieveObject(const std::string& id)
	{
		return objectList.at(id);
	}

	void GraphicsObjectController::createObject(const std::string& id, bool doOverride)
	{
		if (!doOverride && objectList.find(id) != objectList.end())
		{
			return;
		}
		remove(id);
		objectList[id] = std::shared_ptr<GraphicObject>(new GraphicObject);

	}

	std::vector<std::string> GraphicsObjectController::getIds() const
	{
		std::vector<std::string> ids;
		for (auto& [id, _] : objectList) ids.push_back(id);
		return ids;
	}



}