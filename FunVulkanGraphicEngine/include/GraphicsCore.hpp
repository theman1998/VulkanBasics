#pragma once

#include "include/MyGraphicsEngine.hpp"
#include "include/input/InputBase.hpp"

#include <atomic>
#include "thread"


namespace GE
{
	class GraphicsCorePIMPL;
}

namespace MGE
{


	class ThingManager;

	class GraphicsCore
	{
	public:
		GraphicsCore(int argc, char ** argv);
		~GraphicsCore();

		std::string init();


		std::unique_ptr<ThingManager> getThingManager();
		std::atomic<bool>* getShutdownFlag();


		void registerForKeyPress(MGE::InputCallback callback);
		void registerForMouseClick(MGE::InputCallback callback);
		void registerForMouseMovement(MGE::InputCallback callback);
		void registerForScroll(MGE::InputCallback callback);


		/// @brief blocking call that executes the graphics engine
		int run();


	private:
		std::atomic<bool> windowShutdownFlag;
		std::unique_ptr < GE::GraphicsCorePIMPL > core;
	};






}