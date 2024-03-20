#pragma once

#include <atomic>
#include <vector>
#include <utility>
#include <map>
#include <mutex>
#include <chrono>

#include "GraphicEngine/GraphicsPipeline.hpp"

namespace GE{


	class PipelinesIdMapping {
		

		PipelinesIdMapping();
		~PipelinesIdMapping();
		PipelinesIdMapping(const PipelinesIdMapping&) = delete;
		PipelinesIdMapping& operator = (const PipelinesIdMapping&) = delete;
	public:
		using ID = std::size_t;

		struct MetaData {
			std::string pipelineName;
			std::vector<ShaderLoadInfo> shaders;
			std::optional<VkViewport> customSize;
		};


		static PipelinesIdMapping& getInstance();

		//sgf
		bool isBeingModify() const;
		bool isDirty(std::chrono::milliseconds lastUpdate) const;
		std::lock_guard<std::mutex> blockUsage();

		void updateTime();
		std::chrono::milliseconds getLastUpdateTime();


		ID appendNewPipelineData(const MetaData&);
		std::size_t getPipelineDataItemsSize() const;
		std::vector<std::pair<ID, MetaData>> getMetadataList()const;





		
		GraphicPipeline* getPipeline(ID);


	private:
		std::atomic<bool> beingModifyFlag;
		std::chrono::milliseconds timeLastUpdated;
		mutable std::mutex sharedMutex;
		mutable std::mutex mutex;

		
		struct GraphicsPair {
			ID id;
			std::unique_ptr<GraphicPipeline> graphicsPipeline;
		};
		
		std::map<ID, MetaData> graphicsDataMapping;
		std::vector<GraphicsPair> graphicsPipelineList;

		void erasePipeline(ID);
	};






}