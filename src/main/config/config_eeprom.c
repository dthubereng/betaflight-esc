#include "include.h"

#define FLASH_PAGE_SIZE                 ((uint16_t)0x400)
#define FLASH_PAGE_COUNT                64
#define FLASH_TO_RESERVE_FOR_CONFIG     0x800
#define CONFIG_START_FLASH_ADDRESS      (0x08000000 + (uint32_t)((FLASH_PAGE_SIZE * FLASH_PAGE_COUNT) - FLASH_TO_RESERVE_FOR_CONFIG))

static uint8_t calculateChecksum(const uint8_t *data, uint32_t length)
{
    uint8_t checksum = 0;
    const uint8_t *byteOffset;

    for (byteOffset = data; byteOffset < (data + length); byteOffset++)
        checksum ^= *byteOffset;

    return checksum;
}

bool isEEPROMContentValid(void)
{
    const master_t *temp = (const master_t *) CONFIG_START_FLASH_ADDRESS;
    uint8_t checksum = 0;

    if (EEPROM_CONF_VERSION != temp->version) {
        return false;
    }

    if (temp->size != sizeof(master_t) || temp->magic_be != 0xBE || temp->magic_ef != 0xEF) {
        return false;
    }

    checksum = calculateChecksum((const uint8_t *) temp, sizeof(master_t));
    if (checksum != 0) {
        return false;
    }
    return true;
}

void writeEEPROM(void)
{
    FLASH_Status status = 0;
    int8_t attemptsRemaining = 3;

    masterConfig.version = EEPROM_CONF_VERSION;
    masterConfig.size = sizeof(master_t);
    masterConfig.magic_be = 0xBE;
    masterConfig.magic_ef = 0xEF;
    masterConfig.chk = 0; 
    masterConfig.chk = calculateChecksum((const uint8_t *) &masterConfig, sizeof(master_t));

    FLASH_Unlock();
    while (attemptsRemaining--) {
        FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR);

        for (uint32_t wordOffset = 0; wordOffset < sizeof(master_t); wordOffset += 4) {
            if (wordOffset % FLASH_PAGE_SIZE == 0) {
                status = FLASH_ErasePage(CONFIG_START_FLASH_ADDRESS + wordOffset);
                if (status != FLASH_COMPLETE) {
                    break;
                }
            }

            status = FLASH_ProgramWord(CONFIG_START_FLASH_ADDRESS + wordOffset, *(uint32_t *) ((char *) &masterConfig + wordOffset));
            if (status != FLASH_COMPLETE) {
                break;
            }
        }

        if (status == FLASH_COMPLETE) {
            break;
        }
    }

    FLASH_Lock();

    if (status != FLASH_COMPLETE || !isEEPROMContentValid()) {

    }
}

void readEEPROM(void)
{
    if (!isEEPROMContentValid()) {

    }

    memcpy(&masterConfig, (char *) CONFIG_START_FLASH_ADDRESS, sizeof(master_t));
}
