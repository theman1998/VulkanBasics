#pragma once

#include "include/MyGraphicsEngine.hpp"
#include <mutex>
#include <limits>
#include <memory>
#include <unordered_map>

#include "include/object/UID.hpp"
#include "include/object/ThingBase.hpp"

namespace GE
{
	struct ThingManagerPIMPL;
}

namespace MGE 
{
	//trivial color structure
	struct Color 
	{
		uint8_t red;
		uint8_t green;
		uint8_t blue;
		uint8_t alpha;

		inline Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 1) :red(r),green(g),blue(b),alpha(a){}

		template<typename T>
		inline void attachTo(std::vector<T>& colorMapping) const{ 
			colorMapping.push_back(static_cast<T>(red)); 
			colorMapping.push_back(static_cast<T>(green)); 
			colorMapping.push_back(static_cast<T>(blue)); 
			colorMapping.push_back(static_cast<T>(alpha)); 
		}

	};

	class ThingBase;
	class GraphicsCore;
	class Camera;

	class ThingManager
	{
		friend GraphicsCore;
	public:
		ThingManager();
		~ThingManager();

		UID addThing(Point point);

		UID addTile(Point point, float radius, const Color& );

		//void setCamera(Point point, Rotation rotation);
		//void rotateCamera(float x1, float x2, float y1, float y2);
		//void moveCamera(Point point);

		void updateThing(UID id, Point point);

		void updateAll();


		Camera& getCamera();

	private:
		std::unique_ptr<GE::ThingManagerPIMPL>impl;

		std::unordered_map<UID, Point> idsToPoints;

		UID skyId{UID::Empty()};

		std::unique_ptr<Camera> camera;

		//Point cameraPoint;
		//float pitch{ 0 };
		//float yaw{ 0 };
		//Rotation cameraRotation;
	};

}