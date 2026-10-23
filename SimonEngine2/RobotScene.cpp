#include "RobotScene.hpp"

void RobotScene::init() {
    addRootObject(std::make_unique<Robot>(
        EngineMath::Transform(
            EngineMath::Vector3(0.0f, 0.0f, 0.0f),
            EngineMath::Quaternion::identity(),
            EngineMath::Vector3(1.0f, 1.0f, 1.0f)
        ),
        EngineUtil::ColorRGB(50, 185, 235)
    ));
    addRootObject(std::make_unique<AxisDisplay>(
        EngineMath::Transform(
            EngineMath::Vector3(0.0f, 0.0f, 0.0f),
            EngineMath::Quaternion::identity(),
            EngineMath::Vector3(1.0f, 1.0f, 1.0f)
        )
    ));

    std::unique_ptr<Object>& first = rootObjects_.front();

    camera_.setTarget(first->getWorldTransform().position_);
    camera_.setPosition(EngineMath::Vector3(5.0f, 5.0f, 5.0f));

    robot_ = getRobot();
}

#include "Engine.hpp"
#include "ObjScene.hpp"
#include "MagicCubeScene.hpp"

void RobotScene::update() {
    if (InputManager::getInstance().isPressed(27)) {
        Engine::getInstance().safeExit();
    }
}

void RobotScene::deInit() {
    robot_ = nullptr;
}

Robot* RobotScene::getRobot() {
    for (auto& obj : rootObjects_) {
        Robot* robot = dynamic_cast<Robot*>(obj.get());
        if (robot) {
            return robot;
        }
    }
    return nullptr;
}