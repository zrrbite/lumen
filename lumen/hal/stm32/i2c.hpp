#pragma once

/// Minimal STM32 I2C abstraction — direct register access.
/// All wait loops have timeouts to prevent system hangs.

#include <cstdint>

namespace lumen::hal::stm32 {

enum class I2cInstance : uint32_t
{
	I2C1 = 0x40005400,
	I2C2 = 0x40005800,
	I2C3 = 0x40005C00,
};

struct I2cRegs
{
	volatile uint32_t CR1;
	volatile uint32_t CR2;
	volatile uint32_t OAR1;
	volatile uint32_t OAR2;
	volatile uint32_t DR;
	volatile uint32_t SR1;
	volatile uint32_t SR2;
	volatile uint32_t CCR;
	volatile uint32_t TRISE;
	volatile uint32_t FLTR;
};

template <I2cInstance Inst> struct I2c
{
	static constexpr uint32_t TIMEOUT = 100000; // ~5ms at 180MHz

	static I2cRegs* regs() { return reinterpret_cast<I2cRegs*>(static_cast<uint32_t>(Inst)); }

	static void enable_clock()
	{
		volatile uint32_t* rcc_apb1enr = reinterpret_cast<volatile uint32_t*>(0x40023840);
		if constexpr (Inst == I2cInstance::I2C3)
		{
			*rcc_apb1enr |= (1U << 23); // I2C3EN
		}
	}

	/// Init I2C in standard mode (100kHz) assuming APB1 = 45MHz.
	static void init()
	{
		enable_clock();
		regs()->CR1	  = 0;		   // Disable
		regs()->CR2	  = 45;		   // APB1 freq in MHz
		regs()->CCR	  = 225;	   // 45MHz / (2 * 100kHz)
		regs()->TRISE = 46;		   // 45MHz * 1us + 1
		regs()->CR1	  = (1U << 0); // PE: enable
	}

	/// Software reset to recover from bus errors.
	static void reset()
	{
		regs()->CR1 |= (1U << 15);	// SWRST
		regs()->CR1 &= ~(1U << 15); // Clear SWRST
		init();						// Re-init
	}

	static bool start()
	{
		regs()->CR1 |= (1U << 8); // START
		for (uint32_t t = TIMEOUT; t > 0; --t)
		{
			if (regs()->SR1 & (1U << 0))
				return true; // SB set
		}
		return false;
	}

	static void stop()
	{
		regs()->CR1 |= (1U << 9); // STOP
	}

	static bool send_addr(uint8_t addr, bool read)
	{
		regs()->DR = (addr << 1) | (read ? 1 : 0);
		for (uint32_t t = TIMEOUT; t > 0; --t)
		{
			uint32_t sr1 = regs()->SR1;
			if (sr1 & (1U << 10))
			{			// AF (NACK)
				stop(); // Release bus
				return false;
			}
			if (sr1 & (1U << 1))
			{										   // ADDR
				volatile uint32_t dummy = regs()->SR2; // Clear ADDR
				(void)dummy;
				return true;
			}
		}
		stop();
		return false;
	}

	static bool write_byte(uint8_t data)
	{
		for (uint32_t t = TIMEOUT; t > 0; --t)
		{
			if (regs()->SR1 & (1U << 7))
				break; // TXE
			if (t == 1)
				return false;
		}
		regs()->DR = data;
		for (uint32_t t = TIMEOUT; t > 0; --t)
		{
			if (regs()->SR1 & (1U << 2))
				return true; // BTF
		}
		return false;
	}

	static uint8_t read_byte_ack()
	{
		regs()->CR1 |= (1U << 10); // ACK
		for (uint32_t t = TIMEOUT; t > 0; --t)
		{
			if (regs()->SR1 & (1U << 6))
				return static_cast<uint8_t>(regs()->DR); // RXNE
		}
		return 0;
	}

	static uint8_t read_byte_nack()
	{
		regs()->CR1 &= ~(1U << 10); // NACK
		for (uint32_t t = TIMEOUT; t > 0; --t)
		{
			if (regs()->SR1 & (1U << 6))
				return static_cast<uint8_t>(regs()->DR); // RXNE
		}
		return 0;
	}

	/// Write a register on a device. Returns true on success.
	static bool write_reg(uint8_t dev_addr, uint8_t reg, uint8_t val)
	{
		if (!start())
			return false;
		if (!send_addr(dev_addr, false))
			return false;
		if (!write_byte(reg))
		{
			stop();
			return false;
		}
		if (!write_byte(val))
		{
			stop();
			return false;
		}
		stop();
		return true;
	}

	/// Read a register from a device.
	static uint8_t read_reg(uint8_t dev_addr, uint8_t reg)
	{
		if (!start())
			return 0;
		if (!send_addr(dev_addr, false))
			return 0;
		if (!write_byte(reg))
		{
			stop();
			return 0;
		}
		if (!start())
			return 0;
		if (!send_addr(dev_addr, true))
			return 0;
		uint8_t val = read_byte_nack();
		stop();
		return val;
	}

	/// Read two bytes (big-endian 16-bit).
	static uint16_t read_reg16(uint8_t dev_addr, uint8_t reg)
	{
		if (!start())
			return 0;
		if (!send_addr(dev_addr, false))
			return 0;
		if (!write_byte(reg))
		{
			stop();
			return 0;
		}
		if (!start())
			return 0;
		if (!send_addr(dev_addr, true))
			return 0;
		uint8_t hi = read_byte_ack();
		uint8_t lo = read_byte_nack();
		stop();
		return (static_cast<uint16_t>(hi) << 8) | lo;
	}

	/// Read multiple bytes from a register into a buffer.
	static void read_reg_multi(uint8_t dev_addr, uint8_t reg, uint8_t* buf, uint8_t len)
	{
		if (len == 0)
			return;
		if (len == 1)
		{
			buf[0] = read_reg(dev_addr, reg);
			return;
		}

		if (!start())
			return;
		if (!send_addr(dev_addr, false))
			return;
		if (!write_byte(reg))
		{
			stop();
			return;
		}
		if (!start())
			return;
		if (!send_addr(dev_addr, true))
			return;
		for (uint8_t i = 0; i < len; ++i)
		{
			if (i < len - 1)
				buf[i] = read_byte_ack();
			else
				buf[i] = read_byte_nack();
		}
		stop();
	}
};

} // namespace lumen::hal::stm32
