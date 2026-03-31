/// Lumen "Bouncing Rectangles" — animation test.
/// Four colored rectangles bounce around the screen.

#include "lumen/core/scheduler.hpp"
#include "lumen/gfx/canvas.hpp"
#include "platform/desktop_sdl2/board_config.hpp"

using Board = lumen::platform::DesktopSdl2Config<320, 240>;

static constexpr int16_t SCREEN_W = 320;
static constexpr int16_t SCREEN_H = 240;
static constexpr int16_t BOX_W	  = 60;
static constexpr int16_t BOX_H	  = 40;

struct Box
{
	int16_t x, y;
	int16_t dx, dy; // velocity
	lumen::Color color;
};

class DrawTask : public lumen::Task
{
  public:
	explicit DrawTask(Board& board) : board_(board)
	{
		boxes_[0] = {20, 20, 2, 1, lumen::Color::rgb(220, 50, 50)};
		boxes_[1] = {160, 30, -1, 2, lumen::Color::rgb(50, 180, 50)};
		boxes_[2] = {80, 120, 1, -1, lumen::Color::rgb(50, 100, 220)};
		boxes_[3] = {200, 150, -2, -1, lumen::Color::rgb(220, 180, 50)};
	}

	void execute(lumen::TickMs /*now*/) override
	{
		using PF = Board::PixFmt;
		auto* fb = board_.display.framebuffer();
		lumen::gfx::Canvas<PF> canvas(fb, SCREEN_W, SCREEN_H);

		// Clear background
		canvas.set_clip({0, 0, SCREEN_W, SCREEN_H});
		canvas.fill_rect({0, 0, SCREEN_W, SCREEN_H}, lumen::Color::rgb(20, 20, 30));

		// Update and draw each box
		for (auto& box : boxes_)
		{
			// Move
			box.x += box.dx;
			box.y += box.dy;

			// Bounce off walls
			if (box.x <= 0 || box.x + BOX_W >= SCREEN_W)
			{
				box.dx = -box.dx;
				box.x += box.dx;
			}
			if (box.y <= 0 || box.y + BOX_H >= SCREEN_H)
			{
				box.dy = -box.dy;
				box.y += box.dy;
			}

			// Draw filled box with border
			lumen::Rect r{box.x, box.y, BOX_W, BOX_H};
			canvas.fill_rect(r, box.color);
			canvas.draw_rect(r, lumen::Color::white());
		}

		board_.display.flush();
	}

	lumen::TickMs period_ms() const override { return 16; } // ~60 FPS
	uint8_t priority() const override { return 20; }
	const char* name() const override { return "draw"; }

  private:
	Board& board_;
	Box boxes_[4]{};
};

int main()
{
	Board board;
	board.init_hardware();

	lumen::Scheduler scheduler;
	DrawTask draw_task(board);
	scheduler.add(draw_task);

	while (board.poll_events())
	{
		scheduler.tick(board.tick);
		board.tick.delay(1);
	}

	board.display.destroy();
	return 0;
}
