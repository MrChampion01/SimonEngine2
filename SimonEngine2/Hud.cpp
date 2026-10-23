#include "Hud.hpp"

#include <GL/freeglut.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

#include "Render2D.hpp"

namespace Hud {

	using GamerTheme::Rgb;
	using Render2D::Font;

	namespace {
		constexpr float margin = 14.0f;
		const Rgb white = { 1.0f, 1.0f, 1.0f };
		const Rgb black = { 0.0f, 0.0f, 0.0f };

		// Ease-out cubic: fast start, gentle landing. Every slide-in animation uses this.
		float easeOut(float t) {
			t = std::min(std::max(t, 0.0f), 1.0f);
			const float inv = 1.0f - t;
			return 1.0f - inv * inv * inv;
		}

		std::string fixed(float value, int decimals) {
			std::ostringstream out;
			out << std::fixed << std::setprecision(decimals) << value;
			return out.str();
		}

		std::string clock(float seconds) {
			const int total = static_cast<int>(seconds);
			std::ostringstream out;
			out << std::setfill('0') << std::setw(2) << total / 3600 << ":"
				<< std::setw(2) << (total / 60) % 60 << ":" << std::setw(2) << total % 60;
			return out.str();
		}

		// Spreads a string out letter by letter: "CHUCK" -> "C H U C K".
		// Bitmap fonts can't letter-space, so we cheat with literal spaces.
		std::string spaced(const std::string& s) {
			std::string out;
			for (std::size_t i = 0; i < s.size(); ++i) {
				out += s[i];
				if (i + 1 < s.size()) {
					out += ' ';
				}
			}
			return out;
		}

		// The standard Chuck Edition panel: dark glass, chamfered corners, neon rim,
		// and a little title tab sticking out of the top-left
		void panel(float x, float y, float w, float h, const std::string& title, const GamerTheme::Theme& p, float alpha) {
			Render2D::chamferBox(x, y, w, h, 10.0f, GamerTheme::scale(p.skyTop, 0.6f), 0.62f * alpha);
			Render2D::chamferOutline(x, y, w, h, 10.0f, p.accent, 0.55f * alpha);

			if (!title.empty()) {
				const float tabW = static_cast<float>(Render2D::textWidth(title, Font::Tiny)) + 16.0f;
				Render2D::chamferBox(x, y - 12.0f, tabW, 13.0f, 5.0f, p.accent, 0.85f * alpha);
				Render2D::text(x + 8.0f, y - 2.0f, title, Font::Tiny, black, alpha);
			}
		}

		// One "LABEL ........ value" line in a stats panel
		void statRow(float x, float y, float w, const std::string& label, const std::string& value, const Rgb& valueColor, float alpha) {
			Render2D::text(x, y, label, Font::MonoSmall, white, 0.55f * alpha);
			const float vw = static_cast<float>(Render2D::textWidth(value, Font::MonoSmall));
			Render2D::text(x + w - vw, y, value, Font::MonoSmall, valueColor, alpha);
		}

		// ------------------------------------------------------------------ top-left
		void drawLogo(const HudInfo& info, const GamerTheme::Theme& p) {
			const float slide = (1.0f - easeOut(info.reveal)) * -260.0f;
			const float x = margin + slide;

			// The big glowing logo, with a slow "neon flicker" every few seconds
			const float flicker = (std::fmod(info.time, 7.3f) < 0.12f) ? 0.45f : 1.0f;
			Render2D::strokeTextNeon(x + 2.0f, 34.0f, "SIMONENGINE2", 18.0f, p.accent, flicker, 1.5f);

			Render2D::text(x + 3.0f, 50.0f, spaced("CHUCK SPECIAL EDITION"), Font::Tiny, p.accent2, 0.95f);

			// version pill
			const std::string version = "v2.0.69-PRO";
			const float vx = x + 3.0f + Render2D::textWidth(spaced("CHUCK SPECIAL EDITION"), Font::Tiny) + 8.0f;
			const float vw = static_cast<float>(Render2D::textWidth(version, Font::Tiny)) + 10.0f;
			Render2D::chamferBox(vx, 40.0f, vw, 13.0f, 4.0f, p.accent2, 0.85f);
			Render2D::text(vx + 5.0f, 50.0f, version, Font::Tiny, black, 1.0f);
		}

		// ------------------------------------------------------------------ top-right
		void drawPerformance(const HudInfo& info, const GamerTheme::Theme& p, float screenW) {
			const float boxW = 176.0f;
			const float slide = (1.0f - easeOut(info.reveal)) * 260.0f;
			const float x = screenW - margin - boxW + slide;
			const float y = 22.0f;

			panel(x, y, boxW, 104.0f, "PERFORMANCE", p, 1.0f);

			// Big FPS number. Green if smooth, yellow if meh, red if Simon's laptop.
			const Rgb good = { 0.35f, 1.0f, 0.45f };
			const Rgb meh = { 1.0f, 0.85f, 0.2f };
			const Rgb bad = { 1.0f, 0.25f, 0.25f };
			const Rgb fpsColor = info.fps >= 100.0f ? good : (info.fps >= 45.0f ? meh : bad);

			const std::string fpsText = std::to_string(static_cast<int>(std::lround(info.fps)));
			Render2D::strokeTextNeon(x + 12.0f, y + 34.0f, fpsText, 22.0f, fpsColor, 1.0f, 1.0f);
			const float numW = Render2D::strokeTextWidth(fpsText, 22.0f, 1.0f);
			Render2D::text(x + 18.0f + numW, y + 34.0f, "FPS", Font::Body, fpsColor, 0.9f);

			Render2D::text(x + boxW - 66.0f, y + 20.0f, fixed(info.frameMs, 2) + " ms", Font::MonoSmall, white, 0.8f);
			Render2D::text(x + boxW - 66.0f, y + 34.0f, "1%L " + std::to_string(static_cast<int>(info.onePercentLowFps)), Font::MonoSmall, white, 0.55f);

			// ---- frame-time graph: the last N frames as a filled area, newest on the right
			const float gx = x + 10.0f;
			const float gy = y + 44.0f;
			const float gw = boxW - 20.0f;
			const float gh = 34.0f;
			Render2D::rect(gx, gy, gw, gh, black, 0.35f);

			if (info.frameHistoryMs && info.frameHistorySize > 1) {
				float worst = 1.0f;
				for (int i = 0; i < info.frameHistorySize; ++i) {
					worst = std::max(worst, info.frameHistoryMs[i]);
				}
				const float scaleMax = std::max(worst * 1.2f, 20.0f);

				// 60 FPS reference line (16.7 ms) so spikes have something to be judged against
				const float refY = gy + gh - (16.7f / scaleMax) * gh;
				Render2D::line(gx, refY, gx + gw, refY, p.accent2, 0.35f, 1.0f);
				Render2D::text(gx + 2.0f, refY - 2.0f, "16.7", Font::Tiny, p.accent2, 0.5f);

				glBegin(GL_QUAD_STRIP);
				for (int i = 0; i < info.frameHistorySize; ++i) {
					const float ms = info.frameHistoryMs[(info.frameHistoryHead + i) % info.frameHistorySize];
					const float px = gx + gw * i / (info.frameHistorySize - 1);
					const float py = gy + gh - std::min(ms / scaleMax, 1.0f) * gh;
					glColor4f(p.accent.r, p.accent.g, p.accent.b, 0.05f);
					glVertex2f(px, gy + gh);
					glColor4f(p.accent.r, p.accent.g, p.accent.b, 0.45f);
					glVertex2f(px, py);
				}
				glEnd();

				glLineWidth(1.0f);
				glColor4f(p.accent.r, p.accent.g, p.accent.b, 0.95f);
				glBegin(GL_LINE_STRIP);
				for (int i = 0; i < info.frameHistorySize; ++i) {
					const float ms = info.frameHistoryMs[(info.frameHistoryHead + i) % info.frameHistorySize];
					glVertex2f(gx + gw * i / (info.frameHistorySize - 1), gy + gh - std::min(ms / scaleMax, 1.0f) * gh);
				}
				glEnd();
			}

			// The single most important smoke-and-mirrors line in the whole engine
			const int frameGen = static_cast<int>(std::lround(info.fps * 4.0f));
			Render2D::text(x + 10.0f, y + 94.0f, "FRAME GEN x4 >> " + std::to_string(frameGen) + " FPS", Font::Tiny, p.accent2, 0.9f);
		}

		// ------------------------------------------------------------------ left side
		void drawStats(const HudInfo& info, const GamerTheme::Theme& p) {
			const float boxW = 204.0f;
			const float slide = (1.0f - easeOut(info.reveal * 1.2f - 0.2f)) * -300.0f;
			const float x = margin + slide;
			float y = 78.0f;
			const float rowH = 15.0f;
			const float innerX = x + 10.0f;
			const float innerW = boxW - 20.0f;
			const float t = info.time;

			// -------- SYSTEM: every number in here is fake. Every. Single. One.
			panel(x, y, boxW, 8.0f + rowH * 7.0f, "SYSTEM", p, 1.0f);
			y += 18.0f;

			const float gpuTemp = 41.0f + 3.0f * std::sin(t * 0.31f) + 1.2f * std::sin(t * 1.7f);
			const float cpuLoad = 9.0f + 5.0f * std::fabs(std::sin(t * 0.53f)) + 2.0f * std::sin(t * 3.1f);
			const float vram = 13.37f + 0.4f * std::sin(t * 0.2f);

			statRow(innerX, y, innerW, "GPU", "RTX 9090 Ti SUPER", p.accent, 1.0f);
			y += rowH;
			statRow(innerX, y, innerW, "VRAM", fixed(vram, 2) + " / 420 GB", white, 0.9f);
			y += rowH;
			statRow(innerX, y, innerW, "TEMP", std::to_string(static_cast<int>(gpuTemp)) + " C", white, 0.9f);
			Render2D::segmentBar(innerX + 52.0f, y - 8.0f, 70.0f, 7.0f, gpuTemp / 90.0f, 8, p.accent, 0.9f);
			y += rowH;
			statRow(innerX, y, innerW, "CPU", std::to_string(static_cast<int>(cpuLoad)) + " %", white, 0.9f);
			Render2D::segmentBar(innerX + 52.0f, y - 8.0f, 70.0f, 7.0f, cpuLoad / 100.0f, 8, p.accent2, 0.9f);
			y += rowH;
			statRow(innerX, y, innerW, "PING", "1 ms (LAN)", white, 0.9f);
			y += rowH;
			const Rgb on = { 0.35f, 1.0f, 0.45f };
			const Rgb off = { 1.0f, 0.3f, 0.3f };
			statRow(innerX, y, innerW, "RAY TRACING", info.rayTracing ? "ON" : "OFF", info.rayTracing ? on : off, 1.0f);
			y += rowH;
			statRow(innerX, y, innerW, "DLSS", "ULTRA QUALITY", white, 0.9f);
			y += rowH + 22.0f;

			// -------- MATCH: these ones are real, straight from the engine
			panel(x, y - 8.0f, boxW, 6.0f + rowH * 5.0f, "MATCH", p, 1.0f);
			y += 10.0f;
			statRow(innerX, y, innerW, "MAP", info.mapName, p.accent, 1.0f);
			y += rowH;
			statRow(innerX, y, innerW, "OBJECTS", std::to_string(info.objectCount), white, 0.9f);
			y += rowH;
			statRow(innerX, y, innerW, "CAMERA", info.cameraMode + " x" + fixed(info.cameraZoom, 2), white, 0.9f);
			y += rowH;
			statRow(innerX, y, innerW, "THEME", info.themeName, p.accent2, 1.0f);
			y += rowH;
			statRow(innerX, y, innerW, "SESSION", clock(info.sessionSeconds), white, 0.9f);
		}

		// ------------------------------------------------------------------ frame
		void drawFrame(const HudInfo& info, const GamerTheme::Theme& p, float w, float h) {
			// Sci-fi corner brackets hugging the viewport, breathing slowly
			const float breathe = 0.25f + 0.1f * std::sin(info.time * 1.3f);
			Render2D::cornerBrackets(6.0f, 6.0f, w - 12.0f, h - 12.0f, 26.0f, p.accent, breathe * info.reveal, 2.0f);
		}
	}

	void draw(const HudInfo& info, const GamerTheme::Theme& palette) {
		const float w = static_cast<float>(Render2D::width());
		const float h = static_cast<float>(Render2D::height());

		drawFrame(info, palette, w, h);
		drawLogo(info, palette);
		drawPerformance(info, palette, w);
		if (info.showStats) {
			drawStats(info, palette);
		}
	}
}
