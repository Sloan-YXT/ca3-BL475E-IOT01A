#include "hal_config.h"
#include "stdio.h"
#include "string.h"
#include "wifi.h"
//set clock to 80hz
__IO FlagStatus cmdDataReady = 0;
SPI_HandleTypeDef hspi3;
UART_HandleTypeDef huart1;
//task scheduling timer
TIM_HandleTypeDef TIM1_Handler;
//led setting off timer
TIM_HandleTypeDef TIM2_Handler;

TIM_HandleTypeDef TIM4_Handler;
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while(1) {
    HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
    HAL_Delay(500); /* wait 50 ms */
  }
  /* USER CODE END Error_Handler_Debug */
}
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 20;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV8;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV4;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}



void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	//button interrupt:change mode and send a message;
	if(GPIO_Pin == CYCLIC_MODE_TRIGGER_PIN)
	{
		changeModeMark = 1;
		mode = (mode+1)%2;
	}
	//note: the lsm6dsl didn't say the drdy and function interrupt can't work together,
	//so here can't measure lsm6dsl period.
	//1ms deviation
	else if(GPIO_Pin == GPIO_PIN_11)
	{
//		char *message = "in exti 11!\r\n";
//	    HAL_UART_Transmit(&huart1, (uint8_t*)message, strlen(message),0xFFFF);
		uint8_t res = SENSOR_IO_Read(LSM6DSL_ACC_GYRO_I2C_ADDRESS_LOW, 0x53);
		res = (res&(1<<5))&&1;
		//All interruption uses "or"
		if(res)
		{
			 HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14,GPIO_PIN_SET);
			 HAL_TIM_Base_Start_IT(&TIM2_Handler);
			 //char *message = "in tilt interrupt!\r\n";
//			 HAL_UART_Transmit(&huart1, (uint8_t*)message, strlen(message),0xFFFF);

#ifdef TILT_DELAY
			 HAL_Delay(TILT_DELAY);
			 timerDelay(TILT_DELAY*100);
			 recoverDelayMark();
#endif
		}
		//period measurement is not accurate for only accelerometer supports pulse drdy
#ifdef PERIOD_MEASUREMENT
		if(BSP_ACCELERO_Ready())
		{
			int tickNow = HAL_GetTick();
			int tickAcc = abs(tickNow - tasks[ACCELERO].taskTick);
			tasks[ACCELERO].taskTick = tickNow;
			tasks[ACCELERO].periodNum =  tasks[ACCELERO].periodNum+1;
			tasks[ACCELERO].periodSum = tasks[ACCELERO].periodSum+tickAcc;
		}
		if(BSP_GYRO_Ready())
		{
			int tickNow = HAL_GetTick();
			int tickAcc = abs(tickNow - tasks[GYRO].taskTick);
			tasks[GYRO].taskTick = tickNow;
			tasks[GYRO].periodNum =  tasks[GYRO].periodNum+1;
			tasks[GYRO].periodSum = tasks[GYRO].periodSum+tickAcc;
		}
#endif
	}
	else if(GPIO_Pin == WIFI_CMD_DATA_READY_Pin){
		cmdDataReady = HAL_GPIO_ReadPin(WIFI_CMD_DATA_READY_GPIO_Port, WIFI_CMD_DATA_READY_Pin);
	}
#ifdef PERIOD_MEASUREMENT
	else if(GPIO_Pin == GPIO_PIN_10)
	{
		int tickNow = HAL_GetTick();
		int tickAcc = abs(tickNow - tasks[PIEZO].taskTick);
		tasks[PIEZO].taskTick = tickNow;
		tasks[PIEZO].periodNum =  tasks[PIEZO].periodNum+1;
		tasks[PIEZO].periodSum = tasks[PIEZO].periodSum+tickAcc;
	}
	else if(GPIO_Pin == GPIO_PIN_15)
	{
		if(BSP_HSENSOR_Ready())
		{
			int tickNow = HAL_GetTick();
			int tickAcc = abs(tickNow - tasks[HUMI].taskTick);
			tasks[HUMI].taskTick = tickNow;
			tasks[HUMI].periodNum =  tasks[HUMI].periodNum+1;
			tasks[HUMI].periodSum = tasks[HUMI].periodSum+tickAcc;
		}
		if(BSP_TSENSOR_Ready())
		{
			int tickNow = HAL_GetTick();
			int tickAcc = abs(tickNow - tasks[TEMP].taskTick);
			tasks[TEMP].taskTick = tickNow;
			tasks[TEMP].periodNum =  tasks[TEMP].periodNum+1;
			tasks[TEMP].periodSum = tasks[TEMP].periodSum+tickAcc;
		}
	}
	else if(GPIO_Pin == GPIO_PIN_8)
	{
		int tickNow = HAL_GetTick();
		int tickAcc = abs(tickNow - tasks[MEGNETO].taskTick);
		tasks[MEGNETO].taskTick = tickNow;
		tasks[MEGNETO].periodNum =  tasks[MEGNETO].periodNum+1;
		tasks[MEGNETO].periodSum = tasks[MEGNETO].periodSum+tickAcc;
	}
#endif
}
void timerDelay(uint16_t time)
{
	//set period to max
	HAL_TIM_Base_Start(&TIM4_Handler);
	while(1)
	{
		unsigned int counter = __HAL_TIM_GET_COUNTER(&TIM4_Handler);
//		char message[200];
//		sprintf(message,"timer4 counter == %d;%d;%d\r\n",counter,__HAL_TIM_GET_COUNTER(&TIM2_Handler),__HAL_TIM_GET_COUNTER(&TIM1_Handler));
//		HAL_UART_Transmit(&huart1, (uint8_t*)message, strlen(message),0xFFFF);
		if(counter>time)break;
	}
	HAL_TIM_Base_Stop(&TIM4_Handler);
}
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, WIFI_RESET_Pin|WIFI_NSS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : WIFI_RESET_Pin WIFI_NSS_Pin */
  GPIO_InitStruct.Pin = WIFI_RESET_Pin|WIFI_NSS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : WIFI_CMD_DATA_READY_Pin */
  GPIO_InitStruct.Pin = WIFI_CMD_DATA_READY_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(WIFI_CMD_DATA_READY_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

}
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
  hspi3.Init.DataSize = SPI_DATASIZE_16BIT;
  hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi3.Init.NSS = SPI_NSS_SOFT;
  hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi3.Init.CRCPolynomial = 7;
  hspi3.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi3.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI3_Init 2 */

  /* USER CODE END SPI3_Init 2 */

}
void hal_Init(void)
{
    /* Pin configuration for UART. BSP_COM_Init() can do this automatically */
    __HAL_RCC_GPIOB_CLK_ENABLE();	// Enable AHB2 Bus for GPIOB
    __HAL_RCC_GPIOC_CLK_ENABLE();	// Enable AHB2 Bus for GPIOC
    __HAL_RCC_GPIOD_CLK_ENABLE();	// Enable AHB2 Bus for GPIOd
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_3);//interrupt grouping
    GPIO_InitTypeDef GPIO_InitStructUart = {0};
    GPIO_InitStructUart.Alternate = GPIO_AF7_USART1;
    GPIO_InitStructUart.Pin = GPIO_PIN_7|GPIO_PIN_6;
    GPIO_InitStructUart.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStructUart.Pull = GPIO_NOPULL;
    GPIO_InitStructUart.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStructUart);
    GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = LED2_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    //button interrupt
    GPIO_InitTypeDef GPIO_InitStructButton = {0};
    GPIO_InitStructButton.Pin = BUTTON_EXTI13_Pin;
    GPIO_InitStructButton.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStructButton.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStructButton);
    //lsm6dsl exti
    GPIO_InitTypeDef GPIO_InitStructSensor = {0};
    GPIO_InitStructSensor.Pin = GPIO_PIN_11;
    GPIO_InitStructSensor.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStructSensor.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStructSensor);
	//lps22hb exti
    GPIO_InitStructSensor.Pin = GPIO_PIN_10;
    GPIO_InitStructSensor.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStructSensor.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStructSensor);
	//hts221 exti
    GPIO_InitStructSensor.Pin = GPIO_PIN_15;
    GPIO_InitStructSensor.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStructSensor.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStructSensor);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
    HAL_NVIC_SetPriority(EXTI15_10_IRQn,2,0);
    //lis3mdl exti
	GPIO_InitStructSensor.Pin = GPIO_PIN_8;
	GPIO_InitStructSensor.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStructSensor.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStructSensor);
	HAL_NVIC_SetPriority(EXTI9_5_IRQn,3,1);
    /* Configuring UART1 */
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
      while(1);
    }

    TIM1_Handler.Instance = TIM1;
    TIM1_Handler.Init.Prescaler = 799;//us delay
    TIM1_Handler.Init.CounterMode = TIM_COUNTERMODE_UP;
    //TIM1_Handler.Init.Period=1000;//100ms
    TIM1_Handler.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;//tDTS
    TIM1_Handler.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
//	HAL_TIM_Base_Init(&TIM1_Handler);
//    HAL_TIM_Base_Start_IT(&TIM1_Handler);

    TIM2_Handler.Instance = TIM2;
    TIM2_Handler.Init.Prescaler = 7999;//10khz
    TIM2_Handler.Init.CounterMode = TIM_COUNTERMODE_UP;
    TIM2_Handler.Init.Period= 100;
    TIM2_Handler.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;//tDTS
    TIM2_Handler.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
	HAL_TIM_Base_Init(&TIM2_Handler);




	//optional hal_delay timer
	TIM4_Handler.Instance = TIM4;
	TIM4_Handler.Init.CounterMode = TIM_COUNTERMODE_UP;
	TIM4_Handler.Init.Prescaler = 7999;//1khz
	TIM4_Handler.Init.CounterMode = TIM_COUNTERMODE_UP;
	TIM4_Handler.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;//tDTS
	TIM4_Handler.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
	TIM4_Handler.Init.Period = 65535;
	HAL_TIM_Base_Init(&TIM4_Handler);
    //make sure pb14 is low
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14,GPIO_PIN_RESET);
    MX_GPIO_Init();
    MX_SPI3_Init();
    //HAL_NVIC_SetPriority(SysTick_IRQn, 4, 0U);
}
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
	//tick interrupt only has higher priority than led interrupt
	if(htim->Instance == TIM1)
	{
		__HAL_RCC_TIM1_CLK_ENABLE();
		HAL_NVIC_SetPriority(TIM1_UP_TIM16_IRQn,3,0);
		HAL_NVIC_EnableIRQ(TIM1_UP_TIM16_IRQn);
	}
	//led interrupt shouldn't block any interrupt
	else if(htim->Instance == TIM2)
	{
		__HAL_RCC_TIM2_CLK_ENABLE();
		HAL_NVIC_SetPriority(TIM2_IRQn,4,0);
		HAL_NVIC_EnableIRQ(TIM2_IRQn);
	}
	else if(htim->Instance == TIM4)
	{
		__HAL_RCC_TIM4_CLK_ENABLE();
//		HAL_NVIC_SetPriority(TIM2_IRQn,4,0);
//		HAL_NVIC_EnableIRQ(TIM2_IRQn);
	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim->Instance == TIM1)
	{
		//jumpPos();
			pos=1;
	}
	//timer2 responsible for triangle blinking
	else if(htim->Instance == TIM2)
	{
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14,GPIO_PIN_RESET);
		HAL_TIM_Base_Stop_IT(&TIM2_Handler);
	}

}

void TIM1_UP_TIM16_IRQHandler(void)
{
	HAL_TIM_IRQHandler(&TIM1_Handler);
}
void TIM2_IRQHandler(void)
{
	HAL_TIM_IRQHandler(&TIM2_Handler);
}
















