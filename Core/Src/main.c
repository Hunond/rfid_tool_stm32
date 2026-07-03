/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "MFRC522_STM32.h"
#include <stdbool.h>
#include "w25qxx.h"
#include "string.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

#define MAX_CARDS 50        // Максимальное количество карт в памяти
#define CARD_SIZE 4         // UID bytes
#define FLASH_ADDR 0x000000 // Начальный адрес в W25Q16 для нашей базы
#define LED1_Off() HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET)
#define LED1_On() HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET)
#define LED2_Off() HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
#define LED2_On() HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi3;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

// RFID
MFRC522_t rfid;
uint8_t card_uid[4];
typedef enum
{
  MODE_READ,  // Режим обычного чтения (проверка карт)
  MODE_RECORD // Режим записи новых карт
} WorkMode;
WorkMode current_mode = MODE_READ;

// CARD DB
uint8_t card_buffer[MAX_CARDS][CARD_SIZE]; // Буфер карт в ОЗУ
uint16_t card_count = 0;

// FLASH W25Q16
uint8_t w25_rx_buf[1025];
uint8_t w25_tx_buf[10];

char str1[30];

// Буфер приёма CDC — объявлен extern в main.h, используется в usbd_cdc_if.c
volatile uint8_t cdc_rx_buf[64];
volatile uint8_t cdc_rx_len = 0;
volatile uint8_t cdc_rx_ready = 0;



volatile uint8_t button_pressed = 0; // Флаг нажатия кнопки
volatile uint8_t erase_flag = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI3_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == K1buttEXTI_Pin)
  {
    // Простейший программный антидребезг на базе системных тиков
    static uint32_t last_press_tick = 0;
    uint32_t current_tick = HAL_GetTick();
    
    if (current_tick - last_press_tick > 250) // 250 мс таймаут
    {
      button_pressed = 1; // Просто взводим флаг для main()
      last_press_tick = current_tick;
    }
  }
  if (GPIO_Pin == K0buttEXTI_Pin){
    erase_flag = 1;
  }
}


// Перенаправление printf в USART1

int _write(int file, char *ptr, int len)
{
  if (HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, 1000) == HAL_OK)
    return len;
  return 0;
}

/*
@brief Открывает Реле1 на X миллисекунд, затем закрывает его.
*/
static void relay1_open(int x)
{
  HAL_GPIO_WritePin(Relay1_GPIO_Port, Relay1_Pin, GPIO_PIN_SET);
  printf("RELAY1_OK\r\n");
  HAL_Delay(x);
  HAL_GPIO_WritePin(Relay1_GPIO_Port, Relay1_Pin, GPIO_PIN_RESET);
  printf("RELAY1_CLOSED\r\n");
}

void Save_Buffer_To_Flash(void) {
    uint8_t temp_page[256] = {0};
    
    // Записываем размер базы в первые два байта
    temp_page[0] = (card_count >> 8) & 0xFF;
    temp_page[1] = card_count & 0xFF;
    
    // Копируем накопленные карты следом за заголовком
    if (card_count > 0) {
        memcpy(&temp_page[2], card_buffer, card_count * CARD_SIZE);
    }
    
    // Стираем сектор перед записью новой информации
    W25_Erase_Sector(FLASH_ADDR); 
    
    // Записываем страницу (размер: 2 байта заголовка + карты)
    W25_Write_Page(FLASH_ADDR, temp_page, 2 + (card_count * CARD_SIZE));
}

void Load_Buffer_From_Flash(void) {
    uint8_t header[2];
    // Читаем только заголовок, чтобы узнать количество карт
    W25_Read_data(FLASH_ADDR, header, 2);
    card_count = (header[0] << 8) | header[1];
    
    // Если флешка пустая (0xFFFF) или данные повреждены
    if (card_count == 0xFFFF || card_count > MAX_CARDS) {
        card_count = 0;
        return;
    }
    
    // Если карты есть, вычитываем их напрямую в наш двумерный массив ОЗУ
    if (card_count > 0) {
        W25_Read_data(FLASH_ADDR + 2, (uint8_t*)card_buffer, card_count * CARD_SIZE);
    }
    printf("Loaded %d cards from flash.\r\n", card_count);
}


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI3_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  // BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB
  uint32_t w25_id = 0;
  w25_id = W25_Ini();
  printf("W25Qxx ID: 0x%06X\r\n", w25_id);
  current_mode = MODE_READ;
  Load_Buffer_From_Flash();
  LED1_Off();
  LED2_Off();
  
  // Привязываем периферию
  rfid.hspi = &hspi3;
  rfid.csPort = GPIOD;
  rfid.csPin = CS_Pin; // PD0
  rfid.rstPort = GPIOD;
  rfid.rstPin = RST_Pin; // PD1

  // Инициализируем сам чип RC522
  MFRC522_Init(&rfid);
  MFRC522_AntennaOn(&rfid);


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  /*
  TODO:
  1. Реализовать режим записи новых карт (нажатие кнопки переводит в режим записи, сканирование карты сохраняет её UID в память, повторное нажатие кнопки возвращает в режим чтения)
  2. Реализовать хранение UID карт в W25Q16, подряд UID карт по 4 байта)
  3. Реализовать удаление всех карт из памяти - очистка первых байт памяти
  4. Реализовать удаление конкретной карты из памяти - поиск UID в памяти и удаление его (можно просто затирать 4 байта нулями)
  5. В режиме чтения считывать сохраненные карты и включать реле если совпадание найдено.
  */
  while (1)
  {
    if(erase_flag){
      W25_Erase_Sector(FLASH_ADDR);
      erase_flag = 0;
      current_mode = MODE_READ;
      Load_Buffer_From_Flash();
      printf("Erased \r\n");
    }

    // 1. ОБРАБОТКА НАЖАТИЯ КНОПКИ (БЕЗОПАСНЫЙ КОНТЕКСТ)
    if (button_pressed)
    {
      button_pressed = 0; // Сбрасываем флаг прерывания
      
      if (current_mode == MODE_READ)
      {
        current_mode = MODE_RECORD;
        printf("\r\n[MODE] Switched to RECORD mode.\r\n");
        LED1_On(); // Включаем светодиод записи
      }
      else if (current_mode == MODE_RECORD)
      {
        printf("\r\n[MODE] Saving database to Flash...\r\n");
        Save_Buffer_To_Flash(); // Теперь это вызывать абсолютно БЕЗОПАСНО!
        printf("[MODE] Saved. Total cards: %d\r\n", card_count);
        
        current_mode = MODE_READ;
        LED1_Off(); // Выключаем светодиод
        printf("[MODE] Switched to READ mode.\r\n");
      }
    }
uint8_t atqa[2];
    // 2. РАБОТА С РЕЖИМАМИ И RFID
    // Вместо блокирующей waitcardDetect используем одиночный RequestA
    if (MFRC522_RequestA(&rfid, atqa) == STATUS_OK)
    {
      
      // Если карта ответила, пробуем прочитать UID
      if (MFRC522_ReadUid(&rfid, card_uid) == STATUS_OK)
      {
        if (current_mode == MODE_READ)
        {
          printf("Card detected: %02X %02X %02X %02X\r\n", card_uid[0], card_uid[1], card_uid[2], card_uid[3]);
          bool found = false;
          for (uint16_t i = 0; i < card_count; i++)
          {
            if (memcmp(card_buffer[i], card_uid, CARD_SIZE) == 0)
            {
              found = true;
              break;
            }
          }

          if (found)
          {
            printf("Card is authorized. Activating relay.\r\n");
            relay1_open(1000); // Открываем реле на 1 секунду
          }
          else
          {
            printf("Card is NOT authorized.\r\n");
          }
        }
        else if (current_mode == MODE_RECORD)
        {
          printf("Card detected for recording: %02X %02X %02X %02X\r\n", card_uid[0], card_uid[1], card_uid[2], card_uid[3]);
          bool already_exists = false;
          for (uint16_t i = 0; i < card_count; i++)
          {
            if (memcmp(card_buffer[i], card_uid, CARD_SIZE) == 0)
            {
              already_exists = true;
              break;
            }
          }
        
          if (!already_exists && card_count < MAX_CARDS)
          {
            memcpy(card_buffer[card_count], card_uid, CARD_SIZE);
            card_count++;
            printf("Card added to RAM buffer. Total in RAM: %d. (Press button to save to Flash)\r\n", card_count);
          }
          else if (already_exists)
          {
            printf("Card already exists in the database.\r\n");
          }
          else
          {
            printf("Database is full. Cannot record more cards.\r\n");
          }
        }
        
        // Ждем, пока карту уберут от считывателя, чтобы не спамить в цикле
        waitcardRemoval(&rfid);
      }
    }

    HAL_Delay(50); // Небольшая пауза для разгрузки процессора и стабильности опроса
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief SPI3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI3_Init(void)
{

  /* USER CODE BEGIN SPI3_Init 0 */

  /* USER CODE END SPI3_Init 0 */

  /* USER CODE BEGIN SPI3_Init 1 */

  /* USER CODE END SPI3_Init 1 */
  /* SPI3 parameter configuration*/
  hspi3.Instance = SPI3;
  hspi3.Init.Mode = SPI_MODE_MASTER;
  hspi3.Init.Direction = SPI_DIRECTION_2LINES;
  hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi3.Init.NSS = SPI_NSS_SOFT;
  hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi3.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI3_Init 2 */

  /* USER CODE END SPI3_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(Relay1_GPIO_Port, Relay1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LED1_Pin|LED2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(F_CS_GPIO_Port, F_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(Relay2_GPIO_Port, Relay2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, CS_Pin|RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : Relay1_Pin */
  GPIO_InitStruct.Pin = Relay1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Relay1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : K1buttEXTI_Pin K0buttEXTI_Pin */
  GPIO_InitStruct.Pin = K1buttEXTI_Pin|K0buttEXTI_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : LED1_Pin LED2_Pin */
  GPIO_InitStruct.Pin = LED1_Pin|LED2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : F_CS_Pin */
  GPIO_InitStruct.Pin = F_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(F_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : Relay2_Pin */
  GPIO_InitStruct.Pin = Relay2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Relay2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : CS_Pin RST_Pin */
  GPIO_InitStruct.Pin = CS_Pin|RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);

  HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
