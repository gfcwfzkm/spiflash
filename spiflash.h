/*
 * spiflash.h
 * Einfacher SPI-Flash Treiber für den W25Q512JV oder ähnliche ICs
 * Created: 25.01.2021 13:43:42 
 *  Author: gfcwfzkm
 */ 


#ifndef SPIFLASH_H_
#define SPIFLASH_H_

#include <inttypes.h>

/* This delay function needs to be provided by the user!
 * A delay of at least 30us is needed, 50us recommended */
#ifdef __AVR__
#include <util/delay.h>
#define FLASH_RESET_DELAY()	_delay_us(50)
#else
#define FLASH_RESET_DELAY()	delay_us(50)
#endif

#define FLASH_R_WRITE_ENABLE		0x06
#define FLASH_R_VOL_WRITE_ENABLE	0x50
#define FLASH_R_WRITE_DISABLE		0x04
#define FLASH_R_RELEASE_PD_ID		0xAB
#define FLASH_R_MAN_DEV_ID			0x90
#define FLASH_R_JEDEC_ID			0x9F
#define FLASH_R_READ_UNIQUE_ID		0x4B
#define FLASH_R_READ_DATA			0x03
#define FLASH_R_READ_FAST_DATA		0x0C
#define FLASH_R_PAGE_PROGRAMM		0x02
#define FLASH_R_ERASE_SECTOR		0x20
#define FLASH_R_ERASE_HALF_BLOCK	0x52
#define FLASH_R_ERASE_BLOCK			0xD8
#define FLASH_R_ERASE_CHIP			0xC7
#define FLASH_R_READ_STATUS_1		0x05
#define FLASH_R_READ_STATUS_2		0x35
#define FLASH_R_READ_STATUS_3		0x15
#define FLASH_R_WRITE_STATUS_1		0x01
#define FLASH_R_WRITE_STATUS_2		0x31
#define FLASH_R_WRITE_STATUS_3		0x11
#define FLASH_R_STATUS_1_SRP			0x80
#define FLASH_R_STATUS_1_TB				0x40
#define FLASH_R_STATUS_1_BP3			0x20
#define FLASH_R_STATUS_1_BP2			0x10
#define FLASH_R_STATUS_1_BP1			0x08
#define FLASH_R_STATUS_1_BP0			0x04
#define FLASH_R_STATUS_1_WEL			0x02
#define FLASH_R_STATUS_1_BUSY			0x01
#define FLASH_R_STATUS_2_SUS			0x80
#define FLASH_R_STATUS_2_CMP			0x40
#define FLASH_R_STATUS_2_LB3			0x20
#define FLASH_R_STATUS_2_LB2			0x10
#define FLASH_R_STATUS_2_LB1			0x08
#define FLASH_R_STATUS_2_QE				0x02
#define FLASH_R_STATUS_2_SRL			0x01
#define FLASH_R_STATUS_3_DRV1			0x40
#define FLASH_R_STATUS_3_DRV0			0x20
#define FLASH_R_STATUS_3_WPS			0x04
#define FLASH_R_STATUS_3_ADP			0x02
#define FLASH_R_STATUS_3_ADS			0x01
#define FLASH_R_READ_SFDP			0x5A
#define FLASH_R_ERASE_SECURITY		0x44
#define FLASH_R_PROGRAM_SECURITY	0x42
#define FLASH_R_READ_SECURITY		0x48
#define FLASH_R_ERASE_SUSPEND		0x75
#define FLASH_R_ERASE_RESUME		0x7A
#define FLASH_R_POWER_DOWN			0xB9
#define FLASH_R_ENTER_4B_ADDRMODE	0xB7
#define FLASH_R_ENABLE_RESET		0x66
#define FLASH_R_RESET_DEVICE		0x99

#define FLASH_C_BLOCK_SIZE			(1UL << 16)	//  64 kB
#define FLASH_C_SECTOR_SIZE			(1 << 12)	//   4 kB
#define FLASH_C_PAGE_SIZE			(1 << 8)	// 256 B

enum FLASH_ERROR {
	FLASH_NOERROR = 0,
	FLASH_IOERROR = 1,
	FLASH_IDERROR = 2
};

typedef struct
{
	uint32_t total_size;
	uint8_t MF_ID;		// Manufacturer ID
	uint8_t DEV_ID;		// Device ID 
	enum FLASH_ERROR error;
	void *ioInterface;					// Pointer to the IO/Peripheral Interface library
	// Any return value by the IO interface functions have to return zero when successful or
	// non-zero when not successful.
	uint8_t (*startTransaction)(void*);	// Prepare the IO/Peripheral Interface for a transaction
	uint8_t (*sendBytes)(void*,			// Send data function pointer: InterfacePointer,
						uint8_t,		// Address of the PortExpander (8-Bit Address Format!),
						uint8_t*,		// Pointer to send buffer,
						uint16_t);		// Amount of bytes to send
	uint8_t (*transceiveBytes)(void*,	// Send and receive Bytes from the buffer (SPI only)
						uint8_t,		// (8-Bit Address Format!) (ignored if zero),
						uint8_t*,		// Pointer to send buffer,
						uint16_t);		// Amount of bytes to send
	uint8_t (*getBytes)(void*,			// Get data function pointer:InterfacePointer,
						uint8_t,		// Address of the PortExpander (8-Bit Address Format!),
						uint8_t*,		// Pointer to receive buffer,
						uint16_t);		// Amount of bytes to receive
	uint8_t (*endTransaction)(void*);	// Finish the transaction / Release IO/Peripheral
} flash_t;

void flash_configInterface(flash_t *flash,
							void *ioInterface, uint8_t (*startTransaction)(void*),
							uint8_t (*sendBytes)(void*,uint8_t,uint8_t*,uint16_t),
							uint8_t (*transceiveBytes)(void*,uint8_t,uint8_t*,uint16_t),
							uint8_t (*getBytes)(void*,uint8_t,uint8_t*,uint16_t),
							uint8_t (*endTransaction)(void*));

void flash_configMemory(flash_t *flash, uint32_t capacity, 
							uint8_t manufacturerID, uint8_t deviceID);

enum FLASH_ERROR flash_init(flash_t *flash, uint32_t capacity,
							uint8_t manufacturerID);

//-------------- LOW LEVEL --------------------------------------------------------------
enum FLASH_ERROR flash_execCMD(flash_t *flash, uint8_t command);

enum FLASH_ERROR flash_readCMD(flash_t *flash, uint8_t command,
							uint8_t *dataBuf, uint8_t length);

enum FLASH_ERROR flash_writeCMD(flash_t *flash, uint8_t command,
							uint8_t *dataBuf, uint8_t length);

enum FLASH_ERROR flash_eraseCMD(flash_t *flash, uint8_t command,
							uint32_t addr);

enum FLASH_ERROR flash_readMemory(flash_t *flash, uint32_t addr,
							uint8_t *dataBuf, uint32_t length);

/* Only 256 Bytes can be written per Page! Use flash_writeBuffer to write more*/
enum FLASH_ERROR flash_writePage(flash_t *flash, uint32_t addr,
							uint8_t *dataBuf, uint32_t length);

void flash_addrToBuf(uint8_t *addrBuf, uint32_t addr);
//------------ BASIC ACCESS -------------------------------------------------------------
uint8_t flash_readStatus1(flash_t *flash);
uint8_t flash_readStatus2(flash_t *flash);
uint8_t flash_readStatus3(flash_t *flash);
#define flash_waitUntilReady(flash)	\
	while(flash_readStatus1(flash) & (FLASH_R_STATUS_1_WEL | FLASH_R_STATUS_1_BUSY))
enum FLASH_ERROR flash_writeEnable(flash_t *flash);
enum FLASH_ERROR flash_writeDisable(flash_t *flash);
uint32_t flash_getJEDECID(flash_t *flash);
enum FLASH_ERROR flash_readBuffer(flash_t *flash, uint32_t addr, uint8_t *dataBuf, uint32_t length);
uint32_t flash_writeBuffer(flash_t *flash, uint32_t addr, uint8_t *dataBuf, uint32_t length);
enum  FLASH_ERROR flash_eraseSector(flash_t *flash, uint32_t sectorNum);
enum  FLASH_ERROR flash_eraseBlock(flash_t *flash, uint32_t blockNum);
enum  FLASH_ERROR flash_eraseChip(flash_t *flash);

#endif /* SPIFLASH_H_ */
