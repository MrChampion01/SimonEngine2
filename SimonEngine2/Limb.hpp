#ifndef LIMB_HPP
#define LIMB_HPP

#include "Object.hpp"
#include "EngineMath.hpp"
#include "EngineUtil.hpp"
#include "Primitive.hpp"
#include "RectangularPrism.hpp"

class Limb : public Object {
	public:
		Limb() = default;
		Limb(const EngineMath::Transform& localTransform, EngineUtil::ColorRGB color);

		void onStart() override final;
		void update() override final {}

		void setColor(EngineUtil::ColorRGB color);

		void pivotHigh(float radians);
		void pivotMiddle(float radians);

	private:
		Primitive* lowerHalf_ = nullptr; // Forearm / Calf
		Primitive* upperHalf_ = nullptr; // Bicep / Thigh
		EngineMath::Vector3 middlePivot_; // Elbow / Knee
		EngineMath::Vector3 highPivot_;	// Shoulder / Pelvis
		EngineUtil::ColorRGB color_;
};

#endif 