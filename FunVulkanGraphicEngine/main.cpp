#include "public/StandardInclude.hpp"

#include "GraphicEngine/Validation.hpp"
#include "GraphicEngine/GraphicsCore.hpp"

#include <iostream>
#include <thread>




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
	0, 1, 2, 2, 3, 0
	//,
	//4, 5, 6, 6, 7, 4
};



int main() {







	GE::GraphicsCore core;
	if (auto message = core.init(); !message.empty()) {
		std::cout << "init message status: " << message << std::endl;
		return 1;
	}
	if (auto message = core.loadInTexture("Viking","models/viking_room.obj", "textures/viking_room.png"); !message.empty()) {
		std::cout << "loadInFile: " << message << std::endl;
		return 1;
	}
	//if (auto message = core.loadInTexture("Viking2","models/viking_room.obj", "textures/map_01.png"); !message.empty()) {
	//	std::cout << "loadInFile: " << message << std::endl;
	//	return 1;
	//}

	if (auto message = core.loadInTexture("Viking3", vertices, indices, "textures/map_01.png"); !message.empty()) {
		std::cout << "loadInFile: " << message << std::endl;
		return 1;
	}
	
	
	
	core.mainLoop();



	std::cout << "Programming is exiting!" << std::endl;
	return 0;
}
