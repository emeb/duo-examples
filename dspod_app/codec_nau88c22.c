/*
 * codec_nau88c22.c - configure a NAU88C22 codec
 * 05-31-25 E. Brombaugh
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
#define CODEC_ADDR 0x1A
#define REGADDRSZ 1

#define I2C_SLAVE	0x0703	/* Change slave address			*/
#define I2C_SMBUS	0x0720	/* SMBus-level access */
#define I2C_SMBUS_READ	1
#define I2C_SMBUS_WRITE	0
#define I2C_SMBUS_BYTE_DATA	    2 
#define I2C_SMBUS_HALFWORD_DATA	    3 

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
static uint16_t codec_settings_int[] = 
{
	// Reset and power-up
	0,		0x000,	// Software Reset
	1,		0x0CD,	// aux mixers, internal tie-off enable & 80k impedance for slow charge
	69,		0x000,	// low voltage bias
//	127,	250,	// Wait 250ms
	
	// Input routing & ADC setup
	2,		0x03F,	// ADC, PGA, Mix/Boost inputs powered up
	//14,		0x108,	// HPF, 128x
	14,		0x008,	// DC, 128x
	
	44,		0x044,	// PGA input - select line inputs
	45, 	0x010,	// LPGA 0dB, unmuted, immediate, no ZC
	46,		0x010,	// RPGA 0dB, unmuted, immediate, no ZC
	47,		0x030,	// Lchl line in 0dB, no boost
	48,		0x030,	// Rchl line in 0dB, no boost
	
	// Output routing & DAC setup
	3,		0x18F,	// DACs and aux outputs enabled
	10,		0x008,	// 128x rate
//	10,		0x000,	// 64x rate
	49,		0x002,	// thermal shutdown only (default)
	50,		0x001,	// L main mixer input from LDAC (default) NEEDED!
	51,		0x001,	// R main mixer input from RDAC (default) NEEDED!
	56,		0x001,	// LDAC to AUX2 (default) NEEDED!
	57,		0x001,	// RDAC to AUX1 (default) NEEDED!
	
	// Format & clock
	4, 		0x110,	// BCLK inv, 16-bit I2S
#if 0
	// No PLL
	6,		0x000,	// MCLK, no PLL, 1x division, FS, BCLK inputs
	7,		0x000,	// 4wire off, 48k, no timer (default)
#else
	// PLL setting for IMCLK = 12.5MHz from 12.5MHz input
	6,		0x140,	// PLL, 2x division, FS, BCLK inputs (default)
	7,		0x000,	// 4wire off, 48k, no timer (default)
	36,		0x008,	// PLL D = 1, N = 8
	37,		0x000,	// K (high) = 0
	38,		0x000,	// K (mid) = 0
	39,		0x000,	// K (low) = 0
	8,		0x034,	// CSB pin is PLL/16
	1,		0x0ED,	// enable PLL
#endif

	255,	0x000,	// EOF
};

/* Without PLL, with Ext MCLK */
static uint16_t codec_settings_ext[] = 
{
	// Reset and power-up
	0,		0x000,	// Software Reset
	1,		0x0CD,	// aux mixers, internal tie-off enable & 80k impedance for slow charge
	69,		0x000,	// low voltage bias
	127,	250,	// Wait 250ms
	
	// Input routing & ADC setup
	2,		0x03F,	// ADC, PGA, Mix/Boost inputs powered up
	//14,		0x108,	// HPF, 128x
	14,		0x008,	// DC, 128x
	
	44,		0x044,	// PGA input - select line inputs
	45, 	0x010,	// LPGA 0dB, unmuted, immediate, no ZC
	46,		0x010,	// RPGA 0dB, unmuted, immediate, no ZC
	47,		0x030,	// Lchl line in 0dB, no boost
	48,		0x030,	// Rchl line in 0dB, no boost
	
	// Output routing & DAC setup
	3,		0x18F,	// DACs and aux outputs enabled
	10,		0x008,	// 128x rate
	49,		0x002,	// thermal shutdown only (default)
	50,		0x001,	// L main mixer input from LDAC (default) NEEDED!
	51,		0x001,	// R main mixer input from RDAC (default) NEEDED!
	56,		0x001,	// LDAC to AUX2 (default) NEEDED!
	57,		0x001,	// RDAC to AUX1 (default) NEEDED!
	
	// Format & clock
	4, 		0x110,	// BCLK inv, 16-bit I2S

	// No PLL
	6,		0x000,	// MCLK, no PLL, 1x division, FS, BCLK inputs
	7,		0x000,	// 4wire off, 48k, no timer (default)

	255,	0x000,	// EOF
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
  * @brief  Writes to a given register into the TLV320nau88c22 audio codec
			through the control interface (I2C)
  * @param  RegAddr: The address of the register to be written.
  * @param  RegValue: value to be written to register.
  * @retval 0 if success
  */
int Codec_WriteRegister(int file, uint8_t RegAddr, uint16_t RegValue)
{
	union i2c_smbus_data data;
	
	/* build reg/data pair*/
	RegAddr = (RegAddr<<1) | ((RegValue>>8) & 1);
	data.byte = (RegValue & 0xff);
		
	/* send data */
	return i2c_smbus_access(file, I2C_SMBUS_WRITE, RegAddr,
								I2C_SMBUS_BYTE_DATA, &data);
}

/**
  * @brief  Reads from a given register in the TLV320nau88c22 audio codec
			through the control interface (I2C)
  * @param  RegAddr: The address of the register to be read.
  * @param  RegValue: pointer to array of values to be written to registers.
  * @retval 0 if success
  */
int Codec_ReadRegister(int file, uint8_t RegAddr, uint16_t *RegValue)
{
	union i2c_smbus_data data;
	
	/* prepare reg addr */
	RegAddr <<= 1;
		
	/* get data */
	if(!i2c_smbus_access(file, I2C_SMBUS_READ, RegAddr,
								I2C_SMBUS_HALFWORD_DATA, &data))
	{
		*RegValue = ((data.word>>8)&0xff) | ((data.word&0xff)<<8);
		return 0;
	}
	else
		return 1;	
}

/*
 * configure the default settings
 */
int Codec_Config(int file, uint16_t codec_settings[])
{
	int result = 0;
	uint8_t idx = 0, reg, tries;
	uint16_t val;
	
	while((reg = codec_settings[2*idx]) < 0x80)
	{
		/* get value */
		val = codec_settings[2*idx + 1];
		
		if(reg == 0x7f)
		{
			/* delay value ms */
			qprintf("Codec_Config(): Delay %d ms\n\r", val);
			usleep(val * 1000);
		}
		else
		{
			/* send value to reg */
#if 1
			tries = 0;
			while((Codec_WriteRegister(file, reg, val) != 0 ) && (tries++ < 5));
			if(tries <5)
			{
				qprintf("Codec_Config(): Write Addr %3d = 0x%02X (%d retries)\n\r", reg, val, tries);
			}
			else
			{
				qprintf("Codec_Config(): Write Addr %3d failed\n\r", reg);
				result = 1;
			}
#else
			if(!Codec_WriteRegister(file, reg, val))
			{
				qprintf("Codec_Config(): Write Addr %3d = 0x%02X\n\r", reg, val);
			}
			else
			{
				qprintf("Codec_Config(): Write Addr %3d failed\n\r", reg);
				result = 1;
			}
#endif
		}
		idx++;
			
		/* slow things down a bit? */
		usleep(4000);
	}
	
	return result;
}

/*
 * Do all hardware setup for Codec
 */
int codec_nau88c22(int v, int bus, int type, int dump)
{
	int result = 0;
	
	verbose = v;
	
	/* open I2C bus */
	char filename[20];
	sprintf(filename, "/dev/i2c-%d", bus);
	int i2c_file = open(filename, O_RDWR);

	if(i2c_file < 0)
	{
		qprintf("codec_nau88c22: Couldn't open I2C device %s\n", filename);
		result = -errno;
		goto fail1;
	}
	else
		qprintf("codec_nau88c22: opened I2C device %s\n", filename);
	
	/* set I2C slave address */
	if(ioctl(i2c_file, I2C_SLAVE, CODEC_ADDR) < 0)
	{
		qprintf("codec_nau88c22: couldn't set address 0x%02X\n", CODEC_ADDR);
		result =  -errno;
		goto fail2;
	}
	else
		qprintf("codec_nau88c22: set address 0x%02X\n", CODEC_ADDR);
	
	/* Set up codec */
	result = Codec_Config(i2c_file, type ? codec_settings_ext : codec_settings_int );

	if(dump)
	{
		/* dump I2C registers in bank 0 */
		for(int reg=0;reg<128;reg++)
		{
			uint16_t val;
			if(!Codec_ReadRegister(i2c_file, reg, &val))
				printf("Read Addr %3d = 0x%03X\n\r", reg, val);
			else
			{
				printf("Read Addr %3d failed\n\r", reg);
				break;
			}
		}
	}
	
fail2:
	/* close bus */
	close(i2c_file);

fail1:
	return result;
}
