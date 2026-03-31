/// Lumen interactive demo on STM32F429-DISCO.
/// Touch the buttons! Uses ILI9341 display + STMPE811 touch.

#include "lumen/core/application.hpp"
#include "lumen/ui/screen.hpp"
#include "lumen/ui/widgets/button.hpp"
#include "lumen/ui/widgets/label.hpp"
#include "lumen/ui/widgets/progress_bar.hpp"
#include "platform/stm32f429_disco/board_config.hpp"

using Board = lumen::platform::Stm32f429DiscoConfig;

static int touch_count	 = 0;
static uint8_t bar_value = 0;

// Simple int-to-string (no snprintf, no newlib locks)
static void int_to_str(char* buf, const char* prefix, int val)
{
	while (*prefix)
		*buf++ = *prefix++;
	if (val == 0)
	{
		*buf++ = '0';
		*buf   = '\0';
		return;
	}
	if (val < 0)
	{
		*buf++ = '-';
		val	   = -val;
	}
	char tmp[12];
	int len = 0;
	while (val > 0)
	{
		tmp[len++] = '0' + (val % 10);
		val /= 10;
	}
	for (int i = len - 1; i >= 0; --i)
		*buf++ = tmp[i];
	*buf = '\0';
}

class HelloScreen : public lumen::ui::Screen
{
  public:
	HelloScreen()
	{
		title_.set_text("Lumen on STM32!");
		title_.set_bounds({10, 10, 220, 20});
		title_.set_color(lumen::Color::rgb(100, 200, 255));
		title_.set_bg_color(lumen::Color::rgb(20, 20, 30));

		counter_.set_text("Touches: 0");
		counter_.set_bounds({10, 40, 220, 20});
		counter_.set_bg_color(lumen::Color::rgb(20, 20, 30));

		btn_.set_label("Touch Me!");
		btn_.set_bounds({20, 80, 200, 60});
		btn_.set_on_click([] {
			++touch_count;
			bar_value += 10;
			if (bar_value > 100)
				bar_value = 0;
		});

		reset_btn_.set_label("Reset");
		reset_btn_.set_bounds({20, 160, 200, 50});
		reset_btn_.set_color(lumen::Color::rgb(180, 60, 60), lumen::Color::rgb(120, 40, 40));
		reset_btn_.set_on_click([] {
			touch_count = 0;
			bar_value	= 0;
		});

		bar_.set_bounds({20, 230, 200, 20});
		bar_.set_fill_color(lumen::Color::rgb(50, 200, 80));

		status_.set_text("Touch the buttons!");
		status_.set_bounds({10, 270, 220, 20});
		status_.set_color(lumen::Color::rgb(150, 150, 160));
		status_.set_bg_color(lumen::Color::rgb(20, 20, 30));

		add(title_);
		add(counter_);
		add(btn_);
		add(reset_btn_);
		add(bar_);
		add(status_);
	}

	void update_model() override
	{
		char buf[32];
		int_to_str(buf, "Touches: ", touch_count);
		counter_.set_text(buf);
		bar_.set_value(bar_value);
	}

  private:
	lumen::ui::Label title_;
	lumen::ui::Label counter_;
	lumen::ui::Button btn_;
	lumen::ui::Button reset_btn_;
	lumen::ui::ProgressBar bar_;
	lumen::ui::Label status_;
};

int main()
{
	Board board;
	board.init_hardware();

	lumen::Application<Board> app(board);

	HelloScreen screen;
	app.navigate_to(screen);
	app.run();

	return 0;
}
