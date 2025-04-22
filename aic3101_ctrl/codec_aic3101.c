/*
 * codec_aic3101.c - configure a TLV320AIC3101 codec
 * 03-21-25 E. Brombaugh
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>

/* The 7 bits Codec address (sent through I2C interface) */
#define CODEC_ADDR 0x18
#define REGADDRSZ 1

#define I2C_SLAVE	0x0703	/* Change slave address			*/
#define I2C_SMBUS	0x0720	/* SMBus-level access */
#define I2C_SMBUS_READ	1
#define I2C_SMBUS_WRITE	0
#define I2C_SMBUS_BYTE		    1
#define I2C_SMBUS_BYTE_DATA	    2 

/* 
 * Data for SMBus Messages 
 */
#define I2C_SMBUS_BLOCK_MAX	32	/* As specified in SMBus standard */	
#define I2C_SMBUS_I2C_BLOCK_MAX	32	/* Not specified but we use same structure */
union i2c_smbus_data {
	uint8_t byte;
	uint16_t word;
	uint8_t block[I2C_SMBUS_BLOCK_MAX + 2]; /* block[0] is used for length */
										/* and one more for PEC */
};

/* This is the structure as used in the I2C_SMBUS ioctl call */
struct i2c_smbus_ioctl_data {
	char read_write;
	uint8_t command;
	int size;
	union i2c_smbus_data *data;
};

static int verbose;
static int bus;

/* Codec register settings. contents is ADDR, DATA per line */
/* With PLL, with Int MCLK */
static uint8_t codec_settings_int[] = 
{
	3,		0x91,	// PLL A - PLL ena, Q=2, P=1
	4,		0x80,	// PLL B - J=32 : PLL rate = ((32.0 * 2) / (1 * 8)) * BCLK = Fs*256
	7,		0x0A,	// datapath setup - left dac/left in, right dac/right in
	11,		0x02,	// ovfl setup - PLL R = 2
	15,		0x00,	// Left PGA - unmuted, 0dB
	16,		0x00,	// Right PGA - unmuted, 0dB
	19,		0x04,	// Left ADC - enabled, MIC1LP single, 0dB
	22,		0x04,	// Right ADC - enabled, MIC1RP single, 0dB
	37,		0xC0,	// DAC Power - left/right DACs enabled
	41,		0x50,	// DAC Output Switching - use L3/R3 & independent vol
	43,		0x00,	// Left DAC - unmuted, 0dB
	44,		0x00,	// Right DAC - unmuted, 0dB
	86,		0x09,	// Left LOP/M - umuted, 0dB, enabled (NOTE - DS error, bit 0 is R/W)
	93,		0x09,	// Right LOP/M - umuted, 0dB, enabled (NOTE - DS error, bit 0 is R/W)
	102,	0xA2,	// Clockgen - CLKDIV_IN uses BCLK, PLLDIV_IN uses BCLK
	109,	0xC0,	// DAC Current - 100% increase over default
	255,	0x00,	// EOF
};

/* No PLL, with Ext MCLK */
static uint8_t codec_settings_ext[] = 
{
	7,		0x0A,	// datapath setup - left dac/left in, right dac/right in
	19,		0x04,	// Left ADC - enabled, 0dB
	15,		0x00,	// Left PGA - unmuted, 0dB
	16,		0x00,	// Right PGA - unmuted, 0dB
	22,		0x04,	// Right ADC - enabled, 0dB
	37,		0xC0,	// DAC Power - left/right DACs enabled
	41,		0x50,	// DAC Output Switching - use L3/R3 & independent vol
	43,		0x00,	// Left DAC - unmuted, 0dB
	44,		0x00,	// Right DAC - unmuted, 0dB
	86,		0x09,	// Left LOP/M - umuted, 0dB, enabled (NOTE - DS error, bit 0 is R/W)
	93,		0x09,	// Right LOP/M - umuted, 0dB, enabled (NOTE - DS error, bit 0 is R/W)
	101,	0x01,	// Clock - CODEC_CLKIN uses CLKDIV_OUT
	109,	0xC0,	// DAC Current - 100% increase over default
	255,	0x00,	// EOF
};

/* wrapper for fprintf(stderr, ...) to support verbose control */
static void qprintf(char *fmt, ...)
{
	va_list args;
	
	if(verbose)
	{
		va_start(args, fmt);
		vfprintf(stderr, fmt, args);
		va_end(args);
	}
}

/* low-level interface to I2C read/write */
static inline int32_t i2c_smbus_access(int file, char read_write, uint8_t command, 
                                     int size, union i2c_smbus_data *data)
{
	struct i2c_smbus_ioctl_data args;

	args.read_write = read_write;
	args.command = command;
	args.size = size;
	args.data = data;
	return ioctl(file,I2C_SMBUS,&args);
}

/**
  * @brief  Writes to a given register into the TLV320AIC3101 audio codec
			through the control interface (I2C)
  * @param  RegAddr: The address of the register to be written.
  * @param  RegValue: pointer to array of values to be written to registers.
  * @param  sz: number of registers to write.
  * @retval HAL_OK if success
  */
int Codec_WriteRegister(int file, uint8_t RegAddr, uint8_t RegValue)
{
	union i2c_smbus_data data;
	data.byte = RegValue;
		
	/* send data */
	return i2c_smbus_access(file, I2C_SMBUS_WRITE, RegAddr,
								I2C_SMBUS_BYTE_DATA, &data);
}

/*
 * Resets the audio codec by toggling the HW NRST line.
 */
void Codec_Reset(void)
{
}

/*
 * configure the default settings
 */
int Codec_Config(int file, uint8_t codec_settings[])
{
	int result = 0;
	uint8_t idx = 0, reg, val;
	
	while((reg = codec_settings[2*idx]) < 0x80)
	{
		val = codec_settings[2*idx + 1];
		if(!Codec_WriteRegister(file, reg, val))
		{
			qprintf("Codec_Config(): Write Addr %3d = 0x%02X\n\r", reg, val);
		}
		else
		{
			qprintf("Codec_Config(): Write Addr %3d failed\n\r", reg);
			result = 1;
		}
		idx++;
	}
	
	return result;
}

/*
 * Do all hardware setup for Codec
 */
int codec_aic3101(int v, int bus, int type)
{
	int result = 0;
	
	verbose = v;
	
	/* open I2C bus */
	char filename[20];
	sprintf(filename, "/dev/i2c-%d", bus);
	int i2c_file = open(filename, O_RDWR);

	if(i2c_file < 0)
	{
		qprintf("codec_aic3101: Couldn't open I2C device %s\n", filename);
		result = -errno;
		goto fail1;
	}
	else
		qprintf("codec_aic3101: opened I2C device %s\n", filename);
	
	/* set I2C slave address */
	if(ioctl(i2c_file, I2C_SLAVE, CODEC_ADDR) < 0)
	{
		qprintf("codec_aic3101: couldn't set address 0x%02X\n", CODEC_ADDR);
		result =  -errno;
		goto fail2;
	}
	else
		qprintf("codec_aic3101: set address 0x%02X\n", CODEC_ADDR);
	
	/* Toggle the codec NRST pin */
	Codec_Reset();
	
	/* Set up codec */
	result = Codec_Config(i2c_file, type & 1 ? codec_settings_ext : codec_settings_int );
	
fail2:
	/* close bus */
	close(i2c_file);

fail1:
	return result;
}
