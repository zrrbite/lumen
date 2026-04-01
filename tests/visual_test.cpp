/// Lumen visual regression test framework.
/// Renders widgets to an in-memory framebuffer, saves as PPM,
/// compares against reference screenshots.
///
/// Usage:
///   ./visual_test [--update]    # --update overwrites reference images
///
/// Exit code 0 = all pass, 1 = diffs found

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "lumen/core/pixel_format.hpp"
#include "lumen/gfx/canvas.hpp"
#include "lumen/gfx/font.hpp"
#include "lumen/gfx/fonts/liberation_sans_10.hpp"
#include "lumen/gfx/fonts/liberation_sans_14.hpp"
#include "lumen/ui/widgets/button.hpp"
#include "lumen/ui/widgets/label.hpp"
#include "lumen/ui/widgets/progress_bar.hpp"

using namespace lumen;
using namespace lumen::gfx;
using namespace lumen::ui;

static constexpr uint16_t FB_W = 240;
static constexpr uint16_t FB_H = 320;

static uint16_t framebuffer[FB_W * FB_H];

/// Save framebuffer as PPM (RGB, portable, no deps).
static bool save_ppm(const char* path, const uint16_t* fb, uint16_t w, uint16_t h)
{
	FILE* f = fopen(path, "wb");
	if (!f)
		return false;
	fprintf(f, "P6\n%d %d\n255\n", w, h);
	for (uint32_t i = 0; i < static_cast<uint32_t>(w) * h; ++i)
	{
		uint16_t px = fb[i];
		// RGB565 to RGB888
		uint8_t r = static_cast<uint8_t>(((px >> 11) & 0x1F) << 3);
		uint8_t g = static_cast<uint8_t>(((px >> 5) & 0x3F) << 2);
		uint8_t b = static_cast<uint8_t>((px & 0x1F) << 3);
		fwrite(&r, 1, 1, f);
		fwrite(&g, 1, 1, f);
		fwrite(&b, 1, 1, f);
	}
	fclose(f);
	return true;
}

/// Load PPM and compare. Returns number of differing pixels.
static int compare_ppm(const char* path, const uint16_t* fb, uint16_t w, uint16_t h)
{
	FILE* f = fopen(path, "rb");
	if (!f)
		return -1; // Reference not found

	// Skip PPM header (P6\nW H\n255\n)
	char header[64];
	if (!fgets(header, sizeof(header), f))
	{
		fclose(f);
		return -1;
	}
	if (!fgets(header, sizeof(header), f))
	{
		fclose(f);
		return -1;
	}
	if (!fgets(header, sizeof(header), f))
	{
		fclose(f);
		return -1;
	}

	int diffs = 0;
	for (uint32_t i = 0; i < static_cast<uint32_t>(w) * h; ++i)
	{
		uint8_t ref_r = 0;
		uint8_t ref_g = 0;
		uint8_t ref_b = 0;
		if (fread(&ref_r, 1, 1, f) != 1)
			break;
		if (fread(&ref_g, 1, 1, f) != 1)
			break;
		if (fread(&ref_b, 1, 1, f) != 1)
			break;

		uint16_t px = fb[i];
		uint8_t r	= static_cast<uint8_t>(((px >> 11) & 0x1F) << 3);
		uint8_t g	= static_cast<uint8_t>(((px >> 5) & 0x3F) << 2);
		uint8_t b	= static_cast<uint8_t>((px & 0x1F) << 3);

		// Allow small tolerance for rounding
		if (abs(r - ref_r) > 4 || abs(g - ref_g) > 4 || abs(b - ref_b) > 4)
			++diffs;
	}

	fclose(f);
	return diffs;
}

/// Clear framebuffer and get a fresh canvas.
static Canvas<Rgb565> fresh_canvas()
{
	memset(framebuffer, 0, sizeof(framebuffer));
	return Canvas<Rgb565>(framebuffer, FB_W, FB_H);
}

// --- Test cases ---

struct TestCase
{
	const char* name;
	void (*render)(Canvas<Rgb565>& canvas);
};

static void test_label(Canvas<Rgb565>& canvas)
{
	Label label("Hello, Lumen!");
	label.set_bounds({10, 10, 220, 20});
	label.set_font(&liberation_sans_14);
	label.set_color(Color::white());
	label.set_bg_color(Color::rgb(30, 30, 40));
	label.draw(canvas);
}

static void test_button(Canvas<Rgb565>& canvas)
{
	Button btn("Click Me");
	btn.set_bounds({20, 20, 200, 60});
	btn.set_font(&liberation_sans_14);
	btn.draw(canvas);
}

static void test_button_pressed(Canvas<Rgb565>& canvas)
{
	Button btn("Pressed");
	btn.set_bounds({20, 20, 200, 60});
	btn.set_font(&liberation_sans_14);
	// Simulate press
	TouchEvent press{TouchEvent::Type::Press, {120, 50}};
	btn.on_touch(press);
	btn.draw(canvas);
}

static void test_progress_bar(Canvas<Rgb565>& canvas)
{
	ProgressBar bar;
	bar.set_bounds({20, 50, 200, 20});
	bar.set_fill_color(Color::rgb(50, 200, 80));
	bar.set_value(65);
	bar.draw(canvas);
}

static void test_multi_widget(Canvas<Rgb565>& canvas)
{
	canvas.fill_rect({0, 0, FB_W, FB_H}, Color::rgb(20, 20, 30));

	Label title("Dashboard");
	title.set_bounds({10, 10, 220, 20});
	title.set_font(&liberation_sans_14);
	title.set_color(Color::rgb(100, 200, 255));
	title.set_bg_color(Color::rgb(20, 20, 30));
	title.draw(canvas);

	Button btn("Action");
	btn.set_bounds({20, 50, 200, 50});
	btn.set_font(&liberation_sans_10);
	btn.draw(canvas);

	ProgressBar bar;
	bar.set_bounds({20, 120, 200, 16});
	bar.set_value(42);
	bar.draw(canvas);

	Label status("Status: OK");
	status.set_bounds({10, 160, 220, 16});
	status.set_font(&liberation_sans_10);
	status.set_color(Color::rgb(100, 255, 100));
	status.set_bg_color(Color::rgb(20, 20, 30));
	status.draw(canvas);
}

static TestCase tests[] = {
	{"label", test_label},
	{"button", test_button},
	{"button_pressed", test_button_pressed},
	{"progress_bar", test_progress_bar},
	{"multi_widget", test_multi_widget},
};

int main(int argc, char** argv)
{
	bool update_mode = false;
	for (int i = 1; i < argc; ++i)
	{
		if (strcmp(argv[i], "--update") == 0)
			update_mode = true;
	}

	int total	= sizeof(tests) / sizeof(tests[0]);
	int passed	= 0;
	int updated = 0;

	for (int i = 0; i < total; ++i)
	{
		auto& test = tests[i];

		// Render
		auto canvas = fresh_canvas();
		test.render(canvas);

		char ref_path[256];
		char out_path[256];
		snprintf(ref_path, sizeof(ref_path), "tests/screenshots/%s.ppm", test.name);
		snprintf(out_path, sizeof(out_path), "tests/screenshots/%s_actual.ppm", test.name);

		if (update_mode)
		{
			save_ppm(ref_path, framebuffer, FB_W, FB_H);
			printf("  UPDATED %s\n", test.name);
			++updated;
		}
		else
		{
			// Save actual
			save_ppm(out_path, framebuffer, FB_W, FB_H);

			// Compare
			int diffs = compare_ppm(ref_path, framebuffer, FB_W, FB_H);
			if (diffs < 0)
			{
				printf("  NEW     %s (no reference — run with --update)\n", test.name);
			}
			else if (diffs == 0)
			{
				printf("  PASS    %s\n", test.name);
				++passed;
				// Clean up actual file on pass
				remove(out_path);
			}
			else
			{
				printf("  FAIL    %s (%d pixels differ)\n", test.name, diffs);
			}
		}
	}

	if (update_mode)
	{
		printf("\nUpdated %d reference screenshots.\n", updated);
		return 0;
	}

	printf("\n%d/%d tests passed.\n", passed, total);
	return (passed == total) ? 0 : 1;
}
