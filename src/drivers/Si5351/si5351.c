/*
	
*/

#include <zephyr/drivers/i2c.h>
#include "si5351.h"

#define I2C_DEV_NAME DEVICE_DT_NAME(DT_NODELABEL(i2c1))
struct device *i2c_dev = NULL;

double floor(double num) {
    if (num >= LLONG_MAX || num <= LLONG_MIN || num != num) {
        /* handle large values, infinities and nan */
        return num;
    }
    long long n = (long long)num;
    double d = (double)n;
    if (d == num || num >= 0)
        return d;
    else
        return d - 1;
}

/*
	Writes one byte to an 8-bit address.
*/
uint8_t SI5351_register_write(uint8_t address, uint8_t data)
{
	int result;

	result = i2c_reg_write_byte(i2c_dev, SI5351_ADDRESS, address, data);
	
	return result;
}


/*
	Reads and returns a byte from an 8-bit address.
*/
uint8_t SI5351_register_read(uint8_t address)
{
	uint8_t data;

	i2c_reg_read_byte(i2c_dev, SI5351_ADDRESS, address, &data);

	return data;
}


/*
	Turns off the Si5351 module's CLK0 and CLK2 outputs, sets the CLK1's output power to ~25mW (?) and resets the PLLs.
*/
int SI5351_init(void)
{
	i2c_dev = device_get_binding(I2C_DEV_NAME);

	if (NULL == i2c_dev)
	{
		printk("DEVICE BINDING FAIL!\n");
		return -ENODEV;
	}

	int res = i2c_configure(i2c_dev, I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_CONTROLLER);
	if (res)
	{
		printk("DEVICE CONFIGURE FAIL!\n");
		return res;
	}

	SI5351_register_write(3, 0b11111111);											// disable all CLKs
	SI5351_register_write(183, 0b11010010);											// 10pF (default) - ABM8G-25.000MHZ-B4Y-T (10pF, +-30ppm, -20�C +70�C)
	SI5351_register_write(9, 0b00000000);											// 0: OEB pin controls enable/disable state of CLKx output.
	SI5351_register_write(15, 0b00000000);											// Input Source for PLLs.
	SI5351_register_write(16, 0b10001100);											// CLK0: 2mA, PLLA, off
	SI5351_register_write(17, 0b00001111);											// CLK1: 8mA, PLLA, on
	SI5351_register_write(18, 0b10001100);											// CLK2: 2mA, PLLB, off
	SI5351_register_write(19, 0b10001100);											// CLK3: 2mA, PLLB, off
	SI5351_register_write(20, 0b10001100);											// CLK4: 2mA, PLLB, off
	SI5351_register_write(21, 0b10001100);											// CLK5: 2mA, PLLB, off
	SI5351_register_write(22, 0b10001100);											// CLK6: 2mA, PLLB, off
	SI5351_register_write(23, 0b10001100);											// CLK7: 2mA, PLLB, off
	SI5351_register_write(149, 0b00000000);											// Spread Spectrum disable.
	SI5351_register_write(187, 0b00000000);											// Disable Fanout.
	SI5351_register_write(177, 0b10100000);
	
	return 0;										// PLLB and PLLA reset
}


/*
	Turns off the CLK1 output.
*/
void SI5351_deinit(void)
{
	SI5351_register_write(3, 0b11111111);											// disable all CLKs
	SI5351_register_write(17, 0b10001100);											// CLK1: 2mA, PLLA, off
	SI5351_register_write(177, 0b10100000);											// PLLB and PLLA reset
}


/*
	Enables output on CLK1 and disables output on CLK0 and CLK2.
*/
void SI5351_start_TX(void)
{
	SI5351_register_write(3, 0b11111101);											// enable CLK1
}


/*
	Disables output on CLK0, CLK1 and CLK3 - all three output clocks on an Si5351A.
*/
void SI5351_stop_TX(void)
{
	SI5351_register_write(3, 0b11111111);											// disable CLK1
}


/*
	VCO frequency in the range of 600 to 900 MHz.
	Where (a+b/c) is in the range 15+0/1,048,575 to 90+0/1,048,575.
	
	Fvco = Fxtal * (a + b / c)
	
	MSNA_P1 - integer part of the PLLA Feedback Multisynth divider
	MSNA_P2 - numerator for the fractional part of the PLLA Feedback Multisynth divider
	MSNA_P3 - denominator for the fractional part of the PLLA Feedback Multisynth divider
*/
void SI5351_PLLA_frequency(uint64_t freqHZ)
{
	PLLAfreq = freqHZ;
	uint32_t a = freqHZ / XTALfreq;
	uint64_t b = (freqHZ % XTALfreq) * 1048575 / XTALfreq;
	uint32_t c = 1048575;
	uint32_t p1 = 128 * a + floor(128 * b / c) - 512;
	uint32_t p2 = 128 * b - c * floor(128 * b / c);
  
	SI5351_register_write(26, (c & 0x0000FF00) >> 8);								// MSNA_P3[15:8]
	SI5351_register_write(27, (c & 0x000000FF));									// MSNA_P3[7:0]
	SI5351_register_write(28, (p1 & 0x00030000) >> 16);								// MSNA_P1[17:16]
	SI5351_register_write(29, (p1 & 0x0000FF00) >> 8);								// MSNA_P1[15:8]
	SI5351_register_write(30, (p1 & 0x000000FF));									// MSNA_P1[7:0]
	SI5351_register_write(31, ((c & 0x000F0000) >> 12) | ((p2 & 0x000F0000) >> 16));	// MSNA_P3[19:16], MSNA_P2[19:16]
	SI5351_register_write(32, (p2 & 0x0000FF00) >> 8);								// MSNA_P2[15:8]
	SI5351_register_write(33, (p2 & 0x000000FF));									// MSNA_P2[7:0]
}


/*
	Where (a+b/c) is in the range 6+0/1,048,575 to 1800+0/1,048,575.
	
	Fout = Fvco / (Multisynth * R)
	Multisynth = a + b / c
	
	For Fvco=689929800Hz the range is 383,081Hz to 114,988,300Hz. Step size 0.3Hz (?).
	
	MS1_DIVBY4 - MS1 Divide by 4 Enable 0b00/0b11
	R1_DIV - R1 Output Divider: 1/2/4/8/16/32/64/128
	MS1_P1 - integer part of the Multi-Synth1 divider
	MS1_P2 - numerator for the fractional part of the MultiSynth1 Divider
	MS1_P3 - denominator for the fractional part of the MultiSynth0 Divider
*/
void SI5351_MS1_frequency(uint64_t freq001HZ)
{
	uint64_t a = (PLLAfreq * 1000) / freq001HZ;
	uint64_t b = (PLLAfreq * 1000 % freq001HZ) * 1048575 / freq001HZ;
	uint32_t c = 1048575;
	uint32_t p1 = 128 * a + floor(128 * b / c) - 512;
	uint32_t p2 = 128 * b - c * floor(128 * b / c);
  
	SI5351_register_write(50, 0xFF);												// MS1_P3[15:8]
	SI5351_register_write(51, 0xFF);												// MS1_P3[7:0]
	SI5351_register_write(52, (p1 & 0x00030000) >> 16);								// R1_DIV[2:0], MS1_DIVBY4[1:0], MS1_P1[17:16]
	SI5351_register_write(53, (p1 & 0x0000FF00) >> 8);								// MS1_P1[15:8]
	SI5351_register_write(54, (p1 & 0x000000FF));									// MS1_P1[7:0]
	SI5351_register_write(55, 0xF0 | ((p2 & 0x000F0000) >> 16));					// MS1_P3[19:16], MS1_P2[19:16]
	SI5351_register_write(56, (p2 & 0x0000FF00) >> 8);								// MS1_P2[15:8]
	SI5351_register_write(57, (p2 & 0x000000FF));									// MS1_P2[7:0]
}


/*
	VCO frequency in the range of 600 to 900 MHz.
	Where (a+b/c) is in the range 15+0/1,048,575 to 90+0/1,048,575.
	
	Fvco = Fxtal * (a + b / c)
	
	MSNA_P1 - integer part of the PLLA Feedback Multisynth divider
	MSNA_P2 - numerator for the fractional part of the PLLA Feedback Multisynth divider
	MSNA_P3 - denominator for the fractional part of the PLLA Feedback Multisynth divider
*/
void SI5351_PLLA_frequency_ABC(uint32_t a, uint32_t b, uint32_t c)
{
	uint32_t p1 = 128 * a + floor(128 * b / c) - 512;
	uint32_t p2 = 128 * b - c * floor(128 * b / c);
  
	SI5351_register_write(26, (c & 0x0000FF00) >> 8);								// MSNA_P3[15:8]
	SI5351_register_write(27, (c & 0x000000FF));									// MSNA_P3[7:0]
	SI5351_register_write(28, (p1 & 0x00030000) >> 16);								// MSNA_P1[17:16]
	SI5351_register_write(29, (p1 & 0x0000FF00) >> 8);								// MSNA_P1[15:8]
	SI5351_register_write(30, (p1 & 0x000000FF));									// MSNA_P1[7:0]
	SI5351_register_write(31, ((c & 0x000F0000) >> 12) | ((p2 & 0x000F0000) >> 16));	// MSNA_P3[19:16], MSNA_P2[19:16]
	SI5351_register_write(32, (p2 & 0x0000FF00) >> 8);								// MSNA_P2[15:8]
	SI5351_register_write(33, (p2 & 0x000000FF));									// MSNA_P2[7:0]
}


/*
	Where (a+b/c) is in the range 6+0/1,048,575 to 1800+0/1,048,575.
	
	Fout = Fvco / (Multisynth * R)
	Multisynth = a + b / c
	
	For Fvco=689929800Hz the range is 383,081Hz to 114,988,300Hz. Step size 0.3Hz (?).
	
	MS1_DIVBY4 - MS1 Divide by 4 Enable 0b00/0b11
	R1_DIV - R1 Output Divider: 1/2/4/8/16/32/64/128
	MS1_P1 - integer part of the Multi-Synth1 divider
	MS1_P2 - numerator for the fractional part of the MultiSynth1 Divider
	MS1_P3 - denominator for the fractional part of the MultiSynth0 Divider
*/
void SI5351_MS1_frequency_ABC(uint32_t a, uint32_t b, uint32_t c)
{
	uint32_t p2 = 128 * b - c * floor(128 * b / c);
	uint32_t p1 = 128 * a + floor(128 * b / c) - 512;
  
	SI5351_register_write(50, (c & 0x0000FF00) >> 8);								// MS1_P3[15:8]
	SI5351_register_write(51, (c & 0x000000FF));									// MS1_P3[7:0]
	SI5351_register_write(52, (p1 & 0x00030000) >> 16);								// R1_DIV[2:0], MS1_DIVBY4[1:0], MS1_P1[17:16]
	SI5351_register_write(53, (p1 & 0x0000FF00) >> 8);								// MS1_P1[15:8]
	SI5351_register_write(54, (p1 & 0x000000FF));									// MS1_P1[7:0]
	SI5351_register_write(55, ((c & 0x000F0000) >> 12) | ((p2 & 0x000F0000) >> 16));	// MS1_P3[19:16], MS1_P2[19:16]
	SI5351_register_write(56, (p2 & 0x0000FF00) >> 8);								// MS1_P2[15:8]
	SI5351_register_write(57, (p2 & 0x000000FF));									// MS1_P2[7:0]
}


/*
	Sets the defined VCO frequency (limits the range of available TX frequencies).
	Then sets the actual TX frequency (should be ensured the VCO frequency is right for it).
	
	Enter desired frequency * 1000.
*/
void SI5351_frequency(uint64_t freq001Hz)
{
	SI5351_PLLA_frequency(VCO_FREQUENCY);
	SI5351_MS1_frequency(freq001Hz);
}