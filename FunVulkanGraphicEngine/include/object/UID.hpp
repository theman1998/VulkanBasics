#pragma once

#include "include/MyGraphicsEngine.hpp"
#include <cinttypes>
#include <unordered_set>

namespace MGE
{

	MGE_API class UID
	{
		uint64_t id{ 0 };
		inline UID() = default;
	public:
		inline uint64_t operator()() const { return id; }
		inline bool operator==(const UID& rhs) const { return this->id == rhs.id; }

		static UID Create(uint64_t id);
		static UID Empty();


	};
	inline UID UID::Create(uint64_t id) { UID uid; uid.id = id; return uid; }
	inline UID UID::Empty() { return UID(); }
}





template<> struct std::hash<MGE::UID>
{
	std::size_t operator()(const MGE::UID& s) const noexcept
	{
		return std::hash<uint64_t>{}(s());
	}
};