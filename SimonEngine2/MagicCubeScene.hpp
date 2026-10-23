#ifndef MAGIC_CUBE_SCENE_HPP
#define MAGIC_CUBE_SCENE_HPP

// Needed C++ Includes
#include <string>
#include <memory>

#include <iostream>

// Scene Interface
#include "Scene.hpp"

// Other SimonEngine includes
#include "InputManager.hpp"
#include "EngineUtil.hpp"

// Objects
#include "MagicCube.hpp"
#include "AxisDisplay.hpp"

class MagicCubeScene : public Scene {
	public:
		MagicCubeScene() : Scene(244, 244, 10) {}

		~MagicCubeScene() {}

		virtual void init() override final;

		virtual void draw() const override final {}

		virtual void update() override final;

		virtual void deInit() override final;

	private:
		MagicCube* getMagicCube();
		MagicCube* magicCube_ = nullptr;
};

#endif