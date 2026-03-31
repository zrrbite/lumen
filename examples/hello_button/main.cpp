/// Lumen "Hello Button" — interactive widget demo.
/// Shows labels and a clickable button using the Application framework.

#include "lumen/core/application.hpp"
#include "lumen/ui/screen.hpp"
#include "lumen/ui/widgets/button.hpp"
#include "lumen/ui/widgets/label.hpp"
#include "platform/desktop_sdl2/board_config.hpp"

using Board = lumen::platform::DesktopSdl2Config<320, 240>;

static int click_count = 0;

class DemoScreen : public lumen::ui::Screen
{
  public:
	DemoScreen()
	{
		title_.set_text("Lumen GUI Framework");
		title_.set_bounds({10, 10, 300, 30});
		title_.set_color(lumen::Color::rgb(100, 200, 255));

		counter_.set_text("Clicks: 0");
		counter_.set_bounds({10, 60, 200, 30});

		btn_.set_label("Click Me!");
		btn_.set_bounds({10, 120, 140, 50});
		btn_.set_on_click([] { ++click_count; });

		reset_btn_.set_label("Reset");
		reset_btn_.set_bounds({170, 120, 100, 50});
		reset_btn_.set_color(lumen::Color::rgb(180, 60, 60), lumen::Color::rgb(120, 40, 40));
		reset_btn_.set_on_click([] { click_count = 0; });

		add(title_);
		add(counter_);
		add(btn_);
		add(reset_btn_);
	}

	void update_model() override
	{
		// Update counter label — only invalidates if text actually changed
		char buf[32];
		snprintf(buf, sizeof(buf), "Clicks: %d", click_count);
		counter_.set_text(buf);
	}

  private:
	lumen::ui::Label title_;
	lumen::ui::Label counter_;
	lumen::ui::Button btn_;
	lumen::ui::Button reset_btn_;
};

int main()
{
	Board board;
	board.init_hardware();

	lumen::Application<Board> app(board);

	DemoScreen screen;
	app.navigate_to(screen);
	app.run();

	board.display.destroy();
	return 0;
}
