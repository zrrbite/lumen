#pragma once

/// DSI + LTDC + OTM8009A initialization for STM32F769-DISCO.
/// Based on ST's BSP (stm32f769i_discovery_lcd.c).
/// Bare-metal register-level implementation.

#include <cstdint>

namespace lumen::platform::dsi {

// Register base addresses
static constexpr uint32_t DSI_BASE	= 0x40016C00;
static constexpr uint32_t LTDC_BASE = 0x40016800;
static constexpr uint32_t RCC_BASE	= 0x40023800;

// DSI Host registers (offset from DSI_BASE)
struct DsiRegs
{
	volatile uint32_t VR;	  // 0x00 Version
	volatile uint32_t CR;	  // 0x04 Control
	volatile uint32_t CCR;	  // 0x08 Clock Control
	volatile uint32_t LVCIDR; // 0x0C LTDC VCID
	volatile uint32_t LCOLCR; // 0x10 LTDC Color Coding
	volatile uint32_t LPCR;	  // 0x14 LTDC Polarity Config
	volatile uint32_t LPMCR;  // 0x18 LTDC LP Mode Config
	uint32_t reserved0[4];
	volatile uint32_t PCR;	   // 0x2C Protocol Config
	volatile uint32_t GVCIDR;  // 0x30 Generic VCID
	volatile uint32_t MCR;	   // 0x34 Mode Config
	volatile uint32_t VMCR;	   // 0x38 Video Mode Config
	volatile uint32_t VPCR;	   // 0x3C Video Packet Config
	volatile uint32_t VCCR;	   // 0x40 Video Chunks Config
	volatile uint32_t VNPCR;   // 0x44 Video Null Packet Config
	volatile uint32_t VHSACR;  // 0x48 Video HSA Config
	volatile uint32_t VHBPCR;  // 0x4C Video HBP Config
	volatile uint32_t VLCR;	   // 0x50 Video Line Config
	volatile uint32_t VVSACR;  // 0x54 Video VSA Config
	volatile uint32_t VVBPCR;  // 0x58 Video VBP Config
	volatile uint32_t VVFPCR;  // 0x5C Video VFP Config
	volatile uint32_t VVACR;   // 0x60 Video VA Config
	volatile uint32_t LCCR;	   // 0x64 LP Command Config
	volatile uint32_t CMCR;	   // 0x68 Command Mode Config
	volatile uint32_t GHCR;	   // 0x6C Generic Header Config
	volatile uint32_t GPDR;	   // 0x70 Generic Payload Data
	volatile uint32_t GPSR;	   // 0x74 Generic Packet Status
	volatile uint32_t TCCR[6]; // 0x78-0x8C Timeout Counter Configs
	uint32_t reserved1;
	volatile uint32_t CLCR;	  // 0x94 Clock Lane Config
	volatile uint32_t CLTCR;  // 0x98 Clock Lane Timer Config
	volatile uint32_t DLTCR;  // 0x9C Data Lane Timer Config
	volatile uint32_t PCTLR;  // 0xA0 PHY Control
	volatile uint32_t PCONFR; // 0xA4 PHY Config
	volatile uint32_t PUCR;	  // 0xA8 PHY ULPS Control
	volatile uint32_t PTCR;	  // 0xAC PHY TX Triggers
	volatile uint32_t PTTCR;  // 0xB0 PHY TX Timer Config
};

// DSI Wrapper registers (offset 0x400 from DSI_BASE)
struct DsiWrapRegs
{
	volatile uint32_t WCR;	 // 0x00 Wrapper Config
	volatile uint32_t WIER;	 // 0x04 Wrapper Interrupt Enable
	volatile uint32_t WISR;	 // 0x08 Wrapper Interrupt Status
	volatile uint32_t WIFCR; // 0x0C Wrapper Interrupt Flag Clear
	uint32_t reserved;
	volatile uint32_t WPCR[5]; // 0x14-0x24 Wrapper PHY Config
};

// LTDC registers
struct LtdcRegs
{
	uint32_t reserved0[2];
	volatile uint32_t SSCR; // 0x08 Sync Size Config
	volatile uint32_t BPCR; // 0x0C Back Porch Config
	volatile uint32_t AWCR; // 0x10 Active Width Config
	volatile uint32_t TWCR; // 0x14 Total Width Config
	volatile uint32_t GCR;	// 0x18 Global Control
	uint32_t reserved1[2];
	volatile uint32_t SRCR; // 0x24 Shadow Reload Config
	uint32_t reserved2;
	volatile uint32_t BCCR; // 0x2C Background Color Config
	uint32_t reserved3;
	volatile uint32_t IER;	 // 0x34 Interrupt Enable
	volatile uint32_t ISR;	 // 0x38 Interrupt Status
	volatile uint32_t ICR;	 // 0x3C Interrupt Clear
	volatile uint32_t LIPCR; // 0x40 Line Interrupt Position
};

// LTDC Layer registers (offset 0x84 for Layer1, 0x104 for Layer2)
struct LtdcLayerRegs
{
	volatile uint32_t CR;	 // 0x00 Layer Control
	volatile uint32_t WHPCR; // 0x04 Window Horizontal Position
	volatile uint32_t WVPCR; // 0x08 Window Vertical Position
	volatile uint32_t CKCR;	 // 0x0C Color Keying Config
	volatile uint32_t PFCR;	 // 0x10 Pixel Format Config
	volatile uint32_t CACR;	 // 0x14 Constant Alpha Config
	volatile uint32_t DCCR;	 // 0x18 Default Color Config
	volatile uint32_t BFCR;	 // 0x1C Blending Factors Config
	uint32_t reserved[2];
	volatile uint32_t CFBAR;  // 0x28 Color Frame Buffer Address
	volatile uint32_t CFBLR;  // 0x2C Color Frame Buffer Length
	volatile uint32_t CFBLNR; // 0x30 Color Frame Buffer Line Number
};

inline DsiRegs* dsi()
{
	return reinterpret_cast<DsiRegs*>(DSI_BASE);
}
inline DsiWrapRegs* dsi_wrap()
{
	return reinterpret_cast<DsiWrapRegs*>(DSI_BASE + 0x400);
}
inline LtdcRegs* ltdc()
{
	return reinterpret_cast<LtdcRegs*>(LTDC_BASE);
}
inline LtdcLayerRegs* ltdc_layer1()
{
	return reinterpret_cast<LtdcLayerRegs*>(LTDC_BASE + 0x84);
}

// OTM8009A timing (landscape 800x480)
static constexpr uint32_t HACT = 800;
static constexpr uint32_t VACT = 480;
static constexpr uint32_t HSA  = 2;
static constexpr uint32_t HBP  = 34;
static constexpr uint32_t HFP  = 34;
static constexpr uint32_t VSA  = 1;
static constexpr uint32_t VBP  = 15;
static constexpr uint32_t VFP  = 16;

// DSI clock values
static constexpr uint32_t LANE_BYTE_CLK_KHZ = 62500; // 500MHz / 8
static constexpr uint32_t LCD_CLK_KHZ		= 27429;

using DelayFn = void (*)(uint32_t ms);

// Forward declaration
inline void init_otm8009a(bool use_rgb565, DelayFn delay);

/// Send a DSI short write command (DCS or generic).
inline void dsi_short_write(uint8_t channel, uint8_t cmd, uint8_t param)
{
	// Wait for command FIFO not full
	while ((dsi()->GPSR & (1U << 1)) == 0)
	{} // Wait CMDFE (command FIFO empty)

	// Short write, 2 parameters
	dsi()->GHCR = (0x15U << 0) |						  // Data type: DCS short write 1 param
				  (static_cast<uint32_t>(channel) << 6) | // Virtual channel
				  (static_cast<uint32_t>(cmd) << 8) |	  // Command
				  (static_cast<uint32_t>(param) << 16);	  // Parameter
}

/// Send a DSI short write command with no parameter.
inline void dsi_short_write_0p(uint8_t channel, uint8_t cmd)
{
	while ((dsi()->GPSR & (1U << 1)) == 0)
	{}

	dsi()->GHCR = (0x05U << 0) | // DCS short write 0 params
				  (static_cast<uint32_t>(channel) << 6) | (static_cast<uint32_t>(cmd) << 8);
}

/// Send a DSI long write command.
inline void dsi_long_write(uint8_t channel, uint8_t cmd, const uint8_t* data, uint16_t len)
{
	// Wait for command FIFO not full
	while ((dsi()->GPSR & (1U << 1)) == 0)
	{}

	// Fill payload FIFO
	uint32_t total = len + 1; // cmd + data
	uint32_t words = (total + 3) / 4;

	// First word includes the command
	uint32_t first = cmd;
	for (uint32_t i = 0; i < 3 && i < len; ++i)
		first |= static_cast<uint32_t>(data[i]) << ((i + 1) * 8);
	dsi()->GPDR = first;

	// Remaining words
	for (uint32_t w = 1; w < words; ++w)
	{
		uint32_t word = 0;
		for (uint32_t b = 0; b < 4; ++b)
		{
			uint32_t idx = w * 4 + b - 1; // -1 because cmd took first byte
			if (idx < len)
				word |= static_cast<uint32_t>(data[idx]) << (b * 8);
		}
		dsi()->GPDR = word;
	}

	// Send header
	dsi()->GHCR = (0x39U << 0) |										// DCS long write
				  (static_cast<uint32_t>(channel) << 6) | (total << 8); // Word count
}

/// Initialize DSI Host, LTDC, and OTM8009A display.
/// framebuffer must point to SDRAM (RGB565 or ARGB8888).
inline void init_display(void* framebuffer, bool use_rgb565, DelayFn delay)
{
	volatile uint32_t* RCC_CR		  = reinterpret_cast<volatile uint32_t*>(RCC_BASE);
	volatile uint32_t* RCC_PLLSAICFGR = reinterpret_cast<volatile uint32_t*>(RCC_BASE + 0x88);
	volatile uint32_t* RCC_DCKCFGR1	  = reinterpret_cast<volatile uint32_t*>(RCC_BASE + 0x8C);
	volatile uint32_t* RCC_APB2ENR	  = reinterpret_cast<volatile uint32_t*>(RCC_BASE + 0x44);
	volatile uint32_t* RCC_APB2RSTR	  = reinterpret_cast<volatile uint32_t*>(RCC_BASE + 0x24);
	volatile uint32_t* RCC_AHB1ENR	  = reinterpret_cast<volatile uint32_t*>(RCC_BASE + 0x30);

	// 1. LCD Reset (PJ15)
	*RCC_AHB1ENR |= (1U << 9); // GPIOJEN
	volatile uint32_t* GPIOJ_MODER = reinterpret_cast<volatile uint32_t*>(0x40022400);
	volatile uint32_t* GPIOJ_BSRR  = reinterpret_cast<volatile uint32_t*>(0x40022418);
	*GPIOJ_MODER &= ~(3U << 30);
	*GPIOJ_MODER |= (1U << 30);		 // PJ15 = output
	*GPIOJ_BSRR = (1U << (15 + 16)); // PJ15 low (reset active)
	delay(20);
	*GPIOJ_BSRR = (1U << 15); // PJ15 high (release reset)
	delay(10);

	// 2. Enable clocks: LTDC, DSI, DMA2D
	*RCC_APB2ENR |= (1U << 26); // LTDCEN
	*RCC_APB2ENR |= (1U << 27); // DSIEN

	// Reset LTDC and DSI
	*RCC_APB2RSTR |= (1U << 26);
	*RCC_APB2RSTR &= ~(1U << 26);
	*RCC_APB2RSTR |= (1U << 27);
	*RCC_APB2RSTR &= ~(1U << 27);

	// 3. Configure PLLSAI for LTDC clock
	// PLLSAIN=384, PLLSAIR=7 -> 384MHz / 7 = 54.86MHz
	// PLLSAIDIVR = /2 -> 27.43 MHz LTDC clock
	*RCC_CR &= ~(1U << 28); // Disable PLLSAI
	while (*RCC_CR & (1U << 29))
	{} // Wait not ready

	*RCC_PLLSAICFGR = (384U << 6) | // PLLSAIN
					  (7U << 28);	// PLLSAIR

	// PLLSAIDIVR = /2 (bits 17:16 = 00 in DCKCFGR1)
	*RCC_DCKCFGR1 &= ~(3U << 16);

	*RCC_CR |= (1U << 28); // Enable PLLSAI
	while ((*RCC_CR & (1U << 29)) == 0)
	{} // Wait ready

	// 4. Initialize DSI Host
	// Disable DSI wrapper and host
	dsi_wrap()->WCR = 0;
	dsi()->CR		= 0;

	// Configure DSI PLL: NDIV=100, IDF=/5 (010), ODF=/1 (00)
	// PHY clock = HSE/IDF * 2 * NDIV = 25/5 * 2 * 100 = 1000 MHz
	// Lane byte clock = 1000 / 2 lanes / 8 = 62.5 MHz
	volatile uint32_t* DSI_WRPCR = reinterpret_cast<volatile uint32_t*>(DSI_BASE + 0x400 + 0x18);
	*DSI_WRPCR					 = (100U << 2) | // NDIV
				 (2U << 11) |					 // IDF = /5 (encoded as 010)
				 (0U << 14);					 // ODF = /1

	// Enable DSI PLL
	*DSI_WRPCR |= (1U << 0); // PLLEN
	// Wait for PLL lock
	while ((*DSI_WRPCR & (1U << 1)) == 0)
	{}

	// Configure number of lanes and TX escape clock divider
	dsi()->PCONFR = (1U << 0); // NL = 2 lanes (encoded as 01)
	dsi()->CCR	  = 4;		   // TXECKDIV = 62500/15620 ≈ 4

	// Configure DSI clock and data lanes
	dsi()->CLCR = (1U << 0) | // DPCC: clock lane running
				  (1U << 1);  // ACR: automatic clock lane control

	// Clock lane timer
	dsi()->CLTCR = (35U << 0) | // LP2HS time
				   (35U << 16); // HS2LP time

	// Data lane timer
	dsi()->DLTCR = (35U << 0) |	 // LP2HS time
				   (35U << 16) | // HS2LP time
				   (0U << 24);	 // MRD time

	// 5. Configure DSI Video Mode
	dsi()->VMCR = (2U << 0) |  // VMT: burst mode
				  (1U << 8) |  // LPVSAE: LP during VSA
				  (1U << 9) |  // LPVBPE: LP during VBP
				  (1U << 10) | // LPVFPE: LP during VFP
				  (1U << 11) | // LPVAE: LP during VACT
				  (1U << 12) | // LPHBPE: LP during HBP
				  (1U << 13) | // LPHFE: LP during HFP
				  (1U << 15);  // LPCE: LP command enable

	dsi()->VPCR	 = HACT;  // Packet size = HACT
	dsi()->VCCR	 = 0;	  // Number of chunks
	dsi()->VNPCR = 0xFFF; // Null packet size

	// Video timing (convert from LCD clocks to lane byte clocks)
	dsi()->VHSACR = (HSA * LANE_BYTE_CLK_KHZ) / LCD_CLK_KHZ;
	dsi()->VHBPCR = (HBP * LANE_BYTE_CLK_KHZ) / LCD_CLK_KHZ;
	dsi()->VLCR	  = ((HACT + HSA + HBP + HFP) * LANE_BYTE_CLK_KHZ) / LCD_CLK_KHZ;
	dsi()->VVSACR = VSA;
	dsi()->VVBPCR = VBP;
	dsi()->VVFPCR = VFP;
	dsi()->VVACR  = VACT;

	// LP largest packet sizes
	dsi()->LPMCR = (16U << 16) | // LPSIZE (VSA/VBP/VFP)
				   (0U << 0);	 // VLPSIZE (VACT)

	// LTDC VCID and color coding
	dsi()->LVCIDR = 0; // Virtual channel 0
	if (use_rgb565)
		dsi()->LCOLCR = 0; // RGB565 (24-bit on DSI, 16-bit on LTDC)
	else
		dsi()->LCOLCR = 5; // RGB888 / ARGB8888

	// Polarity config (all active high)
	dsi()->LPCR = 0;

	// 6. Configure LTDC
	// Sync config: HSW-1, VSH-1
	ltdc()->SSCR = ((HSA - 1) << 16) | (VSA - 1);
	// Back porch: accumulated HSA+HBP-1, accumulated VSA+VBP-1
	ltdc()->BPCR = ((HSA + HBP - 1) << 16) | (VSA + VBP - 1);
	// Active width: accumulated HSA+HBP+HACT-1
	ltdc()->AWCR = ((HSA + HBP + HACT - 1) << 16) | (VSA + VBP + VACT - 1);
	// Total width: accumulated HSA+HBP+HACT+HFP-1
	ltdc()->TWCR = ((HSA + HBP + HACT + HFP - 1) << 16) | (VSA + VBP + VACT + VFP - 1);

	// Background color (black)
	ltdc()->BCCR = 0;

	// 7. Configure LTDC Layer 1
	auto* layer = ltdc_layer1();

	// Window position
	uint32_t h_start = HSA + HBP;
	uint32_t h_stop	 = h_start + HACT - 1;
	uint32_t v_start = VSA + VBP;
	uint32_t v_stop	 = v_start + VACT - 1;

	layer->WHPCR = (h_start << 0) | (h_stop << 16);
	layer->WVPCR = (v_start << 0) | (v_stop << 16);

	// Pixel format
	if (use_rgb565)
		layer->PFCR = 2; // RGB565
	else
		layer->PFCR = 0; // ARGB8888

	// Constant alpha = 255 (fully opaque)
	layer->CACR = 255;

	// Default color (transparent black)
	layer->DCCR = 0;

	// Blending factors
	layer->BFCR = (6U << 0) | // BF2: pixel alpha * constant alpha
				  (4U << 8);  // BF1: constant alpha

	// Framebuffer address
	layer->CFBAR = reinterpret_cast<uint32_t>(framebuffer);

	// Framebuffer line length and pitch
	uint32_t bpp   = use_rgb565 ? 2 : 4;
	uint32_t pitch = HACT * bpp;
	layer->CFBLR   = (pitch << 16) | (pitch + 3);
	layer->CFBLNR  = VACT;

	// Enable layer
	layer->CR = 1;

	// Reload shadow registers
	ltdc()->SRCR = 1; // IMR: immediate reload

	// 8. Enable LTDC
	ltdc()->GCR |= 1; // LTDCEN

	// 9. Enable DSI Host and Wrapper
	dsi()->CR		= 1;		 // EN
	dsi_wrap()->WCR = (1U << 0); // DSIEN

	delay(10);

	// 10. Send OTM8009A initialization commands via DSI
	init_otm8009a(use_rgb565, delay);
}

/// OTM8009A display initialization via DSI commands.
/// This sends the full register programming sequence.
inline void init_otm8009a(bool use_rgb565, DelayFn delay)
{
	uint8_t ch = 0; // Virtual channel 0

	// Enter CMD2 mode
	dsi_short_write(ch, 0x00, 0x00);
	{
		uint8_t d[] = {0x80, 0x09, 0x01};
		dsi_long_write(ch, 0xFF, d, 3);
	}

	// ORISE command 2, shift 0x80
	dsi_short_write(ch, 0x00, 0x80);
	{
		uint8_t d[] = {0x80, 0x09};
		dsi_long_write(ch, 0xFF, d, 2);
	}

	// SD_PCH_CTRL
	dsi_short_write(ch, 0x00, 0x80);
	dsi_short_write(ch, 0xC4, 0x30);
	delay(10);

	dsi_short_write(ch, 0x00, 0x8A);
	dsi_short_write(ch, 0xC4, 0x40);
	delay(10);

	// PWR_CTRL4
	dsi_short_write(ch, 0x00, 0xB1);
	dsi_short_write(ch, 0xC5, 0xA9);

	// PWR_CTRL2
	dsi_short_write(ch, 0x00, 0x91);
	dsi_short_write(ch, 0xC5, 0x34);

	// P_DRV_M
	dsi_short_write(ch, 0x00, 0xB4);
	dsi_short_write(ch, 0xC0, 0x50);

	// VCOMDC
	dsi_short_write(ch, 0x00, 0x00);
	dsi_short_write(ch, 0xD9, 0x4E);

	// Oscillator adj
	dsi_short_write(ch, 0x00, 0x81);
	dsi_short_write(ch, 0xC1, 0x66);

	// Video mode internal
	dsi_short_write(ch, 0x00, 0xA1);
	dsi_short_write(ch, 0xC1, 0x08);

	// PWR_CTRL2
	dsi_short_write(ch, 0x00, 0x92);
	dsi_short_write(ch, 0xC5, 0x01);

	dsi_short_write(ch, 0x00, 0x95);
	dsi_short_write(ch, 0xC5, 0x34);

	// GVDD/NGVDD
	dsi_short_write(ch, 0x00, 0x00);
	{
		uint8_t d[] = {0x79, 0x79};
		dsi_long_write(ch, 0xD8, d, 2);
	}

	// PWR_CTRL2
	dsi_short_write(ch, 0x00, 0x94);
	dsi_short_write(ch, 0xC5, 0x33);

	// Panel timing
	dsi_short_write(ch, 0x00, 0xA3);
	dsi_short_write(ch, 0xC0, 0x1B);

	// Power control
	dsi_short_write(ch, 0x00, 0x82);
	dsi_short_write(ch, 0xC5, 0x83);

	// Source precharge
	dsi_short_write(ch, 0x00, 0x81);
	dsi_short_write(ch, 0xC4, 0x83);

	dsi_short_write(ch, 0x00, 0xA1);
	dsi_short_write(ch, 0xC1, 0x0E);

	dsi_short_write(ch, 0x00, 0xA6);
	{
		uint8_t d[] = {0x00, 0x01};
		dsi_long_write(ch, 0xB3, d, 2);
	}

	// GOAVST
	dsi_short_write(ch, 0x00, 0x80);
	{
		uint8_t d[] = {0x85, 0x01, 0x00, 0x84, 0x01, 0x00};
		dsi_long_write(ch, 0xCE, d, 6);
	}

	dsi_short_write(ch, 0x00, 0xA0);
	{
		uint8_t d[] = {0x18, 0x04, 0x03, 0x39, 0x00, 0x00, 0x00, 0x18, 0x03, 0x03, 0x3A, 0x00, 0x00, 0x00};
		dsi_long_write(ch, 0xCE, d, 14);
	}

	dsi_short_write(ch, 0x00, 0xB0);
	{
		uint8_t d[] = {0x18, 0x02, 0x03, 0x3B, 0x00, 0x00, 0x00, 0x18, 0x01, 0x03, 0x3C, 0x00, 0x00, 0x00};
		dsi_long_write(ch, 0xCE, d, 14);
	}

	dsi_short_write(ch, 0x00, 0xC0);
	{
		uint8_t d[] = {0x01, 0x01, 0x20, 0x20, 0x00, 0x00, 0x01, 0x02, 0x00, 0x00};
		dsi_long_write(ch, 0xCF, d, 10);
	}

	dsi_short_write(ch, 0x00, 0xD0);
	dsi_short_write(ch, 0xCF, 0x00);

	// CB registers
	dsi_short_write(ch, 0x00, 0x80);
	{
		uint8_t d[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
		dsi_long_write(ch, 0xCB, d, 10);
	}

	dsi_short_write(ch, 0x00, 0x90);
	{
		uint8_t d[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
		dsi_long_write(ch, 0xCB, d, 15);
	}

	dsi_short_write(ch, 0x00, 0xA0);
	{
		uint8_t d[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
		dsi_long_write(ch, 0xCB, d, 15);
	}

	dsi_short_write(ch, 0x00, 0xB0);
	{
		uint8_t d[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
		dsi_long_write(ch, 0xCB, d, 10);
	}

	dsi_short_write(ch, 0x00, 0xC0);
	{
		uint8_t d[] = {0x00, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
		dsi_long_write(ch, 0xCB, d, 15);
	}

	dsi_short_write(ch, 0x00, 0xD0);
	{
		uint8_t d[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00};
		dsi_long_write(ch, 0xCB, d, 15);
	}

	dsi_short_write(ch, 0x00, 0xE0);
	{
		uint8_t d[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
		dsi_long_write(ch, 0xCB, d, 10);
	}

	dsi_short_write(ch, 0x00, 0xF0);
	{
		uint8_t d[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
		dsi_long_write(ch, 0xCB, d, 10);
	}

	// CC registers
	dsi_short_write(ch, 0x00, 0x80);
	{
		uint8_t d[] = {0x00, 0x26, 0x09, 0x0B, 0x01, 0x25, 0x00, 0x00, 0x00, 0x00};
		dsi_long_write(ch, 0xCC, d, 10);
	}

	dsi_short_write(ch, 0x00, 0x90);
	{
		uint8_t d[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x26, 0x0A, 0x0C, 0x02};
		dsi_long_write(ch, 0xCC, d, 15);
	}

	dsi_short_write(ch, 0x00, 0xA0);
	{
		uint8_t d[] = {0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
		dsi_long_write(ch, 0xCC, d, 15);
	}

	dsi_short_write(ch, 0x00, 0xB0);
	{
		uint8_t d[] = {0x00, 0x25, 0x0C, 0x0A, 0x02, 0x26, 0x00, 0x00, 0x00, 0x00};
		dsi_long_write(ch, 0xCC, d, 10);
	}

	dsi_short_write(ch, 0x00, 0xC0);
	{
		uint8_t d[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25, 0x0B, 0x09, 0x01};
		dsi_long_write(ch, 0xCC, d, 15);
	}

	dsi_short_write(ch, 0x00, 0xD0);
	{
		uint8_t d[] = {0x26, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
		dsi_long_write(ch, 0xCC, d, 15);
	}

	// PWR_CTRL1 pump settings
	dsi_short_write(ch, 0x00, 0x81);
	dsi_short_write(ch, 0xC5, 0x66);
	dsi_short_write(ch, 0x00, 0xB6);
	dsi_short_write(ch, 0xF5, 0x06);

	// CABC settings
	dsi_short_write(ch, 0x00, 0xB1);
	dsi_short_write(ch, 0xC6, 0x06);

	// Exit CMD2 mode
	dsi_short_write(ch, 0x00, 0x00);
	{
		uint8_t d[] = {0xFF, 0xFF, 0xFF};
		dsi_long_write(ch, 0xFF, d, 3);
	}

	// Gamma correction 2.2+ table
	dsi_short_write(ch, 0x00, 0x00);
	{
		uint8_t d[] = {0x00, 0x09, 0x0F, 0x0E, 0x07, 0x10, 0x0B, 0x0A, 0x04, 0x07, 0x0B, 0x08, 0x0F, 0x10, 0x0A, 0x01};
		dsi_long_write(ch, 0xE1, d, 16);
	}

	// Gamma correction 2.2- table
	dsi_short_write(ch, 0x00, 0x00);
	{
		uint8_t d[] = {0x00, 0x09, 0x0F, 0x0E, 0x07, 0x10, 0x0B, 0x0A, 0x04, 0x07, 0x0B, 0x08, 0x0F, 0x10, 0x0A, 0x01};
		dsi_long_write(ch, 0xE2, d, 16);
	}

	// Sleep out
	dsi_short_write_0p(ch, 0x11);
	delay(120);

	// Set pixel format
	if (use_rgb565)
		dsi_short_write(ch, 0x3A, 0x55); // RGB565
	else
		dsi_short_write(ch, 0x3A, 0x77); // RGB888

	// MADCTL: landscape orientation
	dsi_short_write(ch, 0x36, 0x00);

	// CASET: column 0-799
	{
		uint8_t d[] = {0x00, 0x00, 0x03, 0x1F};
		dsi_long_write(ch, 0x2A, d, 4);
	}

	// PASET: page 0-479
	{
		uint8_t d[] = {0x00, 0x00, 0x01, 0xDF};
		dsi_long_write(ch, 0x2B, d, 4);
	}

	// Display on
	dsi_short_write_0p(ch, 0x29);
	delay(50);

	// Content adaptive brightness control
	dsi_short_write(ch, 0x51, 0x7F); // Brightness
	dsi_short_write(ch, 0x53, 0x2C); // CTRL display
	dsi_short_write(ch, 0x55, 0x02); // CABC
}

} // namespace lumen::platform::dsi
