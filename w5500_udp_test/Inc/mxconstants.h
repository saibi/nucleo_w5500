/**
  ******************************************************************************
  * File Name          : mxconstants.h
  * Description        : This file contains the common defines of the application
  ******************************************************************************
  *
  * COPYRIGHT(c) 2016 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/

/* USER CODE BEGIN Includes */
void MX_GPIO_Init(void);
void MX_SPI1_Init(void);
void MX_USART1_UART_Init(void);

int wizSystemInit(void);
void W5500HardwareReset(void);
void W5500WriteByte(unsigned char txByte);
unsigned char W5500ReadByte(void);
void W5500Select(void);
void W5500DeSelect(void);
int isLinked(void);
void print_network_information(void);

void MX_USART1_UART_Init(void);

void MX_SPI1_Init(void);

/* USER CODE END Includes */

/* Private define ------------------------------------------------------------*/

#define	W5500_RESET_Pin		GPIO_PIN_5
#define	W5500_RESET_Port    	GPIOC
#define 	SPI1_CS_Pin				GPIO_PIN_4
#define 	SPI1_CS_GPIO_Port   	GPIOA
#define 	LED0_Pin                    	GPIO_PIN_13
#define 	LED0_GPIO_Port         	GPIOB
#define 	LED1_Pin                    	GPIO_PIN_14
#define 	LED1_GPIO_Port      	GPIOB
#define 	LED2_Pin            			GPIO_PIN_15
#define 	LED2_GPIO_Port      	GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

/**
  * @}
  */ 

/**
  * @}
*/ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
