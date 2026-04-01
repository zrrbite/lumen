/// Integration test for STM32F429-DISCO.
/// Runs the real application with scripted interactions.
/// Records state + framebuffer at each step for host verification.
///
/// Each step:
///   1. Optionally inject a touch event
///   2. Tick the application (render + input + animations)
///   3. Render the current screen to the test framebuffer
///   4. Record state (variables, widget values)
///
/// The host reads back step_results[] and framebuffer after all steps.

#include <cstdint>
#include <cstring>

#include "lumen/core/application.hpp"
#include "lumen/core/pixel_format.hpp"
#include "lumen/gfx/canvas.hpp"
#include "lumen/gfx/font.hpp"
#include "lumen/gfx/fonts/liberation_sans_10.hpp"
#include "lumen/gfx/fonts/liberation_sans_14.hpp"
#include "lumen/gfx/fonts/liberation_sans_bold_10.hpp"
#include "lumen/gfx/fonts/liberation_sans_sdf16.hpp"
#include "lumen/hal/tick_source.hpp"
#include "lumen/ui/screen.hpp"
#include "lumen/ui/widgets/button.hpp"
#include "lumen/ui/widgets/label.hpp"
#include "lumen/ui/widgets/progress_bar.hpp"

// sys_tick from startup.cpp
namespace lumen::platform {
extern hal::SysTickSource sys_tick;
}

// --- Test framebuffer ---
static constexpr uint16_t FB_W = 240;
static constexpr uint16_t FB_H = 160;
static uint16_t test_fb[FB_W * FB_H];

// --- Step definition ---
struct TouchAction
{
	enum Type : uint8_t
	{
		None,
		Press,
		Release,
		Move
	};
	Type type = None;
	int16_t x = 0;
	int16_t y = 0;
};

struct StepResult
{
	uint32_t crc;	 // CRC32 of framebuffer
	int32_t vars[8]; // Application state values
	uint8_t var_count;
};

// --- CRC32 ---
static uint32_t crc32(const void* data, uint32_t len)
{
	const uint8_t* p = static_cast<const uint8_t*>(data);
	uint32_t crc	 = 0xFFFFFFFF;
	for (uint32_t i = 0; i < len; ++i)
	{
		crc ^= p[i];
		for (int j = 0; j < 8; ++j)
			crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
	}
	return crc ^ 0xFFFFFFFF;
}

// --- Test application ---
static int touch_count = 0;
static int bar_value   = 0;

class TestScreen : public lumen::ui::Screen
{
  public:
	TestScreen()
	{
		title_.set_text("Test Screen");
		title_.set_bounds({10, 5, 220, 20});
		title_.set_font(&lumen::gfx::liberation_sans_14);
		title_.set_color(lumen::Color::rgb(100, 200, 255));
		title_.set_bg_color(lumen::Color::rgb(20, 20, 30));

		counter_.set_text("Count: 0");
		counter_.set_bounds({10, 30, 220, 16});
		counter_.set_font(&lumen::gfx::liberation_sans_10);
		counter_.set_bg_color(lumen::Color::rgb(20, 20, 30));

		btn_.set_label("Click");
		btn_.set_bounds({20, 55, 200, 40});
		btn_.set_font(&lumen::gfx::liberation_sans_bold_10);
		btn_.set_on_click([] {
			++touch_count;
			bar_value += 25;
			if (bar_value > 100)
				bar_value = 0;
		});

		bar_.set_bounds({20, 105, 200, 16});
		bar_.set_fill_color(lumen::Color::rgb(50, 200, 80));

		add(title_);
		add(counter_);
		add(btn_);
		add(bar_);
	}

	void update_model() override
	{
		btn_.tick_visual();

		char buf[32];
		buf[0] = 'C';
		buf[1] = 'o';
		buf[2] = 'u';
		buf[3] = 'n';
		buf[4] = 't';
		buf[5] = ':';
		buf[6] = ' ';
		buf[7] = '0' + static_cast<char>(touch_count % 10);
		buf[8] = '\0';
		counter_.set_text(buf);
		bar_.set_value(static_cast<uint8_t>(bar_value));
	}

	lumen::ui::Label title_;
	lumen::ui::Label counter_;
	lumen::ui::Button btn_;
	lumen::ui::ProgressBar bar_;
};

// --- Control block read by host ---
struct IntegrationControl
{
	volatile uint32_t magic; // 0xBEEFCAFE = done
	volatile uint32_t step_count;
	volatile uint32_t fb_addr;
	volatile uint32_t fb_w;
	volatile uint32_t fb_h;
	volatile uint32_t results_addr;
};

static IntegrationControl ctrl;
static constexpr uint8_t MAX_STEPS = 16;
static StepResult results[MAX_STEPS];

// --- Test steps ---
struct TestStep
{
	const char* name;
	TouchAction touch;
	uint32_t tick_count; // How many render ticks to run
};

static TestStep steps[] = {
	// Step 0: Initial render — no interaction
	{"initial", {TouchAction::None, 0, 0}, 1},

	// Step 1: Press the button
	{"press_btn", {TouchAction::Press, 120, 75}, 1},

	// Step 2: Release the button (triggers on_click)
	{"release_btn", {TouchAction::Release, 120, 75}, 2},

	// Step 3: Verify counter incremented, render updated
	{"after_click", {TouchAction::None, 0, 0}, 1},

	// Step 4: Press + release again
	{"press_btn_2", {TouchAction::Press, 120, 75}, 1},
	{"release_btn_2", {TouchAction::Release, 120, 75}, 2},

	// Step 5: Check state after 2 clicks
	{"after_2_clicks", {TouchAction::None, 0, 0}, 1},

	// Step 6: Press + release 3rd time
	{"press_btn_3", {TouchAction::Press, 120, 75}, 1},
	{"release_btn_3", {TouchAction::Release, 120, 75}, 2},
};

static void render_to_fb(TestScreen& screen)
{
	memset(test_fb, 0, sizeof(test_fb));
	lumen::gfx::Canvas<lumen::Rgb565> canvas(test_fb, FB_W, FB_H);
	screen.update_model();

	// Collect dirty and draw
	canvas.set_clip({0, 0, FB_W, FB_H});
	screen.draw(canvas);
}

static void record_state(uint8_t step_idx)
{
	results[step_idx].crc		= crc32(test_fb, sizeof(test_fb));
	results[step_idx].vars[0]	= touch_count;
	results[step_idx].vars[1]	= bar_value;
	results[step_idx].var_count = 2;
}

static void inject_touch(TestScreen& screen, const TouchAction& action)
{
	if (action.type == TouchAction::None)
		return;

	lumen::ui::TouchEvent evt;
	evt.pos = {action.x, action.y};

	switch (action.type)
	{
	case TouchAction::Press:
		evt.type = lumen::ui::TouchEvent::Type::Press;
		break;
	case TouchAction::Release:
		evt.type = lumen::ui::TouchEvent::Type::Release;
		break;
	case TouchAction::Move:
		evt.type = lumen::ui::TouchEvent::Type::Move;
		break;
	default:
		return;
	}

	screen.on_touch(evt);
}

int main()
{
	touch_count = 0;
	bar_value	= 0;

	TestScreen screen;

	uint32_t num_steps = sizeof(steps) / sizeof(steps[0]);
	if (num_steps > MAX_STEPS)
		num_steps = MAX_STEPS;

	ctrl.magic		  = 0;
	ctrl.step_count	  = num_steps;
	ctrl.fb_addr	  = reinterpret_cast<uint32_t>(test_fb);
	ctrl.fb_w		  = FB_W;
	ctrl.fb_h		  = FB_H;
	ctrl.results_addr = reinterpret_cast<uint32_t>(results);

	for (uint32_t i = 0; i < num_steps; ++i)
	{
		auto& step = steps[i];

		// Inject touch
		inject_touch(screen, step.touch);

		// Tick (render) N times
		for (uint32_t t = 0; t < step.tick_count; ++t)
		{
			render_to_fb(screen);
		}

		// Record state
		record_state(static_cast<uint8_t>(i));
	}

	// Signal done
	ctrl.magic = 0xBEEFCAFE;

	while (true)
	{
		__asm volatile("wfi");
	}
}
