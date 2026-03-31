/// Lumen interactive demo on STM32F429-DISCO.
/// Touch the buttons! Live-reload over UART (115200 8N1).
/// Connect to ST-LINK VCP and send: SET title.text New Title

#include "lumen/core/application.hpp"
#include "lumen/gfx/fonts/liberation_sans_10.hpp"
#include "lumen/gfx/fonts/liberation_sans_14.hpp"
#include "lumen/gfx/fonts/liberation_sans_bold_10.hpp"
#include "lumen/gfx/fonts/liberation_sans_sdf16.hpp"
#include "lumen/hal/stm32/gpio.hpp"
#include "lumen/hal/stm32/uart.hpp"
#include "lumen/ui/animation.hpp"
#include "lumen/ui/live_reload.hpp"
#include "lumen/ui/screen.hpp"
#include "lumen/ui/widget_registry.hpp"
#include "lumen/ui/widgets/button.hpp"
#include "lumen/ui/widgets/label.hpp"
#include "lumen/ui/widgets/progress_bar.hpp"
#include "lumen/ui/widgets/sdf_label.hpp"
#include "platform/stm32f429_disco/board_config.hpp"

using Board	  = lumen::platform::Stm32f429DiscoConfig;
using App	  = lumen::Application<Board>;
using Usart1  = lumen::hal::stm32::Usart<lumen::hal::stm32::UsartInstance::USART1>;
using UART_TX = lumen::hal::stm32::GpioPin<lumen::hal::stm32::Port::A, 9>;
using UART_RX = lumen::hal::stm32::GpioPin<lumen::hal::stm32::Port::A, 10>;

static int touch_count	= 0;
static float bar_target = 0.0f;
static App* g_app		= nullptr;

static float bar_animated = 0.0f;

static lumen::ui::AnimationManager anim;
static lumen::ui::WidgetRegistry registry;
static lumen::ui::LiveReload* g_live = nullptr;

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

static void uart_respond(const char* text)
{
	Usart1::send_string(text);
}

class HelloScreen : public lumen::ui::Screen
{
  public:
	HelloScreen()
	{
		title_.set_text("Lumen SDF!");
		title_.set_bounds({10, 4, 220, 28});
		title_.set_font(&lumen::gfx::liberation_sans_sdf16);
		title_.set_target_size(20); // Render 16px SDF at 20px
		title_.set_color(lumen::Color::rgb(100, 200, 255));
		title_.set_bg_color(lumen::Color::rgb(20, 20, 30));

		counter_.set_text("Touches: 0");
		counter_.set_bounds({10, 38, 220, 20});
		counter_.set_font(&lumen::gfx::liberation_sans_10);
		counter_.set_bg_color(lumen::Color::rgb(20, 20, 30));

		btn_.set_label("Touch Me!");
		btn_.set_bounds({20, 70, 200, 60});
		btn_.set_font(&lumen::gfx::liberation_sans_bold_10);
		btn_.set_on_click([] {
			++touch_count;
			bar_target += 10.0f;
			if (bar_target > 100.0f)
				bar_target = 0.0f;
			if (g_app)
			{
				anim.cancel_target(&bar_animated);
				anim.animate(&bar_animated,
							 bar_animated,
							 bar_target,
							 300,
							 g_app->board().tick.now(),
							 lumen::ui::ease::out_cubic);
			}
		});

		reset_btn_.set_label("Reset");
		reset_btn_.set_bounds({20, 145, 200, 50});
		reset_btn_.set_font(&lumen::gfx::liberation_sans_bold_10);
		reset_btn_.set_color(lumen::Color::rgb(180, 60, 60), lumen::Color::rgb(120, 40, 40));
		reset_btn_.set_on_click([] {
			touch_count = 0;
			bar_target	= 0.0f;
			if (g_app)
			{
				anim.cancel_target(&bar_animated);
				anim.animate(
					&bar_animated, bar_animated, 0.0f, 500, g_app->board().tick.now(), lumen::ui::ease::out_bounce);
			}
		});

		bar_.set_bounds({20, 210, 200, 20});
		bar_.set_fill_color(lumen::Color::rgb(50, 200, 80));

		perf_.set_text("FPS: --");
		perf_.set_bounds({10, 245, 220, 18});
		perf_.set_font(&lumen::gfx::liberation_sans_10);
		perf_.set_color(lumen::Color::rgb(100, 100, 120));
		perf_.set_bg_color(lumen::Color::rgb(20, 20, 30));

		status_.set_text("UART: 115200 8N1");
		status_.set_bounds({10, 270, 220, 20});
		status_.set_font(&lumen::gfx::liberation_sans_10);
		status_.set_color(lumen::Color::rgb(150, 150, 160));
		status_.set_bg_color(lumen::Color::rgb(20, 20, 30));

		add(title_);
		add(counter_);
		add(btn_);
		add(reset_btn_);
		add(bar_);
		add(perf_);
		add(status_);

		// Register widgets for live reload
		// title_ is SdfLabel, not in registry yet
		registry.add("counter", counter_);
		registry.add("btn", btn_);
		registry.add("reset", reset_btn_);
		registry.add("bar", bar_);
		registry.add("perf", perf_);
		registry.add("status", status_);
	}

	void update_model() override
	{
		char buf[32];
		int_to_str(buf, "Touches: ", touch_count);
		counter_.set_text(buf);
		btn_.tick_visual();
		reset_btn_.tick_visual();

		if (g_app)
			anim.update(g_app->board().tick.now());

		bar_.set_value(static_cast<uint8_t>(bar_animated));

		// Poll UART for live reload commands
		if (g_live)
		{
			while (Usart1::rx_available())
			{
				g_live->feed(Usart1::receive());
			}
		}

		if (g_app)
		{
			const auto& stats = g_app->perf();
			char pbuf[40];
			int_to_str(pbuf, "FPS:", static_cast<int>(stats.fps));
			char* p = pbuf;
			while (*p)
				++p;
			*p++ = ' ';
			int_to_str(p, "R:", static_cast<int>(stats.render_time_ms));
			p = pbuf;
			while (*p)
				++p;
			*p++ = 'm';
			*p++ = 's';
			*p	 = '\0';
			perf_.set_text(pbuf);
		}
	}

	lumen::ui::SdfLabel title_;
	lumen::ui::Label counter_;
	lumen::ui::Button btn_;
	lumen::ui::Button reset_btn_;
	lumen::ui::ProgressBar bar_;
	lumen::ui::Label perf_;
	lumen::ui::Label status_;
};

int main()
{
	Board board;
	board.init_hardware();

	// Init UART1 for live reload (PA9=TX, PA10=RX, AF7)
	lumen::hal::stm32::enable_gpio_clock(lumen::hal::stm32::Port::A);
	UART_TX::init_af(7);
	UART_RX::init_af(7);
	Usart1::init(90000000, 115200); // APB2=90MHz

	lumen::ui::LiveReload live(registry);
	live.set_response_callback(uart_respond);
	g_live = &live;

	App app(board);
	g_app = &app;

	HelloScreen screen;
	app.navigate_to(screen);

	// Send ready message
	Usart1::send_string("Lumen live reload ready\n");

	app.run();

	return 0;
}
