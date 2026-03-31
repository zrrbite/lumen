/// Startup code for STM32F769-DISCO.
/// Configures 216MHz clock (HSE 25MHz + PLL), enables FPU,
/// initializes SDRAM (IS42S32800G via FMC), sets up SysTick.

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

// Global SysTick
namespace lumen::platform {
lumen::hal::SysTickSource sys_tick;
}

extern "C" void Reset_Handler();
extern "C" void SysTick_Handler();
extern "C" void Default_Handler();
extern int main();

extern "C" __attribute__((weak, alias("Default_Handler"))) void NMI_Handler();
extern "C" __attribute__((weak, alias("Default_Handler"))) void HardFault_Handler();
extern "C" __attribute__((weak, alias("Default_Handler"))) void MemManage_Handler();
extern "C" __attribute__((weak, alias("Default_Handler"))) void BusFault_Handler();
extern "C" __attribute__((weak, alias("Default_Handler"))) void UsageFault_Handler();

__attribute__((section(".isr_vector"), used)) const uint32_t vector_table[] = {
	reinterpret_cast<uint32_t>(&_estack),
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
};

// RCC register addresses
static volatile uint32_t* const RCC_CR		= reinterpret_cast<volatile uint32_t*>(0x40023800);
static volatile uint32_t* const RCC_PLLCFGR = reinterpret_cast<volatile uint32_t*>(0x40023804);
static volatile uint32_t* const RCC_CFGR	= reinterpret_cast<volatile uint32_t*>(0x40023808);
static volatile uint32_t* const RCC_AHB1ENR = reinterpret_cast<volatile uint32_t*>(0x40023830);
static volatile uint32_t* const RCC_AHB3ENR = reinterpret_cast<volatile uint32_t*>(0x40023838);
static volatile uint32_t* const RCC_APB1ENR = reinterpret_cast<volatile uint32_t*>(0x40023840);
static volatile uint32_t* const RCC_APB2ENR = reinterpret_cast<volatile uint32_t*>(0x40023844);
static volatile uint32_t* const FLASH_ACR	= reinterpret_cast<volatile uint32_t*>(0x40023C00);
static volatile uint32_t* const PWR_CR1		= reinterpret_cast<volatile uint32_t*>(0x40007000);
static volatile uint32_t* const PWR_CSR1	= reinterpret_cast<volatile uint32_t*>(0x40007004);

/// Configure system clock to 216MHz using HSE (25MHz) + PLL.
static void system_clock_config()
{
	// Enable power controller clock
	*RCC_APB1ENR |= (1U << 28); // PWREN

	// Set voltage regulator scale 1 (required for 216MHz)
	*PWR_CR1 |= (3U << 14); // VOS = Scale 1

	// Enable HSE
	*RCC_CR |= (1U << 16); // HSEON
	while ((*RCC_CR & (1U << 17)) == 0)
	{} // Wait HSE ready

	// Configure Flash: 7 wait states for 216MHz, prefetch + ART
	*FLASH_ACR = (7U << 0) | (1U << 8) | (1U << 9) | (1U << 10);

	// Configure PLL: HSE=25MHz
	// PLLM=25, PLLN=432, PLLP=2, PLLQ=9
	// VCO = 25MHz/25 * 432 = 432MHz
	// SYSCLK = 432/2 = 216MHz
	// USB/SDIO = 432/9 = 48MHz
	*RCC_PLLCFGR = (25U << 0) |	 // PLLM = 25
				   (432U << 6) | // PLLN = 432
				   (0U << 16) |	 // PLLP = /2
				   (1U << 22) |	 // HSE as PLL source
				   (9U << 24);	 // PLLQ = 9

	// Enable PLL
	*RCC_CR |= (1U << 24); // PLLON
	while ((*RCC_CR & (1U << 25)) == 0)
	{} // Wait PLL ready

	// Enable overdrive mode (required for 216MHz on F7)
	*PWR_CR1 |= (1U << 16); // ODEN
	while ((*PWR_CSR1 & (1U << 16)) == 0)
	{} // Wait ODRDY

	// Switch overdrive
	*PWR_CR1 |= (1U << 17); // ODSWEN
	while ((*PWR_CSR1 & (1U << 17)) == 0)
	{} // Wait ODSWRDY

	// APB1 = /4 (54MHz), APB2 = /2 (108MHz), AHB = /1
	*RCC_CFGR = (5U << 10) | // PPRE1 = /4
				(4U << 13);	 // PPRE2 = /2

	// Switch to PLL
	*RCC_CFGR |= (2U << 0); // SW = PLL
	while (((*RCC_CFGR >> 2) & 3U) != 2U)
	{} // Wait PLL as system clock
}

/// Initialize FMC for SDRAM (IS42S32800G, Bank 1).
/// SDRAM at 0xC0000000, 16MB, 32-bit bus.
static void sdram_init()
{
	// Enable FMC clock
	*RCC_AHB3ENR |= (1U << 0); // FMCEN

	// Enable GPIO clocks for SDRAM pins
	// FMC uses many GPIO pins across ports B, C, D, E, F, G, H, I
	*RCC_AHB1ENR |=
		(1U << 1) | (1U << 2) | (1U << 3) | (1U << 4) | (1U << 5) | (1U << 6) | (1U << 7) | (1U << 8); // Ports B-I

	// Configure all FMC SDRAM GPIO pins as AF12 (very high speed)
	// This is ~30 pins. For brevity, using direct register writes.
	// The full pin mapping for F769-DISCO SDRAM:
	// PD0,1,8-10,14,15 = D2,3,13-15,0,1
	// PE0,1,7-15 = NBL0,NBL1,D4-12
	// PF0-5,11-15 = A0-5,SDNRAS,A6-9
	// PG0-2,4,5,8,15 = A10-12,BA0,BA1,SDCLK,SDNCAS
	// PH2,3,5,8-15 = SDCKE0,SDNE0,SDNWE,D16-23
	// PI0-7,9,10 = D24-31,NBL2,NBL3

	// For now, configure using a helper function
	auto config_af12 = [](volatile uint32_t* gpio_base, uint8_t pin) {
		volatile uint32_t* moder   = gpio_base;
		volatile uint32_t* ospeedr = gpio_base + 2;
		volatile uint32_t* pupdr   = gpio_base + 3;
		volatile uint32_t* afrl	   = gpio_base + 8;
		volatile uint32_t* afrh	   = gpio_base + 9;

		// AF mode
		*moder &= ~(3U << (pin * 2));
		*moder |= (2U << (pin * 2));
		// Very high speed
		*ospeedr |= (3U << (pin * 2));
		// No pull
		*pupdr &= ~(3U << (pin * 2));
		// AF12
		if (pin < 8)
		{
			*afrl &= ~(0xFU << (pin * 4));
			*afrl |= (12U << (pin * 4));
		}
		else
		{
			*afrh &= ~(0xFU << ((pin - 8) * 4));
			*afrh |= (12U << ((pin - 8) * 4));
		}
	};

	// GPIO base addresses
	volatile uint32_t* GPIOD = reinterpret_cast<volatile uint32_t*>(0x40020C00);
	volatile uint32_t* GPIOE = reinterpret_cast<volatile uint32_t*>(0x40021000);
	volatile uint32_t* GPIOF = reinterpret_cast<volatile uint32_t*>(0x40021400);
	volatile uint32_t* GPIOG = reinterpret_cast<volatile uint32_t*>(0x40021800);
	volatile uint32_t* GPIOH = reinterpret_cast<volatile uint32_t*>(0x40021C00);
	volatile uint32_t* GPIOI = reinterpret_cast<volatile uint32_t*>(0x40022000);

	// Port D: 0,1,8,9,10,14,15
	static const uint8_t pd[] = {0, 1, 8, 9, 10, 14, 15};
	for (auto p : pd)
		config_af12(GPIOD, p);
	// Port E: 0,1,7,8,9,10,11,12,13,14,15
	static const uint8_t pe[] = {0, 1, 7, 8, 9, 10, 11, 12, 13, 14, 15};
	for (auto p : pe)
		config_af12(GPIOE, p);
	// Port F: 0,1,2,3,4,5,11,12,13,14,15
	static const uint8_t pf[] = {0, 1, 2, 3, 4, 5, 11, 12, 13, 14, 15};
	for (auto p : pf)
		config_af12(GPIOF, p);
	// Port G: 0,1,2,4,5,8,15
	static const uint8_t pg[] = {0, 1, 2, 4, 5, 8, 15};
	for (auto p : pg)
		config_af12(GPIOG, p);
	// Port H: 2,3,5,8,9,10,11,12,13,14,15
	static const uint8_t ph[] = {2, 3, 5, 8, 9, 10, 11, 12, 13, 14, 15};
	for (auto p : ph)
		config_af12(GPIOH, p);
	// Port I: 0,1,2,3,4,5,6,7,9,10
	static const uint8_t pi[] = {0, 1, 2, 3, 4, 5, 6, 7, 9, 10};
	for (auto p : pi)
		config_af12(GPIOI, p);

	// FMC SDRAM registers
	volatile uint32_t* FMC_SDCR1 = reinterpret_cast<volatile uint32_t*>(0xA0000140);
	volatile uint32_t* FMC_SDCR2 = reinterpret_cast<volatile uint32_t*>(0xA0000144);
	volatile uint32_t* FMC_SDTR1 = reinterpret_cast<volatile uint32_t*>(0xA0000148);
	volatile uint32_t* FMC_SDTR2 = reinterpret_cast<volatile uint32_t*>(0xA000014C);
	volatile uint32_t* FMC_SDCMR = reinterpret_cast<volatile uint32_t*>(0xA0000150);
	volatile uint32_t* FMC_SDRTR = reinterpret_cast<volatile uint32_t*>(0xA0000154);
	volatile uint32_t* FMC_SDSR	 = reinterpret_cast<volatile uint32_t*>(0xA0000158);

	// SDCR1: shared fields (SDCLK, RBURST, RPIPE must be set here even for Bank 2)
	*FMC_SDCR1 = (2U << 10) | // SDCLK: HCLK/2
				 (1U << 12) | // RBURST: burst read enabled
				 (0U << 13);  // RPIPE: no delay

	// SDCR2: Bank 2 config (the F769-DISCO may use either bank)
	uint32_t sdcr = (0U << 0) | // NC: 8 column bits
					(1U << 2) | // NR: 12 row bits
					(2U << 4) | // MWID: 32-bit bus width
					(1U << 6) | // NB: 4 internal banks
					(3U << 7) | // CAS: latency 3
					(0U << 9);	// WP: off
	// Set both banks with same config
	*FMC_SDCR1 |= sdcr;
	*FMC_SDCR2 = sdcr;

	// Timing (same for both banks)
	uint32_t sdtr = (1U << 0) |	 // TMRD: 2 cycles
					(6U << 4) |	 // TXSR: 7 cycles
					(3U << 8) |	 // TRAS: 4 cycles
					(6U << 12) | // TRC: 7 cycles
					(1U << 16) | // TWR: 2 cycles
					(1U << 20) | // TRP: 2 cycles
					(1U << 24);	 // TRCD: 2 cycles
	*FMC_SDTR1 = sdtr;
	*FMC_SDTR2 = sdtr;

	// Wait for FMC ready
	while (*FMC_SDSR & (1U << 5))
	{}

	// SDRAM init: target Bank 1 only (per ST BSP)
	uint32_t bank1 = (1U << 3); // CTB1

	// Step 1: Clock configuration enable
	*FMC_SDCMR = (1U << 0) | bank1;
	for (volatile int i = 0; i < 20000; ++i)
	{}
	while (*FMC_SDSR & (1U << 5))
	{}

	// Step 2: PALL (precharge all)
	*FMC_SDCMR = (2U << 0) | bank1;
	while (*FMC_SDSR & (1U << 5))
	{}

	// Step 3: Auto-refresh (8 cycles)
	*FMC_SDCMR = (3U << 0) | bank1 | (7U << 5);
	while (*FMC_SDSR & (1U << 5))
	{}

	// Step 4: Load mode register (from ST BSP: 0x0230)
	// Burst length 1, sequential, CAS 3, standard mode, write burst single
	uint32_t mode_reg = 0x0230;
	*FMC_SDCMR		  = (4U << 0) | bank1 | (mode_reg << 9);
	while (*FMC_SDSR & (1U << 5))
	{}

	// Refresh rate from ST BSP: 0x0603 = 1539 (for ~100MHz SDCLK)
	*FMC_SDRTR = (0x0603U << 1);
}

/// Configure SysTick for 1ms at 216MHz.
static void systick_config()
{
	volatile uint32_t* stk_load = reinterpret_cast<volatile uint32_t*>(0xE000E014);
	volatile uint32_t* stk_val	= reinterpret_cast<volatile uint32_t*>(0xE000E018);
	volatile uint32_t* stk_ctrl = reinterpret_cast<volatile uint32_t*>(0xE000E010);

	*stk_load = 216000 - 1; // 216MHz / 1000
	*stk_val  = 0;
	*stk_ctrl = (1U << 0) | (1U << 1) | (1U << 2);
}

extern "C" void Reset_Handler()
{
	// Copy .data
	uint32_t* src = &_etext;
	uint32_t* dst = &_sdata;
	while (dst < &_edata)
		*dst++ = *src++;

	// Zero .bss
	dst = &_sbss;
	while (dst < &_ebss)
		*dst++ = 0;

	// Enable FPU (Cortex-M7 with FPv5)
	volatile uint32_t* cpacr = reinterpret_cast<volatile uint32_t*>(0xE000ED88);
	*cpacr |= (0xFU << 20);

	// Configure clocks to 216MHz
	system_clock_config();

	// Initialize SDRAM
	sdram_init();

	// Configure SysTick
	lumen::platform::sys_tick.init();
	systick_config();

	main();

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

// Newlib stubs
extern "C" {
int _kill(int, int)
{
	return -1;
}
int _getpid()
{
	return 1;
}
int _isatty(int)
{
	return 0;
}
int _close(int)
{
	return -1;
}
int _read(int, char*, int)
{
	return 0;
}
int _write(int, char*, int)
{
	return 0;
}
int _lseek(int, int, int)
{
	return 0;
}
int _fstat(int, void*)
{
	return 0;
}
void _exit(int)
{
	while (1)
	{}
}
void* _sbrk(int incr)
{
	static char heap[8192];
	static char* heap_end = heap;
	char* prev			  = heap_end;
	heap_end += incr;
	return prev;
}
}
