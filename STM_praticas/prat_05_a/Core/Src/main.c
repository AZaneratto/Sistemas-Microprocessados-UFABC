/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "mx_prat_05_funcoes.h"   // header do arqv das funcoes do programa
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DT_VARRE  5             // inc varredura a cada 5 ms (~200 Hz)
#define DIGITO_APAGADO 0x10     // kte valor p/ apagar um dígito no display
#define PR 80                	// delta t (ms) para piscar o led
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

TIM_HandleTypeDef htim4;

/* USER CODE BEGIN PV */
volatile uint32_t lpa1 = 0x0;
volatile uint32_t miliVolt = 0x0;  // val adc convertido p/ miliVolts
volatile int16_t val_adc = 0;                 // var global: valor lido no ADC

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM4_Init(void);
static void MX_ADC1_Init(void);
static void MX_NVIC_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */
// funcoes do arquivo stm32f1xx_it.c
void set_modo_oper(int);             // seta modo_oper (no stm32f1xx_it.c)
int get_modo_oper(void);             // obtém modo_oper (stm32f1xx_it.c)
// funcoes do arquivo prat_05_funcoes.c
void reset_pin_GPIOs (void);         // reset pinos da SPI
void serializar(int ser_data);       // prot fn serializa dados p/ 74HC595
int16_t conv_7_seg(int NumHex);      // prot fn conv valor --> 7-seg

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  // vars e flags de controle do programa no superloop...
  int milADC = 0,                      // ini milesimo
	  cenADC = 0,                      // ini unidade
	  dezADC = 0,                      // ini dezena
	  uniADC = 0;                      // ini unidade
  int16_t val7seg = 0x00FF,            // inicia 7-seg com 0xF (tudo apagado)
	  serial_data = 0x01FF;            // dado a serializar (dig | val7seg)
  uint32_t tIN_varre = 0;              // registra tempo última varredura

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
  MX_TIM4_Init();
  MX_ADC1_Init();

  /* Initialize interrupts */
  MX_NVIC_Init();

  /* USER CODE BEGIN 2 */
  // inicializa a SPI (pinos 6,9,10 da GPIOB)
  reset_pin_GPIOs ();

  // para controlar vars tempos de entrada na rotina ON/OFF de cada LED
  uint32_t tin_PA1=0, dt_PA1=PR;

  // var de estado que controla a varredura (qual display é mostrado)
  static enum {DIG_UNI, DIG_DEC, DIG_CENS, DIG_MILS} sttVARRE=DIG_UNI;
  static enum {INI_PA1, LIG_PA1, DSLG_PA1} sttPA1=INI_PA1; // var estados de PA1

	// estrutura de dados para programar pedido de interrupção por software
EXTI_HandleTypeDef hexti_1 = {};
hexti_1.Line = EXTI_LINE_1;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

	  /* USER CODE BEGIN 3 */
	  while (1)
	  {
	    /* USER CODE END WHILE */
		  // tarefa #1: se (modo_oper=1) faz uma conversão ADC
		  	if (get_modo_oper()==1)
		  	{
		  	  // dispara por software uma conversão ADC
		  	  set_modo_oper(0);            // muda modo_oper p/ 0
		  	  HAL_ADC_Start_IT(&hadc1);    // dispara ADC p/ conversão por IRQ
		  	}

		  //tarefa #2: depois do IRQ ADC, converte para mVs (decimal, p/ 7-seg)
		      if (get_modo_oper()==2)      // entra qdo valor val_adc atualizado
		      {
		        // converter o valor lido em decimais p/ display
			    set_modo_oper(0);          // zera var modo_oper
		    	miliVolt = val_adc*3300/4095;
		        uniADC = miliVolt/1000;
		        dezADC = (miliVolt-(uniADC*1000))/100;
		        cenADC = (miliVolt-(uniADC*1000)-(dezADC*100))/10;
		        milADC = miliVolt-(uniADC*1000)-(dezADC*100)-(cenADC*10);
		      }

		  // tarefa #3: qdo milis() > DELAY_VARRE ms, desde a última mudança
		    if ((HAL_GetTick()-tIN_varre) > DT_VARRE)    // se ++0,1s atualiza o display
		    {
		  	tIN_varre = HAL_GetTick();         // salva tIN p/ prox tempo varredura
		  	switch(sttVARRE)                   // teste e escolha de qual DIG vai varrer
		  	{
		  	  case DIG_MILS:
		  	  {
		  		sttVARRE = DIG_CENS;           // ajusta p/ prox digito
		  		serial_data = 0x0008;          // display #1
		  		val7seg = conv_7_seg(milADC);
		  		break;
		  	  }
		  	  case DIG_CENS:
		  	  {
		  		sttVARRE = DIG_DEC;            // ajusta p/ prox digito
		  		serial_data = 0x00004;         // display #2
		  		if(cenADC>0 || dezADC>0 || uniADC>0)
		  		{
		  		  val7seg = conv_7_seg(cenADC);
		  		} else {
		  		  val7seg = conv_7_seg(DIGITO_APAGADO);
		  		}
		  		break;
		  	  }
		  	  case DIG_DEC:
		  	  {
		  		sttVARRE = DIG_UNI;            // ajusta p/ prox digito
		  		serial_data = 0x0002;          // display #3
		  		if(dezADC>0 || uniADC>0)
		  		{
		  		  val7seg = conv_7_seg(dezADC);
		  		} else {
		  		  val7seg = conv_7_seg(DIGITO_APAGADO);
		  		}
		  		break;
		  	  }
		  	  case DIG_UNI:
		  	  {
		  		sttVARRE = DIG_MILS;           // ajusta p/ prox digito
		  		serial_data = 0x0001;          // display #3
		  		if(uniADC>0)
		  		{
		  		  val7seg = conv_7_seg(uniADC);
		  		  val7seg &=0x7FFF;            // liga o ponto decimal
		  		} else {
		  		  val7seg = conv_7_seg(DIGITO_APAGADO);
		  		}
		  		break;
		  	  }
		  	}  // fim case
		    tIN_varre = HAL_GetTick();           // tmp atual em que fez essa varredura
		    serial_data |= val7seg;              // OR com val7seg = dado a serializar
		    serializar(serial_data);             // serializa dado p/74HC595 (shift reg)
		    }  // -- fim da tarefa #3 - varredura do display

		 // --- tarefa #4 - controlar IRQ, com interrupção gerada por software
		  switch (sttPA1)
		  {
		  case INI_PA1:                    // vai iniciar a máquina de estado
			sttPA1 = LIG_PA1;              // prox estado da máquina
			break;
		  case LIG_PA1:                    // estado p/ gerar INT_EXT_1
			if((HAL_GetTick()-tin_PA1)>dt_PA1) // se HAL_GetTick()-tin_PA1 > dt_PA1
			{
			  tin_PA1 = HAL_GetTick();     // guarda tempo p/ prox mudança estado
			  if (lpa1 == 1) {
				sttPA1 = DSLG_PA1;             // muda o prox estado da máquina
				//HAL_EXTI_ClearPending(&hexti_1, EXTI_TRIGGER_FALLING);
				HAL_EXTI_GenerateSWI(&hexti_1);  // pedido de int por software
				HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); // ligar led
				lpa1=0;                    // volta lpa1 para desativada
			  }
			}
			break;
		  case DSLG_PA1:                   // estado para desligar o LED
			if((HAL_GetTick()-tin_PA1)>dt_PA1) // se HAL_GetTick()-tin_PA1 > dt_PA1
			{
			  tin_PA1 = HAL_GetTick();     // guarda tempo p/ prox mudança estado
			  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); // ligar led
			  sttPA1 = LIG_PA1;            // muda o prox estado da máquina
			}
			break;
		  };
	  }
	  /* USER CODE END 3 */
}}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief NVIC Configuration.
  * @retval None
  */
static void MX_NVIC_Init(void)
{
  /* EXTI3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(EXTI3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);
  /* EXTI2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);
  /* EXTI1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);
  /* ADC1_2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(ADC1_2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(ADC1_2_IRQn);
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */
  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 359;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 99;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */
  HAL_TIM_MspPostInit(&htim4);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14
                          |GPIO_PIN_15|GPIO_PIN_6|GPIO_PIN_9, GPIO_PIN_RESET);

  /*Configure GPIO pins : PA1 PA2 PA3 */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB10 PB12 PB13 PB14
                           PB15 PB6 PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14
                          |GPIO_PIN_15|GPIO_PIN_6|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */
// fn que atende ao callback da ISR do conversor ADC1
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
  {
    if(hadc->Instance == ADC1)
    {
      val_adc = HAL_ADC_GetValue(&hadc1);  // capta valor adc
      set_modo_oper(2); ;                  // alterou valor lido
    }
  }

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

#ifdef  USE_FULL_ASSERT
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
