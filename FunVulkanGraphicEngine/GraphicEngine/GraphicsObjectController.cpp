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

	void GraphicsObjectController::init(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue)
	{
		std::lock_guard lock(mutex);
		this->device = device;
		this->physicalDevice = physicalDevice;
		this->graphicsQueue = graphicsQueue;
	}

	void GraphicsObjectController::initPipelineMeta(uint64_t pipelineId, VkCommandPool commandPool, VkDescriptorSetLayout descriptorSetLayout)
	{
		std::lock_guard lock(mutex);
		PipelineMetaInfo info;
		info.pipelineId = pipelineId;
		info.commandPool = commandPool;
		info.descriptorSetLayout = descriptorSetLayout;
		this->pipelineMetaInfoOptions.push_back(std::move(info));
	}

	void GraphicsObjectController::resetPipelineMeta()
	{
		std::lock_guard lock(mutex);
		pipelineMetaInfoOptions.clear();
	}


	void GraphicsObjectController::clear()
	{
		std::lock_guard lock(mutex);
		for (auto& [_, obj] : objectList)
		{
			obj->textureHandle.Free();
			obj->verticesHandle.Free();
		}
	}
	void GraphicsObjectController::remove(uint64_t id)
	{
		std::lock_guard lock(mutex);
		removeLockless(id);
	}
	bool GraphicsObjectController::contains(uint64_t id)const
	{
		std::lock_guard lock(mutex);
		return objectList.find(id) != objectList.end();
	}
	GraphObjPtr GraphicsObjectController::retrieveObject(uint64_t id)
	{
		std::lock_guard lock(mutex);
		return objectList.at(id);
	}

	uint64_t GraphicsObjectController::createObject(uint64_t pipelineId)
	{
		std::lock_guard lock(mutex);
		uint64_t thisId = currentCounter++;
		objectList[thisId] = std::shared_ptr<GraphicObject>(new GraphicObject);
		objectList[thisId]->pipelineId = pipelineId;
		return thisId;
	}

	std::vector<uint64_t> GraphicsObjectController::getIds(uint64_t pipelineId) const
	{
		std::lock_guard lock(mutex);
		std::vector<uint64_t> ids;

		for (auto& [id, _] : objectList)
		{
			if (_->pipelineId == pipelineId) { ids.push_back(id); }
		}
		return ids;
	}

	std::vector<GraphicsObjectController::PipelineMetaInfo> GraphicsObjectController::getOptions() const
	{
		std::lock_guard lock(mutex);
		return pipelineMetaInfoOptions;
	}

	std::lock_guard<std::mutex> GraphicsObjectController::Lock()
	{
		return std::lock_guard(mutex);
	}

	void GraphicsObjectController::removeLockless(uint64_t id)
	{
		if (auto it = objectList.find(id); it != objectList.end()) {
			it->second->textureHandle.Free();
			it->second->verticesHandle.Free();
		}
	}
}