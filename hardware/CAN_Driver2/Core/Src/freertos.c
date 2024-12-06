/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "can.h"
#include "JY901.h"
#include <string.h>
#include "math_process.h"
#include "pid.h"
#include "tim.h"
#include "usart.h"
#include "math.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */


/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */




#define MOTOR_SPEED_PID_KP 0.0002f //1:12 4:6
#define MOTOR_SPEED_PID_KI 0.005f  //1:0.1 4:0.00499999989
#define MOTOR_SPEED_PID_KD 0.0f  //1:0.2 4:0.100000001

#define MOTOR_SPEED_PID_MAX_OUT  20.0f
#define MOTOR_SPEED_PID_MAX_IOUT 20.0f

#define MOTOR_ANGLE_PID_KP 0.0002f//3:10 2:10
#define MOTOR_ANGLE_PID_KI 0.005f//3:0.05 2:0
#define MOTOR_ANGLE_PID_KD 0.00f//3:0.01 2:0.1

#define MOTOR_ANGLE_PID_MAX_OUT  20.0f
#define MOTOR_ANGLE_PID_MAX_IOUT 10.0f

#define PWM_MID_MAX 2000-150  //0x95 - 0x99
#define PWM_MID_MIN 2000-152

#define PWM_HIGH_MAX 1270 //
#define PWM_HIGH_MIN 1270 //

#define PWM_LOW_MAX 710 //
#define PWM_LOW_MIN 710 //

uint8_t data[8]={5,2,1,1,3,1,4,1};

uint8_t reset_Z[3] = {0xFF,0xAA,0x52};//����ָ��

uint8_t RxBuffer[1]; //������1����
uint8_t RxBuffer2[1]; //������2����
void GyroSerialData(unsigned char ucData,GYRO *gyro);
void GyroSerialData2(unsigned char ucData,GYRO *gyro);
uint8_t Can_TxMessage(uint8_t ide,uint32_t id,uint8_t len,uint8_t *data);
void MS1_Control(float speed,float angle);
void MS2_Control(float speed,float angle);
void Can_Pack_Send_msg(float angle1,float angle2);
void Can_unpack_msg(uint8_t* buf,ms_ctrl_t *ms);
GYRO gyro1; //������1�ṹ��
GYRO gyro2; //������2�ṹ��

float fb_angle1; //feedback angle1
float fb_angle2; //feedback angle2

float ori_angle1; // original angle1
float ori_angle2; // original angle2

ms_ctrl_t ms1; //1�Ŷ���
ms_ctrl_t ms2; //2�Ŷ���

ms_ctrl_t test1;
ms_ctrl_t test2;

uint8_t reset_flag1= 0;
uint8_t reset_flag2= 0 ;

uint8_t CAN_check_count = 0;
uint8_t checkcount = 0;
uint8_t STATE = 1;

PidTypeDef motorpid_angle1;
PidTypeDef motorpid_angle2;

const static fp32 motor_angle1_pid[3] = { MOTOR_SPEED_PID_KP, MOTOR_SPEED_PID_KI ,MOTOR_SPEED_PID_KD};

const static fp32 motor_angle2_pid[3] = { MOTOR_ANGLE_PID_KP, MOTOR_ANGLE_PID_KI ,MOTOR_ANGLE_PID_KD};

void AngleSerialData2(char Data,float *angle);
void AngleSerialData(char Data,float *angle);

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
osThreadId Control_TaskHandle;
osThreadId CAN_TaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void control_task(void const * argument);
void can_task(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of Control_Task */
  osThreadDef(Control_Task, control_task, osPriorityHigh, 0, 128);
  Control_TaskHandle = osThreadCreate(osThread(Control_Task), NULL);

  /* definition and creation of CAN_Task */
  osThreadDef(CAN_Task, can_task, osPriorityHigh, 0, 128);
  CAN_TaskHandle = osThreadCreate(osThread(CAN_Task), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_control_task */
/**
  * @brief  Function implementing the Control_Task thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_control_task */
void control_task(void const * argument)
{
  /* USER CODE BEGIN control_task */
	PID_Init(&motorpid_angle1,PID_POSITION,motor_angle1_pid,MOTOR_ANGLE_PID_MAX_OUT,MOTOR_ANGLE_PID_MAX_IOUT);
	PID_Init(&motorpid_angle2,PID_POSITION,motor_angle2_pid,MOTOR_SPEED_PID_MAX_OUT,MOTOR_SPEED_PID_MAX_IOUT);
	HAL_TIM_Base_Start(&htim1);
  HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_3);
  HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_4);
	HAL_TIM_Base_Start(&htim4);
  HAL_TIM_PWM_Start(&htim4,TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim4,TIM_CHANNEL_2);
	osDelay(1000);


  /* Infinite loop */
  for(;;)
  {
				
		if(STATE==1)
		{
			MS1_Control(ms1.vx,ms1.wz);
		  MS2_Control(ms2.vx,ms2.wz);
			//MS1_Control(test1.vx,test1.wz);
		  //MS2_Control(test2.vx,test2.wz);		
		}
		else
		{
			ms1.vx =0;
			ms2.vx =0;
			ms1.wz =0;
			ms2.wz =0;		
			MS1_Control(0,0);
		  MS2_Control(0,0);
		}
    osDelay(1);
  }
  /* USER CODE END control_task */
}

/* USER CODE BEGIN Header_can_task */
/**
* @brief Function implementing the CAN_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_can_task */
void can_task(void const * argument)
{
  /* USER CODE BEGIN can_task */
  /* Infinite loop */
  for(;;)
  {
	   if(STATE)
				{
					HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);			  
				}
				else
				{
					HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);			
				}	
		
		  if(CAN_check_count>0)
			{
				STATE = 1;
				CAN_check_count = 0;		
        Can_Pack_Send_msg(fb_angle1,fb_angle2);				
			}
			else
			{
				STATE = 0;
				CAN_check_count = 0;
			}
		osDelay(100);    //����Ҫ����ʱ

  }
  /* USER CODE END can_task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
/* USER CODE BEGIN 4 */
/***********************************************
�������ܣ�Can_unpack_msg ��������CAN���ݽ�����ṹ��
��ڲ�����			
			 buf CAN��������
			 ms  ���ֿ��Ʋ����ṹ��
����ֵ��null
************************************************/
void Can_unpack_msg(uint8_t* buf,ms_ctrl_t *ms)
{
	unsigned char *p1 = (unsigned char*)&ms->vx;
	for(uint8_t i=0 ;i<4;i++)
		{
				p1[i] = buf[i];
		}
	unsigned char *p2 = (unsigned char*)&ms->wz;
	for(uint8_t i=0 ;i<4;i++)	
		{
				p2[i] = buf[4+i];
		}
}

/***********************************************
�������ܣ�Can_Pack_Send_msg �����������ݴ�������鲢����CAN
��ڲ�����			
			 angle1(float)������1�Ƕ�[-90��,90��]����ֵ
			 angle2(float)������2�Ƕ�[-90��,90��]����ֵ
����ֵ��null
************************************************/
void Can_Pack_Send_msg(float angle1,float angle2)
{
	uint8_t data[8];
	memcpy(data,(void*)&angle1,sizeof(angle1));		//����
	memcpy(data+4,(void*)&angle2,sizeof(angle2));		//����
	Can_TxMessage(0,CAN_TX_ID,8,data);//������Ҫ���͵ı�������
}


/***********************************************
�������ܣ�MS1_Control ���Ƶ�1������ٶȺͽǶ�
��ڲ�����			
			 speed(float)������ת��[-1��1]���ֵ
			 angle(float)���ֽǶ�[-90��,90��]����ֵ
����ֵ��null
************************************************/
void MS1_Control(float speed,float angle)
{
	float angle_output;
	if(speed<0)
	{
		HAL_GPIO_WritePin(GPIOB, M1_DIR_Pin, GPIO_PIN_RESET);//����
	}
	else if(speed>0)
	{
    HAL_GPIO_WritePin(GPIOB, M1_DIR_Pin, GPIO_PIN_SET);//����
	}
	HAL_GPIO_WritePin(GPIOB, S1_DIR1_Pin, GPIO_PIN_RESET);//0����
	HAL_GPIO_WritePin(GPIOB, S1_DIR2_Pin, GPIO_PIN_RESET);//0����
	TIM1->CCR1 = Map(-1,1,PWM_LOW_MIN,PWM_HIGH_MAX,speed);//����
	TIM1->CCR2 = Map(0,1,0,1999,ABS(speed)); //����
	
	//PID
 // TIM4->CCR1 = Map(-2000,2000,PWM_LOW_MIN,PWM_HIGH_MAX,PID_Calc(&motorpid_angle1,gyro1.ryp.z,angle));
	
	if (angle>0)
	{
		angle_output = Map(0,145,1000,1300,angle);//�������
	}
	else
	{
		angle_output = Map(-140,0,700,1000,angle);//�������
	}
	
	// angle_output = Map(-148,128,710,1280,angle);//�������
	
	if((ABS(fb_angle1-angle ))<8&& fabs(angle)>5)
	{
		angle_output = angle_output + PID_Calc(&motorpid_angle1,fb_angle1,angle);
	}	
	
	TIM4->CCR1 =angle_output;

}

/***********************************************
�������ܣ�MS2_Control ���Ƶ�2������ٶȺͽǶ�
��ڲ�����			
			 speed(float)������ת��[-1��1]���ֵ
			 angle(float)���ֽǶ�[-90��,90��]����ֵ
����ֵ��null
************************************************/
void MS2_Control(float speed,float angle)
{
	float angle_output;
		if(speed<0)
	{
		HAL_GPIO_WritePin(GPIOB, M2_DIR_Pin, GPIO_PIN_RESET);//����
	}
	else if(speed>0)
	{
		HAL_GPIO_WritePin(GPIOB, M2_DIR_Pin, GPIO_PIN_SET);//����
	}
	HAL_GPIO_WritePin(GPIOB, S2_DIR1_Pin, GPIO_PIN_RESET);//0����
	HAL_GPIO_WritePin(GPIOB, S2_DIR2_Pin, GPIO_PIN_RESET);//0����
	TIM1->CCR3 = Map(-1,1,PWM_LOW_MIN,PWM_HIGH_MAX,speed);//����
	TIM1->CCR4 = Map(0,1,0,1999,ABS(speed)); //����
	/*
	//PID
	TIM4->CCR2 =PID_Calc(&motorpid_angle2,gyro2.ryp.z,angle) ;
	*/ 
	//angle_output = Map(-130,123,710,1280,angle);//�������
	
		if (angle>0)
	{
		angle_output = Map(0,135,1000,1300,angle);//�������
	}
	else
	{
		angle_output = Map(-145,0,700,1000,angle);//�������
	}
	
	
	if((ABS(fb_angle2 - angle))<8&& fabs(angle)>5)
	{
		angle_output = angle_output + PID_Calc(&motorpid_angle2,fb_angle2,angle);
	}
	TIM4->CCR2 =angle_output;
	
}

/***********************************************
�������ܣ�AngleSerialData����1�жϵ��ú���������ÿ�յ�һ�����ݣ�����һ�������������JYME1���������ݡ�
��ڲ�����			
			Data
			angle
			
����ֵ��null
************************************************/
void AngleSerialData2(char Data,float *angle)
{
	
	static uint8_t RxCnt = 0;
	 static char DataBuf[20];
	static uint8_t mark1 = 0;
	float count_angle=0;
	static uint8_t fb_flag = 0;
	float angle_output = 0;
	
	DataBuf[RxCnt]=Data;
	if(DataBuf[0]==':')
	{
		if(Data=='.')
		{
			mark1 = RxCnt;
		}
		
		if(Data=='\r')
		{
			//:123.456\n
			//012345678
			for(uint8_t i=mark1-1;i>0;i--)
			{	
				count_angle += (DataBuf[i]-0x30)*pow(10,(mark1-1-i));				
			}
			for(uint8_t i=mark1+1;i<(RxCnt);i++)
			{
				count_angle +=(DataBuf[i]-0x30)*pow(10,(mark1-i));
			}
				if(count_angle>180)
			{
				count_angle = count_angle-360;
			}
			angle_output = -1*(count_angle)/1.2; // 
			RxCnt = 0;
			
			if(fb_flag == 0)
			{
				ori_angle2 = angle_output;
				fb_flag = 1;
			}
			
			*angle = angle_output - ori_angle2;
			
			return;
		}
		RxCnt++;
	}
	else
	{
		RxCnt = 0;
		return;
	}
}

/***********************************************
�������ܣ�AngleSerialData����1�жϵ��ú���������ÿ�յ�һ�����ݣ�����һ�������������JYME1���������ݡ�
��ڲ�����			
			Data
			angle
			
����ֵ��null
************************************************/
void AngleSerialData(char Data,float *angle)
{
	
		static uint8_t RxCnt = 0;
	 static char DataBuf[20];
	static uint8_t mark1 = 0;
	float count_angle=0;
	static uint8_t fb_flag = 0;
	float angle_output = 0;
	
	DataBuf[RxCnt]=Data;
	if(DataBuf[0]==':')
	{
		if(Data=='.')
		{
			mark1 = RxCnt;
		}
		
		if(Data=='\r')
		{
			//:123.456\n
			//012345678
			for(uint8_t i=mark1-1;i>0;i--)
			{	
				count_angle += (DataBuf[i]-0x30)*pow(10,(mark1-1-i));				
			}
			for(uint8_t i=mark1+1;i<(RxCnt);i++)
			{
				count_angle +=(DataBuf[i]-0x30)*pow(10,(mark1-i));
			}
			if(count_angle>180)
			{
				count_angle = count_angle-360;
			}
			
			angle_output = -1*(count_angle)/1.2; 
			
			RxCnt = 0;
			
			if(fb_flag == 0)
			{
				ori_angle1 = angle_output;
				fb_flag = 1;
			}
			
			*angle = angle_output - ori_angle1;
			
			return;
		}
		RxCnt++;
	}
	else
	{
		RxCnt = 0;
		return;
	}
}


/***********************************************
�������ܣ�GyroSerialData2����2�жϵ��ú���������ÿ�յ�һ�����ݣ�����һ�������������WT901C���������ݡ�
��ڲ�����			
			ucData: �����ֽ�
			*gyro: �����ǽṹ��
����ֵ��null
************************************************/
void GyroSerialData2(unsigned char ucData,GYRO *gyro)
{
	static unsigned char ucRxBuffer[250];
	static unsigned char ucRxCnt = 0;	
	
	ucRxBuffer[ucRxCnt++]=ucData;	//���յ������ݴ��뻺������
	if (ucRxBuffer[0]!=0x55) //����ͷ���ԣ������¿�ʼѰ��0x55����ͷ
	{
		ucRxCnt=0;
		return;
	}
	if (ucRxCnt<11) {return;}//���ݲ���11�����򷵻�
	else
	{
		switch(ucRxBuffer[1])//�ж��������������ݣ�Ȼ���俽������Ӧ�Ľṹ���У���Щ���ݰ���Ҫͨ����λ���򿪶�Ӧ������󣬲��ܽ��յ�������ݰ�������
		{
			case 0x50:	memcpy(&(gyro->stime),&ucRxBuffer[2],8);break;//memcpyΪ�������Դ����ڴ濽��������������"string.h"�������ջ��������ַ����������ݽṹ�����棬�Ӷ�ʵ�����ݵĽ�����
			case 0x51:	memcpy(&(gyro->sacc),&ucRxBuffer[2],8);break;
			case 0x52:	memcpy(&(gyro->sgyro),&ucRxBuffer[2],8);break;
			case 0x53:	memcpy(&(gyro->sangel),&ucRxBuffer[2],8);break;
			case 0x54:	memcpy(&(gyro->smag),&ucRxBuffer[2],8);break;
			case 0x55:	memcpy(&(gyro->sdstatus),&ucRxBuffer[2],8);break;
			case 0x56:	memcpy(&(gyro->spress),&ucRxBuffer[2],8);break;
			case 0x57:	memcpy(&(gyro->slonlat),&ucRxBuffer[2],8);break;
			case 0x58:	memcpy(&(gyro->sgpsv),&ucRxBuffer[2],8);break;
			case 0x59:	memcpy(&(gyro->sq),&ucRxBuffer[2],8);break;
		}
		ucRxCnt=0;//��ջ�����
	}
	gyro->ryp.x = (float)gyro->sangel.Angle[0]/32768*180;
	gyro->ryp.y = (float)gyro->sangel.Angle[1]/32768*180;
	gyro->ryp.z = (float)gyro->sangel.Angle[2]/32768*180;
}

/***********************************************
�������ܣ�GyroSerialData�����жϵ��ú���������ÿ�յ�һ�����ݣ�����һ�������������WT901C���������ݡ�
��ڲ�����			
			ucData: �����ֽ�
			*gyro: �����ǽṹ��
����ֵ��null
************************************************/
void GyroSerialData(unsigned char ucData,GYRO *gyro)
{
	static unsigned char ucRxBuffer[250];
	static unsigned char ucRxCnt = 0;	
	
	ucRxBuffer[ucRxCnt++]=ucData;	//���յ������ݴ��뻺������
	if (ucRxBuffer[0]!=0x55) //����ͷ���ԣ������¿�ʼѰ��0x55����ͷ
	{
		ucRxCnt=0;
		return;
	}
	if (ucRxCnt<11) {return;}//���ݲ���11�����򷵻�
	else
	{
		switch(ucRxBuffer[1])//�ж��������������ݣ�Ȼ���俽������Ӧ�Ľṹ���У���Щ���ݰ���Ҫͨ����λ���򿪶�Ӧ������󣬲��ܽ��յ�������ݰ�������
		{
			case 0x50:	memcpy(&(gyro->stime),&ucRxBuffer[2],8);break;//memcpyΪ�������Դ����ڴ濽��������������"string.h"�������ջ��������ַ����������ݽṹ�����棬�Ӷ�ʵ�����ݵĽ�����
			case 0x51:	memcpy(&(gyro->sacc),&ucRxBuffer[2],8);break;
			case 0x52:	memcpy(&(gyro->sgyro),&ucRxBuffer[2],8);break;
			case 0x53:	memcpy(&(gyro->sangel),&ucRxBuffer[2],8);break;
			case 0x54:	memcpy(&(gyro->smag),&ucRxBuffer[2],8);break;
			case 0x55:	memcpy(&(gyro->sdstatus),&ucRxBuffer[2],8);break;
			case 0x56:	memcpy(&(gyro->spress),&ucRxBuffer[2],8);break;
			case 0x57:	memcpy(&(gyro->slonlat),&ucRxBuffer[2],8);break;
			case 0x58:	memcpy(&(gyro->sgpsv),&ucRxBuffer[2],8);break;
			case 0x59:	memcpy(&(gyro->sq),&ucRxBuffer[2],8);break;
		}
		ucRxCnt=0;//��ջ�����
	}
	gyro->ryp.x = (float)gyro->sangel.Angle[0]/32768*180;
	gyro->ryp.y = (float)gyro->sangel.Angle[1]/32768*180;
	gyro->ryp.z = (float)gyro->sangel.Angle[2]/32768*180;
}

/***********************************************
�������ܣ�can��������
��ڲ�����
			ide��	0����׼֡
					1����չ֡
			id��	֡ID
			len��	���ݳ���
			data��	����
����ֵ��0���ɹ���1��ʧ��
************************************************/
uint8_t Can_TxMessage(uint8_t ide,uint32_t id,uint8_t len,uint8_t *data)
{
	uint32_t   TxMailbox;
	CAN_TxHeaderTypeDef CAN_TxHeader;
	HAL_StatusTypeDef   HAL_RetVal; 
	uint16_t i=0;
	if(ide == 0)
	{
		CAN_TxHeader.IDE = CAN_ID_STD;	//��׼֡
		CAN_TxHeader.StdId = id;
	}
	else 
	{
		CAN_TxHeader.IDE = CAN_ID_EXT;			//��չ֡
		CAN_TxHeader.ExtId = id;
	}
	CAN_TxHeader.DLC = len;
	CAN_TxHeader.RTR = CAN_RTR_DATA;//����֡,CAN_RTR_REMOTEң��֡
	CAN_TxHeader.TransmitGlobalTime = DISABLE;
	while(HAL_CAN_GetTxMailboxesFreeLevel(&hcan) == 0)
	{
		i++;
		if(i>0xfffe)
			return 1;
	}
	HAL_Delay(5);
	HAL_RetVal = HAL_CAN_AddTxMessage(&hcan,&CAN_TxHeader,data,&TxMailbox);
	if(HAL_RetVal != HAL_OK)
		return 1;
	return 0;
}
/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
