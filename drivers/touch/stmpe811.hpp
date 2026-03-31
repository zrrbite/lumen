#pragma once

/// STMPE811 resistive touch controller driver.
/// Used on STM32F429-DISCO.

#include "lumen/hal/touch_driver.hpp"

namespace lumen::drivers {

/// Delay callback type.
using DelayMs = void (*)(uint32_t ms);

template <typename I2cDrv> class Stmpe811
{
  public:
	static constexpr uint8_t ADDR = 0x41; // Default I2C address

	explicit Stmpe811(DelayMs delay_fn) : delay_(delay_fn) {}

	void init()
	{
		// Soft reset
		I2cDrv::write_reg(ADDR, 0x03, 0x02); // SYS_CTRL1: soft reset
		delay_(10);
		I2cDrv::write_reg(ADDR, 0x03, 0x00); // Clear reset

		// Enable TSC and ADC clocks
		I2cDrv::write_reg(ADDR, 0x04, 0x0C); // SYS_CTRL2: enable TSC + ADC

		// Configure ADC
		I2cDrv::write_reg(ADDR, 0x20, 0x49); // ADC_CTRL1: sample time, 12-bit
		delay_(2);
		I2cDrv::write_reg(ADDR, 0x21, 0x01); // ADC_CTRL2: ADC clock speed

		// Configure GPIO AF for touchscreen
		I2cDrv::write_reg(ADDR, 0x17, 0x00); // GPIO_AF: all pins as ADC/TSC

		// Configure touchscreen
		I2cDrv::write_reg(ADDR, 0x41, 0x9A); // TSC_CFG: averaging, touch detect delay
		I2cDrv::write_reg(ADDR, 0x4A, 0x01); // FIFO_TH: FIFO threshold = 1
		I2cDrv::write_reg(ADDR, 0x4B, 0x01); // FIFO_STA: reset FIFO
		I2cDrv::write_reg(ADDR, 0x4B, 0x00); // FIFO_STA: unreset

		// Set fraction Z
		I2cDrv::write_reg(ADDR, 0x56, 0x07); // TSC_FRACT_Z
		I2cDrv::write_reg(ADDR, 0x42, 0x01); // TSC_I_DRIVE: 50mA

		// Enable touchscreen
		I2cDrv::write_reg(ADDR, 0x40, 0x01); // TSC_CTRL: enable, no window
	}

	bool poll(hal::TouchPoint& out)
	{
		uint8_t status = I2cDrv::read_reg(ADDR, 0x40); // TSC_CTRL
		bool touching  = (status & (1U << 7)) != 0;

		if (touching)
		{
			// Read XY data (12-bit each, packed in 3 bytes)
			uint16_t raw_x = I2cDrv::read_reg16(ADDR, 0x4D); // TSC_DATA_X
			uint16_t raw_y = I2cDrv::read_reg16(ADDR, 0x4F); // TSC_DATA_Y

			// Map to display coordinates (240x320)
			// STMPE811 returns ~200-3900 range, map to pixels
			int16_t px = static_cast<int16_t>(((raw_x - 200) * 240) / 3700);
			int16_t py = static_cast<int16_t>(((raw_y - 200) * 320) / 3700);

			// Clamp
			if (px < 0)
				px = 0;
			if (px > 239)
				px = 239;
			if (py < 0)
				py = 0;
			if (py > 319)
				py = 319;

			out.pos		 = {px, py};
			out.pressed	 = true;
			out.pressure = 128;

			// Reset FIFO
			I2cDrv::write_reg(ADDR, 0x4B, 0x01);
			I2cDrv::write_reg(ADDR, 0x4B, 0x00);

			return true;
		}
		else if (was_pressed_)
		{
			out.pos		 = last_pos_;
			out.pressed	 = false;
			out.pressure = 0;
			was_pressed_ = false;
			return true;
		}

		return false;
	}

	uint8_t max_points() const { return 1; }

  private:
	DelayMs delay_;
	bool was_pressed_ = false;
	Point last_pos_	  = {0, 0};
};

} // namespace lumen::drivers
