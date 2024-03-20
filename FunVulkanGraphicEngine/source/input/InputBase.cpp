#include "include/input/InputBase.hpp"

namespace MGE
{


	constexpr int ModShift = 0x0001;
	constexpr int ModControl = 0x0002;
	constexpr int ModAlt = 0x0004;
	constexpr int ModSuper = 0x0008;
	constexpr int ModCapsLock = 0x0010;
	constexpr int ModLock = 0x0020;


	Input::Input() : type(InputType::Unknown), action(InputAction::Unknown), xyData(), keyCode(0), keyMods(0) { xyData[0] = 0; xyData[1] = 0; }


	Input Input::CreateKeyInput(int key, InputAction action, bool isShift, bool isCtrl, bool isAlt)
	{
		Input input;
		input.action = action;
		input.type = InputType::Key;
		input.keyCode = key;
		int specialCommand = 0;// (int)isShift + ((int)isShift << 1) + ((int)isAlt << 2);
		if (isShift) specialCommand |= ModShift;
		if (isCtrl) specialCommand |= ModControl;
		if (isAlt) specialCommand |= ModAlt;
		input.keyMods = specialCommand;
		return input;
	}

	Input Input::CreateMouseInput(double x, double y, InputAction action)
	{
		Input input;
		input.action = action;
		input.type = InputType::Mouse;
		input.xyData[0] = x;
		input.xyData[1] = y;
		return input;
	}

	Input Input::CreateScrollInput(double xAxis, double yAxis)
	{
		Input input;
		input.action = InputAction::Move;
		input.type = InputType::Scroll;
		input.xyData[0] = xAxis;
		input.xyData[1] = yAxis;
		return input;
	}


	InputType Input::getType()const
	{
		return type;
	}
	InputAction Input::getAction()const
	{
		return action;
	}
	int Input::getKey()const
	{
		return keyCode;
	}
	double Input::getXPos()const
	{
		return xyData[0];
	}
	double Input::getYPos()const
	{
		return xyData[1];
	}
	bool Input::isShifted()const { return (keyMods & ModShift) ? true : false; }
	bool Input::isCtrl() const { return (keyMods & ModControl) ? true : false; }
	bool Input::isAlt() const { return (keyMods & ModAlt) ? true : false; }


	void Input::setType(InputType d) { type = d; }
	void Input::setAction(InputAction d) { action = d; }
	void Input::setKey(int key) { keyCode = key; }
	void Input::setXY(double x, double y) { xyData[0] = x; xyData[1] = y; }
	void Input::setShift(bool d) {
		if (d) { keyMods |= ModShift;}
		else{ keyMods &= ~ModShift;}
	}
	void Input::setCtrl(bool d)
	{
		if (d) { keyMods |= ModControl; }
		else { keyMods &= ~ModControl; }
	}
	void Input::setAlt(bool d)
	{
		if (d) { keyMods |= ModAlt; }
		else { keyMods &= ~ModAlt; }
	}
	
}












namespace EnumPP {
	template<>
	inline const std::map<MGE::InputType, std::string_view>& EnumMapping()
	{
		using II = MGE::InputType;

		static std::map<MGE::InputType, std::string_view>map = {
		{II::Key, "Key" },{II::Mouse,"Mouse"},{II::Scroll, "Scroll"},{II::Unknown,"Unknown"}
		};
		return map;
	}


	template<>
	inline const std::map<MGE::InputAction, std::string_view>& EnumMapping()
	{

		static std::map<MGE::InputAction, std::string_view>map = {
		{MGE::InputAction::Down, "Down" },{MGE::InputAction::Up, "Up"}, {MGE::InputAction::Hold,"Hold"}, {MGE::InputAction::Move,"Move"},{MGE::InputAction::Unknown,"Unknown"}
		};
		return map;
	}
}



namespace std
{
	std::ostream& operator<<(std::ostream& os, const MGE::Input& input) {
		os << "Input : {";
		auto type = input.getType();
		os << " type: " << type;
		os << ", action: " << input.getAction();
		if (type == MGE::InputType::Key)
		{
			os << ", key: " << (char)input.getKey();
			os << std::boolalpha;
			os << ", shift: " << input.isShifted();
			os << ", ctrl: " << input.isCtrl();
			os << ", alt: " << input.isAlt();
		}
		else if(type != MGE::InputType::Unknown)
		{
			if (input.getAction() != MGE::InputAction::Move) { os << ", key: " << input.getKey(); }
			os << ", x: " << input.getXPos();
			os << ", y: " << input.getYPos();
		}
		os << "}";
		return os;
	
	}
}