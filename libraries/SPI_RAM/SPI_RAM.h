#ifndef _SPI_RAM_H_
#define _SPI_RAM_H_

#include <SPI.h>

/*
				SRAM
Instruction	Binary Code		Hex Code	Description
READ		0000 0011		0x03		Read data from memory array beginning at selected address
WRITE		0000 0010		0x02		Write data to memory array beginning at selected address
EDIO		0011 1011		0x3B		Enter Dual I/O access
EQIO		0011 1000		0x38		Enter Quad I/O access
RSTIO		1111 1111		0xFF		Reset Dual and Quad I/O access
RDMR		0000 0101		0x05		Read Mode Register
WRMR		0000 0001		0x01		Write Mode Register
*/
#define SPI_RAM_CMD_READ	 0x03
#define SPI_RAM_CMD_WRITE	 0x02
#define SPI_RAM_CMD_EDIO	 0x3B
#define SPI_RAM_CMD_EQIO	 0x38
#define SPI_RAM_CMD_RSTIO	 0xFF
#define SPI_RAM_CMD_RDMR	 0x05
#define SPI_RAM_CMD_WRMR	 0x01

/*
				FRAM	MRAM
Instruction	Description				Binary Code		Hex Code
WREN		Write Enable			0000 0110		06
WRDI		Write Disable			0000 0100		04
RDSR*		Read Status Register	0000 0101		05
WRSR		Write Status Register	0000 0001		01
READ		Read Data Bytes			0000 0011		03
WRITE		Write Data Bytes		0000 0010		02
SLEEP		Enter Sleep Mode		1011 1001		B9
WAKE		Exit Sleep Mode			1010 1011		AB
RDID		Read Device ID			1001 1111		9F
SNR			Read S/N				1100 0011		C3

* An RDSR command cannot immediately follow a READ command. If an RDSR command immediately follows a READ command,
the output data will not be correct. Any other sequence of commands is allowed. If an RDSR command is required
immediately following a READ command, it is necessary that another command be inserted before the RDSR is executed.
Alternatively, two successive RDSR commands can be issued following the READ command. The second RDSR will output the
proper state of the Status Register. 

*/
#define SPI_RAM_CMD_RDSR	SPI_RAM_CMD_RDMR
#define SPI_RAM_CMD_WRSR	SPI_RAM_CMD_WRMR
#define SPI_RAM_CMD_WREN	0x06
#define SPI_RAM_CMD_WRDI	0x04
#define SPI_RAM_CMD_SLEEP	0xB9
#define SPI_RAM_CMD_WAKE	0xAB
#define SPI_RAM_CMD_RDID	0x9F
#define SPI_RAM_CMD_SNR		0xC3

class SPI_RAM
{
	public:
		void begin(SPISettings settings, spi_devno dev_no, spi_modes spi_mode);
		void WriteCMD(uint8_t cmd);
		void Read(uint32_t addr, uint8_t *buff, uint8_t count);
		void Write(uint32_t addr, const uint8_t *values, uint8_t count);

		// Status Register
		void WriteRegister(const uint8_t value);
		uint8_t ReadRegister(void);
		void inline ReadRegister(uint8_t value) { value = ReadRegister(); };

		// FRAM/MRAM
		void inline WriteEnable(bool enable) { WriteCMD( enable ? SPI_RAM_CMD_WREN : SPI_RAM_CMD_WRDI ); };
		void inline SleepEnable(bool enable) { WriteCMD( enable ? SPI_RAM_CMD_SLEEP : SPI_RAM_CMD_WAKE ); };

	private:
		void Read_(uint32_t addr, uint8_t *buff, uint8_t count);
		void Write_(uint32_t addr, const uint8_t *values, uint8_t count);
		SPISettings _settings;
		spi_devno _dev_no;
		spi_modes _spi_mode;
};

#endif