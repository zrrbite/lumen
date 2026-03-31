#pragma once

/// FT6206/FT6236 capacitive touch controller driver.
/// Used on STM32F769-DISCO and many other boards.
/// I2C address: 0x38

#include "lumen/hal/touch_driver.hpp"

namespace lumen::drivers {

template <typename I2cDrv> class Ft6206
{
  public:
	static constexpr uint8_t ADDR = 0x38;

	void init()
	{
		// FT6206 self-calibrates on power-up.
		// Set threshold for touch detection.
		I2cDrv::write_reg(ADDR, 0x80, 40); // TH_GROUP: touch threshold
		I2cDrv::write_reg(ADDR, 0x00, 0);  // DEV_MODE: operating mode
	}

	bool poll(hal::TouchPoint& out)
	{
		// Read number of touch points (register 0x02)
		uint8_t num_points = I2cDrv::read_reg(ADDR, 0x02) & 0x0F;

		if (num_points > 0 && num_points < 6)
		{
			// Read first touch point (registers 0x03-0x06)
			uint8_t xh = I2cDrv::read_reg(ADDR, 0x03);
			uint8_t xl = I2cDrv::read_reg(ADDR, 0x04);
			uint8_t yh = I2cDrv::read_reg(ADDR, 0x05);
			uint8_t yl = I2cDrv::read_reg(ADDR, 0x06);

			uint8_t event = (xh >> 6) & 0x03;
			int16_t raw_x = static_cast<int16_t>(((xh & 0x0F) << 8) | xl);
			int16_t raw_y = static_cast<int16_t>(((yh & 0x0F) << 8) | yl);

			// Clamp to display bounds (caller should configure these)
			if (raw_x < 0)
				raw_x = 0;
			if (raw_x >= disp_w_)
				raw_x = disp_w_ - 1;
			if (raw_y < 0)
				raw_y = 0;
			if (raw_y >= disp_h_)
				raw_y = disp_h_ - 1;

			out.pos		 = {raw_x, raw_y};
			out.pressed	 = (event != 1); // 0=press down, 2=contact, 1=lift up
			out.pressure = 128;

			was_pressed_ = out.pressed;
			last_pos_	 = out.pos;
			return true;
		}

		if (was_pressed_)
		{
			// Report release
			out.pos		 = last_pos_;
			out.pressed	 = false;
			out.pressure = 0;
			was_pressed_ = false;
			return true;
		}

		return false;
	}

	void set_display_size(int16_t w, int16_t h)
	{
		disp_w_ = w;
		disp_h_ = h;
	}

	uint8_t max_points() const { return 2; }

  private:
	bool was_pressed_ = false;
	Point last_pos_	  = {0, 0};
	int16_t disp_w_	  = 800;
	int16_t disp_h_	  = 480;
};

} // namespace lumen::drivers
