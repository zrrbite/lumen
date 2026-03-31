#pragma once

#include "lumen/core/scheduler.hpp"
#include "lumen/gfx/canvas.hpp"
#include "lumen/gfx/dirty_manager.hpp"
#include "lumen/hal/touch_driver.hpp"
#include "lumen/ui/screen.hpp"

namespace lumen {

/// The main application class. Ties together BoardConfig, scheduler,
/// rendering, and input. Template on BoardConfig for zero-overhead
/// hardware abstraction.
template <typename BoardConfig> class Application
{
  public:
	explicit Application(BoardConfig& board) : board_(board) {}

	/// Set the active screen.
	void navigate_to(ui::Screen& screen)
	{
		if (active_screen_)
		{
			active_screen_->on_leave();
		}
		active_screen_ = &screen;
		active_screen_->on_enter();
		// Force full redraw
		dirty_.add({0, 0, BoardConfig::Display::width(), BoardConfig::Display::height()});
	}

	/// Run the main loop. Never returns.
	void run()
	{
		// Register built-in tasks
		scheduler_.add(input_task_);
		scheduler_.add(render_task_);

		while (board_.poll_events())
		{
			scheduler_.tick(board_.tick);
			board_.tick.delay(1);
		}
	}

	Scheduler& scheduler() { return scheduler_; }
	BoardConfig& board() { return board_; }

  private:
	BoardConfig& board_;
	Scheduler scheduler_;
	ui::Screen* active_screen_ = nullptr;
	gfx::DirtyManager dirty_;

	// Track touch state for press/release detection
	bool was_touching_ = false;
	Point last_touch_pos_{};

	/// Input task — polls touch and dispatches events.
	class InputTaskImpl : public Task
	{
	  public:
		explicit InputTaskImpl(Application& app) : app_(app) {}
		void execute(TickMs /*now*/) override { app_.process_input(); }
		TickMs period_ms() const override { return 0; } // Every cycle
		uint8_t priority() const override { return 10; }
		const char* name() const override { return "input"; }

	  private:
		Application& app_;
	};

	/// Render task — processes dirty rects and draws.
	class RenderTaskImpl : public Task
	{
	  public:
		explicit RenderTaskImpl(Application& app) : app_(app) {}
		void execute(TickMs /*now*/) override { app_.render(); }
		TickMs period_ms() const override { return 16; } // ~60 FPS
		uint8_t priority() const override { return 20; }
		const char* name() const override { return "render"; }

	  private:
		Application& app_;
	};

	InputTaskImpl input_task_{*this};
	RenderTaskImpl render_task_{*this};

	void process_input()
	{
		if (!active_screen_)
			return;

		hal::TouchPoint tp;
		if (board_.touch.poll(tp))
		{
			if (tp.pressed && !was_touching_)
			{
				// Press
				ui::TouchEvent event{ui::TouchEvent::Type::Press, tp.pos};
				active_screen_->on_touch(event);
			}
			else if (!tp.pressed && was_touching_)
			{
				// Release
				ui::TouchEvent event{ui::TouchEvent::Type::Release, last_touch_pos_};
				active_screen_->on_touch(event);
			}
			else if (tp.pressed)
			{
				// Move
				ui::TouchEvent event{ui::TouchEvent::Type::Move, tp.pos};
				active_screen_->on_touch(event);
			}
			was_touching_	= tp.pressed;
			last_touch_pos_ = tp.pos;
		}
	}

	void render()
	{
		if (!active_screen_)
			return;

		// Update model
		active_screen_->update_model();

		// Collect dirty regions from widgets
		collect_dirty(*active_screen_);

		if (!dirty_.has_dirty())
			return;

		// Render into framebuffer
		using PF = typename BoardConfig::PixFmt;
		auto* fb = board_.display.framebuffer();
		gfx::Canvas<PF> canvas(fb, BoardConfig::Display::width(), BoardConfig::Display::height());

		for (uint8_t i = 0; i < dirty_.count(); ++i)
		{
			canvas.set_clip(dirty_.rect(i));
			active_screen_->draw(canvas);
		}

		board_.display.flush();
		dirty_.clear();
	}

	void collect_dirty(ui::Widget& widget)
	{
		if (widget.is_dirty())
		{
			dirty_.add(widget.bounds());
			widget.mark_clean();
		}
		// Recurse into containers
		if (auto* container = dynamic_cast<ui::Container*>(&widget))
		{
			for (uint8_t i = 0; i < container->child_count(); ++i)
			{
				collect_dirty(*container->child(i));
			}
		}
	}
};

} // namespace lumen
