#include "main.h"

/* USER CODE BEGIN Includes */
#include "pn532_stm32f4.h"
#include "rfid_helper.h"
#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart2;
PN532 pn532;

/* USER CODE BEGIN PD */
#define VIB_THRESHOLD_SOFT  4050
#define VIB_THRESHOLD_HARD  4050
#define VALID_WINDOW_MS     20000
/* USER CODE END PD */

/* USER CODE BEGIN PV */
const size_t num_records = 2;
/* USER CODE END PV */

/* Function prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_ADC1_Init(void);

/* USER CODE BEGIN PFP */

void print(const char *msg) {
    HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
}

static uint32_t read_vibration_raw(void) {
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
    uint32_t val = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    return val;
}

static uint8_t read_ldr(void) {
    return (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6) == GPIO_PIN_RESET) ? 1 : 0;
}

static void set_alarm(uint8_t on) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void i2c_recover(void) {
    HAL_I2C_DeInit(&hi2c1);
    HAL_Delay(5);
    MX_I2C1_Init();
    PN532_Init(&pn532);
}

/* USER CODE END PFP */

int main(void) {

    uint8_t  fw_buf[255];
    uint8_t  uid[MIFARE_UID_MAX_LENGTH] = {0};
    int32_t  uid_len = 0;
    uint32_t valid_window_start = 0;
    uint8_t  window_active = 0;

    rfid_record_t records[2] = {
        {{0x89, 0xA2, 0xCD, 0x59}, 4, "Blue Tag"},
        {{0x3A, 0xDD, 0xEA, 0x3F}, 4, "Card!"}
    };

    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_USART2_UART_Init();
    MX_ADC1_Init();

    PN532_Init(&pn532);

    if (PN532_GetFirmwareVersion(&pn532, fw_buf) == PN532_STATUS_OK) {
        print("Vault Guard Ready\r\n");
    } else {
        print("PN532 Init Failed!\r\n");
        return -1;
    }

    PN532_SamConfiguration(&pn532);
    set_alarm(0);

// Add these ABOVE while(1)
// Add these ABOVE while(1)
uint8_t access_granted = 0;
uint32_t access_start_time = 0;
uint8_t last_printed_sec = 0;
uint32_t last_rfid_prompt = 0;

while (1)
{
	
    uint32_t vib_raw  = read_vibration_raw();
    uint8_t  ldr      = read_ldr();
	//char dbg[30];
	//sprintf(dbg,"VIB: %lu", vib_raw);
	//print(dbg);

    uint8_t vib_trigger = (vib_raw < VIB_THRESHOLD_SOFT);  // corrected logic

    // =========================================================
    // -------- STATE 1: IDLE (WAITING FOR RFID) --------
    // =========================================================
    if (!access_granted)
    {
        // -------- PROMPT USER --------
        if ((HAL_GetTick() - last_rfid_prompt) >= 1000)
        {
static uint8_t dot_count = 0;
    char anim_msg[15];

    // Clear the line and print "Scan RFID" with trailing dots
    // The spaces at the end ensure old dots are erased when resetting
    switch(dot_count) {
        case 0: sprintf(anim_msg, "\rScan RFID    "); break;
        case 1: sprintf(anim_msg, "\rScan RFID.   "); break;
        case 2: sprintf(anim_msg, "\rScan RFID..  "); break;
        case 3: sprintf(anim_msg, "\rScan RFID... "); break;
    }

    print(anim_msg);

    dot_count = (dot_count + 1) % 4; // Cycle 0, 1, 2, 3
    last_rfid_prompt = HAL_GetTick();        }

        // -------- INTRUSION CHECK --------
        if (ldr || vib_trigger)
        {
            print("\rINTRUSION\r\n");
            set_alarm(1);
            while (1); // stop system
        }

        // -------- RFID READ --------
        memset(uid, 0, sizeof(uid));
        uid_len = PN532_ReadPassiveTarget(&pn532, uid, PN532_MIFARE_ISO14443A, 100);

        if (uid_len > 0)
        {
            uint8_t valid = 0;

            for (int i = 0; i < (int)num_records; i++)
            {
                if (uid_len == records[i].uid_len &&
                    memcmp(uid, records[i].uid, records[i].uid_len) == 0)
                {
                    valid = 1;
                    break;
                }
            }

            if (valid)
            {
                print("\rValid RFID - Access Started\r\n");
                set_alarm(0);

                access_granted = 1;
                access_start_time = HAL_GetTick();
                last_printed_sec = 0;
            }
            else
            {
                print("\rInvalid RFID\r\n");
							HAL_Delay(2000);
            }
        }
    }

    // =========================================================
    // -------- STATE 2: ACCESS WINDOW (15 sec) --------
    // =========================================================
    else
    {
        uint32_t elapsed_sec = (HAL_GetTick() - access_start_time) / 1000;

        // -------- PRINT TIMER --------
        if (elapsed_sec > last_printed_sec && elapsed_sec <= 15)
        {
            char msg[32];
            sprintf(msg, "\rTime: %lu sec\r", elapsed_sec);
            print(msg);

            last_printed_sec = elapsed_sec;
        }

        // -------- LDR DETECTION --------
        if (ldr)
        {
            print("Locker Open\r\n");

            // Wait until light disappears
            while (read_ldr())
            {
                HAL_Delay(100);
            }

            print("Locker Closed\r\n");
						HAL_Delay(5000);
            access_granted = 0;
            last_printed_sec = 0;
        }

        // -------- TIMEOUT --------
        else if ((HAL_GetTick() - access_start_time) >= 15000)
        {
            print("Access Timeout\r\n");

            access_granted = 0;
            last_printed_sec = 0;
        }
    }

    HAL_Delay(100);
}}

void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM       = 4;
    RCC_OscInitStruct.PLL.PLLN       = 180;
    RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ       = 2;
    RCC_OscInitStruct.PLL.PLLR       = 2;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

    if (HAL_PWREx_EnableOverDrive() != HAL_OK) Error_Handler();

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) Error_Handler();
}

static void MX_ADC1_Init(void) {
    ADC_ChannelConfTypeDef sConfig = {0};

    hadc1.Instance                   = ADC1;
    hadc1.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution            = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode          = DISABLE;
    hadc1.Init.ContinuousConvMode    = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion       = 1;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    if (HAL_ADC_Init(&hadc1) != HAL_OK) Error_Handler();

    // PA7 = ADC1 Channel 7 (vibration sensor)
    sConfig.Channel      = ADC_CHANNEL_7;
    sConfig.Rank         = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) Error_Handler();
}

static void MX_I2C1_Init(void) {
    hi2c1.Instance             = I2C1;
    hi2c1.Init.ClockSpeed      = 100000;
    hi2c1.Init.DutyCycle       = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1     = 0;
    hi2c1.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2     = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) Error_Handler();
}

static void MX_USART2_UART_Init(void) {
    huart2.Instance          = USART2;
    huart2.Init.BaudRate     = 115200;
    huart2.Init.WordLength   = UART_WORDLENGTH_8B;
    huart2.Init.StopBits     = UART_STOPBITS_1;
    huart2.Init.Parity       = UART_PARITY_NONE;
    huart2.Init.Mode         = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK) Error_Handler();
}

static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // PA5 = Alarm output (LED/Buzzer)
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin   = GPIO_PIN_5;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // PA6 = LDR digital input
    GPIO_InitStruct.Pin  = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // PA7 = Vibration sensor analog input for ADC
    GPIO_InitStruct.Pin  = GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void Error_Handler(void) {
    __disable_irq();
    while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {}
#endif