#include <SPI_RAM.h>

void SPI_RAM::begin(SPISettings settings, spi_devno dev_no, spi_modes spi_mode)
{
	_settings = settings;
	_dev_no = dev_no;

	SPI.begin(_dev_no);

	// Make sure we are in SIO mode first
	_spi_mode = SPI_QIO;
	WriteCMD(SPI_RAM_CMD_RSTIO);
	_spi_mode = SPI_DIO;
	WriteCMD(SPI_RAM_CMD_RSTIO);

	if (spi_mode != SPI_SIO) {	// Now switch to another mode if need to
		_spi_mode = SPI_SIO;
		WriteCMD( spi_mode == SPI_DIO ? SPI_RAM_CMD_EDIO : SPI_RAM_CMD_EQIO );
	}
	_spi_mode = spi_mode;
}

void SPI_RAM::WriteCMD(uint8_t cmd)
{
	SPI.beginTransaction(_settings, _dev_no, _spi_mode);
	SPI1U |= SPIUCOMMAND;
	SPI1U &= ~(SPIUMOSI|SPIUMISO|SPIUDUMMY|SPIUADDR);
	SPI1U2 = (((7 & SPIMCOMMAND) << SPILCOMMAND) | ((uint32_t)cmd) );
	SPI1CMD |= SPIBUSY;
	while(SPI1CMD & SPIBUSY) {}
	SPI.endTransaction();
}

void SPI_RAM::Read(uint32_t addr, uint8_t *buff, uint8_t count)
{
	SPI.beginTransaction(_settings, _dev_no, _spi_mode);
	while(count) {
		if(count > 64) {
			Read_(addr, buff, 64);
			count -= 64;
			buff += 64;
			addr += 64;
		} else {
			Read_(addr, buff, count);
			count = 0;
		}
	}
	SPI.endTransaction();
}

void SPI_RAM::Read_(uint32_t addr, uint8_t *buff, uint8_t count)
{
	uint32_t i;

	if ( addr > 0x00FFFFFF ) return;	// Bad Address - only 24 bits allowed

	if ( _spi_mode == SPI_SIO ) {
		SPI1U |= (SPIUCOMMAND | SPIUADDR | SPIUMISO);
		SPI1U &= (SPIUFLASHMODE | SPIUMOSI | SPIUDUMMY);
		SPI1U1 = (((0 & SPIMMOSI) << SPILMOSI) |					// no data out
					((((count<<3) - 1) & SPIMMISO) << SPILMISO) |	// data in bits
					((23 & SPIMADDR) << SPILADDR));					// address is 24 bits A0-A23
		SPI1A = addr << 8;											// read address
		SPI1U2 = (((7 & SPIMCOMMAND) << SPILCOMMAND) | SPI_RAM_CMD_READ);
	} else {
		SPI1U |= (SPIUADDR | SPIUMISO | SPIUDUMMY);
		SPI1U &= (SPIUFLASHMODE | SPIUMOSI | SPIUCOMMAND);
		SPI1U1 = (((0 & SPIMMOSI) << SPILMOSI) |					// no data out
					((((count<<3) - 1) & SPIMMISO) << SPILMISO) |	// data in bits
					((31 & SPIMADDR) << SPILADDR) |					// 8 bits CMD (A24-A31) + 24 bits address (A0-A23)
					((1 & SPIMDUMMY) << SPILDUMMY));				// 8 bits dummy cycle
		SPI1A = ((SPI_RAM_CMD_READ << 24) | addr);					// CMD + read address
	}

	SPI1CMD |= SPIBUSY;
	while(SPI1CMD & SPIBUSY) {}
	for (i = 0; i < count/4; i++) {
		uint32_t d = SPI1W(i);
		for (uint8_t j = 0; j < 4; j++) buff[4*i + j] = 0xFF & (d >> 8*j);
	}
	for (i = count%4; i > 0; i--) buff[count - i] = 0xFF & (SPI1W(count/4) >> 8*i);	// Unaligned data
}

void SPI_RAM::Write(uint32_t addr, const uint8_t *values, uint8_t count)
{
	SPI.beginTransaction(_settings, _dev_no, _spi_mode);
	while(count) {
		if(count > 64) {
			Write_(addr, values, 64);
			count -= 64;
			values += 64;
			addr += 64;
		} else {
			Write_(addr, values, count);
			count = 0;
		}
	}
	SPI.endTransaction();
}

void SPI_RAM::Write_(uint32_t addr, const uint8_t *values, uint8_t count)
{
	uint8_t i;

	if ( addr > 0x00FFFFFF ) return;	// Bad Address - only 24 bits allowed

	if ( _spi_mode == SPI_SIO ) {
		SPI1U |= (SPIUCOMMAND | SPIUADDR | SPIUMOSI);
		SPI1U &= (SPIUFLASHMODE | SPIUMISO | SPIUDUMMY);
		SPI1U1 = (((((count<<3) - 1) & SPIMMOSI) << SPILMOSI) |	// data out bits
					((0 & SPIMMISO) << SPILMISO) |				// no data in
					((23 & SPIMADDR) << SPILADDR));				// address is 24 bits A0-A23
		SPI1A = addr << 8;										// write address
		SPI1U2 = (((7 & SPIMCOMMAND) << SPILCOMMAND) | SPI_RAM_CMD_WRITE);
	} else {
		SPI1U |= (SPIUADDR | SPIUMOSI);
		SPI1U &= (SPIUFLASHMODE |SPIUMISO | SPIUCOMMAND | SPIUDUMMY);
		SPI1U1 = (((((count<<3) - 1) & SPIMMOSI) << SPILMOSI) |	// data out bits
					((0 & SPIMMISO) << SPILMISO) |				// no data in
					((31 & SPIMADDR) << SPILADDR));				// 8 bits CMD (A24-A31) + 24 bits address (A0-A23)
		SPI1A = ((SPI_RAM_CMD_WRITE << 24) | addr);				// CMD + write address
	}

	for (i=0; i < count/4; i++) SPI1W(i) = (values[i*4+0] << 0) | (values[i*4+1] << 8) | (values[i*4+2] << 16) | (values[i*4+3] << 24);
	if (i = count%4) {	// Unaligned data
		uint32_t d = 0;
		while (i > 0) d = ((d << 8) | values[count - i--]);
		d <<= (32 - ((count%4) << 2));
		SPI1W((count >> 2) + 1) = d;
	}

	SPI1CMD |= SPIBUSY;
	while(SPI1CMD & SPIBUSY) {}
}

void SPI_RAM::WriteRegister(const uint8_t value)
{
	SPI1U |= (SPIUCOMMAND | SPIUMOSI);
	SPI1U &= (SPIUFLASHMODE | SPIUADDR | SPIUMISO | SPIUDUMMY);
	SPI1U1 = ((7 & SPIMMOSI) << SPILMOSI);	// 8 bits data out, no data in or address
	SPI1U2 = (((7 & SPIMCOMMAND) << SPILCOMMAND) | SPI_RAM_CMD_WRMR);

	SPI1W0 = (uint32_t)value;

	SPI1CMD |= SPIBUSY;
	while(SPI1CMD & SPIBUSY) {}
}

uint8_t SPI_RAM::ReadRegister()
{
	SPI1U |= (SPIUCOMMAND | SPIUMISO);
	SPI1U &= (SPIUFLASHMODE | SPIUADDR | SPIUMOSI | SPIUDUMMY);
	SPI1U1 = ((7 & SPIMMISO) << SPILMISO);	// 8 bits data in, no data out or address
	SPI1U2 = (((7 & SPIMCOMMAND) << SPILCOMMAND) | SPI_RAM_CMD_RDMR);

	SPI1CMD |= SPIBUSY;
	while(SPI1CMD & SPIBUSY) {}

	return (SPI1W0 & 0xFF);
}
