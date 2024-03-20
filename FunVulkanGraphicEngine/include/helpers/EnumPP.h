#pragma once

#include <algorithm>
#include <cctype>
#include <map>
#include <ostream>
#include <optional>
#include <string>
#include <type_traits>

namespace EnumPP {


	template<class T, typename = typename std::enable_if<std::is_integral<T>::value >::type>
	inline static const T NaN = std::numeric_limits<T>::max();

	template<class E, typename = typename std::enable_if<std::is_enum<E>::value>::type>
	using EType = std::underlying_type_t<E>;


	template<class E, typename = typename std::enable_if<std::is_enum<E>::value>::type>
	inline const std::map<E, std::string_view>& EnumMapping();

	template<class E, typename = typename std::enable_if<std::is_enum<E>::value>::type>
	inline std::string toString(E e);

	template<class E, typename = typename std::enable_if<std::is_enum<E>::value>::type>
	inline std::optional<E> toEnum(std::string_view sv);

	/// @brief Same as toEnum, but is case sensitive
	template<class E, typename = typename std::enable_if<std::is_enum<E>::value>::type>
	inline std::optional<E> toEnumCS(std::string_view sv);

	template< class E, typename T = std::underlying_type_t<E>, typename = typename std::enable_if<std::is_enum<E>::value>::type>
	T toVal(E e);


	/// @brief use this with flags to express intent.
	namespace Logic 
	{
		template<typename T, class E, typename = typename std::enable_if<std::is_enum<E>::value>::type>
		bool Contains(T inValue, E e);

		template<typename T, class E, typename = typename std::enable_if<std::is_enum<E>::value>::type>
		void Or(T& inValue, E e);

		template<typename T, class E, typename = typename std::enable_if<std::is_enum<E>::value>::type>
		void And(T& inValue, E e);

		template<typename T, class E, typename = typename std::enable_if<std::is_enum<E>::value>::type>
		void XOr(T& inValue, E e);
	}
}

/// For Debugging purposes
namespace std
{
	template<class E, typename = typename std::enable_if<std::is_enum<E>::value>::type>
	inline ostream& operator<<(ostream& os, const std::optional<E>& obj) {
		if (obj.has_value())os << EnumPP::toString(*obj);//static_cast<std::underlying_type_t<E>>
		return os;
	}
	template<class E, typename = typename std::enable_if<std::is_enum<E>::value>::type>
	inline ostream& operator<<(ostream& os, const E& obj) {
		os << EnumPP::toString(obj);
		return os;
	}

}


namespace EnumPP {

	template<class E, typename>
	inline const std::map<E, std::string_view>& EnumMapping() {
		static std::map<E, std::string_view> map;
		return std::map<E, std::string_view>();
	}

	template<class E, typename>
	inline std::string toString(E e) {
		const auto& map = EnumMapping<E>();
		if (auto it = map.find(e); it != map.end()) { return std::string(it->second); }
		return "";
	}

	template<class E, typename >
	inline std::optional<E> toEnum(std::string_view sv)
	{
		std::string s(sv);
		std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::toupper(c); });

		static auto UpperCaseMap = []() ->  std::map<E, std::string_view> { // Save processing with multiple calls
			auto copyMap = EnumMapping<E>();
			for (auto it = copyMap.begin(); it != copyMap.end(); it++)
			{
				std::string* stringVal = new std::string(it->second); // Must be a pointer within the heap, otherwise it be destroyed.
				std::transform(stringVal->begin(), stringVal->end(), stringVal->begin(), [](unsigned char c) { return std::toupper(c); });
				it->second = *stringVal;
			}
			return copyMap;
			}();

			for (const auto& [e, s2] : UpperCaseMap)
			{
				if (s == s2) { return e; }
			}
			return std::optional<E>();
	}

	template<class E, typename>
	inline std::optional<E> toEnumCS(std::string_view sv)
	{
		for (const auto& [e, s] : EnumMapping<E>())
		{
			if (s == sv) { return e; }
		}
		return std::optional<E>();
	}

	template< class E, typename T, typename>
	inline T toVal(E e) {
		return static_cast<T>(e);
	}

	namespace Logic
	{
		template<typename T, class E, typename>
		inline bool Contains(T inValue, E e)
		{
			return (inValue & toVal(e)) == toVal(e);
		}

		template<typename T, class E, typename>
		inline void Or(T& inValue, E e)
		{
			inValue |= toVal(e);
		}

		template<typename T, class E, typename>
		inline void And(T& inValue, E e)
		{
			inValue &= toVal(e);
		}

		template<typename T, class E, typename>
		inline void XOr(T& inValue, E e)
		{
			inValue ^= toVal(e);
		}
	}
}
