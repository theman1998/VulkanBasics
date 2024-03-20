#pragma once

#include "GraphicEngine/GraphicsVertex.hpp"
#include <unordered_map>
#include <mutex>
#include <memory>

#include <optional>

namespace GE
{

	class GraphicObject {
	public:
		VerticesHandle verticesHandle;
		GraphicsTextureHandle textureHandle;

		UniformBufferObject getUBO() const;
		void setUBO(const UniformBufferObject&);

		uint64_t pipelineId{ 0 };

	private:
		UniformBufferObject uniformObject{};
		mutable std::mutex mutex;
	};
	using GraphObjPtr = std::shared_ptr<GraphicObject>;


	class GraphicsObjectController
	{
		struct PipelineMetaInfo {
			uint64_t pipelineId;
			VkCommandPool commandPool{ nullptr };
			VkDescriptorSetLayout descriptorSetLayout{ nullptr };
		};


	public:
		GraphicsObjectController() = default;
		void init(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue);
		void initPipelineMeta(uint64_t pipelineId, VkCommandPool commandPool, VkDescriptorSetLayout descriptorSetLayout);
		void resetPipelineMeta();

		void clear();
		void remove(uint64_t id);
		bool contains(uint64_t id)const;
		uint64_t createObject(uint64_t pipelineId);
		GraphObjPtr retrieveObject(uint64_t id);

		std::vector<uint64_t> getIds(uint64_t pipelineId) const;

		std::vector<PipelineMetaInfo> getOptions() const;

		std::lock_guard<std::mutex> Lock();

	private:
		void removeLockless(uint64_t id);

		std::unordered_map<uint64_t, GraphObjPtr> objectList;
		uint64_t currentCounter = 1;


		VkDevice device{ nullptr };
		VkPhysicalDevice physicalDevice{ nullptr };
		VkQueue graphicsQueue{ nullptr };



		std::vector<PipelineMetaInfo> pipelineMetaInfoOptions;

		//VkCommandPool commandPool{ nullptr };
		//VkDescriptorSetLayout descriptorSetLayout{ nullptr };
		mutable std::mutex mutex;

	};





}