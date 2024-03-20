#include "include/StandardInclude.hpp"

#include "GraphicEngine/Validation.hpp"

#include <iostream>
#include <thread>


#include "include/object/ThingBase.hpp"
#include "include/object/UID.hpp"


#include "GraphicEngine/GraphicsCorePIMPL.hpp"

#include "include/GraphicsCore.hpp"

#include "include/object/ThingManager.hpp"
#include "include/object/Camera.hpp"


#include "GraphicEngine/PipelinesIdMapping.hpp"

// Green Channels represent hotizontal coordinates and red vertical coordinates.
// Yellow and Black corners represent interpolated on the corners.
const std::vector<GE::Vertex> vertices = {
	{{-0.5f, -0.5f,0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
	{{0.5f, -0.5f,0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
	{{0.5f, 0.5f,0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
	{{-0.5f, 0.5f,0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
	{{-0.3f, -0.3f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
	{{0.3f, -0.3f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
	{{0.3f, 0.3f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
	{{-0.3f, 0.3f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
};
const std::vector<uint32_t> indices = {
	0, 1, 2, 2, 3, 0,
	0, 1, 2, 2, 3, 0
	//,
	//4, 5, 6, 6, 7, 4
};


int main(int argc, char** argv) {

	GE::PipelinesIdMapping& textureCreater = GE::PipelinesIdMapping::getInstance();
	GE::PipelinesIdMapping::MetaData metaData;
	metaData.pipelineName = "custom1";
	GE::ShaderLoadInfo i1; i1.fileName = "shaders/vert.spv"; i1.name = "main"; i1.type = GE::ShaderType::Vertex;
	GE::ShaderLoadInfo i2; i2.fileName = "shaders/fragAlter.spv"; i2.name = "main"; i2.type = GE::ShaderType::Fragment;
	metaData.shaders.push_back(i1);
	metaData.shaders.push_back(i2);
	textureCreater.appendNewPipelineData(metaData);



	MGE::GraphicsCore core(argc,argv);
	core.init();
	auto thingManager = core.getThingManager();
	auto* ptr = thingManager.get();

	auto& camera = thingManager->getCamera();

	thingManager->addTile({ 0,0,12 }, 500, MGE::Color(0x00,0x00,0xFF) );

	std::vector<MGE::Color> colors = { MGE::Color(0x00,0xFF,0x00),MGE::Color(0x00,0x5F,0x00) };
	
	int colorCounter = 0;
	for (int i = -3; i <= 3; i++)
	{
		const float Radius = 5;
		for (int j = -3; j <= 3; j++)
		{
			thingManager->addTile({ (float)i*Radius*2,(float)j * Radius * 2,0 }, Radius, colors.at(colorCounter++ % colors.size()));
		}
	}

	thingManager->addThing(MGE::Point(1, 0, 0));
	thingManager->addThing(MGE::Point(1, 1, 0));
	thingManager->addThing(MGE::Point(-1, 1, 0));


	core.registerForScroll([ptr, &camera](const MGE::Input& i) {camera.tickFov((float)i.getYPos()); ptr->updateAll(); });

	core.registerForKeyPress([&core](const MGE::Input& i) {if (i.getKey() == MGE::KeyCodes::Escape) { core.getShutdownFlag()->store(true); }});


	core.registerForKeyPress([ptr, &camera](const MGE::Input& i) {
		
		static float velocity = 0;
		
		if (i.getAction() != MGE::InputAction::Down && i.getAction() != MGE::InputAction::Hold)return;

		if (i.getAction() == MGE::InputAction::Down) { velocity = 0.3f; }
		else { velocity = std::min<float>(velocity + 0.1f, 0.6f); }

		MGE::Point p1(0,0,0);

		if ((char)i.getKey() == 'A') { p1.x = -velocity; }
		if ((char)i.getKey() == 'S') { p1.y = -velocity; }
		if ((char)i.getKey() == 'D') { p1.x = velocity; }
		if ((char)i.getKey() == 'W') { p1.y = velocity; }
		if ((char)i.getKey() == 'E') { p1.z = velocity; }
		if ((char)i.getKey() == 'Q') { p1.z = -velocity; }
		
		camera.move(p1);
		ptr->updateAll();
		});

	core.registerForMouseMovement([ptr, &camera](const MGE::Input& i) {

		static bool keyDownFlag = false;

		auto action = i.getAction();
		

		if (!keyDownFlag && (action == MGE::InputAction::Down)) {
			keyDownFlag = true;
		}
		else if (action == MGE::InputAction::Up) { keyDownFlag = false; return; }
		else if(!keyDownFlag) { return; }

		float x1 = (float)i.getXPos();
		//static float x2 = i.getXPos();
		float y1 = (float)i.getYPos();
		
		std::optional<float> x2;
		std::optional<float> y2;

		if (action == MGE::InputAction::Down) {
			x2 = x1;
			y2 = y1;
		}
		
		//static float y2 = i.getYPos();
		camera.rotateBaseOnLastCoord(x1,y1,x2,y2);
		ptr->updateAll();
		//ptr->addThing(MGE::Point(i.getXPos() / 100,i.getYPos()/ 100,0));
		
		//x2 = i.getXPos();
		//y2 = i.getYPos();
		});//std::cout << i << std::endl; });
	core.registerForMouseClick([ptr](const MGE::Input& i) {


		ptr->addThing(MGE::Point(i.getXPos() / 200, i.getYPos() / 200, 0));
		});

	thingManager->updateAll();
	core.run();

	//GE::GraphicsCorePIMPL core;
	//if (auto message = core.init(); !message.empty()) {
	//	std::cout << "init message status: " << message << std::endl;
	//	return 1;
	//}
	//core.regirsterForMouseMovement([](const MGE::Input& i) {std::cout << i << std::endl; });
	//core.mainLoop();



	std::cout << "Programming is exiting!" << std::endl;
	return 0;
}
