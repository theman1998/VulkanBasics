#pragma once

#include "graphic/GLFWHelpers.h"
#include <memory>

#include <glm/glm.hpp>
#include <array>
#include <vector>
#include <type_traits>
#include <glm/gtx/hash.hpp>

namespace V
{

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
	glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription();

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();


	bool operator==(const Vertex& other) const;
};

// This is where the triangle is now being created
//const static inline std::vector<Vertex> vertices = {
//    {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
//    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
//    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
//};


//const std::vector<Vertex> vertices = {
//    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},//red - textures are based on [0-1 of position]
//    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}, // green
//    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}, // blue
//    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}} // white
//};


// Green Channels represent hotizontal coordinates and red vertical coordinates.
// Yellow and Black corners represent interpolated on the corners.
//const std::vector<Vertex> vertices = {
//	{{-0.5f, -0.5f,0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
//	{{0.5f, -0.5f,0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
//	{{0.5f, 0.5f,0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
//	{{-0.5f, 0.5f,0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
//	{{-0.3f, -0.3f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
//	{{0.3f, -0.3f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
//	{{0.3f, 0.3f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
//	{{-0.3f, 0.3f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
//};

//const std::vector<uint16_t> indices = {
//	0, 1, 2, 2, 3, 0,
//	4, 5, 6, 6, 7, 4
//};

// Alignment is important. Things need to be multiple of 16
// Can for alignments with #define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES 
// Avoid GLM_FORCE_DEFAULT_ALIGNED_GENTYPES and be explicit with the alignas
struct UniformBufferObject
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};


// Each graphic card offers different types of memory, with due to operations and characteristics. 
uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);




	// Used to define the space that will get viewed and also where to snip the from the raterize phase
	struct ViewportContainer
	{
		VkViewport viewport{};
		VkRect2D scissor{};
		VkPipelineViewportStateCreateInfo viewportState{};
	};

	// After fragment stage, this phase will combine the colors from frame buffer and shader
	struct ColorBlendContainer
	{
		VkPipelineColorBlendAttachmentState attachment{};
		VkPipelineColorBlendStateCreateInfo info{};
	};


	// Programmable stages for GPU
	VkShaderModule createShaderModule(VkDevice vkDevice, const std::vector<char>& code);
	VkPipelineShaderStageCreateInfo createShaderStageInfoVertex(VkShaderModule shaderModule);
	VkPipelineShaderStageCreateInfo createShaderStageInfoFragment(VkShaderModule shaderModule);

	


	// Fixed stages
	VkPipelineInputAssemblyStateCreateInfo createBasicAssemblyInfo();
	std::shared_ptr < ViewportContainer> createBasicViewportState(VkExtent2D* swapChainExtent);
	VkPipelineRasterizationStateCreateInfo createBasicRasterizeInfo();
	VkPipelineMultisampleStateCreateInfo createBasicMultisamplingInfo();
	std::shared_ptr < ColorBlendContainer > createBasicColorBlendState();
	VkPipelineLayoutCreateInfo createBasicPipelineLayoutInfo();



}



namespace std {
	template<> struct hash<V::Vertex> {
		size_t operator()(V::Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}