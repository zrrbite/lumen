#pragma once

#ifdef LUMEN_PLATFORM_DESKTOP

#include <SDL2/SDL.h>

#include <cstring>

#include "lumen/core/types.hpp"

namespace lumen::drivers {

/// SDL2-based display simulator for desktop development.
/// Renders to a resizable window. Supports Rgb565 and Argb8888.
template <uint16_t W, uint16_t H, typename PixFmt = Rgb565> class Sdl2Display
{
  public:
	using PixelFormat = PixFmt;
	using pixel_t	  = typename PixFmt::pixel_t;

	static constexpr uint16_t width() { return W; }
	static constexpr uint16_t height() { return H; }

	void init()
	{
		SDL_Init(SDL_INIT_VIDEO);
		window_	  = SDL_CreateWindow("Lumen Simulator",
									 SDL_WINDOWPOS_CENTERED,
									 SDL_WINDOWPOS_CENTERED,
									 W * scale_,
									 H * scale_,
									 SDL_WINDOW_SHOWN);
		renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
		texture_  = SDL_CreateTexture(renderer_, pixel_format_sdl(), SDL_TEXTUREACCESS_STREAMING, W, H);
		std::memset(framebuffer_, 0, sizeof(framebuffer_));
	}

	void destroy()
	{
		if (texture_)
			SDL_DestroyTexture(texture_);
		if (renderer_)
			SDL_DestroyRenderer(renderer_);
		if (window_)
			SDL_DestroyWindow(window_);
		SDL_Quit();
	}

	/// Set the active drawing window (for partial updates).
	void set_window(Rect area)
	{
		win_x_		= area.x;
		win_y_		= area.y;
		win_w_		= area.w;
		win_h_		= area.h;
		win_cursor_ = 0;
	}

	/// Write pixels into the current window.
	void write_pixels(const pixel_t* data, uint32_t count)
	{
		for (uint32_t i = 0; i < count; ++i)
		{
			uint16_t lx = win_cursor_ % win_w_;
			uint16_t ly = win_cursor_ / win_w_;
			uint16_t gx = win_x_ + lx;
			uint16_t gy = win_y_ + ly;
			if (gx < W && gy < H)
			{
				framebuffer_[gy * W + gx] = data[i];
			}
			++win_cursor_;
		}
	}

	/// DMA stubs for desktop — just forward to blocking write.
	void write_pixels_dma(const pixel_t* data, uint32_t count) { write_pixels(data, count); }
	void write_pixels_dma_wait() {}
	bool dma_busy() const { return false; }

	/// Push framebuffer to screen.
	void flush()
	{
		SDL_UpdateTexture(texture_, nullptr, framebuffer_, W * sizeof(pixel_t));
		SDL_RenderClear(renderer_);
		SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
		SDL_RenderPresent(renderer_);
	}

	/// Direct framebuffer access for canvas rendering.
	pixel_t* framebuffer() { return framebuffer_; }
	const pixel_t* framebuffer() const { return framebuffer_; }

	/// Process SDL events. Returns false if window closed.
	bool poll_events()
	{
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_QUIT)
				return false;
		}
		return true;
	}

	/// Get mouse state for touch simulation.
	bool get_mouse(Point& pos)
	{
		int mx, my;
		uint32_t buttons = SDL_GetMouseState(&mx, &my);
		pos.x			 = static_cast<int16_t>(mx / scale_);
		pos.y			 = static_cast<int16_t>(my / scale_);
		return (buttons & SDL_BUTTON(1)) != 0;
	}

  private:
	static constexpr uint32_t pixel_format_sdl()
	{
		if constexpr (sizeof(pixel_t) == 2)
			return SDL_PIXELFORMAT_RGB565;
		else
			return SDL_PIXELFORMAT_ARGB8888;
	}

	int scale_				= 2; // 2x scale for visibility
	SDL_Window* window_		= nullptr;
	SDL_Renderer* renderer_ = nullptr;
	SDL_Texture* texture_	= nullptr;
	pixel_t framebuffer_[W * H]{};

	// Window cursor for set_window/write_pixels
	int16_t win_x_		 = 0;
	int16_t win_y_		 = 0;
	uint16_t win_w_		 = W;
	uint16_t win_h_		 = H;
	uint32_t win_cursor_ = 0;
};

} // namespace lumen::drivers

#endif // LUMEN_PLATFORM_DESKTOP
