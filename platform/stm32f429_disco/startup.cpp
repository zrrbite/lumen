/// Minimal startup code for STM32F429.
/// Sets up clocks to 180MHz, configures SysTick at 1ms.

#include <cstdint>
#include <cstring>

#include "lumen/hal/tick_source.hpp"

// Linker symbols
extern uint32_t _etext;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;
extern uint32_t _estack;

// Global SysTick source — referenced by board_config.hpp
namespace lumen::platform {
lumen::hal::SysTickSource sys_tick;
}

// Forward declarations
extern "C" void Reset_Handler();
extern "C" void SysTick_Handler();
extern "C" void Default_Handler();
extern int main();

// Weak aliases for IRQ handlers
extern "C" __attribute__((weak, alias("Default_Handler"))) void NMI_Handler();
extern "C" __attribute__((weak, alias("Default_Handler"))) void HardFault_Handler();
extern "C" __attribute__((weak, alias("Default_Handler"))) void MemManage_Handler();
extern "C" __attribute__((weak, alias("Default_Handler"))) void BusFault_Handler();
extern "C" __attribute__((weak, alias("Default_Handler"))) void UsageFault_Handler();

// Vector table
__attribute__((section(".isr_vector"), used)) const uint32_t vector_table[] = {
	reinterpret_cast<uint32_t>(&_estack), // Initial SP
	reinterpret_cast<uint32_t>(Reset_Handler),
	reinterpret_cast<uint32_t>(NMI_Handler),
	reinterpret_cast<uint32_t>(HardFault_Handler),
	reinterpret_cast<uint32_t>(MemManage_Handler),
	reinterpret_cast<uint32_t>(BusFault_Handler),
	reinterpret_cast<uint32_t>(UsageFault_Handler),
	0,
	0,
	0,
	0, // Reserved
	0, // SVCall
	0, // Debug Monitor
	0, // Reserved
	0, // PendSV
	reinterpret_cast<uint32_t>(SysTick_Handler),
	// IRQs follow — extend as needed
};

/// Configure system clock to 180MHz using HSE (8MHz) + PLL.
static void system_clock_config()
{
	// RCC registers
	volatile uint32_t* rcc_cr	   = reinterpret_cast<volatile uint32_t*>(0x40023800);
	volatile uint32_t* rcc_cfgr	   = reinterpret_cast<volatile uint32_t*>(0x40023808);
	volatile uint32_t* rcc_pllcfgr = reinterpret_cast<volatile uint32_t*>(0x40023804);

	// Flash registers
	volatile uint32_t* flash_acr = reinterpret_cast<volatile uint32_t*>(0x40023C00);

	// Enable HSE
	*rcc_cr |= (1U << 16); // HSEON
	while ((*rcc_cr & (1U << 17)) == 0)
	{} // Wait for HSE ready

	// Configure Flash: 5 wait states for 180MHz
	*flash_acr = (5U << 0) | (1U << 8) | (1U << 9) | (1U << 10);

	// Configure PLL: HSE=8MHz, PLLM=8, PLLN=360, PLLP=2, PLLQ=7
	// VCO = 8MHz/8 * 360 = 360MHz, SYSCLK = 360/2 = 180MHz
	*rcc_pllcfgr = (8U << 0) |	 // PLLM = 8
				   (360U << 6) | // PLLN = 360
				   (0U << 16) |	 // PLLP = /2 (00)
				   (1U << 22) |	 // HSE as PLL source
				   (7U << 24);	 // PLLQ = 7

	// Enable PLL
	*rcc_cr |= (1U << 24); // PLLON
	while ((*rcc_cr & (1U << 25)) == 0)
	{} // Wait for PLL ready

	// APB1 = /4 (45MHz), APB2 = /2 (90MHz)
	*rcc_cfgr = (5U << 10) | // PPRE1 = /4
				(4U << 13);	 // PPRE2 = /2

	// Switch to PLL
	*rcc_cfgr |= (2U << 0); // SW = PLL
	while (((*rcc_cfgr >> 2) & 3U) != 2U)
	{} // Wait for PLL as system clock
}

/// Configure SysTick for 1ms interrupt at 180MHz.
static void systick_config()
{
	volatile uint32_t* stk_load = reinterpret_cast<volatile uint32_t*>(0xE000E014);
	volatile uint32_t* stk_val	= reinterpret_cast<volatile uint32_t*>(0xE000E018);
	volatile uint32_t* stk_ctrl = reinterpret_cast<volatile uint32_t*>(0xE000E010);

	*stk_load = 180000 - 1; // 180MHz / 1000 = 1ms
	*stk_val  = 0;
	*stk_ctrl = (1U << 0) | // Enable
				(1U << 1) | // Interrupt enable
				(1U << 2);	// Use processor clock
}

extern "C" void Reset_Handler()
{
	// Copy .data from Flash to RAM
	uint32_t* src = &_etext;
	uint32_t* dst = &_sdata;
	while (dst < &_edata)
	{
		*dst++ = *src++;
	}

	// Zero .bss
	dst = &_sbss;
	while (dst < &_ebss)
	{
		*dst++ = 0;
	}

	// Configure clocks
	system_clock_config();

	// Configure SysTick
	lumen::platform::sys_tick.init();
	systick_config();

	// Call main
	main();

	// Should never reach here
	while (true)
	{}
}

extern "C" void SysTick_Handler()
{
	lumen::platform::sys_tick.increment();
}

extern "C" void Default_Handler()
{
	while (true)
	{}
}
