/// Lumen "Hello Rectangle" — first visual test.
/// Draws colored rectangles on the SDL2 simulator.

#include "lumen/core/scheduler.hpp"
#include "lumen/gfx/canvas.hpp"
#include "platform/desktop_sdl2/board_config.hpp"

using Board = lumen::platform::DesktopSdl2Config<320, 240>;

/// A simple task that draws some rectangles and flushes the display.
class DrawTask : public lumen::Task
{
  public:
	explicit DrawTask(Board& board) : board_(board) {}

	void execute(lumen::TickMs now) override
	{
		using PF = Board::PixFmt;
		auto* fb = board_.display.framebuffer();
		lumen::gfx::Canvas<PF> canvas(fb, Board::Display::width(), Board::Display::height());

		// Background
		canvas.set_clip({0, 0, 320, 240});
		canvas.fill_rect({0, 0, 320, 240}, lumen::Color::rgb(30, 30, 40));

		// Colored rectangles
		canvas.fill_rect({20, 20, 120, 80}, lumen::Color::rgb(220, 50, 50));	// Red
		canvas.fill_rect({160, 20, 120, 80}, lumen::Color::rgb(50, 180, 50));	// Green
		canvas.fill_rect({20, 120, 120, 80}, lumen::Color::rgb(50, 100, 220));	// Blue
		canvas.fill_rect({160, 120, 120, 80}, lumen::Color::rgb(220, 180, 50)); // Yellow

		// Borders
		canvas.draw_rect({20, 20, 120, 80}, lumen::Color::white());
		canvas.draw_rect({160, 20, 120, 80}, lumen::Color::white());
		canvas.draw_rect({20, 120, 120, 80}, lumen::Color::white());
		canvas.draw_rect({160, 120, 120, 80}, lumen::Color::white());

		board_.display.flush();
	}

	lumen::TickMs period_ms() const override { return 16; } // ~60 FPS
	uint8_t priority() const override { return 20; }
	const char* name() const override { return "draw"; }

  private:
	Board& board_;
};

int main()
{
	Board board;
	board.init_hardware();

	lumen::Scheduler scheduler;
	DrawTask draw_task(board);
	scheduler.add(draw_task);

	// Main loop
	while (board.poll_events())
	{
		scheduler.tick(board.tick);
		board.tick.delay(1); // Don't burn CPU
	}

	board.display.destroy();
	return 0;
}
