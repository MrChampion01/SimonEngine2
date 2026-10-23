#ifndef CHUCK_EDITION_HPP
#define CHUCK_EDITION_HPP

/*
 *    ___ _  _ _   _  ___ _  __  ___ ___ ___ _____ ___ ___  _  _
 *   / __| || | | | |/ __| |/ / | __|   \_ _|_   _|_ _/ _ \| \| |
 *  | (__| __ | |_| | (__| ' <  | _|| |) | |  | |  | | (_) | .` |
 *   \___|_||_|\___/ \___|_|\_\ |___|___/___| |_| |___\___/|_|\_|
 *
 *  SimonEngine2, but better. The CHUCK SPECIAL EDITION mission control.
 *
 *  This singleton is the one place every piece of Chuck Edition eye candy hangs off of.
 *  The original engine only needed a few one-line hooks to support it:
 *
 *     Engine::init()          -> init()          once the window + GL context exist
 *     Engine::updateTimer()   -> update(scene)   every tick, BEFORE the scene updates
 *     GAMEdraw()              -> beginFrame()    top of the display callback
 *                             -> drawBackdrop()  right after glClear, before the 3D scene
 *     Scene::sceneDraw()      -> beginWorld()    right after the camera is applied
 *                             -> endWorld()      after the scene's objects are drawn
 *     GAMEdraw()              -> drawOverlay()   after the scene: HUD, toasts, post FX
 *
 *  The homework logic (scenes, objects, input) is untouched; Chuck Edition only *reads*
 *  input to react to it, and only *consumes* it while the splash screen is up.
 */

#include <chrono>
#include <string>

#include "GamerTheme.hpp"

class Scene;

class ChuckEdition {
	public:
		static ChuckEdition& getInstance() {
			static ChuckEdition instance;
			return instance;
		}

		// ------------------------------------------------------------ engine hooks
		void init();
		void update(Scene* scene);
		void beginFrame();
		void drawBackdrop();
		void beginWorld(const Scene& scene);
		void endWorld(const Scene& scene);
		void drawOverlay(const Scene& scene);

		// ------------------------------------------------------------ shared state
		float time() const; // seconds since boot, high resolution
		const GamerTheme::Theme& palette() const { return palette_; } // resolved for this frame

		float fps() const { return fps_; }
		float frameMs() const { return frameMs_; }

		bool isRayTracingOn() const { return lightingEnabled_; } // (it's studio lighting, shh)

		// "de_robot", "de_teapot", ... (Counter-Strike map naming, obviously)
		static std::string mapName(const Scene& scene);

		ChuckEdition(const ChuckEdition&) = delete;
		ChuckEdition& operator=(const ChuckEdition&) = delete;
		ChuckEdition(const ChuckEdition&&) = delete;
		ChuckEdition& operator=(const ChuckEdition&&) = delete;

	private:
		ChuckEdition() : bootTime_(std::chrono::steady_clock::now()) {}
		~ChuckEdition() {}

		void printConsoleBanner() const;
		void cycleTheme();
		void updateWindowTitle(const Scene* scene);

		std::chrono::steady_clock::time_point bootTime_;

		// ---- theme crossfade
		int themeIndex_ = 0;
		GamerTheme::Theme fromTheme_ = GamerTheme::theme(0);
		float themeSwitchTime_ = -100.0f;
		GamerTheme::Theme palette_ = GamerTheme::theme(0);

		// ---- feature toggles
		bool lightingEnabled_ = true; // F7

		// ---- frame timing (real numbers, measured with a steady clock)
		float lastFrameTime_ = 0.0f;
		float fpsWindowStart_ = 0.0f;
		int framesInWindow_ = 0;
		float fps_ = 0.0f;
		float frameMs_ = 0.0f;
		float lastTitleUpdate_ = -1.0f;
		std::string lastMapName_;
};

#endif
