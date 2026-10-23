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
#include "Hud.hpp"

#include "RobotScene.hpp"
#include "ObjScene.hpp"
#include "MagicCubeScene.hpp"
#include "ChuckWorldScene.hpp"

namespace {
	// How long a theme crossfade takes, in seconds. Fast enough to feel snappy,
	// slow enough that you SEE the whole engine melt from one palette into the next.
	constexpr float themeFadeSeconds = 0.6f;

	// FPS is averaged over windows of this many seconds so the number is readable
	// instead of flickering like a slot machine.
	constexpr float fpsWindowSeconds = 0.5f;

	// TAB map transition timing: the wipe covers the screen, the map swaps while hidden,
	// then the wipe uncovers the new map
	constexpr float wipeCoverSeconds = 0.32f;
	constexpr float wipeTotalSeconds = 0.80f;
	constexpr int mapCount = 4;

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

	updateMapTransition();

	// TAB = next map (ASCII 9)
	if (scene && input.isPressed(9) && transitionStart_ < 0.0f) {
		startMapTransition(*scene);
	}

	if (input.isSpecialKeyPressed(GLUT_KEY_F2)) {
		cycleTheme();
	}
	if (input.isSpecialKeyPressed(GLUT_KEY_F10)) {
		sceneryEnabled_ = !sceneryEnabled_;
	}
	if (input.isSpecialKeyPressed(GLUT_KEY_F3)) {
		statsVisible_ = !statsVisible_;
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

	frameHistory_[frameHistoryHead_] = frameMs_;
	frameHistoryHead_ = (frameHistoryHead_ + 1) % frameHistorySize;

	++framesInWindow_;
	if (now - fpsWindowStart_ >= fpsWindowSeconds) {
		fps_ = framesInWindow_ / (now - fpsWindowStart_);
		framesInWindow_ = 0;
		fpsWindowStart_ = now;

		// "1% low" = the FPS you'd get if every frame were as slow as the slowest 1%.
		// With 120 samples that's the 2nd-worst frame. Pros quote this one, not the average.
		std::array<float, frameHistorySize> sorted = frameHistory_;
		std::nth_element(sorted.begin(), sorted.begin() + 1, sorted.end(), [](float a, float b) { return a > b; });
		onePercentLow_ = sorted[1] > 0.0f ? 1000.0f / sorted[1] : 0.0f;
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
		Backdrop::draw(palette_, time(), horizonFraction_);
	}
}

void ChuckEdition::beginWorld(const Scene& scene) {
	// Everything the world pass changes gets rolled back in endWorld(), so the
	// next thing drawn (the HUD) starts from exactly the state the engine expects
	glPushAttrib(GL_ALL_ATTRIB_BITS);

	// Grab the camera matrices so the HUD can pin labels to 3D positions later
	glGetDoublev(GL_MODELVIEW_MATRIX, modelview_);
	glGetDoublev(GL_PROJECTION_MATRIX, projection_);
	glGetIntegerv(GL_VIEWPORT, viewport_);
	haveMatrices_ = true;

	// In perspective, the real horizon is where a point infinitely far away, at camera
	// height, lands on screen. Any point at EXACTLY camera height projects onto that same
	// line no matter how far out it is, so probe 50 units ahead (inside the far plane, or
	// worldToScreen rejects it) and park the 2D backdrop's horizon (sun + skyline) there.
	// Now the 3D floor and the 2D sky actually meet.
	// Ortho cameras have no horizon at all, so they keep the default.
	const Camera& camera = scene.getCamera();
	horizonFraction_ = 0.60f;
	if (camera.getProjectionMode() == Camera::ProjectionMode::Perspective) {
		EngineMath::Vector3 flat = camera.getTarget() - camera.getPosition();
		flat.y_ = 0.0f;
		if (flat.lengthSquared() > 1e-6f) {
			const EngineMath::Vector3 distantPoint = camera.getPosition() + flat.normalized() * 50.0f;
			float sx = 0.0f;
			float sy = 0.0f;
			if (worldToScreen(distantPoint, sx, sy) && viewport_[3] > 0) {
				horizonFraction_ = std::min(std::max(sy / viewport_[3], 0.05f), 0.95f);
			}
		}
	}

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
		float extent = 8.0f;
		float spacing = 0.5f;
		floorFor(scene, floorY, padRadius, extent, spacing);
		NeonWorld::drawFloor(palette_, time(), floorY, padRadius, extent, spacing);
	}

	// Map-specific world extras (Chuck World's blob shadows and core light pools)
	if (const ChuckWorldScene* world = dynamic_cast<const ChuckWorldScene*>(&scene)) {
		world->drawWorldExtras();
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

	// ---- the HUD: fill in this frame's numbers, let Hud draw them
	Hud::HudInfo info;
	info.time = time();
	info.reveal = std::min(1.0f, time() / 0.9f);
	info.fps = fps_;
	info.frameMs = frameMs_;
	info.onePercentLowFps = onePercentLow_;
	info.frameHistoryMs = frameHistory_.data();
	info.frameHistorySize = frameHistorySize;
	info.frameHistoryHead = frameHistoryHead_;
	info.mapName = mapName(scene);
	info.themeName = p.name;
	info.cameraMode = scene.getCamera().getProjectionMode() == Camera::ProjectionMode::Perspective ? "PERSP" : "ORTHO";
	info.cameraZoom = static_cast<float>(scene.getCamera().getZoom());
	info.objectCount = static_cast<int>(scene.countObjects());
	info.sessionSeconds = time();
	info.rayTracing = lightingEnabled_;
	info.showStats = statsVisible_;
	Hud::draw(info, p);

	// Chuck World brings its own game HUD (objective, score, nametag, popups...)
	if (const ChuckWorldScene* world = dynamic_cast<const ChuckWorldScene*>(&scene)) {
		world->drawHud(p, time());
	}

	// Watermark, bottom-right. Every legit special edition needs one.
	const std::string tag = "CHUCK SPECIAL EDITION";
	const float tagW = Render2D::strokeTextWidth(tag, 11.0f, 2.0f);
	Render2D::strokeTextNeon(w - tagW - 14.0f, h - 14.0f, tag, 11.0f, p.accent, 0.85f, 2.0f);

	// The TAB loading wipe goes over EVERYTHING
	drawMapTransition(w, h);

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
	if (dynamic_cast<const ChuckWorldScene*>(&scene)) {
		return "de_chuckworld";
	}
	return "de_unknown";
}

bool ChuckEdition::worldToScreen(const EngineMath::Vector3& world, float& x, float& y) const {
	if (!haveMatrices_) {
		return false;
	}

	GLdouble wx = 0.0;
	GLdouble wy = 0.0;
	GLdouble wz = 0.0;
	if (gluProject(world.x_, world.y_, world.z_, modelview_, projection_, viewport_, &wx, &wy, &wz) != GL_TRUE) {
		return false;
	}

	// Window depth outside [0, 1] = behind the camera (or past the far plane)
	if (wz < 0.0 || wz > 1.0) {
		return false;
	}

	// gluProject gives window coordinates with y pointing UP; the HUD's y points DOWN
	x = static_cast<float>(wx);
	y = static_cast<float>(viewport_[3] - wy);
	return true;
}

// ============================================================================ map rotation

int ChuckEdition::mapIndex(const Scene& scene) {
	if (dynamic_cast<const ChuckWorldScene*>(&scene)) {
		return 1;
	}
	if (dynamic_cast<const ObjScene*>(&scene)) {
		return 2;
	}
	if (dynamic_cast<const MagicCubeScene*>(&scene)) {
		return 3;
	}
	return 0; // the homework robot
}

const char* ChuckEdition::mapNameByIndex(int index) {
	static const char* const names[mapCount] = { "de_robot", "de_chuckworld", "de_teapot", "de_magiccube" };
	return names[((index % mapCount) + mapCount) % mapCount];
}

void ChuckEdition::startMapTransition(const Scene& scene) {
	transitionStart_ = time();
	transitionTarget_ = (mapIndex(scene) + 1) % mapCount;
	transitionSwapped_ = false;

	// The stats panels cover half the arena, so tuck them away for the game map (F3 brings them back)
	if (transitionTarget_ == 1) {
		statsVisible_ = false;
	}
	std::cout << "  [MAP] Loading " << mapNameByIndex(transitionTarget_) << "...\n";
}

void ChuckEdition::updateMapTransition() {
	if (transitionStart_ < 0.0f) {
		return;
	}

	const float t = time() - transitionStart_;

	// Swap maps only once the wipe fully covers the screen, so you never see the pop
	if (!transitionSwapped_ && t >= wipeCoverSeconds) {
		transitionSwapped_ = true;
		switch (transitionTarget_) {
			case 1:  Engine::getInstance().changeScene(std::make_unique<ChuckWorldScene>()); break;
			case 2:  Engine::getInstance().changeScene(std::make_unique<ObjScene>()); break;
			case 3:  Engine::getInstance().changeScene(std::make_unique<MagicCubeScene>()); break;
			default: Engine::getInstance().changeScene(std::make_unique<RobotScene>()); break;
		}
	}

	if (t >= wipeTotalSeconds) {
		transitionStart_ = -1.0f;
	}
}

void ChuckEdition::drawMapTransition(float w, float h) const {
	if (transitionStart_ < 0.0f) {
		return;
	}

	const float t = time() - transitionStart_;
	const float skew = h * 0.35f;
	const GamerTheme::Theme& p = palette_;

	// Leading and trailing edge of a slanted panel. Cover: the leading edge sweeps in from
	// the left. Uncover: the trailing edge chases it off to the right.
	float lead = 0.0f;
	float trail = 0.0f;
	if (t < wipeCoverSeconds) {
		lead = smoothstep01(t / wipeCoverSeconds);
		trail = 0.0f;
	}
	else {
		lead = 1.0f;
		trail = smoothstep01((t - wipeCoverSeconds) / (wipeTotalSeconds - wipeCoverSeconds));
	}

	const float span = w + skew * 2.0f;
	const float leadX = -skew + lead * span;
	const float trailX = -skew + trail * span;

	// The panel: a parallelogram leaning right
	const GamerTheme::Rgb panelColor = GamerTheme::scale(p.skyTop, 0.8f);
	glColor4f(panelColor.r, panelColor.g, panelColor.b, 1.0f);
	glBegin(GL_QUADS);
	glVertex2f(trailX - skew, 0.0f);
	glVertex2f(leadX + skew, 0.0f);
	glVertex2f(leadX - skew, h);
	glVertex2f(trailX - skew * 3.0f, h);
	glEnd();

	// A neon stripe riding each edge
	Render2D::blendAdditive();
	glColor4f(p.accent.r, p.accent.g, p.accent.b, 0.9f);
	glBegin(GL_QUADS);
	glVertex2f(leadX + skew, 0.0f);
	glVertex2f(leadX + skew + 10.0f, 0.0f);
	glVertex2f(leadX - skew + 10.0f, h);
	glVertex2f(leadX - skew, h);
	glEnd();
	glColor4f(p.accent2.r, p.accent2.g, p.accent2.b, 0.9f);
	glBegin(GL_QUADS);
	glVertex2f(trailX - skew - 6.0f, 0.0f);
	glVertex2f(trailX - skew, 0.0f);
	glVertex2f(trailX - skew * 3.0f, h);
	glVertex2f(trailX - skew * 3.0f - 6.0f, h);
	glEnd();
	Render2D::blendNormal();

	// While fully covered: LOADING de_whatever, with a spinner
	const float coverAlpha = (t > wipeCoverSeconds * 0.8f && trail < 0.25f) ? 1.0f - trail * 4.0f : 0.0f;
	if (coverAlpha > 0.0f) {
		const std::string loading = "LOADING";
		const std::string map = mapNameByIndex(transitionTarget_);
		const float lw = static_cast<float>(Render2D::textWidth(loading, Render2D::Font::Body));
		const float mw = Render2D::strokeTextWidth(map, 26.0f, 2.0f);
		Render2D::text((w - lw) * 0.5f, h * 0.5f - 30.0f, loading, Render2D::Font::Body, p.accent2, coverAlpha);
		Render2D::strokeTextNeon((w - mw) * 0.5f, h * 0.5f + 8.0f, map, 26.0f, p.accent, coverAlpha, 2.0f);

		for (int i = 0; i < 8; ++i) {
			const float a = time() * 8.0f + i * (2.0f * EngineMath::PI / 8.0f);
			Render2D::rect(w * 0.5f + std::cos(a) * 14.0f - 2.0f, h * 0.5f + 40.0f + std::sin(a) * 14.0f - 2.0f, 4.0f, 4.0f, p.accent, coverAlpha * (0.2f + 0.8f * i / 8.0f));
		}
	}
}

void ChuckEdition::floorFor(const Scene& scene, float& floorY, float& padRadius, float& extent, float& spacing) {
	extent = 8.0f;
	spacing = 0.5f;

	if (dynamic_cast<const ChuckWorldScene*>(&scene)) {
		// Chuck World: real floor at 0, a big arena, chunkier grid, spawn pad at the origin
		floorY = 0.0f;
		padRadius = 1.0f;
		extent = ChuckWorldScene::floorExtent;
		spacing = ChuckWorldScene::floorSpacing;
		return;
	}

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
