/// Lumen Dashboard Demo — a polished screen with auto-layout.

#include <cstdio>

#include "lumen/core/application.hpp"
#include "lumen/ui/layout.hpp"
#include "lumen/ui/screen.hpp"
#include "lumen/ui/widgets/button.hpp"
#include "lumen/ui/widgets/label.hpp"
#include "lumen/ui/widgets/progress_bar.hpp"
#include "lumen/ui/widgets/slider.hpp"
#include "lumen/ui/widgets/toggle.hpp"
#include "platform/desktop_sdl2/board_config.hpp"

using Board = lumen::platform::DesktopSdl2Config<480, 320>;

static int16_t brightness = 70;
static bool wifi_on		  = true;
static bool led_on		  = false;
static uint8_t battery	  = 82;
static int counter		  = 0;

class DashboardScreen : public lumen::ui::Screen
{
  public:
	DashboardScreen()
	{
		// Title bar
		title_.set_text("Lumen Dashboard");
		title_.set_color(lumen::Color::rgb(100, 200, 255));
		title_.set_bg_color(lumen::Color::rgb(30, 40, 55));

		// Status labels
		wifi_label_.set_text("WiFi: On");
		wifi_label_.set_bg_color(lumen::Color::rgb(30, 40, 55));

		battery_label_.set_text("Battery: 82%");
		battery_label_.set_bg_color(lumen::Color::rgb(30, 40, 55));

		// Battery bar
		battery_bar_.set_value(battery);
		battery_bar_.set_fill_color(lumen::Color::rgb(50, 200, 80));

		// Brightness slider
		bright_label_.set_text("Brightness");
		bright_label_.set_bg_color(lumen::Color::rgb(25, 30, 40));
		slider_.set_range(0, 100);
		slider_.set_value(brightness);
		slider_.set_on_change([](int16_t val) { brightness = val; });

		bright_value_.set_text("70%");
		bright_value_.set_bg_color(lumen::Color::rgb(25, 30, 40));

		// Toggle switches
		wifi_toggle_label_.set_text("WiFi");
		wifi_toggle_label_.set_bg_color(lumen::Color::rgb(25, 30, 40));
		wifi_toggle_.set_on(true);
		wifi_toggle_.set_on_change([](bool val) { wifi_on = val; });

		led_toggle_label_.set_text("LED");
		led_toggle_label_.set_bg_color(lumen::Color::rgb(25, 30, 40));
		led_toggle_.set_on(false);
		led_toggle_.set_on_change([](bool val) { led_on = val; });

		// Action button
		action_btn_.set_label("Send Data");
		action_btn_.set_on_click([] { ++counter; });

		counter_label_.set_text("Sent: 0");
		counter_label_.set_bg_color(lumen::Color::rgb(25, 30, 40));

		// Manual layout (a real app would use BoxLayout/GridLayout)
		title_.set_bounds({0, 0, 480, 24});
		wifi_label_.set_bounds({0, 24, 240, 20});
		battery_label_.set_bounds({240, 24, 180, 20});
		battery_bar_.set_bounds({420, 26, 55, 16});

		bright_label_.set_bounds({10, 55, 100, 20});
		slider_.set_bounds({120, 58, 250, 14});
		bright_value_.set_bounds({380, 55, 60, 20});

		wifi_toggle_label_.set_bounds({10, 90, 60, 24});
		wifi_toggle_.set_bounds({80, 90, 50, 24});
		led_toggle_label_.set_bounds({160, 90, 60, 24});
		led_toggle_.set_bounds({230, 90, 50, 24});

		action_btn_.set_bounds({10, 130, 150, 35});
		counter_label_.set_bounds({180, 135, 120, 25});

		// Add all widgets
		add(title_);
		add(wifi_label_);
		add(battery_label_);
		add(battery_bar_);
		add(bright_label_);
		add(slider_);
		add(bright_value_);
		add(wifi_toggle_label_);
		add(wifi_toggle_);
		add(led_toggle_label_);
		add(led_toggle_);
		add(action_btn_);
		add(counter_label_);
	}

	void update_model() override
	{
		char buf[32];

		snprintf(buf, sizeof(buf), "WiFi: %s", wifi_on ? "On" : "Off");
		wifi_label_.set_text(buf);

		snprintf(buf, sizeof(buf), "Brightness: %d%%", brightness);
		bright_value_.set_text(buf);

		// Simulate battery drain
		static int tick = 0;
		if (++tick % 120 == 0 && battery > 0)
		{
			--battery;
		}
		snprintf(buf, sizeof(buf), "Battery: %d%%", battery);
		battery_label_.set_text(buf);
		battery_bar_.set_value(battery);

		if (battery < 20)
		{
			battery_bar_.set_fill_color(lumen::Color::rgb(220, 50, 50));
		}
		else
		{
			battery_bar_.set_fill_color(lumen::Color::rgb(50, 200, 80));
		}

		snprintf(buf, sizeof(buf), "Sent: %d", counter);
		counter_label_.set_text(buf);
	}

  private:
	lumen::ui::Label title_;
	lumen::ui::Label wifi_label_;
	lumen::ui::Label battery_label_;
	lumen::ui::ProgressBar battery_bar_;
	lumen::ui::Label bright_label_;
	lumen::ui::Slider slider_;
	lumen::ui::Label bright_value_;
	lumen::ui::Label wifi_toggle_label_;
	lumen::ui::Toggle wifi_toggle_;
	lumen::ui::Label led_toggle_label_;
	lumen::ui::Toggle led_toggle_;
	lumen::ui::Button action_btn_;
	lumen::ui::Label counter_label_;
};

int main()
{
	Board board;
	board.init_hardware();

	lumen::Application<Board> app(board);

	DashboardScreen screen;
	app.navigate_to(screen);
	app.run();

	board.display.destroy();
	return 0;
}
