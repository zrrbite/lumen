/// Hardware-in-the-loop test for STM32F429-DISCO.
///
/// Renders test scenarios to a SRAM framebuffer, one at a time.
/// The host reads back the framebuffer via probe-rs after each step.
///
/// Protocol:
///   1. Host flashes this firmware
///   2. Firmware renders test N, writes result struct, enters WFI
///   3. Host reads control struct to get status
///   4. Host reads framebuffer, saves as PPM, compares
///   5. Host writes next test index to control struct
///   6. Firmware wakes, renders next test, repeat
///
/// Control struct at fixed address (start of BSS in SRAM1):
///   [0] magic     = 0xBEEFCAFE when ready
///   [1] current   = current test index
///   [2] total     = total test count
///   [3] fb_addr   = framebuffer address
///   [4] fb_w      = framebuffer width
///   [5] fb_h      = framebuffer height
///   [6] fb_bpp    = bytes per pixel (2 for RGB565)
///   [7] next_cmd  = host writes test index here to advance

#include <cstdint>
#include <cstring>

#include "lumen/core/pixel_format.hpp"
#include "lumen/core/types.hpp"
#include "lumen/gfx/canvas.hpp"
#include "lumen/gfx/font.hpp"
#include "lumen/gfx/fonts/liberation_sans_10.hpp"
#include "lumen/gfx/fonts/liberation_sans_14.hpp"
#include "lumen/hal/tick_source.hpp"
#include "lumen/ui/animation.hpp"
#include "lumen/ui/widgets/button.hpp"
#include "lumen/ui/widgets/label.hpp"
#include "lumen/ui/widgets/progress_bar.hpp"

// sys_tick defined in startup.cpp

// Control struct — host communicates via probe-rs memory read/write
struct HWTestControl
{
	volatile uint32_t magic;	// 0xBEEFCAFE = ready
	volatile uint32_t current;	// Current test index
	volatile uint32_t total;	// Total tests
	volatile uint32_t fb_addr;	// Framebuffer address
	volatile uint32_t fb_w;		// Width
	volatile uint32_t fb_h;		// Height
	volatile uint32_t fb_bpp;	// Bytes per pixel
	volatile uint32_t next_cmd; // Host writes next test index here (0xFFFF = done)
};

// Test framebuffer in SRAM
static constexpr uint16_t TEST_W = 240;
static constexpr uint16_t TEST_H = 160;
static uint16_t test_fb[TEST_W * TEST_H];

// Control struct — placed in BSS, address determined at link time
// Host finds it by reading the first 8 words at a known offset
static HWTestControl ctrl;

static lumen::gfx::Canvas<lumen::Rgb565> fresh_canvas()
{
	memset(test_fb, 0, sizeof(test_fb));
	return lumen::gfx::Canvas<lumen::Rgb565>(test_fb, TEST_W, TEST_H);
}

// --- Test scenarios ---

static void test_blank(lumen::gfx::Canvas<lumen::Rgb565>& canvas)
{
	canvas.fill_rect({0, 0, TEST_W, TEST_H}, lumen::Color::rgb(30, 30, 40));
}

static void test_label(lumen::gfx::Canvas<lumen::Rgb565>& canvas)
{
	canvas.fill_rect({0, 0, TEST_W, TEST_H}, lumen::Color::rgb(20, 20, 30));
	lumen::ui::Label label("Hello, Lumen!");
	label.set_bounds({10, 10, 220, 24});
	label.set_font(&lumen::gfx::liberation_sans_14);
	label.set_color(lumen::Color::rgb(100, 200, 255));
	label.set_bg_color(lumen::Color::rgb(20, 20, 30));
	label.draw(canvas);
}

static void test_button_normal(lumen::gfx::Canvas<lumen::Rgb565>& canvas)
{
	canvas.fill_rect({0, 0, TEST_W, TEST_H}, lumen::Color::rgb(20, 20, 30));
	lumen::ui::Button btn("Click Me");
	btn.set_bounds({20, 20, 200, 60});
	btn.set_font(&lumen::gfx::liberation_sans_14);
	btn.draw(canvas);
}

static void test_button_pressed(lumen::gfx::Canvas<lumen::Rgb565>& canvas)
{
	canvas.fill_rect({0, 0, TEST_W, TEST_H}, lumen::Color::rgb(20, 20, 30));
	lumen::ui::Button btn("Pressed!");
	btn.set_bounds({20, 20, 200, 60});
	btn.set_font(&lumen::gfx::liberation_sans_14);
	lumen::ui::TouchEvent press{lumen::ui::TouchEvent::Type::Press, {120, 50}};
	btn.on_touch(press);
	btn.draw(canvas);
}

static void test_progress_50(lumen::gfx::Canvas<lumen::Rgb565>& canvas)
{
	canvas.fill_rect({0, 0, TEST_W, TEST_H}, lumen::Color::rgb(20, 20, 30));
	lumen::ui::ProgressBar bar;
	bar.set_bounds({20, 60, 200, 20});
	bar.set_fill_color(lumen::Color::rgb(50, 200, 80));
	bar.set_value(50);
	bar.draw(canvas);
}

static void test_multi_widget(lumen::gfx::Canvas<lumen::Rgb565>& canvas)
{
	canvas.fill_rect({0, 0, TEST_W, TEST_H}, lumen::Color::rgb(20, 20, 30));

	lumen::ui::Label title("Dashboard");
	title.set_bounds({10, 5, 220, 20});
	title.set_font(&lumen::gfx::liberation_sans_14);
	title.set_color(lumen::Color::rgb(100, 200, 255));
	title.set_bg_color(lumen::Color::rgb(20, 20, 30));
	title.draw(canvas);

	lumen::ui::Button btn("Action");
	btn.set_bounds({20, 35, 200, 40});
	btn.set_font(&lumen::gfx::liberation_sans_10);
	btn.draw(canvas);

	lumen::ui::ProgressBar bar;
	bar.set_bounds({20, 90, 200, 14});
	bar.set_value(75);
	bar.draw(canvas);

	lumen::ui::Label status("Status: OK");
	status.set_bounds({10, 115, 220, 18});
	status.set_font(&lumen::gfx::liberation_sans_10);
	status.set_color(lumen::Color::rgb(100, 255, 100));
	status.set_bg_color(lumen::Color::rgb(20, 20, 30));
	status.draw(canvas);
}

using TestFn = void (*)(lumen::gfx::Canvas<lumen::Rgb565>&);

static TestFn tests[] = {
	test_blank,
	test_label,
	test_button_normal,
	test_button_pressed,
	test_progress_50,
	test_multi_widget,
};

static constexpr uint32_t NUM_TESTS = sizeof(tests) / sizeof(tests[0]);

int main()
{
	ctrl.magic	  = 0;
	ctrl.total	  = NUM_TESTS;
	ctrl.fb_addr  = reinterpret_cast<uint32_t>(test_fb);
	ctrl.fb_w	  = TEST_W;
	ctrl.fb_h	  = TEST_H;
	ctrl.fb_bpp	  = 2; // RGB565
	ctrl.next_cmd = 0;

	// Run all tests in sequence
	for (uint32_t i = 0; i < NUM_TESTS; ++i)
	{
		ctrl.current = i;
		auto canvas	 = fresh_canvas();
		tests[i](canvas);

		// Signal ready for readback
		ctrl.magic = 0xBEEFCAFE;

		// Wait for host to acknowledge (write next_cmd)
		while (ctrl.next_cmd == i)
		{
			__asm volatile("wfi");
		}

		ctrl.magic = 0; // Processing next test
	}

	// All done
	ctrl.current  = NUM_TESTS;
	ctrl.next_cmd = 0xFFFF;
	ctrl.magic	  = 0xBEEFCAFE;

	while (true)
	{
		__asm volatile("wfi");
	}
}
