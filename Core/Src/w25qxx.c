#include "w25qxx.h"
#include "main.h"

void SPI1_Send(uint8_t *dt, uint16_t cnt)
{
    HAL_SPI_Transmit(&W25_SPI, dt, cnt, 5000);
}
void SPI1_Recv(uint8_t *dt, uint16_t cnt)
{
    HAL_SPI_Receive(&W25_SPI, dt, cnt, 5000);
}

void W25_Reset(void)
{
    uint8_t tx[4];
    cs_set();
    tx[0] = W25_EN_RESET;
    tx[1] = W25_RESET;
    SPI1_Send(tx, 2);
    cs_reset();
}

void W25_Read_data(uint32_t addr, uint8_t *data, uint32_t sz)
{
    cs_set();
    uint8_t tx[4];
    tx[0] = W25_READ_DATA;
    tx[1] = (addr >> 16) & 0xFF;
    tx[2] = (addr >> 8) & 0xFF;
    tx[3] = addr & 0xFF;
    SPI1_Send(tx, 4);
    SPI1_Recv(data, sz);
    cs_reset();
}

uint8_t W25_Read_Status1()
{
    cs_set();
    uint8_t tx[1];
    uint8_t rx[1];
    tx[0] = W25_READ_STATUS_REG1;
    SPI1_Send(tx, 1);
    SPI1_Recv(rx, 1);
    cs_reset();
    return rx[0];
}

void W25_Write_Enable(void)
{
    cs_set();
    uint8_t tx[1];
    tx[0] = W25_WRITE_EN;
    SPI1_Send(tx, 1);
    cs_reset();
}

void W25_Erase_Sector(uint32_t addr)
{
    W25_Write_Enable();
    cs_set();
    uint8_t tx[4];
    tx[0] = W25_SECTOR_ERASE_4K;
    tx[1] = (addr >> 16) & 0xFF;
    tx[2] = (addr >> 8) & 0xFF;
    tx[3] = addr & 0xFF;
    SPI1_Send(tx, 4);
    cs_reset();
    int timeout = 0;
    while ((W25_Read_Status1() & 0x01) && timeout < 60)
    { // Ждем завершения стирания
        timeout++;
        HAL_Delay(100);
    }
}
/*
@brief Записывает данные в страницу W25Qxx, начиная с адреса addr.
@param addr 24-битный адрес в памяти W25Qxx, с которого начнется запись.
@param data указатель на буфер, из которого будут записаны данные.
@param sz количество байт для записи.
*/
void W25_Write_Page(uint32_t addr, uint8_t *data, uint16_t sz)
{
    W25_Write_Enable();
    uint8_t tx[4];

    cs_set();
    tx[0] = W25_PAGE_PROGRAM;
    tx[1] = (addr >> 16) & 0xFF;
    tx[2] = (addr >> 8) & 0xFF;
    tx[3] = addr & 0xFF;
    SPI1_Send(tx, 4);
    SPI1_Send(data, sz);
    cs_reset();
    int timeout = 0;
    while ((W25_Read_Status1() & 0x01) && timeout < 60)
    { // Ждем завершения записи
        timeout++;
        HAL_Delay(100);
    }
}


//-------------------------------------------------------------
uint32_t W25_Read_ID(void)
{
    uint8_t dt[4] = {0};
    uint8_t tx[1];
    tx[0] = W25_JEDEC_ID;
    cs_set();
    SPI1_Send(tx, 1);
    SPI1_Recv(dt, 3);
    cs_reset();
    return (dt[0] << 16) | (dt[1] << 8) | dt[2];
}

//-------------------------------------------------------------
uint32_t W25_Ini(void)
{
    W25_Reset();
    return W25_Read_ID();
}

