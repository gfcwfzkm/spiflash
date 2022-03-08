/*
 * spiflash.c
 *
 * Created: 25.01.2021 13:43:31
 *  Author: gfcwfzkm
 */ 

#include "spiflash.h"

void flash_configInterface(flash_t *flash,
    	void *ioInterface, uint8_t (*startTransaction)(void*),
	    uint8_t (*sendBytes)(void*,uint8_t,uint8_t*,uint16_t),
	    uint8_t (*transceiveBytes)(void*,uint8_t,uint8_t*,uint16_t),
	    uint8_t (*getBytes)(void*,uint8_t,uint8_t*,uint16_t),
	    uint8_t (*endTransaction)(void*))
{
	flash->error = FLASH_NOERROR;
	flash->ioInterface = ioInterface;
	flash->startTransaction = startTransaction;
	flash->sendBytes = sendBytes;
	flash->transceiveBytes = transceiveBytes;
	flash->getBytes = getBytes;
	flash->endTransaction = endTransaction;
}

enum FLASH_ERROR flash_init(flash_t *flash, uint32_t capacity,
        uint8_t manufacturerID)
{
	uint8_t readBuf[5];
	
	flash->total_size = capacity;
	
	/* Testing if flash can be accessed by reading the Manufacturer/Device ID */
	flash_readCMD(flash, FLASH_R_MAN_DEV_ID, readBuf, 5);

	/* Lets store the read IDs regardless of the success, so the user can decide */
	flash->MF_ID = readBuf[3];
	flash->DEV_ID = readBuf[4];

	if ((flash->error == FLASH_NOERROR) && (readBuf[3] != manufacturerID))
	{
		flash->error = FLASH_IDERROR;
	}
	else if (flash->error == FLASH_NOERROR)
	{		
		while(flash_readStatus1(flash) & FLASH_R_STATUS_1_BUSY){}
		
		flash_execCMD(flash, FLASH_R_ENABLE_RESET);
		flash_execCMD(flash, FLASH_R_RESET_DEVICE);
		
		FLASH_RESET_DELAY();
		
		flash_execCMD(flash, FLASH_R_ENTER_4B_ADDRMODE);
		flash_writeDisable(flash);
		flash_waitUntilReady(flash);
	}
		
	return flash->error;
}

//-------------- LOW LEVEL --------------------------------------------------------------
enum FLASH_ERROR flash_execCMD(flash_t *flash, uint8_t command)
{
	flash->error = FLASH_NOERROR;
	
	flash->startTransaction(flash->ioInterface);
	flash->sendBytes(flash->ioInterface, 0, &command, 1);
	flash->endTransaction(flash->ioInterface);
	
	return flash->error;
}

enum FLASH_ERROR flash_readCMD(flash_t *flash, uint8_t command, uint8_t *dataBuf, uint8_t length)
{
	flash->error = FLASH_NOERROR;	
	
	flash->startTransaction(flash->ioInterface);
	
	flash->sendBytes(flash->ioInterface, 0, &command, 1);
	
	flash->getBytes(flash->ioInterface, 0, dataBuf, length);
	
	flash->endTransaction(flash->ioInterface);
	
	return flash->error;
}

enum FLASH_ERROR flash_writeCMD(flash_t *flash, uint8_t command, uint8_t *dataBuf, uint8_t length)
{
	flash->error = FLASH_NOERROR;
	
	flash->startTransaction(flash->ioInterface);
	flash->sendBytes(flash->ioInterface, 0, &command, 1);
	flash->sendBytes(flash->ioInterface, 0, dataBuf, length);
	flash->endTransaction(flash->ioInterface);
	
	return flash->error;
}

enum FLASH_ERROR flash_eraseCMD(flash_t *flash, uint8_t command, uint32_t addr)
{
	uint8_t command_address[5];
	flash->error = FLASH_NOERROR;
	
	command_address[0] = command;
	flash_addrToBuf(command_address + 1, addr);	
	
	flash->startTransaction(flash->ioInterface);
	
	flash->sendBytes(flash->ioInterface, 0, command_address, 5);
	
	flash->endTransaction(flash->ioInterface);
	
	return flash->error;
}

enum FLASH_ERROR flash_readMemory(flash_t *flash, uint32_t addr, uint8_t *dataBuf, uint32_t length)
{
	/* Fast read wird benutzt - dummy byte am Anfang benutzt, deswegen 6 statt 5 Bytes */
	uint8_t command_address[6];
	flash->error = FLASH_NOERROR;
	
	command_address[0] = FLASH_R_READ_FAST_DATA;
	flash_addrToBuf(command_address + 1, addr);
	command_address[5] = 0;	
	
	flash->startTransaction(flash->ioInterface);
	
	flash->sendBytes(flash->ioInterface, 0, command_address, 6);
	flash->getBytes(flash->ioInterface, 0, dataBuf, length);
	
	flash->endTransaction(flash->ioInterface);
	
	return flash->error;
}

enum FLASH_ERROR flash_writePage(flash_t *flash, uint32_t addr, uint8_t *dataBuf, uint32_t length)
{
	uint8_t command_address[5];
	flash->error = FLASH_NOERROR;
	
	command_address[0] = FLASH_R_PAGE_PROGRAMM;
	flash_addrToBuf(command_address + 1, addr);
	
	flash->startTransaction(flash->ioInterface);
	
	flash->sendBytes(flash->ioInterface, 0, command_address, 5);
	flash->sendBytes(flash->ioInterface, 0, dataBuf, length);
	
	flash->endTransaction(flash->ioInterface);
	
	return flash->error;
}

void flash_addrToBuf(uint8_t *addrBuf, uint32_t addr)
{
	*addrBuf++ = (addr >> 24) & 0xFF;
	*addrBuf++ = (addr >> 16) & 0xFF;
	*addrBuf++ = (addr >> 8) & 0xFF;
	*addrBuf++ = addr & 0xFF;	
}

uint8_t flash_readStatus1(flash_t *flash)
{
	uint8_t status;
	flash->error = FLASH_NOERROR;
	
	flash_readCMD(flash, FLASH_R_READ_STATUS_1, &status, 1);
	
	return status;	
}

uint8_t flash_readStatus2(flash_t *flash)
{
	uint8_t status;
	flash->error = FLASH_NOERROR;
	
	flash_readCMD(flash, FLASH_R_READ_STATUS_2, &status, 1);
	
	return status;
}

uint8_t flash_readStatus3(flash_t *flash)
{
	uint8_t status;
	flash->error = FLASH_NOERROR;
	
	flash_readCMD(flash, FLASH_R_READ_STATUS_3, &status, 1);
	
	return status;
}

enum FLASH_ERROR flash_writeEnable(flash_t *flash)
{
	flash->error = FLASH_NOERROR;
	
	flash_execCMD(flash, FLASH_R_WRITE_ENABLE);
	
	return flash->error;
}

enum FLASH_ERROR flash_writeDisable(flash_t *flash)
{
	flash->error = FLASH_NOERROR;
	
	flash_execCMD(flash, FLASH_R_WRITE_DISABLE);
	
	return flash->error;
}

uint32_t flash_getJEDECID(flash_t *flash)
{
	uint8_t jedec[3];
	flash->error = FLASH_NOERROR;
	
	flash_readCMD(flash, FLASH_R_JEDEC_ID, jedec, 3);
	
	return (((uint32_t)jedec[0] << 16) | (jedec[1] << 8) | jedec[2]);
}

enum FLASH_ERROR flash_readBuffer(flash_t *flash, uint32_t addr, uint8_t *dataBuf, uint32_t length)
{
	flash->error = FLASH_NOERROR;
	
	flash_waitUntilReady(flash);
	flash_readMemory(flash, addr, dataBuf, length);
	return flash->error;
}

uint32_t flash_writeBuffer(flash_t *flash, uint32_t addr, uint8_t *dataBuf, uint32_t length)
{
	uint32_t RemainsToWrite = length;
	uint32_t leftOnPage;
	uint32_t toWrite;
	flash->error = FLASH_NOERROR;
	
	while (RemainsToWrite)
	{
		flash_waitUntilReady(flash);
		flash_writeEnable(flash);
		
		leftOnPage = FLASH_C_PAGE_SIZE - (addr & (FLASH_C_PAGE_SIZE - 1));
		toWrite = (RemainsToWrite < leftOnPage) ? RemainsToWrite : leftOnPage;
		
		if (flash_writePage(flash, addr, dataBuf, toWrite) != FLASH_NOERROR)
		{
			break;
		}
		
		RemainsToWrite -= toWrite;
		dataBuf += toWrite;
		addr += toWrite;
	}
	
	length -= RemainsToWrite;
	
	return length;
}

enum FLASH_ERROR flash_eraseSector(flash_t *flash, uint32_t sectorNum)
{
	flash->error = FLASH_NOERROR;
	
	flash_waitUntilReady(flash);
	flash_writeEnable(flash);
	
	flash_eraseCMD(flash, FLASH_R_ERASE_SECTOR, sectorNum * FLASH_C_SECTOR_SIZE);
	
	return flash->error;
}

enum FLASH_ERROR flash_eraseBlock(flash_t *flash, uint32_t blockNum)
{
	flash->error = FLASH_NOERROR;
	
	flash_waitUntilReady(flash);
	flash_writeEnable(flash);
	
	flash_eraseCMD(flash, FLASH_R_ERASE_BLOCK, blockNum * FLASH_C_BLOCK_SIZE);
	
	return flash->error;
}

enum FLASH_ERROR flash_eraseChip(flash_t *flash)
{
	flash->error = FLASH_NOERROR;
	
	flash_waitUntilReady(flash);
	flash_writeEnable(flash);
	
	flash_execCMD(flash, FLASH_R_ERASE_CHIP);
	
	return flash->error;
}
