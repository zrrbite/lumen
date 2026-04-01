/// Lumen demo on Raspberry Pi 4.
/// Displays widgets on HDMI via DRM/KMS with touch input via evdev.

#ifdef __linux__

#include <cstdio>

#include "lumen/core/application.hpp"
#include "lumen/gfx/fonts/liberation_sans_14.hpp"
#include "lumen/gfx/fonts/liberation_sans_sdf16.hpp"
#include "lumen/ui/screen.hpp"
#include "lumen/ui/widgets/button.hpp"
#include "lumen/ui/widgets/label.hpp"
#include "lumen/ui/widgets/progress_bar.hpp"
#include "platform/raspberry_pi/board_config.hpp"

using Board = lumen::platform::RaspberryPiConfig;
using App	= lumen::Application<Board>;

static int counter = 0;

class DemoScreen : public lumen::ui::Screen
{
  public:
	DemoScreen()
	{
		title_.set_text("Lumen on Raspberry Pi!");
		title_.set_bounds({20, 20, 400, 40});
		title_.set_font(&lumen::gfx::liberation_sans_sdf16, 28);
		title_.set_color(lumen::Color::rgb(100, 200, 255));
		title_.set_bg_color(lumen::Color::rgb(20, 20, 30));

		info_.set_text("DRM/KMS + evdev touch");
		info_.set_bounds({20, 70, 400, 24});
		info_.set_font(&lumen::gfx::liberation_sans_14);
		info_.set_bg_color(lumen::Color::rgb(20, 20, 30));

		btn_.set_label("Click Me!");
		btn_.set_bounds({40, 120, 300, 80});
		btn_.set_font(&lumen::gfx::liberation_sans_14);
		btn_.set_on_click([] { ++counter; });

		bar_.set_bounds({40, 230, 300, 24});
		bar_.set_fill_color(lumen::Color::rgb(50, 200, 80));

		status_.set_text("Touches: 0");
		status_.set_bounds({20, 280, 400, 24});
		status_.set_font(&lumen::gfx::liberation_sans_14);
		status_.set_bg_color(lumen::Color::rgb(20, 20, 30));

		add(title_);
		add(info_);
		add(btn_);
		add(bar_);
		add(status_);
	}

	void update_model() override
	{
		btn_.tick_visual();
		bar_.set_value(static_cast<uint8_t>(counter % 101));

		char buf[32];
		snprintf(buf, sizeof(buf), "Touches: %d", counter);
		status_.set_text(buf);
	}

  private:
	lumen::ui::Label title_;
	lumen::ui::Label info_;
	lumen::ui::Button btn_;
	lumen::ui::ProgressBar bar_;
	lumen::ui::Label status_;
};

int main()
{
	Board board;
	board.init_hardware();

	if (board.display.width() == 0)
	{
		fprintf(stderr, "Failed to initialize DRM display\n");
		return 1;
	}

	printf("Lumen on RPi: %dx%d display\n", board.display.width(), board.display.height());

	App app(board);
	DemoScreen screen;
	app.navigate_to(screen);
	app.run();

	return 0;
}

#else

#include <cstdio>
int main()
{
	fprintf(stderr, "This example requires Linux\n");
	return 1;
}

#endif
