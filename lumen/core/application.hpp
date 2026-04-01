#pragma once

#include <cstddef>

#include "lumen/core/scheduler.hpp"
#include "lumen/gfx/canvas.hpp"
#include "lumen/gfx/dirty_manager.hpp"
#include "lumen/hal/touch_driver.hpp"
#include "lumen/ui/screen.hpp"
#include "lumen/ui/transition.hpp"

namespace lumen {

/// The main application class. Ties together BoardConfig, scheduler,
/// rendering, and input. Template on BoardConfig for zero-overhead
/// hardware abstraction.
template <typename BoardConfig> class Application
{
  public:
	explicit Application(BoardConfig& board) : board_(board) {}

	/// Navigate to a new screen with a transition animation.
	void navigate_to(ui::Screen& screen, ui::Transition trans)
	{
		if (trans.type == ui::TransitionType::Instant)
		{
			// Instant: switch immediately
			if (active_screen_)
				active_screen_->on_leave();
			active_screen_ = &screen;
			active_screen_->on_enter();
			dirty_.add({0, 0, BoardConfig::Display::width(), BoardConfig::Display::height()});
		}
		else
		{
			// Animated: keep both screens alive during transition
			outgoing_screen_ = active_screen_;
			active_screen_	 = &screen;
			active_screen_->on_enter();
			transition_.start(trans, board_.tick.now(), BoardConfig::Display::width(), BoardConfig::Display::height());
			// Force full redraw every frame during transition
			dirty_.add({0, 0, BoardConfig::Display::width(), BoardConfig::Display::height()});
		}
	}

	/// Navigate with instant transition (convenience overload).
	void navigate_to(ui::Screen& screen) { navigate_to(screen, ui::Transition{}); }

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
	const PerfStats& perf() const { return perf_; }

  private:
	BoardConfig& board_;
	Scheduler scheduler_;
	ui::Screen* active_screen_	 = nullptr;
	ui::Screen* outgoing_screen_ = nullptr;
	gfx::DirtyManager dirty_;
	PerfStats perf_{};
	ui::TransitionState transition_;

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
				ui::TouchEvent event{ui::TouchEvent::Type::Press, tp.pos};
				active_screen_->on_touch(event);
			}
			else if (!tp.pressed && was_touching_)
			{
				ui::TouchEvent event{ui::TouchEvent::Type::Release, last_touch_pos_};
				active_screen_->on_touch(event);
			}
			else if (tp.pressed)
			{
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

		TickMs frame_start = board_.tick.now();

		// Update FPS counter (counts all frames, not just rendered ones)
		perf_.frame_count++;
		perf_.fps_frame_count_++;
		if (frame_start - perf_.fps_last_tick_ >= 1000)
		{
			perf_.fps			   = static_cast<uint16_t>(perf_.fps_frame_count_);
			perf_.fps_frame_count_ = 0;
			perf_.fps_last_tick_   = frame_start;
		}

		// Handle transition
		bool in_transition = transition_.is_active();
		if (in_transition)
		{
			transition_.update(board_.tick.now());
			if (!transition_.is_active())
			{
				// Transition complete
				if (outgoing_screen_)
					outgoing_screen_->on_leave();
				outgoing_screen_ = nullptr;
				in_transition	 = false;
			}
			// Force full redraw every frame during transition
			dirty_.add({0, 0, BoardConfig::Display::width(), BoardConfig::Display::height()});
		}

		// Update model
		active_screen_->update_model();
		if (in_transition && outgoing_screen_)
			outgoing_screen_->update_model();

		// Collect dirty regions from widgets
		collect_dirty(*active_screen_);
		if (in_transition && outgoing_screen_)
			collect_dirty(*outgoing_screen_);

		if (!dirty_.has_dirty())
			return;

		TickMs render_start = board_.tick.now();

		// Render — adapt to boards with or without framebuffer
		using PF				  = typename BoardConfig::PixFmt;
		using pixel_t			  = typename PF::pixel_t;
		constexpr uint16_t disp_w = BoardConfig::Display::width();
		constexpr uint16_t disp_h = BoardConfig::Display::height();

		if constexpr (BoardConfig::framebuffer_count > 0)
		{
			// Framebuffer mode (SDL2, LTDC with SDRAM)
			auto* fb = board_.display.framebuffer();
			gfx::Canvas<PF> canvas(fb, disp_w, disp_h);
			for (uint8_t i = 0; i < dirty_.count(); ++i)
			{
				canvas.set_clip(dirty_.rect(i));
				active_screen_->draw(canvas);
			}
			board_.display.flush();
		}
		else
		{
			// Direct-to-display mode (SPI TFT, no framebuffer)
			// Render line-by-line using scratch buffer
			static constexpr size_t scratch_pixels = BoardConfig::scratch_buffer_size / sizeof(pixel_t);
			static pixel_t scratch[scratch_pixels];

			for (uint8_t d = 0; d < dirty_.count(); ++d)
			{
				Rect dirty = dirty_.rect(d);
				// Render in horizontal bands that fit the scratch buffer
				uint16_t band_h = scratch_pixels / dirty.w;
				if (band_h == 0)
					band_h = 1;

				for (int16_t band_y = dirty.y; band_y < dirty.bottom(); band_y += band_h)
				{
					uint16_t h = band_h;
					if (band_y + h > dirty.bottom())
						h = dirty.bottom() - band_y;

					uint32_t band_pixels = static_cast<uint32_t>(dirty.w) * h;

					// Clear scratch buffer (DMA2D on STM32, CPU loop on desktop)
					BoardConfig::hw_fill(scratch, dirty.w, h, 0);

					Rect band{dirty.x, band_y, dirty.w, h};
					gfx::Canvas<PF> canvas(scratch, dirty.w, h);

					if (in_transition && outgoing_screen_)
					{
						auto& tr  = transition_;
						auto type = tr.transition().type;

						if (type == ui::TransitionType::SlideBoth || type == ui::TransitionType::SlideIn ||
							type == ui::TransitionType::SlideOut)
						{
							// Draw outgoing with offset
							Point out_off = tr.outgoing_offset();
							canvas.set_origin(static_cast<int16_t>(dirty.x - out_off.x),
											  static_cast<int16_t>(band_y - out_off.y));
							canvas.set_clip(band);
							outgoing_screen_->draw(canvas);

							// Draw incoming with offset (overwrites where it overlaps)
							Point in_off = tr.incoming_offset();
							canvas.set_origin(static_cast<int16_t>(dirty.x - in_off.x),
											  static_cast<int16_t>(band_y - in_off.y));
							canvas.set_clip(band);
							active_screen_->draw(canvas);
						}
						else if (type == ui::TransitionType::WipeDown || type == ui::TransitionType::WipeRight)
						{
							// Outgoing for its clip region
							Rect out_clip = tr.outgoing_clip().intersection(band);
							if (!out_clip.empty())
							{
								canvas.set_origin(dirty.x, band_y);
								canvas.set_clip(out_clip);
								outgoing_screen_->draw(canvas);
							}
							// Incoming for its clip region
							Rect in_clip = tr.incoming_clip().intersection(band);
							if (!in_clip.empty())
							{
								canvas.set_origin(dirty.x, band_y);
								canvas.set_clip(in_clip);
								active_screen_->draw(canvas);
							}
						}
						else
						{
							// Fallback: just draw incoming
							canvas.set_origin(dirty.x, band_y);
							canvas.set_clip(band);
							active_screen_->draw(canvas);
						}
					}
					else
					{
						canvas.set_origin(dirty.x, band_y);
						canvas.set_clip(band);
						active_screen_->draw(canvas);
					}

					board_.display.set_window(band);
					board_.display.write_pixels(scratch, band_pixels);
				}
			}
		}

		dirty_.clear();

		// Update perf stats
		TickMs now			 = board_.tick.now();
		perf_.render_time_ms = now - render_start;
		perf_.frame_time_ms	 = now - frame_start;
	}

	void collect_dirty(ui::Widget& widget)
	{
		if (widget.is_dirty())
		{
			dirty_.add(widget.bounds());
			widget.mark_clean();
		}
		// Recurse into containers (no RTTI needed)
		if (widget.is_container())
		{
			auto* container = static_cast<ui::Container*>(&widget);
			for (uint8_t i = 0; i < container->child_count(); ++i)
			{
				collect_dirty(*container->child(i));
			}
		}
	}
};

} // namespace lumen
