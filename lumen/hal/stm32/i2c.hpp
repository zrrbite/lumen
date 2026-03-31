#pragma once

/// Minimal STM32 I2C abstraction — direct register access.

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

	static void start()
	{
		regs()->CR1 |= (1U << 8); // START
		while ((regs()->SR1 & (1U << 0)) == 0)
		{} // Wait SB
	}

	static void stop()
	{
		regs()->CR1 |= (1U << 9); // STOP
	}

	static void send_addr(uint8_t addr, bool read)
	{
		regs()->DR = (addr << 1) | (read ? 1 : 0);
		while ((regs()->SR1 & (1U << 1)) == 0)
		{}									   // Wait ADDR
		volatile uint32_t dummy = regs()->SR2; // Clear ADDR
		(void)dummy;
	}

	static void write_byte(uint8_t data)
	{
		while ((regs()->SR1 & (1U << 7)) == 0)
		{} // Wait TXE
		regs()->DR = data;
		while ((regs()->SR1 & (1U << 2)) == 0)
		{} // Wait BTF
	}

	static uint8_t read_byte_ack()
	{
		regs()->CR1 |= (1U << 10); // ACK
		while ((regs()->SR1 & (1U << 6)) == 0)
		{} // Wait RXNE
		return static_cast<uint8_t>(regs()->DR);
	}

	static uint8_t read_byte_nack()
	{
		regs()->CR1 &= ~(1U << 10); // NACK
		while ((regs()->SR1 & (1U << 6)) == 0)
		{} // Wait RXNE
		return static_cast<uint8_t>(regs()->DR);
	}

	/// Write a register on a device.
	static void write_reg(uint8_t dev_addr, uint8_t reg, uint8_t val)
	{
		start();
		send_addr(dev_addr, false);
		write_byte(reg);
		write_byte(val);
		stop();
	}

	/// Read a register from a device.
	static uint8_t read_reg(uint8_t dev_addr, uint8_t reg)
	{
		start();
		send_addr(dev_addr, false);
		write_byte(reg);
		start(); // Repeated start
		send_addr(dev_addr, true);
		uint8_t val = read_byte_nack();
		stop();
		return val;
	}

	/// Read two bytes (big-endian 16-bit).
	static uint16_t read_reg16(uint8_t dev_addr, uint8_t reg)
	{
		start();
		send_addr(dev_addr, false);
		write_byte(reg);
		start();
		send_addr(dev_addr, true);
		uint8_t hi = read_byte_ack();
		uint8_t lo = read_byte_nack();
		stop();
		return (static_cast<uint16_t>(hi) << 8) | lo;
	}
};

} // namespace lumen::hal::stm32
