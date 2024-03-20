#pragma once

#include "include/MyGraphicsEngine.hpp"
#include <mutex>
#include <limits>
#include <memory>

#include <glm/ext/vector_float3.hpp>

namespace MGE
{
	//using Degree = double;
	//using Meter = double;
	//template <typename Unit>
	//static constexpr Unit MyNAN = std::numeric_limits<Unit>::max();
	//MGE_API struct Point {
	//	Meter x{ MyNAN<Meter> };
	//	Meter y{ MyNAN<Meter> };
	//	Meter z{ MyNAN<Meter> };
	//	inline Point & operator+=(const Point& rhs) {
	//		x += rhs.x;
	//		y += rhs.y;
	//		z += rhs.z;
	//		return *this;
	//	}
	//};


	using Point = glm::vec3;
	using Rotation = glm::vec3;
	using Radians = float;


	//MGE_API struct Rotation {
	//	Degree gamma{ MyNAN<Degree> }; // x axis
	//	Degree beta{ MyNAN<Degree> }; // y axis
	//	Degree alpha{ MyNAN<Degree> }; // z axis
	//	inline Rotation& operator+=(const Rotation& rhs) {
	//		gamma += rhs.gamma;
	//		beta += rhs.beta;
	//		alpha += rhs.alpha;
	//		return *this;
	//	}
	//};


	//class VisualsImpl;
	//MGE_API struct Visuals{
	//public:
	//	std::shared_ptr<>
	//};


	MGE_API class ThingBase {

	public:
		virtual ~ThingBase() = default;


		virtual Point getPosition() const = 0;
		virtual Rotation getRotation() const = 0;

	};


	MGE_API class Thing : public ThingBase {
	public:
		Thing();


		Point getPosition() const override;
		Rotation getRotation() const override;

		void setPosition(Point);
		void setRotation(Rotation);
		void addToPosition(Point);
		void addToRotation(Rotation);

	private:
		Point position;
		Rotation orientation;

		mutable std::mutex mutex;
	};






}