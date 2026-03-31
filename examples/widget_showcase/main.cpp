/// Lumen Widget Showcase — demonstrates all available widgets.

#include "lumen/core/application.hpp"
#include "lumen/ui/screen.hpp"
#include "lumen/ui/widgets/button.hpp"
#include "lumen/ui/widgets/checkbox.hpp"
#include "lumen/ui/widgets/label.hpp"
#include "lumen/ui/widgets/progress_bar.hpp"
#include "lumen/ui/widgets/slider.hpp"
#include "lumen/ui/widgets/toggle.hpp"
#include "platform/desktop_sdl2/board_config.hpp"

using Board = lumen::platform::DesktopSdl2Config<400, 300>;

// Shared state
static int16_t slider_value	  = 50;
static bool checkbox_state	  = false;
static bool toggle_state	  = false;
static uint8_t progress_value = 0;

class ShowcaseScreen : public lumen::ui::Screen
{
  public:
	ShowcaseScreen()
	{
		// Title
		title_.set_text("Lumen Widget Showcase");
		title_.set_bounds({10, 5, 380, 25});
		title_.set_color(lumen::Color::rgb(100, 200, 255));

		// Button
		btn_label_.set_text("Button:");
		btn_label_.set_bounds({10, 40, 80, 25});
		btn_.set_label("Press Me");
		btn_.set_bounds({100, 38, 120, 30});
		btn_.set_on_click([] {
			progress_value += 10;
			if (progress_value > 100)
				progress_value = 0;
		});

		// Checkbox
		cb_label_.set_text("Checkbox:");
		cb_label_.set_bounds({10, 80, 80, 25});
		checkbox_.set_bounds({100, 78, 28, 28});
		checkbox_.set_on_change([](bool val) { checkbox_state = val; });
		cb_status_.set_text("Off");
		cb_status_.set_bounds({140, 80, 60, 25});

		// Toggle
		tg_label_.set_text("Toggle:");
		tg_label_.set_bounds({10, 120, 80, 25});
		toggle_.set_bounds({100, 118, 56, 28});
		toggle_.set_on_change([](bool val) { toggle_state = val; });
		tg_status_.set_text("Off");
		tg_status_.set_bounds({170, 120, 60, 25});

		// Slider
		sl_label_.set_text("Slider:");
		sl_label_.set_bounds({10, 160, 80, 25});
		slider_.set_range(0, 100);
		slider_.set_value(50);
		slider_.set_bounds({100, 165, 200, 16});
		slider_.set_on_change([](int16_t val) { slider_value = val; });
		sl_value_.set_text("50");
		sl_value_.set_bounds({310, 160, 60, 25});

		// Progress bar
		pg_label_.set_text("Progress:");
		pg_label_.set_bounds({10, 200, 80, 25});
		progress_.set_bounds({100, 203, 200, 14});

		// Status line
		status_.set_text("Interactive demo — click, drag, toggle!");
		status_.set_bounds({10, 240, 380, 25});
		status_.set_color(lumen::Color::rgb(180, 180, 180));
		status_.set_bg_color(lumen::Color::rgb(30, 30, 40));

		// Add all widgets
		add(title_);
		add(btn_label_);
		add(btn_);
		add(cb_label_);
		add(checkbox_);
		add(cb_status_);
		add(tg_label_);
		add(toggle_);
		add(tg_status_);
		add(sl_label_);
		add(slider_);
		add(sl_value_);
		add(pg_label_);
		add(progress_);
		add(status_);
	}

	void update_model() override
	{
		// Update displays from shared state
		cb_status_.set_text(checkbox_state ? "On" : "Off");
		tg_status_.set_text(toggle_state ? "On" : "Off");

		char buf[16];
		snprintf(buf, sizeof(buf), "%d", slider_value);
		sl_value_.set_text(buf);

		progress_.set_value(progress_value);
	}

  private:
	lumen::ui::Label title_;
	lumen::ui::Label btn_label_;
	lumen::ui::Button btn_;
	lumen::ui::Label cb_label_;
	lumen::ui::Checkbox checkbox_;
	lumen::ui::Label cb_status_;
	lumen::ui::Label tg_label_;
	lumen::ui::Toggle toggle_;
	lumen::ui::Label tg_status_;
	lumen::ui::Label sl_label_;
	lumen::ui::Slider slider_;
	lumen::ui::Label sl_value_;
	lumen::ui::Label pg_label_;
	lumen::ui::ProgressBar progress_;
	lumen::ui::Label status_;
};

int main()
{
	Board board;
	board.init_hardware();

	lumen::Application<Board> app(board);

	ShowcaseScreen screen;
	app.navigate_to(screen);
	app.run();

	board.display.destroy();
	return 0;
}
