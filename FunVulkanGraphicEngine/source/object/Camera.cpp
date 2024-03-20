#pragma once


#include "include/object/Camera.hpp"
#include <glm/detail/func_geometric.inl>
#include <glm/detail/func_trigonometric.inl>
#include <cmath>
#include <algorithm>

#include <iostream>	

namespace MGE {

	Camera::Camera() : position({ 0,0,1 }), orientation({ 1,0,0 }), pitch(0), yaw(0), screenCenterPoint({ 0,0 }), lastPlacePoint({0,0}) { }
	
	void Camera::setScreenSize(float width, float height)
	{
		screenCenterPoint[0] = width / 2;
		screenCenterPoint[1] = height / 2;
	}
	
	Point Camera::getPosition() const
	{
		return position;
	}

	Rotation Camera::getRotation() const
	{
		return orientation;
	}
	Radians Camera::getFov()const
	{
		return glm::radians(fov);
	}

	void Camera::move(const Point& p)
	{

		float s = std::sin(glm::radians(yaw));
		float c = std::cos(glm::radians(yaw));

		float x = (c * p.x) + (s * p.y);
		float y = -(s * p.x) + (c * p.y);

		position.x += x;
		position.y += y;
		position.z += p.z;
	}

	void Camera::rotateBaseOnCenter(float xPlane, float yPlane)
	{
		rotate(xPlane,yPlane, screenCenterPoint[0], screenCenterPoint[1]);
	}

	void Camera::tickFov(float amount)
	{
		fov = std::clamp<float>(fov - amount, 20, 80);
	}


	void Camera::rotateBaseOnLastCoord(float xPlane, float yPlane, const std::optional<float>& xStart , const std::optional<float>& yStart)
	{
		if (xStart.has_value()) { lastPlacePoint[0] = *xStart; }
		if (yStart.has_value()) { lastPlacePoint[1] = *yStart; }

		rotate(xPlane, yPlane, lastPlacePoint[0], lastPlacePoint[1]);
		lastPlacePoint[0] = xPlane;
		lastPlacePoint[1] = yPlane;
	}

	void Camera::rotate(float xPlane, float yPlane, float x2Plane, float y2Plane)
	{
		float xOffset = xPlane - x2Plane;
		float yOffset = yPlane - y2Plane;

		constexpr float Sensitive = 0.5;

		xOffset = std::clamp(xOffset, -Sensitive, Sensitive);
		yOffset = std::clamp(yOffset, -Sensitive, Sensitive);

		yaw += xOffset;
		pitch += yOffset;

		if (pitch > 89.0f)
			pitch = 89.0f;
		if (pitch < -89.0f)
			pitch = -89.0f;


		glm::vec3 direction;
		direction.y = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		direction.z = -sin(glm::radians(pitch));
		direction.x = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		orientation = glm::normalize(direction);
		//std::cout << orientation.x << ", " << orientation.y << ", " << orientation.z << std::endl;;
	}

	void Camera::hardSetPosition(const Point& p)
	{
		position = p;
	}
	void Camera::hardSetOrientation(const Point& p )
	{
		orientation = glm::normalize(p);
	}

}