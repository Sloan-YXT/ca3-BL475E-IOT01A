  /******************************************************************************
  * @introduction : project2 main file
  * Name:YaoXueTao
  * Matriculation:A0267413E
  ******************************************************************************/

/* Includes ------------------------------------------------------------------*/

#include "main.h"
#include "cyclic.h"
#include "sensor_config.h"
#include "hal_config.h"
#include "string.h"
#include "stdio.h"
//seconds counter, used a timer to count seconds
#include "ai_platform.h"
#include "network.h"
#include "network_data.h"
#include "wifi.h"
ai_handle network;
float aiInData[AI_NETWORK_IN_1_SIZE];
float aiOutData[AI_NETWORK_OUT_1_SIZE];
ai_u8 activations[AI_NETWORK_DATA_ACTIVATIONS_SIZE];
const char* activities[AI_NETWORK_OUT_1_SIZE] = {
  "stationary", "walking", "running"
};
ai_buffer * ai_input;
ai_buffer * ai_output;
static void AI_Init(void);
static void AI_Run(float *pIn, float *pOut);
static uint32_t argmax(const float * values, uint32_t len);
uint32_t write_index = 0;


static void AI_Init(void)
{
  ai_error err;
  __HAL_RCC_CRC_CLK_ENABLE();
  /* Create a local array with the addresses of the activations buffers */
  const ai_handle act_addr[] = { activations };
  char message[100];
  /* Create an instance of the model */
  err = ai_network_create_and_init(&network, act_addr, NULL);
  if (err.type != AI_ERROR_NONE) {
	sprintf(message,"AI ai_network_run error - type=%d code=%d\r\n", err.type, err.code);
	HAL_UART_Transmit(&huart1, (uint8_t*)message, strlen(message),0xFFFF);
    Error_Handler();
  }
  ai_input = ai_network_inputs_get(network, NULL);
  ai_output = ai_network_outputs_get(network, NULL);
}
static void AI_Run(float *pIn, float *pOut)
{
  ai_i32 batch;
  ai_error err;

  /* Update IO handlers with the data payload */
  ai_input[0].data = AI_HANDLE_PTR(pIn);
  ai_output[0].data = AI_HANDLE_PTR(pOut);
  char message[100];
  batch = ai_network_run(network, ai_input, ai_output);
  if (batch != 1) {
    err = ai_network_get_error(network);
    sprintf(message,"AI ai_network_run error - type=%d code=%d\r\n", err.type, err.code);
    HAL_UART_Transmit(&huart1, (uint8_t*)message, strlen(message),0xFFFF);
    Error_Handler();
  }
}
static uint32_t argmax(const float * values, uint32_t len)
{
  float max_value = values[0];
  uint32_t max_index = 0;
  for (uint32_t i = 1; i < len; i++) {
    if (values[i] > max_value) {
      max_value = values[i];
      max_index = i;
    }
  }
  return max_index;
}
char ssid[] = "HUAWEI-1CR2PZ";
char passphrase[] = "86443860";
WIFI_HandleTypeDef hwifi;
float temp,humi;
const char *state="idle";
static void WIFI_Init_main(){

	hwifi.handle = &hspi3;
	hwifi.ssid = ssid;
	hwifi.passphrase = passphrase;
	hwifi.securityType = WPA_MIXED;
	hwifi.DHCP = SET;
	hwifi.ipStatus = IP_V4;
	hwifi.transportProtocol = WIFI_TCP_PROTOCOL;
	hwifi.port = 8080;

	WIFI_Init(&hwifi);
}
void taskAcc(void)
{
	float accXYZ[3];
	int16_t accXYZ_in[3];
	BSP_ACCELERO_AccGetXYZ(accXYZ_in);
	accXYZ[0] = accXYZ_in[0]/100;
	accXYZ[1] = accXYZ_in[1]/100;
	accXYZ[2] = accXYZ_in[2]/100;
	char message[100];
	sprintf(message,"Major Cycle %d |Minor Cycle %d| Accel X:%8.4f; Accel Y:%8.4f; Accel Z:%8.4f (m/s2)\r\n",major_cycle,minor_cycle,accXYZ[0],accXYZ[1],accXYZ[2]);
	HAL_UART_Transmit(&huart1, (uint8_t*)message, strlen(message),0xFFFF);
	aiInData[write_index + 0] = (float)accXYZ_in[0]/4000.0f;
    aiInData[write_index + 1] = (float)accXYZ_in[1]/4000.0f;
	aiInData[write_index + 2] = (float)accXYZ_in[2]/4000.0f;
	write_index += 3;
	if (write_index == AI_NETWORK_IN_1_SIZE) {
	        write_index = 0;

	        sprintf(message,"Running inference\r\n");
	        HAL_UART_Transmit(&huart1, (uint8_t*)message, strlen(message),0xFFFF);
	        AI_Run(aiInData, aiOutData);

	        /* Output results */
	        for (uint32_t i = 0; i < AI_NETWORK_OUT_1_SIZE; i++) {
	          sprintf(message,"%8.6f ", aiOutData[i]);
	          HAL_UART_Transmit(&huart1, (uint8_t*)message, strlen(message),0xFFFF);
	        }
	        uint32_t class = argmax(aiOutData, AI_NETWORK_OUT_1_SIZE);
	        sprintf(message,": %d - %s\r\n", (int) class, activities[class]);
	        state = activities[class];
	        HAL_UART_Transmit(&huart1, (uint8_t*)message, strlen(message),0xFFFF);
  }
}
void taskTemp(void)
{
	temp = BSP_TSENSOR_ReadTemp();
	char message[100];
	sprintf(message,"Major Cycle %d |Minor Cycle %d| Temperature : %8.4f(Celsius)\r\n",major_cycle,minor_cycle,temp);
	HAL_UART_Transmit(&huart1, (uint8_t*)message, strlen(message),0xFFFF);
}
void taskMegneto(void)
{
	float megXYZ[3];
	int16_t megXYZ_in[3];
	BSP_MAGNETO_GetXYZ(megXYZ_in);
	megXYZ[0] = megXYZ_in[0]/1000;
	megXYZ[1] = megXYZ_in[1]/1000;
	megXYZ[2] = megXYZ_in[2]/1000;
	char message[100];
	sprintf(message,"Major Cycle %d |Minor Cycle %d| Megneto X:%8.4f; Megneto Y:%8.4f; Megneto Z:%8.4f (Gauss) \r\n",major_cycle,minor_cycle,megXYZ[0],megXYZ[1],megXYZ[2]);
	HAL_UART_Transmit(&huart1, (uint8_t*)message, strlen(message),0xFFFF);
}
void taskHumi(void)
{
	humi = BSP_HSENSOR_ReadHumidity();
	char message[100];
	sprintf(message,"Major Cycle %d |Minor Cycle %d| Humidity : %8.4f(rH%%)\r\n",major_cycle,minor_cycle,humi);
	HAL_UART_Transmit(&huart1, (uint8_t*)message, strlen(message),0xFFFF);
}
void taskGyro(void)
{
	float gyroXYZ[3];
	float gyroXYZ_in[3];
	BSP_GYRO_GetXYZ(gyroXYZ_in);
	gyroXYZ[0] = gyroXYZ_in[0]/1000;
	gyroXYZ[1] = gyroXYZ_in[1]/1000;
	gyroXYZ[2] = gyroXYZ_in[2]/1000;
	char message[100];
	sprintf(message,"Major Cycle %d |Minor Cycle %d| Gyro X:%8.4f; Gyro Y:%8.4f; Gyro Z:%8.4f (dps)\r\n",major_cycle,minor_cycle,gyroXYZ[0],gyroXYZ[1],gyroXYZ[2]);
	HAL_UART_Transmit(&huart1, (uint8_t*)message, strlen(message),0xFFFF);
}
void taskPiezo(void)
{
	float pressure = BSP_PSENSOR_ReadPressure();
	char message[100];
	sprintf(message,"Major Cycle %d |Minor Cycle %d| Pressure : %8.4f(hPa)\r\n",major_cycle,minor_cycle,pressure);
	HAL_UART_Transmit(&huart1, (uint8_t*)message, strlen(message),0xFFFF);
}
void taskSendMessage(void)
{
	//__set_PRIMASK(1);
	WIFI_SendStr(&hwifi,state);
	char temp_in[11],humi_in[11];
	memset(temp_in,0,11);
	memset(humi_in,0,11);
	sprintf(temp_in,"%010.4f",temp);
	sprintf(humi_in,"%010.4f",humi);
    WIFI_SendStr(&hwifi,temp_in);
    WIFI_SendStr(&hwifi,humi_in);
    //__set_PRIMASK(0);
}
int main(void)
{
    //int seconds_count = 0;
    HAL_Init();
    SystemClock_Config();
    hal_Init();
    HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
	BSP_ACCELERO_Init();
	BSP_GYRO_Init();
	BSP_MAGNETO_Init();
	BSP_TSENSOR_Init();
	BSP_HSENSOR_Init();
	BSP_PSENSOR_Init();
	sensor_config();
	WIFI_Init_main();
	WIFI_JoinNetwork(&hwifi);
	//WIFI_ConnectServer(&hwifi,"192.168.3.3","12345");
	//WIFI_ConnectServer(&hwifi,"192.168.3.5","6666");
	WIFI_ConnectServer(&hwifi,"47.108.170.207","6666");
	const char *name = "CA3 IOT NODE";
	const char *position = "westcove 16";
	const char *type = "B-L475E-IOT01A";
	WIFI_SendStr(&hwifi,name);
	WIFI_SendStr(&hwifi,type);
	WIFI_SendStr(&hwifi,position);
	//odr(accelerometor ) = 104
	registerTask(taskAcc,"Accelero reading",0,0,ACCELERO,floor(1000/104));
	//odr(temperature)  = 12.5
	registerTask(taskTemp,"Temperature reading",1,0,TEMP,floor(1000/12.5));
	//1s 1 message
	registerTask(taskSendMessage,"Sending Message",3,0,WIFI,1000);
	//odr(humidity) = 12.5
	registerTask(taskHumi,"Humidity reading",2,0,HUMI,floor(1000/12.5));
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
	AI_Init();
	task_scheduler();
	while(1);
}

