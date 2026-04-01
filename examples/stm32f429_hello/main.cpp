/// Lumen screen transition demo on STM32F429-DISCO.
/// Two screens with different colors. Touch "Next" to slide between them.

#include "lumen/core/application.hpp"
#include "lumen/gfx/fonts/liberation_sans_10.hpp"
#include "lumen/gfx/fonts/liberation_sans_14.hpp"
#include "lumen/gfx/fonts/liberation_sans_bold_10.hpp"
#include "lumen/gfx/fonts/liberation_sans_sdf16.hpp"
#include "lumen/gfx/images/test_icon.hpp"
#include "lumen/ui/screen.hpp"
#include "lumen/ui/transition.hpp"
#include "lumen/ui/widgets/button.hpp"
#include "lumen/ui/widgets/image.hpp"
#include "lumen/ui/widgets/label.hpp"
#include "lumen/ui/widgets/progress_bar.hpp"
#include "platform/stm32f429_disco/board_config.hpp"

using Board = lumen::platform::Stm32f429DiscoConfig;
using App	= lumen::Application<Board>;

static App* g_app = nullptr;

// Forward declarations
class ScreenA;
class ScreenB;
static ScreenA* screen_a = nullptr;
static ScreenB* screen_b = nullptr;
static void nav_slide_to_b();
static void nav_wipe_to_b();
static void nav_slide_to_a();
static void nav_wipe_to_a();

// --- Screen A: Blue theme ---
class ScreenA : public lumen::ui::Screen
{
  public:
	ScreenA()
	{
		title_.set_text("Screen A");
		title_.set_bounds({10, 10, 220, 28});
		title_.set_font(&lumen::gfx::liberation_sans_sdf16, 20);
		title_.set_color(lumen::Color::rgb(100, 200, 255));
		title_.set_bg_color(lumen::Color::rgb(20, 30, 60));

		info_.set_text("Blue theme");
		info_.set_bounds({10, 50, 220, 20});
		info_.set_font(&lumen::gfx::liberation_sans_10);
		info_.set_bg_color(lumen::Color::rgb(20, 30, 60));

		bar_.set_bounds({20, 90, 200, 16});
		bar_.set_fill_color(lumen::Color::rgb(50, 150, 255));
		bar_.set_value(65);

		next_.set_label("Slide Right >");
		next_.set_bounds({20, 130, 200, 50});
		next_.set_font(&lumen::gfx::liberation_sans_bold_10);
		next_.set_color(lumen::Color::rgb(40, 80, 180), lumen::Color::rgb(30, 60, 140));
		next_.set_on_click(nav_slide_to_b);
		wipe_.set_label("Wipe Down v");
		wipe_.set_bounds({20, 195, 200, 50});
		wipe_.set_font(&lumen::gfx::liberation_sans_bold_10);
		wipe_.set_color(lumen::Color::rgb(40, 120, 80), lumen::Color::rgb(30, 90, 60));
		wipe_.set_on_click(nav_wipe_to_b);

		icon_.set_image_rgb565(lumen::assets::TEST_ICON_DATA, lumen::assets::TEST_ICON_W, lumen::assets::TEST_ICON_H);
		icon_.set_bounds({170, 50, 50, 40});

		perf_.set_text("FPS: --");
		perf_.set_bounds({10, 260, 220, 16});
		perf_.set_font(&lumen::gfx::liberation_sans_10);
		perf_.set_color(lumen::Color::rgb(80, 80, 100));
		perf_.set_bg_color(lumen::Color::rgb(20, 30, 60));

		add(title_);
		add(info_);
		add(icon_);
		add(bar_);
		add(next_);
		add(wipe_);
		add(perf_);
	}

	void update_model() override
	{
		next_.tick_visual();
		wipe_.tick_visual();
		if (g_app)
		{
			char buf[32];
			auto fps = g_app->perf().fps;
			buf[0]	 = 'F';
			buf[1]	 = 'P';
			buf[2]	 = 'S';
			buf[3]	 = ':';
			buf[4]	 = '0' + (fps / 10) % 10;
			buf[5]	 = '0' + fps % 10;
			buf[6]	 = '\0';
			perf_.set_text(buf);
		}
	}

  private:
	lumen::ui::Label title_;
	lumen::ui::Label info_;
	lumen::ui::Image icon_;
	lumen::ui::ProgressBar bar_;
	lumen::ui::Button next_;
	lumen::ui::Button wipe_;
	lumen::ui::Label perf_;
};

// --- Screen B: Red/orange theme ---
class ScreenB : public lumen::ui::Screen
{
  public:
	ScreenB()
	{
		title_.set_text("Screen B");
		title_.set_bounds({10, 10, 220, 28});
		title_.set_font(&lumen::gfx::liberation_sans_sdf16, 20);
		title_.set_color(lumen::Color::rgb(255, 180, 80));
		title_.set_bg_color(lumen::Color::rgb(50, 20, 20));

		info_.set_text("Red theme");
		info_.set_bounds({10, 50, 220, 20});
		info_.set_font(&lumen::gfx::liberation_sans_10);
		info_.set_bg_color(lumen::Color::rgb(50, 20, 20));

		bar_.set_bounds({20, 90, 200, 16});
		bar_.set_fill_color(lumen::Color::rgb(255, 100, 50));
		bar_.set_value(35);

		back_.set_label("< Slide Back");
		back_.set_bounds({20, 130, 200, 50});
		back_.set_font(&lumen::gfx::liberation_sans_bold_10);
		back_.set_color(lumen::Color::rgb(180, 60, 40), lumen::Color::rgb(140, 40, 30));
		back_.set_on_click(nav_slide_to_a);
		wipe_.set_label("^ Wipe Right");
		wipe_.set_bounds({20, 195, 200, 50});
		wipe_.set_font(&lumen::gfx::liberation_sans_bold_10);
		wipe_.set_color(lumen::Color::rgb(120, 80, 40), lumen::Color::rgb(90, 60, 30));
		wipe_.set_on_click(nav_wipe_to_a);

		perf_.set_text("FPS: --");
		perf_.set_bounds({10, 260, 220, 16});
		perf_.set_font(&lumen::gfx::liberation_sans_10);
		perf_.set_color(lumen::Color::rgb(100, 80, 80));
		perf_.set_bg_color(lumen::Color::rgb(50, 20, 20));

		add(title_);
		add(info_);
		add(bar_);
		add(back_);
		add(wipe_);
		add(perf_);
	}

	void update_model() override
	{
		back_.tick_visual();
		wipe_.tick_visual();
		if (g_app)
		{
			char buf[32];
			auto fps = g_app->perf().fps;
			buf[0]	 = 'F';
			buf[1]	 = 'P';
			buf[2]	 = 'S';
			buf[3]	 = ':';
			buf[4]	 = '0' + (fps / 10) % 10;
			buf[5]	 = '0' + fps % 10;
			buf[6]	 = '\0';
			perf_.set_text(buf);
		}
	}

  private:
	lumen::ui::Label title_;
	lumen::ui::Label info_;
	lumen::ui::ProgressBar bar_;
	lumen::ui::Button back_;
	lumen::ui::Button wipe_;
	lumen::ui::Label perf_;
};

// Navigation callbacks (defined after both screen types are complete)
static void nav_slide_to_b()
{
	if (g_app && screen_b)
		g_app->navigate_to(*screen_b, lumen::ui::transitions::slide_left(400));
}
static void nav_wipe_to_b()
{
	if (g_app && screen_b)
		g_app->navigate_to(*screen_b, lumen::ui::transitions::wipe_down(500));
}
static void nav_slide_to_a()
{
	if (g_app && screen_a)
		g_app->navigate_to(*screen_a, lumen::ui::transitions::slide_right(400));
}
static void nav_wipe_to_a()
{
	if (g_app && screen_a)
		g_app->navigate_to(*screen_a, lumen::ui::transitions::wipe_right(500));
}

int main()
{
	Board board;
	board.init_hardware();

	App app(board);
	g_app = &app;

	ScreenA sa;
	ScreenB sb;
	screen_a = &sa;
	screen_b = &sb;

	app.navigate_to(sa);
	app.run();

	return 0;
}
