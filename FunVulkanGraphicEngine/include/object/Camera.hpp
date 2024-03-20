#pragma once


#include "include/object/ThingBase.hpp"
#include <array>
#include <optional>

namespace MGE {


	class Camera : public ThingBase
	{
	public:
		Camera();
		Point getPosition() const override;
		Rotation getRotation() const override;
		Radians getFov()const;

		void setScreenSize(float width, float height);
		void move(const Point&);

		void rotateBaseOnCenter(float xPlane, float yPlane);
		void rotateBaseOnLastCoord(float xPlane, float yPlane, const std::optional<float>& xStart = std::optional<float>(), const std::optional<float>& yStart = std::optional<float>());

		void tickFov(float amount);

		void hardSetPosition(const Point&);
		void hardSetOrientation(const Point&);

	private:
		void rotate(float xPlane, float yPlane, float x2Plane, float y2Plane);

		Point position;
		Rotation orientation;

		float pitch{ 0 };
		float yaw{ 0 };
		float fov{ 45 };

		std::array<float, 2> screenCenterPoint;
		std::array<float, 2> lastPlacePoint;
	};

}