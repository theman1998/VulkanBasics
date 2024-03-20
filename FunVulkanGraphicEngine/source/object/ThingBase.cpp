#include "include/object/ThingBase.hpp"



namespace MGE {




	Thing::Thing() : position({ 0.0f,0.0f,0.0f }) {}


	Point Thing::getPosition() const {
		std::lock_guard lock(mutex);
		return position;
	}

	Rotation Thing::getRotation() const {
		std::lock_guard lock(mutex);
		return orientation;
	}

	void Thing::setPosition(Point p)
	{
		std::lock_guard lock(mutex);
		position = p;
	}
	void Thing::setRotation(Rotation r)
	{
		std::lock_guard lock(mutex);
		orientation = r;
	}
	void Thing::addToPosition(Point p)
	{
		std::lock_guard lock(mutex);
		position += p;
	}
	void Thing::addToRotation(Rotation r)
	{
		std::lock_guard lock(mutex);
		orientation += r;
	}







}