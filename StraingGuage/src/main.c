//********************************************************************
//*							Strain Test
//*==================================================================*
// Measures strain gauge analogue signal for torque test on ADC1CH5 input
// from PA5.
// Includes servo position output on timer channel
//
//====================================================================
#include <stdio.h>
#include <stdlib.h>
#include "stdio.h"
#include "math.h"
#include "lcd_stm32f0.h"
#include "stm32f0xx.h"
#include "stm32f0xx_gpio.h"
#include "stm32f0xx_adc.h"
//Global Variables
char lcdstring[16],usart_char[16];;
int sysclock,temp=39500,tmr=0;
uint32_t ADCval;
//Function Declarations
void init_GPIO(void);
void init_ADC(void);
void ADC_NVIC(void);
void init_TIM2(void);
void init_TIM14(void);
void TIM14_NVIC(void);
void init_USART1(void);
void set_servo(float degree);
void send_packet(const char *str);
//Main Loops
void main(void){
	//Init primary functions
	init_LCD();
	init_GPIO();
	//Get operating CLK freq
	RCC_ClocksTypeDef CLK;
	RCC_GetClocksFreq(&CLK);
	sysclock= (CLK.SYSCLK_Frequency)/((float)(pow(10,6)));
	//Sys clocks in MHz
	sprintf(lcdstring,"CLK:%d MHz",sysclock);
	lcd_putstring(lcdstring);
	lcd_command(LINE_TWO);
	lcd_putstring("Strain Gauge");
	GPIO_Write(GPIOB,0x00);
	while((GPIO_ReadInputData(GPIOA)&GPIO_IDR_0)){}
	//Run loop check
	lcd_command(CLEAR);
	GPIO_Write(GPIOB,0xff);
	//Init secondary functions
	init_ADC();
	init_TIM2();
	init_USART1();
	init_TIM14();
	// Start ADC
	ADC_StartOfConversion(ADC1);
	//For Loop
	for(;;){
		int cmddeg,deg;
		lcd_command(CURSOR_HOME);
		sprintf(lcdstring,"DEG:%d       ",(int)(ADCval));
		lcd_putstring(lcdstring);
		sprintf(lcdstring,"CMD:%d       ",(int)(TIM2->CCR3));
		lcd_command(LINE_TWO);
		lcd_putstring(lcdstring);
	}
}

void init_GPIO(void){
	RCC_AHBPeriphClockCmd((RCC_AHBENR_GPIOAEN|RCC_AHBENR_GPIOBEN),ENABLE);
	GPIO_InitTypeDef GPIOA_struct,GPIOB_struct,GPIOAADC1_struct,GPIOAUSART,GPIOBTIM2_struct;
	// GPIOA PA0-PA2 Inputs
	GPIOA_struct.GPIO_Mode=GPIO_Mode_IN;
	GPIOA_struct.GPIO_OType=GPIO_OType_PP;
	GPIOA_struct.GPIO_Pin=(GPIO_Pin_0|GPIO_Pin_1);
	GPIOA_struct.GPIO_PuPd=GPIO_PuPd_UP;
	GPIOA_struct.GPIO_Speed=GPIO_Speed_Level_2;
	GPIO_Init(GPIOA,&GPIOA_struct);
	// GPIOA PA5 ADC1CH5 input
	GPIOAADC1_struct.GPIO_Mode=GPIO_Mode_AN;
	GPIOAADC1_struct.GPIO_OType=GPIO_OType_PP;
	GPIOAADC1_struct.GPIO_Pin=GPIO_Pin_5;
	GPIOAADC1_struct.GPIO_PuPd=GPIO_PuPd_UP;
	GPIOAADC1_struct.GPIO_Speed=GPIO_Speed_Level_3;
	GPIO_Init(GPIOA,&GPIOAADC1_struct);
	//PA9-PA10 USART1 RX/TX
	GPIOAUSART.GPIO_Mode=GPIO_Mode_AF;
	GPIOAUSART.GPIO_OType=GPIO_OType_PP;
	GPIOAUSART.GPIO_Pin=(GPIO_Pin_9|GPIO_Pin_10);
	GPIOAUSART.GPIO_PuPd=GPIO_PuPd_NOPULL;
	GPIOAUSART.GPIO_Speed=GPIO_Speed_Level_3;
	GPIO_Init(GPIOA,&GPIOAUSART);
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource9,GPIO_AF_1);
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource10,GPIO_AF_1);
	// GPIOB PB0-PB7 Outputs Init
	GPIOB_struct.GPIO_Mode=GPIO_Mode_OUT;
	GPIOB_struct.GPIO_OType=GPIO_OType_PP;
	GPIOB_struct.GPIO_Pin=(GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7);
	GPIOB_struct.GPIO_PuPd=GPIO_PuPd_NOPULL;
	GPIOB_struct.GPIO_Speed=GPIO_Speed_Level_2;
	GPIO_Init(GPIOB,&GPIOB_struct);
	// GPIOB PB10 TIM2CH3 PWM output
	GPIOBTIM2_struct.GPIO_Mode=GPIO_Mode_AF;
	GPIOBTIM2_struct.GPIO_OType=GPIO_OType_PP;
	GPIOBTIM2_struct.GPIO_Pin=(GPIO_Pin_10);
	GPIOBTIM2_struct.GPIO_PuPd=GPIO_PuPd_NOPULL;
	GPIOBTIM2_struct.GPIO_Speed=GPIO_Speed_Level_3;
	GPIO_Init(GPIOB,&GPIOBTIM2_struct);
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource10,GPIO_AF_2);
}

void init_ADC(void){
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1,ENABLE);
	ADC_InitTypeDef ADC1_struct;
	ADC1_struct.ADC_ContinuousConvMode=DISABLE;
	ADC1_struct.ADC_DataAlign=ADC_DataAlign_Right;
	ADC1_struct.ADC_ExternalTrigConv=ADC_ExternalTrigConvEdge_None;
	ADC1_struct.ADC_ExternalTrigConvEdge=ADC_ExternalTrigConvEdge_None;
	ADC1_struct.ADC_Resolution=ADC_Resolution_10b;
	ADC1_struct.ADC_ScanDirection=ADC_ScanDirection_Upward;
	ADC_Init(ADC1,&ADC1_struct);
	ADC_ChannelConfig(ADC1,ADC_Channel_5,ADC_SampleTime_55_5Cycles);
	ADC_ITConfig(ADC1,ADC_IT_EOC,ENABLE);
	ADC_NVIC();
	ADC_Cmd(ADC1,ENABLE);
	while(ADC_GetFlagStatus(ADC1,ADC_FLAG_ADRDY)){}
}
void ADC_NVIC(void){
	NVIC_InitTypeDef ADCNVIC;
	ADCNVIC.NVIC_IRQChannel=ADC1_COMP_IRQn;
	ADCNVIC.NVIC_IRQChannelPriority=0x01;
	ADCNVIC.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&ADCNVIC);
}
void ADC1_COMP_IRQHandler(void){
	ADCval=ADC_GetConversionValue(ADC1);
	ADC_StartOfConversion(ADC1);
	ADC_ClearITPendingBit(ADC1,ADC_IT_EOC);
}
void init_TIM2(void){
	RCC_APB1PeriphClockCmd(RCC_APB1ENR_TIM2EN,ENABLE);
	TIM_TimeBaseInitTypeDef TIM2_struct;
	// Timer base struct at 330Hz intervals
	TIM2_struct.TIM_ClockDivision=0;
	TIM2_struct.TIM_CounterMode=TIM_CounterMode_Up;
	TIM2_struct.TIM_Period=48485;
	TIM2_struct.TIM_Prescaler=2;
	TIM2_struct.TIM_RepetitionCounter=0;
	TIM_TimeBaseInit(TIM2,&TIM2_struct);
	// OCcompare PWM mode for CH3
	TIM_OCInitTypeDef TIM2_OCstruct={0,};
	TIM2_OCstruct.TIM_OCMode=TIM_OCMode_PWM1;
	TIM2_OCstruct.TIM_Pulse=(int)(0);
	TIM2_OCstruct.TIM_OutputState=TIM_OutputState_Enable;
	TIM2_OCstruct.TIM_OCPolarity=TIM_OCPolarity_High;
	//Init OC3 output at 50% Duty Cycle for testing
	TIM_OC3Init(TIM2,&TIM2_OCstruct);
	TIM_Cmd(TIM2,ENABLE);
}
void init_TIM14(void){
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM14,ENABLE);
	TIM_TimeBaseInitTypeDef TIM14_struct;
	TIM14_struct.TIM_ClockDivision=0x0;
	TIM14_struct.TIM_CounterMode=TIM_CounterMode_Up;
	TIM14_struct.TIM_Period=60000;
	TIM14_struct.TIM_Prescaler=1024;
	TIM14_struct.TIM_RepetitionCounter=0;
	TIM_TimeBaseInit(TIM14,&TIM14_struct);
	TIM_ITConfig(TIM14,TIM_IT_Update,ENABLE);
	TIM14_NVIC();
	TIM_Cmd(TIM14,ENABLE);
}
void TIM14_NVIC(void){
	NVIC_InitTypeDef NVIC_TIM14;
	NVIC_TIM14.NVIC_IRQChannel=TIM14_IRQn;
	NVIC_TIM14.NVIC_IRQChannelPriority=0;
	NVIC_TIM14.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_TIM14);
}
void TIM14_IRQHandler(void){
	tmr++;
	if(temp<90){
		temp+=15;
	}
	else{
		temp = -90;
	}
	set_servo(temp);
	sprintf(usart_char,"%d\n", 240);
	send_packet(usart_char);
	sprintf(usart_char,"%d\n",tmr);
	send_packet(usart_char);
	sprintf(usart_char,"%d\n",(int) (ADCval));
	send_packet(usart_char);
	sprintf(usart_char,"%d\n",(int) (TIM2->CCR3));
	send_packet(usart_char);
	TIM_ClearITPendingBit(TIM14,TIM_IT_Update);
}
void init_USART1(void){
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);
	USART_InitTypeDef USART1_struct;
	USART1_struct.USART_BaudRate=115200;
	USART1_struct.USART_HardwareFlowControl=USART_HardwareFlowControl_None;
	USART1_struct.USART_Mode=(USART_Mode_Rx|USART_Mode_Tx);
	USART1_struct.USART_Parity=USART_Parity_No;
	USART1_struct.USART_StopBits=USART_StopBits_2;
	USART1_struct.USART_WordLength=USART_WordLength_8b;
	USART_Init(USART1,&USART1_struct);
	USART_Cmd(USART1,ENABLE);
}
void set_servo(float degree){
	if(degree<0){
		TIM_SetCompare3(TIM2,(int) (10600*(degree+216.5)/90));
	}
	if(degree>=0){
		TIM_SetCompare3(TIM2,(int) (13000*(degree+176.53)/90));
	}

}
void send_packet(const char *str){
	while(*str){
		while(USART_GetFlagStatus(USART1,USART_FLAG_TXE)==0);
		USART_SendData(USART1,*str++);
	}
}
// ----------------------------------------------------------------------------