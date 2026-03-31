/// Lumen Hello World on STM32F429-DISCO.
/// Draws colored rectangles and text on the ILI9341 display.

#include "lumen/core/types.hpp"
#include "lumen/gfx/font.hpp"
#include "platform/stm32f429_disco/board_config.hpp"

using Board = lumen::platform::Stm32f429DiscoConfig;

int main()
{
	Board board;
	board.init_hardware();

	auto& display = board.display;

	// Fill screen with dark background
	display.fill({0, 0, 240, 320}, lumen::Color::rgb(20, 20, 30).to_rgb565());

	// Draw colored rectangles
	display.fill({20, 20, 200, 60}, lumen::Color::rgb(220, 50, 50).to_rgb565());
	display.fill({20, 100, 200, 60}, lumen::Color::rgb(50, 180, 50).to_rgb565());
	display.fill({20, 180, 200, 60}, lumen::Color::rgb(50, 100, 220).to_rgb565());

	// Draw text using scratch buffer approach
	// (direct-to-display rendering — no framebuffer on M4)
	uint16_t line_buf[240]; // One line scratch

	// Render "Hello, Lumen!" at y=270 line by line
	const auto& font		= lumen::gfx::font_6x8;
	const char* text		= "Hello, Lumen!";
	lumen::Color text_color = lumen::Color::white();

	for (uint8_t row = 0; row < font.char_height; ++row)
	{
		// Clear line
		for (int col = 0; col < 240; ++col)
		{
			line_buf[col] = lumen::Color::rgb(20, 20, 30).to_rgb565();
		}

		// Render font row into line buffer
		const char* ptr	 = text;
		int16_t cursor_x = 20;
		while (*ptr)
		{
			uint8_t ch = static_cast<uint8_t>(*ptr);
			if (ch >= font.first_char && ch <= font.last_char)
			{
				uint16_t glyph_offset = (ch - font.first_char) * font.char_height * font.bytes_per_row;
				uint8_t byte_val	  = font.data[glyph_offset + row * font.bytes_per_row];
				for (uint8_t col = 0; col < font.char_width; ++col)
				{
					if (byte_val & (1 << (7 - col)))
					{
						if (cursor_x + col < 240)
						{
							line_buf[cursor_x + col] = text_color.to_rgb565();
						}
					}
				}
			}
			cursor_x += font.char_width + 1;
			++ptr;
		}

		// Send line to display
		display.set_window({0, static_cast<int16_t>(270 + row), 240, 1});
		display.write_pixels(line_buf, 240);
	}

	// Idle loop
	while (true)
	{
		// WFI would go here for power saving
	}

	return 0;
}
