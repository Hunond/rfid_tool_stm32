#ifndef W25QXX_H_
#define W25QXX_H_
#include "main.h"
extern SPI_HandleTypeDef hspi1; // Внешнее объявление SPI1, чтобы использовать его в w25qxx.c



#define W25_SPI hspi1




#define W25_WRITE_EN                    0x06        /**< write enable */
#define W25_VOLATILE_SR_WRITE_EN          0x50        /**< sr write enable */
#define W25_WRITE_DISABLE                     0x04        /**< write disable */
#define W25_READ_STATUS_REG1                  0x05        /**< read status register-1 */
#define W25_READ_STATUS_REG2                  0x35        /**< read status register-2 */
#define W25_READ_STATUS_REG3                  0x15        /**< read status register-3 */
#define W25_WRITE_STATUS_REG1                 0x01        /**< write status register-1 */
#define W25_WRITE_STATUS_REG2                 0x31        /**< write status register-2 */
#define W25_WRITE_STATUS_REG3                 0x11        /**< write status register-3 */
#define W25_CHIP_ERASE                        0xC7        /**< chip erase */
#define W25_ERASE_PROGRAM_SUSPEND             0x75        /**< erase suspend */
#define W25_ERASE_PROGRAM_RESUME              0x7A        /**< erase resume */
#define W25_POWER_DOWN                        0xB9        /**< power down */
#define W25_RELEASE_POWER_DOWN                0xAB        /**< release power down */
#define W25_READ_MANUFACTURER                 0x90        /**< manufacturer */
#define W25_JEDEC_ID                          0x9F        /**< jedec id */
#define W25_GLOBAL_BLOCK_SECTOR_LOCK          0x7E        /**< global block lock */
#define W25_GLOBAL_BLOCK_SECTOR_UNLOCK        0x98        /**< global block unlock */
#define W25_ENTER_QSPI_MODE                   0x38        /**< enter spi mode */
#define W25_EN_RESET                          0x66        /**< enable reset */
#define W25_RESET                             0x99        /**< reset device */
#define W25_READ_UNIQUE_ID                    0x4B        /**< read unique id */
#define W25_PAGE_PROGRAM                      0x02        /**< page program */
#define W25_QUAD_PAGE_PROGRAM                 0x32        /**< quad page program */
#define W25_SECTOR_ERASE_4K                   0x20        /**< sector erase */
#define W25_BLOCK_ERASE_32K                   0x52        /**< block erase */
#define W25_BLOCK_ERASE_64K                   0xD8        /**< block erase */
#define W25_READ_DATA                         0x03        /**< read data */
#define W25_FAST_READ                         0x0B        /**< fast read */
#define W25_FAST_READ_DUAL_OUTPUT             0x3B        /**< fast read dual output */
#define W25_FAST_READ_QUAD_OUTPUT             0x6B        /**< fast read quad output */
#define W25_READ_SFDP_REGISTER                0x5A        /**< read SFDP register */
#define W25_ERASE_SECURITY_REGISTER           0x44        /**< erase security register */
#define W25_PROGRAM_SECURITY_REGISTER         0x42        /**< program security register */
#define W25_READ_SECURITY_REGISTER            0x48        /**< read security register */
#define W25_INDIVIDUAL_BLOCK_LOCK             0x36        /**< individual block lock */
#define W25_INDIVIDUAL_BLOCK_UNLOCK           0x39        /**< individual block unlock */
#define W25_READ_BLOCK_LOCK                   0x3D        /**< read block lock */
#define W25_FAST_READ_DUAL_IO                 0xBB        /**< fast read dual I/O */
#define W25_DEVICE_ID_DUAL_IO                 0x92        /**< device id dual I/O */
#define W25_SET_BURST_WITH_WRAP               0x77        /**< set burst with wrap */
#define W25_FAST_READ_QUAD_IO                 0xEB        /**< fast read quad I/O */
#define W25_WORD_READ_QUAD_IO                 0xE7        /**< word read quad I/O */
#define W25_OCTAL_WORD_READ_QUAD_IO           0xE3        /**< octal word read quad I/O */
#define W25_DEVICE_ID_QUAD_IO                 0x94        /**< device id quad I/O */
#define cs_set() HAL_GPIO_WritePin(F_CS_GPIO_Port, F_CS_Pin, GPIO_PIN_RESET)
#define cs_reset() HAL_GPIO_WritePin(F_CS_GPIO_Port, F_CS_Pin, GPIO_PIN_SET)

void SPI1_Send(uint8_t *dt, uint16_t cnt);
void SPI1_Recv(uint8_t *dt, uint16_t cnt);
void W25_Reset(void);
void W25_Read_data(uint32_t addr, uint8_t *data, uint32_t sz);
uint8_t W25_Read_Status1(void);
void W25_Erase_Sector(uint32_t addr);
void W25_Write_Page(uint32_t addr, uint8_t *data, uint16_t sz);
void W25_Read_Data(uint32_t addr, uint8_t *data);
uint32_t W25_Ini(void);
#endif /* W25QXX_H_ */
