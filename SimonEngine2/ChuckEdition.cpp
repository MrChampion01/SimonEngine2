#include "ChuckEdition.hpp"

#include <GL/freeglut.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>
#include <iomanip>

#include "Engine.hpp"
#include "Scene.hpp"
#include "InputManager.hpp"
#include "Render2D.hpp"
#include "StudioLighting.hpp"
#include "Backdrop.hpp"
#include "NeonWorld.hpp"

#include "RobotScene.hpp"
#include "ObjScene.hpp"
#include "MagicCubeScene.hpp"

namespace {
	// How long a theme crossfade takes, in seconds. Fast enough to feel snappy,
	// slow enough that you SEE the whole engine melt from one palette into the next.
	constexpr float themeFadeSeconds = 0.6f;

	// FPS is averaged over windows of this many seconds so the number is readable
	// instead of flickering like a slot machine.
	constexpr float fpsWindowSeconds = 0.5f;

	// The original window title, kept so the homework grader still knows whose engine this is
	const char* const homeworkSignature = "(Simon Halaszi) (811196947)";

	float smoothstep01(float t) {
		t = std::min(std::max(t, 0.0f), 1.0f);
		return t * t * (3.0f - 2.0f * t);
	}
}

// ============================================================================ boot

void ChuckEdition::init() {
	printConsoleBanner();
	updateWindowTitle(nullptr);
}

void ChuckEdition::printConsoleBanner() const {
	// Plain escaped literals (not a raw string) so no compiler ever mistakes the
	// trailing backslashes in the art for line continuations.
	static const char* const banner[] = {
		"  ____  _                       _____             _            ____",
		" / ___|(_)_ __ ___   ___  _ __ | ____|_ __   __ _(_)_ __   ___|___ \\",
		" \\___ \\| | '_ ` _ \\ / _ \\| '_ \\|  _| | '_ \\ / _` | | '_ \\ / _ \\ __) |",
		"  ___) | | | | | | | (_) | | | | |___| | | | (_| | | | | |  __// __/",
		" |____/|_|_| |_| |_|\\___/|_| |_|_____|_| |_|\\__, |_|_| |_|\\___|_____|",
		"                                            |___/",
		"              >>>  C H U C K   S P E C I A L   E D I T I O N  <<<",
		"                      \"SimonEngine2, but better.\"  v2.0.69-PRO",
	};

	std::cout << "\n";
	for (const char* line : banner) {
		std::cout << line << "\n";
	}
	std::cout << "\n";

	std::cout << "  [ OK ] RGB subsystem online (" << GamerTheme::themeCount() << " themes loaded)\n";
	std::cout << "  [ OK ] Frame pacer overclocked\n";
	std::cout << "  [ OK ] Gamer posture calibrated\n";
	std::cout << "  [ OK ] Homework mode preserved: " << homeworkSignature << "\n\n";
	std::cout << "  F2 = cycle RGB theme   |   F7 = ray tracing   |   ESC = rage quit\n\n";
}

// ============================================================================ per-tick

void ChuckEdition::update(Scene* scene) {
	InputManager& input = InputManager::getInstance();

	if (input.isSpecialKeyPressed(GLUT_KEY_F2)) {
		cycleTheme();
	}
	if (input.isSpecialKeyPressed(GLUT_KEY_F10)) {
		sceneryEnabled_ = !sceneryEnabled_;
	}
	if (input.isSpecialKeyPressed(GLUT_KEY_F6)) {
		outlinesEnabled_ = !outlinesEnabled_;
	}
	if (input.isSpecialKeyPressed(GLUT_KEY_F7)) {
		lightingEnabled_ = !lightingEnabled_;
		std::cout << "  [RTX] Ray tracing " << (lightingEnabled_ ? "ON (trust me bro)" : "OFF (console mode)") << "\n";
	}
}

void ChuckEdition::cycleTheme() {
	// Start the crossfade FROM whatever is on screen right now (even mid-fade),
	// so mashing F2 never causes a visible jump.
	fromTheme_ = palette_;
	themeIndex_ = (themeIndex_ + 1) % GamerTheme::themeCount();
	themeSwitchTime_ = time();

	std::cout << "  [RGB] Theme -> " << GamerTheme::theme(themeIndex_).name << "\n";
}

// ============================================================================ per-frame

float ChuckEdition::time() const {
	const std::chrono::duration<float> elapsed = std::chrono::steady_clock::now() - bootTime_;
	return elapsed.count();
}

void ChuckEdition::beginFrame() {
	const float now = time();

	// ---- frame timing
	frameMs_ = (now - lastFrameTime_) * 1000.0f;
	lastFrameTime_ = now;

	++framesInWindow_;
	if (now - fpsWindowStart_ >= fpsWindowSeconds) {
		fps_ = framesInWindow_ / (now - fpsWindowStart_);
		framesInWindow_ = 0;
		fpsWindowStart_ = now;
	}

	// ---- resolve this frame's palette: crossfade between themes, then apply rainbow
	const float fade = smoothstep01((now - themeSwitchTime_) / themeFadeSeconds);
	const GamerTheme::Theme blended = GamerTheme::blend(fromTheme_, GamerTheme::theme(themeIndex_), fade);
	palette_ = GamerTheme::resolve(blended, now);

	// ---- the window title doubles as a live FPS readout (every pro streamer needs one)
	if (now - lastTitleUpdate_ >= fpsWindowSeconds) {
		lastTitleUpdate_ = now;
		updateWindowTitle(Engine::getInstance().getCurrentScene());
	}
}

void ChuckEdition::updateWindowTitle(const Scene* scene) {
	std::ostringstream title;
	title << "SimonEngine2 :: CHUCK SPECIAL EDITION";
	if (scene) {
		title << "  |  " << static_cast<int>(std::lround(fps_)) << " FPS  |  " << mapName(*scene);
	}
	title << "  |  " << homeworkSignature;
	glutSetWindowTitle(title.str().c_str());
}

void ChuckEdition::drawBackdrop() {
	if (sceneryEnabled_) {
		Backdrop::draw(palette_, time());
	}
}

void ChuckEdition::beginWorld(const Scene& scene) {
	// Everything the world pass changes gets rolled back in endWorld(), so the
	// next thing drawn (the HUD) starts from exactly the state the engine expects
	glPushAttrib(GL_ALL_ATTRIB_BITS);

	if (lightingEnabled_) {
		StudioLighting::apply(palette_);
	}
	else {
		StudioLighting::disable();
	}
}

void ChuckEdition::endWorld(const Scene& scene) {
	if (sceneryEnabled_) {
		float floorY = 0.0f;
		float padRadius = 1.0f;
		floorFor(scene, floorY, padRadius);
		NeonWorld::drawFloor(palette_, time(), floorY, padRadius);
	}

	glPopAttrib();
}

int ChuckEdition::outlinePassCount() const {
	return outlinesEnabled_ ? NeonWorld::outlinePassCount() : 0;
}

void ChuckEdition::beginOutlinePass(int pass) {
	NeonWorld::beginOutlinePass(palette_, pass);
}

void ChuckEdition::endOutlinePass() {
	NeonWorld::endOutlinePass();
}

void ChuckEdition::drawOverlay(const Scene& scene) {
	Render2D::begin();

	const GamerTheme::Theme& p = palette_;
	const float w = static_cast<float>(Render2D::width());
	const float h = static_cast<float>(Render2D::height());

	// A handful of big blurry embers floating IN FRONT of the robot = instant depth
	if (sceneryEnabled_) {
		Backdrop::drawForegroundEmbers(p, time());
	}

	// Watermark, bottom-right. Every legit special edition needs one.
	const std::string tag = "CHUCK SPECIAL EDITION";
	const float tagW = Render2D::strokeTextWidth(tag, 11.0f, 2.0f);
	Render2D::strokeTextNeon(w - tagW - 14.0f, h - 14.0f, tag, 11.0f, p.accent, 0.85f, 2.0f);

	Render2D::end();
}

// ============================================================================ helpers

std::string ChuckEdition::mapName(const Scene& scene) {
	if (dynamic_cast<const RobotScene*>(&scene)) {
		return "de_robot";
	}
	if (dynamic_cast<const ObjScene*>(&scene)) {
		return "de_teapot";
	}
	if (dynamic_cast<const MagicCubeScene*>(&scene)) {
		return "de_magiccube";
	}
	return "de_unknown";
}

void ChuckEdition::floorFor(const Scene& scene, float& floorY, float& padRadius) {
	// Hand-measured so every model stands exactly ON the neon floor:
	//   robot:  legs are 1.5 tall, hanging below a 1.5 tall torso centered on 0 -> -2.25
	//   teapot: the .obj's lowest vertex is y = 0
	//   cube:   spins, so give it a generous gap underneath
	if (dynamic_cast<const ObjScene*>(&scene)) {
		floorY = -0.01f;
		padRadius = 1.05f;
	}
	else if (dynamic_cast<const MagicCubeScene*>(&scene)) {
		floorY = -2.0f;
		padRadius = 1.35f;
	}
	else {
		floorY = -2.26f;
		padRadius = 1.15f;
	}
}
