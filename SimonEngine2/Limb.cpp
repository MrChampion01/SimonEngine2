#include "Limb.hpp"

Limb::Limb(const EngineMath::Transform& localTransform, EngineUtil::ColorRGB color) {
	localTransform_ = localTransform;
	color_ = color; 
}

void Limb::onStart(){
    EngineMath::Transform lower(
        EngineMath::Vector3(0.0f, -0.25f, 0.0f),
        EngineMath::Quaternion::identity(),
        EngineMath::Vector3(1.0f, 0.5f, 1.0f)
    );

    EngineMath::Transform upper(
        EngineMath::Vector3(0.0f, 0.25f, 0.0f),
        EngineMath::Quaternion::identity(),
        EngineMath::Vector3(1.0f, 0.5f, 1.0f)
    );

    auto lowerPart = std::make_unique<RectangularPrism>(lower, color_);
    auto upperPart = std::make_unique<RectangularPrism>(upper, color_);

    lowerHalf_ = lowerPart.get();
    upperHalf_ = upperPart.get();

    addChild(std::move(lowerPart));
    addChild(std::move(upperPart));

    middlePivot_ = EngineMath::Vector3(0.0f, 0.0f, 0.0f);
    highPivot_ = EngineMath::Vector3(0.0f, 0.5f, 0.0f);
}

void Limb::setColor(EngineUtil::ColorRGB color) {
    color_ = color;
	if (lowerHalf_) {
		lowerHalf_->setColor(color);
	}
	if (upperHalf_) {
		upperHalf_->setColor(color);
	}
}

void Limb::pivotHigh(float radians) {
    EngineMath::Transform lt = getLocalTransform();
    lt.rotateAround(highPivot_, EngineMath::Vector3(0, 0, 1), radians);
    setLocalTransform(lt);
}

void Limb::pivotMiddle(float radians) {
    if (!lowerHalf_) {
        return;
    }
    EngineMath::Transform lt = lowerHalf_->getLocalTransform();
    lt.rotateAround(middlePivot_, EngineMath::Vector3(0, 0, 1), radians);
    lowerHalf_->setLocalTransform(lt);
}