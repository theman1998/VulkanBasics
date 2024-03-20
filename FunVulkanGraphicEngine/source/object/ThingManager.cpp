#include "include/object/ThingManager.hpp"
#include "include/object/ThingBase.hpp"
#include "include/object/Camera.hpp"

#include "GraphicEngine/GraphicsObjectController.hpp"
#include "GraphicEngine/ThingManagerPIMPL.hpp"
#include "GraphicEngine/PipelinesIdMapping.hpp"

#include <iostream>

namespace MGE
{


	class Shape {
	public:
		float x{1};
		float y{1};
		float z{1};

		void setRadius(float f) { x = f; y = f; }

		//std::vector<glm::vec3>

	};



	ThingManager::ThingManager() : impl(nullptr), camera(new Camera()) { camera->setScreenSize(800, 600); }
	ThingManager::~ThingManager() = default;

	UID ThingManager::addThing(Point point)
	{
		if (impl == nullptr) return UID::Empty();


		auto optionList = impl->controller->getOptions();
		if (optionList.empty())return UID::Empty();

		// Currently Random to test multiple pipelines
		auto itemControls = optionList[ idsToPoints.size() % optionList.size()];


		std::string objectName = "models/viking_room.obj";
		std::string textureName = "textures/viking_room.png";

		uint64_t thisId = impl->controller->createObject(itemControls.pipelineId);
		auto objectPtr = impl->controller->retrieveObject(thisId);



		{
			bool state = objectPtr->verticesHandle.init(objectName, impl->device, impl->physicalDevice, impl->queue, itemControls.commandPool, itemControls.descriptorSetLayout);
			if (!state) {
				return UID::Empty();
			}
		}


		{
			bool state = objectPtr->textureHandle.init(textureName, impl->device, impl->physicalDevice, impl->queue, itemControls.commandPool, itemControls.descriptorSetLayout);
			if (!state) {
				return UID::Empty();
			}
		}


		std::cout << "Finish uploading ubo to thing " << thisId << std::endl;

		updateThing(UID::Create(thisId), point);

		idsToPoints[UID::Create(thisId)] = point;

		return UID::Create(thisId);
	}

	UID ThingManager::addTile(Point point, float scale, const Color& color)
	{


		//point.z = 0;

		//float scale = 5;
		const std::vector<GE::Vertex> vertices = {
			{{-scale, -scale,0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
			{{scale, -scale,0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
			{{scale, scale,0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
			{{-scale, scale,0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
			
			{{-scale, -scale, 0}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
			{{-scale, scale, 0}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
			{{-scale, scale, scale}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
			{{-scale, -scale, scale}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},





		};
		const std::vector<uint32_t> indices = {
			0, 1, 2, 2, 3, 0
			//,
			//4, 5, 6, 6, 7, 4
		};




		GE::TextureMetaData texture;
		std::vector<unsigned char> colorD;// = { 0x00, 0xFF,0x00,0xFF };
		color.attachTo(colorD);
		texture.imageData = colorD.data();
		texture.pictureHeight = 1;
		texture.pictureWidth = 1;
		texture.pixelSize = 4;


		size_t indexPicked = 2;
		auto & idsMappings = GE::PipelinesIdMapping::getInstance();
		for (auto& metaData : idsMappings.getMetadataList())
		{
			if (metaData.second.pipelineName == "default")
			{
				indexPicked = metaData.first;
				break;
			}
		}

		std::string objectName = "models/viking_room.obj";
		uint64_t thisId = impl->controller->createObject(indexPicked);
		auto objectPtr = impl->controller->retrieveObject(thisId);
		{
			bool state = objectPtr->verticesHandle.init(vertices, indices, impl->device, impl->physicalDevice, impl->queue, impl->commandPool, impl->descriptorSetLayout);
			if (!state) {
				return UID::Empty();
			}
		}
		{
			bool state = objectPtr->textureHandle.init(texture, impl->device, impl->physicalDevice, impl->queue, impl->commandPool, impl->descriptorSetLayout);
			if (!state) {
				return UID::Empty();
			}
		}


		std::cout << "Finish uploading ubo to thing " << thisId << std::endl;



		if (skyId() == 0) {
			skyId = UID::Create(thisId);
		}


		updateThing(UID::Create(thisId), point);

		idsToPoints[UID::Create(thisId)] = point;

		return UID::Create(thisId);


	}

	void ThingManager::updateThing(UID id, Point point)
	{
		glm::vec3 cameraUp = glm::vec3(0.0f, 0.0f, 1.0f);
		auto cameraPoint = camera->getPosition();

		GE::UniformBufferObject ubo{};
		//ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), point3);
		ubo.model = glm::translate(glm::mat4(1.0f), point);
		if (id == skyId)
		{
			ubo.model = glm::rotate(ubo.model, glm::radians(180.0f), {1,0,0});
		}

		ubo.view = glm::lookAt(cameraPoint, cameraPoint + camera->getRotation(), cameraUp);
		ubo.proj = glm::perspective(camera->getFov(), 1.5f, 0.1f, 100.0f);
		ubo.proj[1][1] *= -1;

		impl->controller->retrieveObject(id())->setUBO(ubo);
	}

	void ThingManager::updateAll()
	{
		for (auto& [id, point] : idsToPoints) updateThing(id, point);
	}

	Camera& ThingManager::getCamera()
	{
		return *(camera.get());
	}

	//void ThingManager::setCamera(Point point, Rotation rotation)
	//{
	//	cameraPoint = point;
	//	//cameraRotation = rotation;
	//}
	//void ThingManager::rotateCamera(float x1, float x2, float y1, float y2)
	//{
	//	float xOffset = x1 - x2;
	//	float yOffset = y1 - y2;

	//	yaw += xOffset;
	//	pitch += yOffset;

	//	if (pitch > 89.0f)
	//		pitch = 89.0f;
	//	if (pitch < -89.0f)
	//		pitch = -89.0f;


	//	glm::vec3 direction;
	//	direction.y = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	//	direction.z = -sin(glm::radians(pitch));
	//	direction.x = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	//	cameraRotation = glm::normalize(direction);

	//	//cameraRotation.
	//}

	//void ThingManager::moveCamera(Point point)
	//{
	//	cameraPoint += point;
	//}

}