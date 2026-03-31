/// STM32F769-DISCO display bring-up.
/// Init order matches ST BSP exactly.

#include <cstdint>

#include "lumen/hal/stm32/gpio.hpp"
#include "lumen/hal/tick_source.hpp"

namespace lumen::platform {
extern hal::SysTickSource sys_tick;
}

using LD1 = lumen::hal::stm32::GpioPin<lumen::hal::stm32::Port::J, 13>;
using LD2 = lumen::hal::stm32::GpioPin<lumen::hal::stm32::Port::J, 5>;

static void delay(uint32_t ms)
{
	lumen::platform::sys_tick.delay(ms);
}

static constexpr uint32_t D = 0x40016C00; // DSI base
static constexpr uint32_t L = 0x40016800; // LTDC base
static constexpr uint32_t R = 0x40023800; // RCC base

static volatile uint32_t& r(uint32_t base, uint32_t off)
{
	return *reinterpret_cast<volatile uint32_t*>(base + off);
}

static constexpr uint32_t FB = 0xC0000000;

static void dsi_wait()
{
	for (volatile int i = 0; i < 100000; ++i)
		if (r(D, 0x74) & 1U)
			return;
}
static void dw(uint8_t c, uint8_t p)
{
	dsi_wait();
	r(D, 0x6C) = 0x15U | (uint32_t(c) << 8) | (uint32_t(p) << 16);
}
static void dw0(uint8_t c)
{
	dsi_wait();
	r(D, 0x6C) = 0x05U | (uint32_t(c) << 8);
}
static void dwl(uint8_t c, const uint8_t* d, uint16_t n)
{
	dsi_wait();
	uint32_t tot = n + 1;
	uint32_t w0	 = c;
	for (uint32_t i = 0; i < 3 && i < n; ++i)
		w0 |= uint32_t(d[i]) << ((i + 1) * 8);
	r(D, 0x70) = w0;
	for (uint32_t w = 1; w < (tot + 3) / 4; ++w)
	{
		uint32_t v = 0;
		for (uint32_t b = 0; b < 4; ++b)
		{
			uint32_t idx = w * 4 + b - 1;
			if (idx < n)
				v |= uint32_t(d[idx]) << (b * 8);
		}
		r(D, 0x70) = v;
	}
	r(D, 0x6C) = 0x39U | (tot << 8);
}

static void otm_init()
{
	dw(0x00, 0x00);
	{
		uint8_t d[] = {0x80, 0x09, 0x01};
		dwl(0xFF, d, 3);
	}
	dw(0x00, 0x80);
	{
		uint8_t d[] = {0x80, 0x09};
		dwl(0xFF, d, 2);
	}
	dw(0x00, 0x80);
	dw(0xC4, 0x30);
	delay(10);
	dw(0x00, 0x8A);
	dw(0xC4, 0x40);
	delay(10);
	dw(0x00, 0xB1);
	dw(0xC5, 0xA9);
	dw(0x00, 0x91);
	dw(0xC5, 0x34);
	dw(0x00, 0xB4);
	dw(0xC0, 0x50);
	dw(0x00, 0x00);
	dw(0xD9, 0x4E);
	dw(0x00, 0x81);
	dw(0xC1, 0x66);
	dw(0x00, 0xA1);
	dw(0xC1, 0x08);
	dw(0x00, 0x92);
	dw(0xC5, 0x01);
	dw(0x00, 0x95);
	dw(0xC5, 0x34);
	dw(0x00, 0x00);
	{
		uint8_t d[] = {0x79, 0x79};
		dwl(0xD8, d, 2);
	}
	dw(0x00, 0x94);
	dw(0xC5, 0x33);
	dw(0x00, 0xA3);
	dw(0xC0, 0x1B);
	dw(0x00, 0x82);
	dw(0xC5, 0x83);
	dw(0x00, 0x81);
	dw(0xC4, 0x83);
	dw(0x00, 0xA1);
	dw(0xC1, 0x0E);
	dw(0x00, 0xA6);
	{
		uint8_t d[] = {0x00, 0x01};
		dwl(0xB3, d, 2);
	}
	dw(0x00, 0x80);
	{
		uint8_t d[] = {0x85, 0x01, 0x00, 0x84, 0x01, 0x00};
		dwl(0xCE, d, 6);
	}
	dw(0x00, 0xA0);
	{
		uint8_t d[] = {0x18, 0x04, 0x03, 0x39, 0x00, 0x00, 0x00, 0x18, 0x03, 0x03, 0x3A, 0x00, 0x00, 0x00};
		dwl(0xCE, d, 14);
	}
	dw(0x00, 0xB0);
	{
		uint8_t d[] = {0x18, 0x02, 0x03, 0x3B, 0x00, 0x00, 0x00, 0x18, 0x01, 0x03, 0x3C, 0x00, 0x00, 0x00};
		dwl(0xCE, d, 14);
	}
	dw(0x00, 0xC0);
	{
		uint8_t d[] = {0x01, 0x01, 0x20, 0x20, 0x00, 0x00, 0x01, 0x02, 0x00, 0x00};
		dwl(0xCF, d, 10);
	}
	dw(0x00, 0xD0);
	dw(0xCF, 0x00);
	dw(0x00, 0x80);
	{
		uint8_t d[10] = {};
		dwl(0xCB, d, 10);
	}
	dw(0x00, 0x90);
	{
		uint8_t d[15] = {};
		dwl(0xCB, d, 15);
	}
	dw(0x00, 0xA0);
	{
		uint8_t d[15] = {};
		dwl(0xCB, d, 15);
	}
	dw(0x00, 0xB0);
	{
		uint8_t d[10] = {};
		dwl(0xCB, d, 10);
	}
	dw(0x00, 0xC0);
	{
		uint8_t d[] = {0x00, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
		dwl(0xCB, d, 15);
	}
	dw(0x00, 0xD0);
	{
		uint8_t d[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00};
		dwl(0xCB, d, 15);
	}
	dw(0x00, 0xE0);
	{
		uint8_t d[10] = {};
		dwl(0xCB, d, 10);
	}
	dw(0x00, 0xF0);
	{
		uint8_t d[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
		dwl(0xCB, d, 10);
	}
	dw(0x00, 0x80);
	{
		uint8_t d[] = {0x00, 0x26, 0x09, 0x0B, 0x01, 0x25, 0x00, 0x00, 0x00, 0x00};
		dwl(0xCC, d, 10);
	}
	dw(0x00, 0x90);
	{
		uint8_t d[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x26, 0x0A, 0x0C, 0x02};
		dwl(0xCC, d, 15);
	}
	dw(0x00, 0xA0);
	{
		uint8_t d[] = {0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
		dwl(0xCC, d, 15);
	}
	dw(0x00, 0xB0);
	{
		uint8_t d[] = {0x00, 0x25, 0x0C, 0x0A, 0x02, 0x26, 0x00, 0x00, 0x00, 0x00};
		dwl(0xCC, d, 10);
	}
	dw(0x00, 0xC0);
	{
		uint8_t d[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25, 0x0B, 0x09, 0x01};
		dwl(0xCC, d, 15);
	}
	dw(0x00, 0xD0);
	{
		uint8_t d[] = {0x26, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
		dwl(0xCC, d, 15);
	}
	dw(0x00, 0x81);
	dw(0xC5, 0x66);
	dw(0x00, 0xB6);
	dw(0xF5, 0x06);
	dw(0x00, 0xB1);
	dw(0xC6, 0x06);
	dw(0x00, 0x00);
	{
		uint8_t d[] = {0xFF, 0xFF, 0xFF};
		dwl(0xFF, d, 3);
	}
	dw(0x00, 0x00);
	{
		uint8_t d[] = {0x00, 0x09, 0x0F, 0x0E, 0x07, 0x10, 0x0B, 0x0A, 0x04, 0x07, 0x0B, 0x08, 0x0F, 0x10, 0x0A, 0x01};
		dwl(0xE1, d, 16);
	}
	dw(0x00, 0x00);
	{
		uint8_t d[] = {0x00, 0x09, 0x0F, 0x0E, 0x07, 0x10, 0x0B, 0x0A, 0x04, 0x07, 0x0B, 0x08, 0x0F, 0x10, 0x0A, 0x01};
		dwl(0xE2, d, 16);
	}
	dw0(0x11);
	delay(120);
	dw(0x3A, 0x77); // RGB888
	{
		uint8_t d[] = {0x00, 0x00, 0x03, 0x1F};
		dwl(0x2A, d, 4);
	}
	{
		uint8_t d[] = {0x00, 0x00, 0x01, 0xDF};
		dwl(0x2B, d, 4);
	}
	dw0(0x29);
	delay(50);
	dw(0x51, 0x7F);
	dw(0x53, 0x2C);
	dw(0x55, 0x02);
}

int main()
{
	lumen::hal::stm32::enable_gpio_clock(lumen::hal::stm32::Port::J);
	LD1::init_output();
	LD2::init_output();
	LD1::high();

	// === BSP ORDER ===

	// 1. LCD Reset (PJ15)
	r(R, 0x30) |= (1U << 9);
	volatile uint32_t* JM = reinterpret_cast<volatile uint32_t*>(0x40022400);
	volatile uint32_t* JB = reinterpret_cast<volatile uint32_t*>(0x40022418);
	*JM &= ~(3U << 30);
	*JM |= (1U << 30);
	*JB = (1U << 31);
	delay(20);
	*JB = (1U << 15);
	delay(10);

	// 2. MspInit: enable clocks, reset peripherals
	r(R, 0x44) |= (1U << 26) | (1U << 27); // LTDC + DSI clocks
	r(R, 0x24) |= (1U << 26) | (1U << 27); // Reset
	r(R, 0x24) &= ~((1U << 26) | (1U << 27));

	// 3. HAL_DSI_Init equivalent
	// 3a. Enable regulator
	r(D, 0x430) |= (1U << 24);
	for (volatile int t = 0; t < 1000000 && !(r(D, 0x40C) & (1U << 12)); ++t)
	{}
	// 3b. Configure PLL: NDIV=100, IDF=5, ODF=1
	r(D, 0x430) &= ~((0x7FU << 2) | (0xFU << 11) | (0x3U << 16));
	r(D, 0x430) |= (100U << 2) | (2U << 11) | (0U << 16);
	r(D, 0x430) |= (1U << 0); // PLLEN
	delay(1);
	for (volatile int t = 0; t < 1000000 && !(r(D, 0x40C) & (1U << 8)); ++t)
	{}
	// 3c. D-PHY enable (before lane config, per HAL)
	r(D, 0xA0) |= (1U << 1) | (1U << 2); // PCTLR: CKE + DEN
	// 3d. Clock lane config
	r(D, 0x94) = (1U << 0); // CLCR: DPCC only (no auto clock lane)
	// 3e. Number of lanes + TX escape clock
	r(D, 0xA4) = 1; // PCONFR: 2 data lanes
	r(D, 0x08) = 4; // CCR: TXEscapeCkdiv
	// 3f. UIX4 = 8 (unit interval for 500MHz PHY clock)
	r(D, 0x418) = 8; // WPCR[0]: UIX4
	// 3g. Clock lane + data lane timers
	r(D, 0x98) = (35U << 0) | (35U << 16); // CLTCR
	r(D, 0x9C) = (35U << 0) | (35U << 16); // DLTCR
	// 3h. Disable error interrupts
	r(D, 0xC4) = 0; // IER[0]
	r(D, 0xC8) = 0; // IER[1]

	// 4. HAL_DSI_ConfigVideoMode equivalent
	r(D, 0x34) &= ~1U; // MCR: video mode (clear CMDM)
	r(D, 0x38) =
		(2U << 0) | (1U << 8) | (1U << 9) | (1U << 10) | (1U << 11) | (1U << 12) | (1U << 13) | (1U << 15); // VMCR
	r(D, 0x3C)			  = 800;																			// VPCR
	r(D, 0x40)			  = 0;																				// VCCR
	r(D, 0x44)			  = 0xFFF;																			// VNPCR
	constexpr uint32_t LB = 62500, LC = 27429;
	r(D, 0x48) = (2 * LB) / LC;
	r(D, 0x4C) = (34 * LB) / LC;
	r(D, 0x50) = (870 * LB) / LC; // total = 800+2+34+34
	r(D, 0x54) = 1;				  // VSA
	r(D, 0x58) = 15;			  // VBP
	r(D, 0x5C) = 16;			  // VFP
	r(D, 0x60) = 480;			  // VACT
	r(D, 0x18) = (16U << 16);	  // LPMCR
	r(D, 0x0C) = 0;				  // LVCIDR
	r(D, 0x10) = 5;				  // LCOLCR: RGB888
	r(D, 0x14) = 0;				  // LPCR

	// 5. HAL_LTDC_Init equivalent
	constexpr uint32_t HSA = 2, HBP = 34, HFP = 34, HACT = 800;
	constexpr uint32_t VSA = 1, VBP = 15, VFP = 16, VACT = 480;
	// 5a. PLLSAI for LTDC clock
	r(R, 0x00) &= ~(1U << 28);
	for (volatile int t = 0; t < 100000 && (r(R, 0x00) & (1U << 29)); ++t)
	{}
	r(R, 0x88) = (384U << 6) | (7U << 28);
	r(R, 0x8C) &= ~(3U << 16);
	r(R, 0x00) |= (1U << 28);
	for (volatile int t = 0; t < 1000000 && !(r(R, 0x00) & (1U << 29)); ++t)
	{}
	// 5b. LTDC timing
	r(L, 0x08) = ((HSA - 1) << 16) | (VSA - 1);
	r(L, 0x0C) = ((HSA + HBP - 1) << 16) | (VSA + VBP - 1);
	r(L, 0x10) = ((HSA + HBP + HACT - 1) << 16) | (VSA + VBP + VACT - 1);
	r(L, 0x14) = ((HSA + HBP + HACT + HFP - 1) << 16) | (VSA + VBP + VACT + VFP - 1);
	r(L, 0x2C) = 0;
	// 5c. GCR: polarity + enable (BSP: PCPolarity=IPC, no VS/HS inversion for DSI)
	r(L, 0x18) = 1; // Just LTDCEN, no polarity inversion
	r(L, 0x24) = 1; // SRCR

	// 6. HAL_DSI_Start
	r(D, 0x04)	= 1;					 // CR: enable host
	r(D, 0x404) = (1U << 2) | (1U << 3); // WCR: LTDCEN + DSIEN

	delay(10);

	// 7. Fill framebuffer before layer config
	volatile uint32_t* fb = reinterpret_cast<volatile uint32_t*>(FB);
	for (uint32_t i = 0; i < 800 * 480; ++i)
		fb[i] = 0xFFFF0000; // Red ARGB

	// 8. Layer config (BSP does this AFTER DSI_Start)
	uint32_t hs = HSA + HBP, vs = VSA + VBP;
	r(L, 0x88) = (hs) | ((hs + HACT - 1) << 16);
	r(L, 0x8C) = (vs) | ((vs + VACT - 1) << 16);
	r(L, 0x94) = 0; // ARGB8888
	r(L, 0x98) = 255;
	r(L, 0x9C) = 0;
	r(L, 0xA0) = (6U) | (4U << 8);
	r(L, 0xAC) = FB;
	r(L, 0xB0) = (3200U << 16) | 3203U; // pitch=3200, line=3203
	r(L, 0xB4) = 480;
	r(L, 0x84) = 1; // Enable layer
	r(L, 0x24) = 1; // SRCR: reload

	// 9. OTM8009A init
	otm_init();

	// Done
	LD1::low();
	while (true)
	{
		LD2::high();
		delay(500);
		LD2::low();
		delay(500);
	}
}
