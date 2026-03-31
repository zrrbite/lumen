#pragma once

/// STMPE811 resistive touch controller driver.
/// Init sequence and touch reading based on ST's BSP driver.
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
		delay_(2);

		// SYS_CTRL2: enable GPIO + TSC + ADC, disable only temp sensor
		// Must enable GPIO clock BEFORE writing GPIO_AF register
		I2cDrv::write_reg(ADDR, 0x04, 0x08); // SYS_CTRL2: only TS_OFF=1

		// Select TSC pins in TSC alternate mode (GPIO clock must be on)
		I2cDrv::write_reg(ADDR, 0x17, 0x00); // GPIO_AF: all pins as TSC

		// Configure ADC: sample time=80 clocks, 12-bit, internal ref
		I2cDrv::write_reg(ADDR, 0x20, 0x48); // ADC_CTRL1
		delay_(2);

		// ADC clock speed: 3.25 MHz
		I2cDrv::write_reg(ADDR, 0x21, 0x01); // ADC_CTRL2

		// Touch screen config:
		// - Touch average control: 4 samples
		// - Touch delay time: 500 uS
		// - Panel driver settling time: 500 uS
		I2cDrv::write_reg(ADDR, 0x41, 0x9A); // TSC_CFG

		// FIFO threshold = 1
		I2cDrv::write_reg(ADDR, 0x4A, 0x01); // FIFO_TH

		// Reset FIFO
		I2cDrv::write_reg(ADDR, 0x4B, 0x01); // FIFO_STA: reset
		I2cDrv::write_reg(ADDR, 0x4B, 0x00); // FIFO_STA: unreset

		// Fractional part = 1, whole part = 7 (for Z pressure)
		I2cDrv::write_reg(ADDR, 0x56, 0x01); // TSC_FRACT_XYZ

		// Driving capability: 50mA
		I2cDrv::write_reg(ADDR, 0x58, 0x01); // TSC_I_DRIVE

		// Enable TSC: XY acquisition mode, tracking enabled, window=127
		// 0x73 = EN | OP_MOD=XY_only(11) | TRACK(1) | TRACK_0=11
		I2cDrv::write_reg(ADDR, 0x40, 0x73); // TSC_CTRL

		// Clear all pending interrupt status
		I2cDrv::write_reg(ADDR, 0x0B, 0xFF); // INT_STA

		delay_(2);
	}

	bool poll(hal::TouchPoint& out)
	{
		// Use TOUCH_DET (TSC_CTRL bit 7) for finger presence —
		// stays set as long as finger is on screen, unlike FIFO
		// which can briefly empty between ADC conversions.
		uint8_t tsc_ctrl = I2cDrv::read_reg(ADDR, 0x40); // TSC_CTRL
		bool touching	 = (tsc_ctrl & (1U << 7)) != 0;

		// Read position from FIFO if data available
		uint8_t fifo_level = I2cDrv::read_reg(ADDR, 0x4C); // FIFO_SIZE
		if (fifo_level > 0)
		{
			uint8_t data[3];
			I2cDrv::read_reg_multi(ADDR, 0xD7, data, 3); // TSC_DATA_NON_INC

			uint32_t raw = (static_cast<uint32_t>(data[0]) << 16) | (static_cast<uint32_t>(data[1]) << 8) |
						   static_cast<uint32_t>(data[2]);
			uint16_t raw_x = (raw >> 12) & 0xFFF;
			uint16_t raw_y = raw & 0xFFF;

			// Map 12-bit ADC values to display coordinates (240x320)
			// STM32F429-DISCO: no swap, Y axis inverted
			int16_t px = static_cast<int16_t>((raw_x * 240) / 4096);
			int16_t py = static_cast<int16_t>(320 - (raw_y * 320) / 4096);

			if (px < 0)
				px = 0;
			if (px > 239)
				px = 239;
			if (py < 0)
				py = 0;
			if (py > 319)
				py = 319;

			last_pos_ = {px, py};

			// Reset FIFO
			I2cDrv::write_reg(ADDR, 0x4B, 0x01);
			I2cDrv::write_reg(ADDR, 0x4B, 0x00);
		}

		if (touching)
		{
			if (!was_pressed_)
			{
				// New press
				was_pressed_ = true;
				out.pos		 = last_pos_;
				out.pressed	 = true;
				out.pressure = 128;
				return true;
			}
			// Still touching — report as move with updated position
			out.pos		 = last_pos_;
			out.pressed	 = true;
			out.pressure = 128;
			return true;
		}

		if (was_pressed_)
		{
			// Finger lifted
			was_pressed_ = false;
			out.pos		 = last_pos_;
			out.pressed	 = false;
			out.pressure = 0;
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
