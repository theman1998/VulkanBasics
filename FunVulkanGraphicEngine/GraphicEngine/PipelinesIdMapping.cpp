#include "GraphicEngine/PipelinesIdMapping.hpp"

#include <iostream>

namespace GE
{
	PipelinesIdMapping::PipelinesIdMapping() : beingModifyFlag(false), timeLastUpdated(std::chrono::milliseconds(0)){
		MetaData metaData;
		metaData.pipelineName = "default";
		ShaderLoadInfo i1; i1.fileName = "shaders/vert.spv"; i1.name = "main"; i1.type = ShaderType::Vertex;
		ShaderLoadInfo i2; i2.fileName = "shaders/frag.spv"; i2.name = "main"; i2.type = ShaderType::Fragment;
		metaData.shaders.push_back(i1);
		metaData.shaders.push_back(i2);
		appendNewPipelineData(metaData);
	}
	PipelinesIdMapping::~PipelinesIdMapping() {}

	PipelinesIdMapping& PipelinesIdMapping::getInstance()
	{
		static PipelinesIdMapping instance;
		return instance;
	}

	bool PipelinesIdMapping::isBeingModify()const
	{
		return beingModifyFlag;
	}
	bool PipelinesIdMapping::isDirty(std::chrono::milliseconds lastUpdate) const
	{
		std::lock_guard lock(sharedMutex);
		return lastUpdate != timeLastUpdated;
	}
	std::lock_guard<std::mutex> PipelinesIdMapping::blockUsage()
	{
		beingModifyFlag = true;
		return std::lock_guard<std::mutex>(mutex);
	}

	void PipelinesIdMapping::updateTime()
	{
		std::lock_guard lock(sharedMutex);
		auto now = std::chrono::system_clock::now();
		auto epochMs = std::chrono::time_point_cast<std::chrono::milliseconds>(now).time_since_epoch();
		timeLastUpdated = std::chrono::duration_cast<std::chrono::milliseconds>(epochMs);
	}
	std::chrono::milliseconds PipelinesIdMapping::getLastUpdateTime()
	{
		std::lock_guard lock(sharedMutex);
		return timeLastUpdated;
	}

	PipelinesIdMapping::ID PipelinesIdMapping::appendNewPipelineData(const MetaData& d)
	{
		std::lock_guard lock(sharedMutex);
		ID id = graphicsDataMapping.size() + 1;
		graphicsDataMapping[id] = d;
		graphicsPipelineList.push_back({ id, std::make_unique<GraphicPipeline>() });
		graphicsPipelineList.back().graphicsPipeline->pipelineId = id;
		return id;
	}
	std::size_t PipelinesIdMapping::getPipelineDataItemsSize() const
	{
		std::lock_guard lock(sharedMutex);
		return graphicsDataMapping.size();
	}
	std::vector<std::pair<PipelinesIdMapping::ID, PipelinesIdMapping::MetaData>> PipelinesIdMapping::getMetadataList()const
	{
		std::vector<std::pair<PipelinesIdMapping::ID, PipelinesIdMapping::MetaData>> res;
		std::lock_guard lock(sharedMutex);
		for (auto& pair : graphicsDataMapping) { res.push_back(pair); }
		return res;

	}

	void PipelinesIdMapping::erasePipeline(ID id)
	{
		std::lock_guard lock(sharedMutex);
		for (auto it = graphicsPipelineList.begin(); it != graphicsPipelineList.end();it++)
		{
			if (it->id == id) {
				graphicsPipelineList.erase(it);
				break;
			}
		}
		if (auto it = graphicsDataMapping.find(id); it != graphicsDataMapping.end()) { graphicsDataMapping.erase(it); }
		//888
	}

	GraphicPipeline* PipelinesIdMapping::getPipeline(ID id)
	{
		std::lock_guard lock(sharedMutex);
		for (auto it = graphicsPipelineList.begin(); it != graphicsPipelineList.end();it++)
		{
			if (it->id == id) { return it->graphicsPipeline.get(); }
		}
		return nullptr;
	}
}