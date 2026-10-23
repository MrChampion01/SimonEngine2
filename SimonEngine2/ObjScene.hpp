#ifndef OBJ_SCENE_HPP
#define OBJ_SCENE_HPP

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
#include "AxisDisplay.hpp"
#include "ObjMesh.hpp"

class ObjScene : public Scene {
	public:
		ObjScene() : Scene(244, 244, 10) {}

		~ObjScene() {}

		virtual void init() override final;

		virtual void draw() const override final {}

		virtual void update() override final;

		virtual void deInit() override final;

	private:
		ObjMesh* getObjMesh();
		ObjMesh* objMesh_ = nullptr;
};

#endif