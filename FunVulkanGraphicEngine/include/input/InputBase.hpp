#pragma once

#include "include/helpers/EnumPP.h"

#include <sstream>
#include <functional>

namespace MGE
{


	enum class InputType : uint16_t {
		/// @brief Represented by ASCII Capital Letters
		Unknown = 0,
		Key = 0x01,
		Mouse = 0x02,
		Scroll = 0x04
	};
	enum class InputAction : uint16_t {
		Unknown = 0,
		Down = 0x01,
		Up = 0x02,
		/// @brief If a key is held down
		Hold = 0x04,
		/// @brief If the mouse moves around
		Move = 0x08
	};



	class Input {
	public:
		Input();

		static Input CreateKeyInput(int key, InputAction action, bool isShift = false, bool isCtrl = false, bool isAlt = false);
		static Input CreateMouseInput(double x, double y, InputAction action);
		static Input CreateScrollInput(double xAxis, double yAxis);

		InputType getType()const;
		InputAction getAction()const;
		double getXPos()const;
		double getYPos()const;
		int getKey()const;

		bool isShifted()const;
		bool isCtrl() const;
		bool isAlt() const;


		void setType(InputType);
		void setAction(InputAction);
		void setKey(int key);
		void setXY(double x, double y);
		void setShift(bool = true);
		void setCtrl(bool = true);
		void setAlt(bool = true);

	private:
		InputType type;
		InputAction action;
		/// @brief represents either x,y pos or the x and y axis with scrolling
		double xyData[2];
		int keyCode;
		int keyMods;
	};




	using InputCallback = std::function<void(const Input&)>;






	/// @brief These values should be 1 to 1 with the GLFW library.
	/// Duplicating the work just incase if I decide to add compatibility with other multi media libraries.
	namespace KeyCodes 
	{
		constexpr int MouseLeft = 0;
		constexpr int MouseRight = 1;
		constexpr int MouseMiddle = 2;
		constexpr int MouseExtra1 = 3;
		constexpr int MouseExtra2 = 4;
		constexpr int MouseExtra3 = 5;
		constexpr int MouseExtra4 = 6;
		constexpr int MouseExtra5 = 7;

		constexpr int Escape = 256;
		constexpr int Enter = 257;
		constexpr int Tab = 258;
		constexpr int Backspace = 259;
		constexpr int Insert = 260;
		constexpr int Delete = 261;
		constexpr int Right = 262;
		constexpr int Left = 263;
		constexpr int Down = 264;
		constexpr int Up = 265;
		constexpr int PageUp = 266;
		constexpr int PageDown = 267;
		constexpr int Home = 268;
		constexpr int End = 269;
		constexpr int CapsLock = 270;
		constexpr int ScrollLock = 271;
		constexpr int NumLock = 273;
		constexpr int PrintScreen = 273;
		constexpr int Pause = 274;

		constexpr int LeftShift = 340;
		constexpr int LeftControl = 341;
		constexpr int LeftAlt = 342;
		constexpr int LeftSuper = 343;
		constexpr int RightShift = 344;
		constexpr int RightControl = 345;
		constexpr int RightAlt = 346;
		constexpr int RightSuper = 347;
		constexpr int RightMenu = 348;
	}


}


namespace EnumPP {
	template<> const std::map<MGE::InputType, std::string_view>& EnumMapping();
	template<> const std::map<MGE::InputAction, std::string_view>& EnumMapping();
}

namespace std
{
	std::ostream& operator<<(std::ostream& os, const MGE::Input& input);
}