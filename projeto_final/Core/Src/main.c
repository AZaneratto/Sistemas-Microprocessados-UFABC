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
#define DT_D1 200                       // delay do LED D1
#define DT_D2 200                     // delay do LED D2
#define DT_D3 200                      // delay do LED D3
#define DT_D4 200                     // delay do LED D4
#define DT_D5 200                     // delay do LED D4
#define DT_D6 200                      // delay do LED D4
#define quem_pisca_max4 4
#define quem_pisca_max6 6

#define PER_PB7 3000                   // tempo do ciclo (ms) do led
#define PWM_ARR 200                    // resolução (num steps) do PWM
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DT_VARRE  5             // inc varredura a cada 5 ms (~200 Hz)
#define DIGITO_APAGADO 0x10    // kte valor p/ apagar um dígito no display
#define CMP_MODO 0
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

TIM_HandleTypeDef htim4;

/* USER CODE BEGIN PV */
volatile uint32_t lb12;
volatile uint32_t lb13;
volatile uint32_t lb14;
volatile uint32_t lb15;
volatile uint32_t lpa1;
volatile uint32_t lb7;
volatile uint32_t lpa2 = 0;
               // var global: valor lido no ADC
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM4_Init(void);
static void MX_ADC1_Init(void);
static void MX_NVIC_Init(void);
/* USER CODE BEGIN PFP */
// funcoes do arquivo stm32f1xx_it.c
void set_modo_oper(int);             // seta modo_oper (no stm32f1xx_it.c)
int get_modo_oper(void);
int get_modo_oper_estado(void);//	 obtém modo_oper (stm32f1xx_it.c)
// funcoes do arquivo prat_05_funcoes.c
void reset_pin_GPIOs (void);         // reset pinos da SPI
void serializar(int ser_data);       // prot fn serializa dados p/ 74HC595
int16_t conv_7_seg(int NumHex);      // prot fn conv valor --> 7-seg
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int16_t val_adc = 0;                 // var global: valor lido no ADC
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	  // vars e flags de controle do programa no superloop...
	  int decSeg = 0,                      // ini decimo de seg
		  uniSeg = 0,                      // ini unidade de seg
		  dezSeg = 0,                      // ini dezena de seg
		  uniMin = 0,
	  	  dezMin = 0;
	  int16_t val7seg = 0x00FF,            // inicia 7-seg com 0xF (tudo apagado)
	  serial_data = 0x01FF;            // dado a serializar (dig | val7seg)

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

  EXTI_HandleTypeDef hexti_1 = {};
  hexti_1.Line = EXTI_LINE_1;

  // inicializa a SPI (pinos 6,9,10 da GPIOB)
  reset_pin_GPIOs ();
  // var de estado que controla a varredura (qual display é mostrado)


    // garantir que PC13 começa desligado
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

    // para iniciar vars de controle para máquinas de estados dos leds
      static enum {INI_D1, LIG_D1, DSLG_D1} sttD1=INI_D1; // var estados de D1
      static enum {INI_D2, LIG_D2, DSLG_D2} sttD2=INI_D2; // var estados de D2
      static enum {INI_D3, LIG_D3, DSLG_D3} sttD3=INI_D3; // var estados de D3
      static enum {INI_D4, LIG_D4, DSLG_D4} sttD4=INI_D4; // var estados de D4
      static enum {INI_D5, LIG_D5, DSLG_D5} sttD5=INI_D5; // var estados de D5
      static enum {INI_D6, LIG_D6, DSLG_D6} sttD6=INI_D6; // var estados de D6
      static enum {INI_D7, INC_D7, DEC_D7}  sttD7=INI_D7; // var estados de D7
      static enum {INI_PA1, LIG_PA1, DSLG_PA1} sttPA1=INI_PA1; // var estados de PA1


    // para controlar vars tempos de entrada na rotina ON/OFF de cada LED
      uint32_t tin_D1=0, tin_D2=0, tin_D3=0, tin_D4=0,tin_D5=0,tin_D6=0,tin_D7, tin_PA1=0, quem_pisca = 1;;
      uint32_t  dt_PA1=80;
      uint16_t dc_D7 = 0;
      lpa2 = 0;
      int dt_D7 = PER_PB7/(2*PWM_ARR);
      int modo = get_modo_oper_estado();          // inicia modo como 0

      HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2); // inicializa PWM no T4C2
      TIM4->CCR2 = dc_D7;                // inicia comparador PWM c/ ARR-1
  	// estrutura de dados para programar pedido de interrupção por software

    /* USER CODE END 2 */


  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

	  /* USER CODE BEGIN 3 */


	  		modo = get_modo_oper_estado();
	  		switch(modo)
	  		{
	  		  case 0:   // mo modo '0' ajusta dt_xx dos LEDs
	  			if(quem_pisca >quem_pisca_max4) quem_pisca =1;
	  			  if (quem_pisca==1)
	  			  	  {
	  				switch (sttD1)
	  				{
	  				  case INI_D1:                 // vai iniciar a máquina de estado
	  					tin_D1 = HAL_GetTick();    // tempo inicial que iniciou a tarefa
	  					sttD1 = LIG_D1;            // prox estado da máquina
	  					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET); // desl o LED

	  					break;
	  				  case LIG_D1:                 // estado para ligar o LED
	  					if((HAL_GetTick()-tin_D1)>DT_D1) // se HAL_GetTick()-tin_D1 > DT_D1
	  					{
	  					  tin_D1 = HAL_GetTick();  // guarda tempo p/ prox mudança estado
	  					  sttD1 = DSLG_D1;         // muda o prox estado da máquina
	  					  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET); // ligaLED
	  					  lb12 = 0;
	  					}
	  					break;
	  				  case DSLG_D1:                // estado para desligar o LED
	  					if((HAL_GetTick()-tin_D1)>DT_D1) // se HAL_GetTick()-tin_D1 > DT_D1
	  					{
	  					  tin_D1 = HAL_GetTick();  // guarda tempo p/ prox mudança estado
	  					  sttD1 = LIG_D1;          // muda o prox estado da máquina
	  					  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET); // desl LED
	  					  lb12 = 1;
	  					  quem_pisca ++;
	  					}
	  					break;
	  				}}



	  			if (quem_pisca==2)
	  			{
	  				switch (sttD2)
	  				{
	  				  case INI_D2:                 // vai iniciar a máquina de estado
	  					tin_D2 = HAL_GetTick();    // tempo inicial que iniciou a tarefa
	  					sttD2 = LIG_D2;            // prox estado da máquina
	  					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET); // desl o LED
	  					break;
	  				  case LIG_D2:                 // estado para ligar o LED
	  					if((HAL_GetTick()-tin_D2)>DT_D2) // se HAL_GetTick()-tin_D2 > DT_D2
	  					{
	  					  tin_D2 = HAL_GetTick();  // guarda tempo p/ prox mudança estado
	  					  sttD2 = DSLG_D2;         // muda o prox estado da máquina
	  					  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET); // ligaLED
	  					  lb13 = 0;
	  					}
	  					break;
	  				  case DSLG_D2:                // estado para desligar o LED
	  					if((HAL_GetTick()-tin_D2)>DT_D2) // se HAL_GetTick()-tin_D2 > DT_D2
	  					{
	  					  tin_D2 = HAL_GetTick();  // guarda tempo p/ prox mudança estado
	  					  sttD2 = LIG_D2;          // muda o prox estado da máquina
	  					  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET); // desl LED
	  					  lb13 = 1;
	  					  quem_pisca ++;
	  					}
	  					break;
	  				}}



	  			if (quem_pisca==3)
	  			{
	  				switch (sttD3)
	  				{
	  				  case INI_D3:                 // vai iniciar a máquina de estado
	  					tin_D3 = HAL_GetTick();    // tempo inicial que iniciou a tarefa
	  					sttD3 = LIG_D3;            // prox estado da máquina
	  					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET); // desl o LED
	  					break;
	  				  case LIG_D3:                 // estado para ligar o LED
	  					if((HAL_GetTick()-tin_D3)>DT_D3) // se HAL_GetTick()-tin_D3 > DT_D3
	  					{
	  					  tin_D3 = HAL_GetTick();  // guarda tempo p/ prox mudança estado
	  					  sttD3 = DSLG_D3;         // muda o prox estado da máquina
	  					  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET); // ligaLED
	  					  lb14 = 0;
	  					}
	  					break;
	  				  case DSLG_D3:                // estado para desligar o LED
	  					if((HAL_GetTick()-tin_D3)>DT_D3) // se HAL_GetTick()-tin_D3 > DT_D3
	  					{
	  					  tin_D3 = HAL_GetTick();  // guarda tempo p/ prox mudança estado
	  					  sttD3 = LIG_D3;          // muda o prox estado da máquina
	  					  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET); // desl LED
	  					  lb14 = 1;
	  					  quem_pisca ++;
	  					}
	  					break;
	  				}}



	  			if (quem_pisca==4)
	  			{

	  				switch (sttD4)
	  				{
	  				  case INI_D4:                 // vai iniciar a máquina de estado
	  					tin_D4 = HAL_GetTick();    // tempo inicial que iniciou a tarefa
	  					sttD4 = LIG_D4;            // prox estado da máquina
	  					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET); // desl o LED
	  					break;
	  				  case LIG_D4:                 // estado para ligar o LED
	  					if((HAL_GetTick()-tin_D4)>DT_D4) // se HAL_GetTick()-tin_D4 > DT_D4
	  					{
	  					  tin_D4 = HAL_GetTick();  // guarda tempo p/ prox mudança estado
	  					  sttD4 = DSLG_D4;         // muda o prox estado da máquina
	  					  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET); // ligaLED
	  					  lb15 = 0;
	  					}
	  					break;
	  				  case DSLG_D4:                // estado para desligar o LED
	  					if((HAL_GetTick()-tin_D4)>DT_D4) // se HAL_GetTick()-tin_D4 > DT_D4
	  					{
	  					  tin_D4 = HAL_GetTick();  // guarda tempo p/ prox mudança estado
	  					  sttD4 = LIG_D4;          // muda o prox estado da máquina
	  					  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET); // desl LED
	  					  lb15 = 1;
	  					  quem_pisca ++;
	  					}
	  					break;
	  				}}
	  				break;


	  		  	  case 1:

	  				if(quem_pisca >  quem_pisca_max6) quem_pisca = 1;
	  					if(quem_pisca == 1){

	  						switch (sttD1){
	  							  case INI_D1:                     // vai iniciar a máquina de estado
	  								tin_D1 = HAL_GetTick();        // tempo inicial que iniciou a tarefa
	  								sttD1 = LIG_D1;                // prox estado da máquina
	  								HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET); // desliga o LED
	  								break;
	  							  case LIG_D1:          // estado para ligar o LED
	  								if((HAL_GetTick()-tin_D1)>DT_D1) // se HAL_GetTick()-tin_D1 > dt_D1
	  								{
	  								  tin_D1 = HAL_GetTick();	   // guarda tempo p/ prox mudança estado
	  								  sttD1 = DSLG_D1;             // muda o prox estado da máquina
	  								  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET); // liga o LED
	  								  lb12 = 0;
	  								}
	  								break;
	  							  case DSLG_D1:                    // estado para desligar o LED
	  								if((HAL_GetTick()-tin_D1)>DT_D1) // se HAL_GetTick()-tin_D1 > dt_D1
	  								{
	  								  tin_D1 = HAL_GetTick();	   // guarda tempo p/ prox mudança estado
	  								  sttD1 = LIG_D1;              // muda o prox estado da máquina
	  								  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET); // desliga o LED
	  								  lb12 = 1;
	  								  quem_pisca++;
	  								}
	  								break;
	  							};
	  							}


	  				if(quem_pisca ==2){
	  					switch (sttD2)
	  						{

	  						  case INI_D2:                     // vai iniciar a máquina de estado
	  							tin_D2 = HAL_GetTick();        // tempo inicial que iniciou a tarefa
	  							sttD2 = LIG_D2;                // prox estado da máquina
	  							HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET); // desliga o LED
	  							break;
	  						  case LIG_D2:          // estado para ligar o LED
	  							if((HAL_GetTick()-tin_D2)>DT_D2) // se HAL_GetTick()-tin_D2 > dt_D2
	  							{
	  							  tin_D2 = HAL_GetTick();	   // guarda tempo p/ prox mudança estado
	  							  sttD2 = DSLG_D2;             // muda o prox estado da máquina
	  							  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET); // liga o LED
	  							  lb13 = 0;
	  							}
	  							break;
	  						  case DSLG_D2:                    // estado para desligar o LED
	  							if((HAL_GetTick()-tin_D2)>DT_D2) // se HAL_GetTick()-tin_D2 > dt_D2
	  							{
	  							  tin_D2 = HAL_GetTick();	   // guarda tempo p/ prox mudança estado
	  							  sttD2 = LIG_D2;              // muda o prox estado da máquina
	  							  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET); // desliga o LED
	  							  lb13 = 1;
	  							  quem_pisca++;
	  							}
	  							break;
	  						};
	  						}


	  				if(quem_pisca ==3){
	  					switch (sttD3)

	  						{
	  						  case INI_D3:                     // vai iniciar a máquina de estado
	  							tin_D3 = HAL_GetTick();        // tempo inicial que iniciou a tarefa
	  							sttD3 = LIG_D3;                // prox estado da máquina
	  							HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET); // desliga o LED
	  							break;
	  						  case LIG_D3:          // estado para ligar o LED
	  							if((HAL_GetTick()-tin_D3)>DT_D3) // se HAL_GetTick()-tin_D3 > dt_D3
	  							{
	  							  tin_D3 = HAL_GetTick();	   // guarda tempo p/ prox mudança estado
	  							  sttD3 = DSLG_D3;             // muda o prox estado da máquina
	  							  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET); // liga o LED
	  							  lb14 = 0;
	  							}
	  							break;
	  						  case DSLG_D3:                    // estado para desligar o LED
	  							if((HAL_GetTick()-tin_D3)>DT_D3) // se HAL_GetTick()-tin_D3 > dt_D3
	  							{
	  							  tin_D3 = HAL_GetTick();	   // guarda tempo p/ prox mudança estado
	  							  sttD3 = LIG_D3;              // muda o prox estado da máquina
	  							  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET); // desliga o LED
	  							  lb14 = 1;
	  							  quem_pisca++;
	  							}
	  							break;
	  						};}


	  				if(quem_pisca ==4){
	  				 switch (sttD4)
	  				{


	  						  case INI_D4:                     // vai iniciar a máquina de estado
	  							tin_D4 = HAL_GetTick();        // tempo inicial que iniciou a tarefa
	  							sttD4 = LIG_D4;                // prox estado da máquina
	  							HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET); // desliga o LED
	  							break;
	  						  case LIG_D4:          // estado para ligar o LED
	  							if((HAL_GetTick()-tin_D4)>DT_D4) // se HAL_GetTick()-tin_D4 > dt_D4
	  							{
	  							  tin_D4 = HAL_GetTick();	   // guarda tempo p/ prox mudança estado
	  							  sttD4 = DSLG_D4;             // muda o prox estado da máquina
	  							  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET); // liga o LED
	  							  lb15 = 0;
	  							}
	  							break;
	  						  case DSLG_D4:                    // estado para desligar o LED
	  							if((HAL_GetTick()-tin_D4)>DT_D4) // se HAL_GetTick()-tin_D4 > dt_D4
	  							{
	  							  tin_D4 = HAL_GetTick();	   // guarda tempo p/ prox mudança estado
	  							  sttD4 = LIG_D4;              // muda o prox estado da máquina
	  							  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET); // desliga o LED
	  							  lb15 = 1;
	  							  quem_pisca++;
	  							}
	  							break;
	  						}}
	  				if(quem_pisca == 5){

	  						switch (sttD5){
	  							  case INI_D5:                     // vai iniciar a máquina de estado
	  								tin_D5 = HAL_GetTick();        // tempo inicial que iniciou a tarefa
	  								sttD5 = LIG_D5;                // prox estado da máquina
	  								HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET); // desliga o LED
	  								break;
	  							  case LIG_D5:          // estado para ligar o LED
	  								if((HAL_GetTick()-tin_D5)>DT_D5) // se HAL_GetTick()-tin_D1 > dt_D1
	  								{
	  								  tin_D5 = HAL_GetTick();	   // guarda tempo p/ prox mudança estado
	  								  sttD5 = DSLG_D5;             // muda o prox estado da máquina
	  								  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET); // liga o LED
	  								  lb14 = 0;
	  								}
	  								break;
	  							  case DSLG_D5:                    // estado para desligar o LED
	  								if((HAL_GetTick()-tin_D5)>DT_D5) // se HAL_GetTick()-tin_D1 > dt_D1
	  								{
	  								  tin_D5 = HAL_GetTick();	   // guarda tempo p/ prox mudança estado
	  								  sttD5 = LIG_D5;              // muda o prox estado da máquina
	  								  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET); // desliga o LED
	  								  lb14 = 1;
	  								  quem_pisca++;
	  								}
	  								break;
	  							};
	  							}
	  				if(quem_pisca == 6){

	  						switch (sttD6){
	  							  case INI_D6:                     // vai iniciar a máquina de estado
	  								tin_D6 = HAL_GetTick();        // tempo inicial que iniciou a tarefa
	  								sttD6 = LIG_D6;                // prox estado da máquina
	  								HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET); // desliga o LED
	  								break;
	  							  case LIG_D6:          // estado para ligar o LED
	  								if((HAL_GetTick()-tin_D6)>DT_D6) // se HAL_GetTick()-tin_D1 > dt_D1
	  								{
	  								  tin_D6 = HAL_GetTick();	   // guarda tempo p/ prox mudança estado
	  								  sttD6 = DSLG_D6;             // muda o prox estado da máquina
	  								  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET); // liga o LED
	  								  lb13 = 0;
	  								}
	  								break;
	  							  case DSLG_D6:                    // estado para desligar o LED
	  								if((HAL_GetTick()-tin_D6)>DT_D6) // se HAL_GetTick()-tin_D1 > dt_D1
	  								{
	  								  tin_D6 = HAL_GetTick();	   // guarda tempo p/ prox mudança estado
	  								  sttD6 = LIG_D6;              // muda o prox estado da máquina
	  								  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET); // desliga o LED
	  								  lb13 = 1;
	  								  quem_pisca++;
	  								}
	  								break;
	  							};
	  							}


	  			break;


	  		}
	  		switch (sttD7)
	  				{
	  				  case INI_D7:                 // vai iniciar a máquina de estado
	  					tin_D7 = HAL_GetTick();    // tempo em que iniciou a tarefa
	  					sttD7 = INC_D7;            // prox estado da máquina
	  			        TIM4->CCR2 = 0;            // inicia val dc_D7 = 0
	  					break;
	  				  case INC_D7:                 // estado para incrementar PWM
	  					if((HAL_GetTick()-tin_D7)>dt_D7)  // se o milis-tin_D7 > dt_PD7
	  					{
	  					  tin_D7 = HAL_GetTick();  // tempo p/ prox mudança dc_D7
	  					  ++ dc_D7;                // incrementa dc_D7
	  					  if (dc_D7>=PWM_ARR) sttD7=DEC_D7;  // muda maq est p/ DEC_D7
	  			          TIM4->CCR2 = dc_D7;      // set comparador com valor dc_D7
	  			          lb7 = dc_D7;
	  					}
	  					break;
	  				  case DEC_D7:                 // estado para decrementar o PWM
	  					if((HAL_GetTick()-tin_D7)>dt_D7)  // se o milis-tin_D7 > dt_PD7
	  					{
	  					  tin_D7 = HAL_GetTick();  // tempo p/ prox mudança dc_D7
	  					  -- dc_D7;                // decrementa dc_D7
	  					  if (dc_D7 <= 0) sttD7=INC_D7;  // muda maq est p/ INC_D7
	  			          TIM4->CCR2 = dc_D7;      // set comparador com valor dc_D7
	  			          lb7 = dc_D7;
	  					}
	  					break;
	  				};




#if CMP_MODO==0                        // DIRETIVA PARA O COMPILADOR !!!

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
				sttPA1 = DSLG_PA1;
				quem_pisca=1;// muda o prox estado da máquina
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
	  // -- final da tarefa 5A ...
#else                                  // DIRETIVA PARA O COMPILADOR !!!
		  // --- tarefa #5B - MODO = 1   (com interrupção gerada pelo modo)
		  switch (sttPA1)
		  {
		  case INI_PA1:                    // vai iniciar a máquina de estado
			sttPA1 = LIG_PA1;              // prox estado da máquina
			break;
		  case LIG_PA1:                    // estado para inc modo_oper
			if((HAL_GetTick()-tin_PA1)>dt_PA1) // se HAL_GetTick()-tin_D4 > dt_D4
			{
			  tin_PA1 = HAL_GetTick();     // guarda tempo p/ prox mudança estado
			  if (lpa1==1)
			  {
				sttPA1 = DSLG_PA1;         // muda o prox estado da máquina
				HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); // ligar led
				set_modo_oper();          // set do modo atual
			  }
			}
			break;
		  case DSLG_PA1:                   // estado para desligar o LED
			if((HAL_GetTick()-tin_PA1)>dt_PA1) // se HAL_GetTick()-tin_D4 > dt_D4
			{
			  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); // desligar led
			  tin_PA1 = HAL_GetTick();     // guarda tempo p/ prox mudança estado
			  sttPA1 = LIG_PA1;            // muda o prox estado da máquina
			  lpa1=0;                      // volta lpa1=0 para desativar
			}
			break;
		  };

#endif                                 // FIM DA DIRETIVA PARA O COMPILADOR !!!









  	  }
}
// -- fim do loop infinito
	    /* USER CODE END 3 */


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
  /* EXTI1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);
  /* EXTI2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);
  /* EXTI3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(EXTI3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);
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
  htim4.Init.Prescaler = 179;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 199;
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
