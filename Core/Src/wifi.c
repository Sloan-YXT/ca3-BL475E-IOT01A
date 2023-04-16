/* Includes ------------------------------------------------------------------*/
#include "wifi.h"



/**
  * @brief  Receives data over the defined SPI interface and writes
  * 		it in buffer.
  * @param  hwifi: Wifi handle, which decides which Wifi instance is used.
  * @param  buffer: A char buffer, where the received data will be saved in.
  * @param  size: Buffer size
  * @retval WIFI_StatusTypeDef
  */
char wifiTxBuffer[WIFI_TX_BUFFER_SIZE];
char wifiRxBuffer[WIFI_RX_BUFFER_SIZE];
void WIFI_DEBUG(char *cmd,char *resp)
{
	char message[WIFI_RX_BUFFER_SIZE*2];
	char cmd_box[WIFI_RX_BUFFER_SIZE];
	sprintf(cmd_box,cmd);
	trimstr(cmd_box,strlen(cmd_box)+1,'\r');
	sprintf(message,"[%s]%s",cmd_box,resp);
	HAL_UART_Transmit(&huart1, (uint8_t*)message, strlen(message),0xFFFF);
}
uint32_t htonl(uint32_t data)
{
	uint32_t new_data = ((data&0xff)<<24)|((data&0xff00)<<8)|((data&0xff0000)>>8)|((data&0xff000000)>>24);
	return new_data;
}
WIFI_StatusTypeDef WIFI_SPI_Receive(WIFI_HandleTypeDef* hwifi, char* buffer, uint16_t size){

	uint16_t cnt = 0;
	memset(buffer, '\0', size); // Erase buffer

	while (WIFI_IS_CMDDATA_READY())
	{
		// Fill buffer as long there is still space
		if ( (cnt > (size - 2)) || (HAL_SPI_Receive(hwifi->handle , (uint8_t*) buffer + cnt, 1, WIFI_TIMEOUT) != HAL_OK) )
		  {
			Error_Handler();
		  }
		cnt+=2;
	}

	// Trim padding chars from data
	trimstr(buffer, size, (char) WIFI_RX_PADDING);

	return WIFI_OK;
}


/**
  * @brief  Sends data over the defined SPI interface which it
  * 		reads from buffer.
  * @param  hwifi: Wifi handle, which decides which Wifi instance is used.
  * @param  buffer: A char buffer, where the data to be sent is saved in.
  * @param  size: Buffer size (including \0, so it is compatible with sizeof())
  * @retval WIFI_StatusTypeDef
  */

WIFI_StatusTypeDef WIFI_SPI_Transmit(WIFI_HandleTypeDef* hwifi, char* buffer, uint16_t size){

	char bTx[(size/2)*2 + 1]; // Make a buffer that has an even amount of bytes (even is meant for the chars excluding \0)
	snprintf( bTx, size, buffer ); // Copy buffer in bTx
	//size % 2 ==0 : mLen is odd
	if ( !(size % 2) ) strcat(bTx, (char) WIFI_TX_PADDING); // If buffer had an odd amount of bytes, append a filler char to bTx

	if (HAL_SPI_Transmit(hwifi->handle, (uint8_t*)bTx, size/2, WIFI_TIMEOUT) != HAL_OK) // size must be halved since 16bits are sent via SPI
	  {
		Error_Handler();
	  }

	return WIFI_OK;
}
/**
  * @brief  Resets and initialises the Wifi module.
  * @param  hwifi: Wifi handle, which decides which Wifi instance is used.
  * @retval WIFI_StatusTypeDef
  */

WIFI_StatusTypeDef WIFI_Init(WIFI_HandleTypeDef* hwifi){

	int msgLength = 0;

	WIFI_RESET_MODULE();
	WIFI_ENABLE_NSS();

	while(!WIFI_IS_CMDDATA_READY());

	if(WIFI_SPI_Receive(hwifi, wifiRxBuffer, WIFI_RX_BUFFER_SIZE) != WIFI_OK) Error_Handler();

	if( strcmp(wifiRxBuffer, WIFI_MSG_POWERUP) ) Error_Handler();

	WIFI_DISABLE_NSS();


	msgLength = sprintf(wifiTxBuffer, "Z3=0\r");
	WIFI_SendATCommand(hwifi, wifiTxBuffer, WIFI_TX_BUFFER_SIZE, wifiRxBuffer, WIFI_RX_BUFFER_SIZE);

	msgLength = sprintf(wifiTxBuffer, "Z0\r");
	WIFI_SendATCommand(hwifi, wifiTxBuffer, WIFI_TX_BUFFER_SIZE, wifiRxBuffer, WIFI_RX_BUFFER_SIZE);
	//printf("Answer reset:\n %s", wifiRxBuffer);
//	msgLength = sprintf(wifiTxBuffer, "\r");
//	WIFI_SendATCommand(hwifi, wifiTxBuffer, msgLength+1, wifiRxBuffer, WIFI_RX_BUFFER_SIZE);

	return WIFI_OK;
}


/**
  * @brief  Sends an AT command to the Wifi module and write the response
  * 		in a buffer.
  * @param  hwifi: Wifi handle, which decides which Wifi instance is used.
  * @param  bCmd: Char buffer that contains command.
  * @param  sizeCmd: Command buffer size
  * @param  bRx: Response buffer
  * @param  sizeCmd: Response buffer size
  * @retval WIFI_StatusTypeDef
  */

WIFI_StatusTypeDef WIFI_SendATCommand(WIFI_HandleTypeDef* hwifi, char* bCmd, uint16_t sizeCmd, char* bRx, uint16_t sizeRx){

	while(!WIFI_IS_CMDDATA_READY());

	WIFI_ENABLE_NSS();

	if(WIFI_SPI_Transmit(hwifi, bCmd, sizeCmd) != WIFI_OK) Error_Handler();

	WIFI_DISABLE_NSS();

	while(!WIFI_IS_CMDDATA_READY());

	WIFI_ENABLE_NSS();

	if(WIFI_SPI_Receive(hwifi, bRx, sizeRx) != WIFI_OK) Error_Handler();

	if(WIFI_IS_CMDDATA_READY()) Error_Handler(); // If CMDDATA_READY is still high, then the buffer is too small for the data

	WIFI_DISABLE_NSS();

	return WIFI_OK;
}
WIFI_StatusTypeDef WIFI_SendATData(WIFI_HandleTypeDef* hwifi, char* bCmd, uint16_t sizeCmd, char* bRx, uint16_t sizeRx){

	while(!WIFI_IS_CMDDATA_READY());

	WIFI_ENABLE_NSS();

	if (HAL_SPI_Transmit(hwifi->handle, (uint8_t*)bCmd, sizeCmd/2, WIFI_TIMEOUT) != HAL_OK) // size must be halved since 16bits are sent via SPI
	{
		Error_Handler();
	}

	WIFI_DISABLE_NSS();

	while(!WIFI_IS_CMDDATA_READY());

	WIFI_ENABLE_NSS();

	if(WIFI_SPI_Receive(hwifi, bRx, sizeRx) != WIFI_OK) Error_Handler();

	if(WIFI_IS_CMDDATA_READY()) Error_Handler(); // If CMDDATA_READY is still high, then the buffer is too small for the data

	WIFI_DISABLE_NSS();

	return WIFI_OK;
}


/**
  * @brief  Joins an existing Network using the network configuration in
  * 		the Wifi handle.
  * @param  hwifi: Wifi handle, which decides which Wifi instance is used.
  * @retval WIFI_StatusTypeDef
  */

WIFI_StatusTypeDef WIFI_JoinNetwork(WIFI_HandleTypeDef* hwifi){

	int msgLength = 0;

	// Set SSID
	msgLength = sprintf(wifiTxBuffer, "C1=%s\r", hwifi->ssid);
	WIFI_SendATCommand(hwifi, wifiTxBuffer, msgLength+1, wifiRxBuffer, WIFI_RX_BUFFER_SIZE);

	// Set passphrase
	msgLength = sprintf(wifiTxBuffer, "C2=%s\r", hwifi->passphrase);
	WIFI_SendATCommand(hwifi, wifiTxBuffer, msgLength+1, wifiRxBuffer, WIFI_RX_BUFFER_SIZE);

	// Set security type
	msgLength = sprintf(wifiTxBuffer, "C3=%d\r", hwifi->securityType);
	WIFI_SendATCommand(hwifi, wifiTxBuffer, msgLength+1, wifiRxBuffer, WIFI_RX_BUFFER_SIZE);

	// Set if IP is requested via DHCP
	msgLength = sprintf(wifiTxBuffer, "C4=%d\r", hwifi->DHCP);
	WIFI_SendATCommand(hwifi, wifiTxBuffer, msgLength+1, wifiRxBuffer, WIFI_RX_BUFFER_SIZE);

	// If DHCP is not used, set the additionally needed configurations
	if(hwifi->DHCP != SET){

		// Set module's IP address
		msgLength = sprintf(wifiTxBuffer, "C6=%s\r", hwifi->ipAddress);
		WIFI_SendATCommand(hwifi, wifiTxBuffer, msgLength+1, wifiRxBuffer, WIFI_RX_BUFFER_SIZE);

		// Set module's network mask
		msgLength = sprintf(wifiTxBuffer, "C7=%s\r", hwifi->networkMask);
		WIFI_SendATCommand(hwifi, wifiTxBuffer, msgLength+1, wifiRxBuffer, WIFI_RX_BUFFER_SIZE);

		// Set module's default gateway
		msgLength = sprintf(wifiTxBuffer, "C8=%s\r", hwifi->defaultGateway);
		WIFI_SendATCommand(hwifi, wifiTxBuffer, msgLength+1, wifiRxBuffer, WIFI_RX_BUFFER_SIZE);

		// Set module's primary DNS server
		msgLength = sprintf(wifiTxBuffer, "C9=%s\r", hwifi->primaryDNSServer);
		WIFI_SendATCommand(hwifi, wifiTxBuffer, msgLength+1, wifiRxBuffer, WIFI_RX_BUFFER_SIZE);

	}

	// Join the network
	msgLength = sprintf(wifiTxBuffer, "C0\r");
	WIFI_SendATCommand(hwifi, wifiTxBuffer, msgLength+1, wifiRxBuffer, WIFI_RX_BUFFER_SIZE);

	// If there was an error, call the error handler
	if(strstr(wifiRxBuffer, "ERROR") != NULL) Error_Handler();

	// If the module's IP address was assigned by DHCP, then parse it
	// from the response and save it in the Wifi handle.
	if(hwifi->DHCP == SET){
		// The IP address is between the first and second comma
		char* startPos = strstr(wifiRxBuffer, ",");
		char* endPos = strstr(startPos+1, ",");

		// Check whether the commas have been found
		if(startPos == NULL || endPos == NULL) Error_Handler();

		// Copy the IP address from the response buffer into the Wifi handle
		// For n set IP_length+1, because the ending char \0 must be considered
		snprintf(hwifi->ipAddress, endPos - startPos, startPos+1);
	}

	return WIFI_OK;
}






WIFI_StatusTypeDef WIFI_ConnectServer(WIFI_HandleTypeDef* hwifi,char *ip,char *port)
{
	int msgLength = 0;
	msgLength = sprintf(wifiTxBuffer,"P1=0\r");
	WIFI_SendATCommand(hwifi, wifiTxBuffer, msgLength+1, wifiRxBuffer, WIFI_RX_BUFFER_SIZE);
	WIFI_DEBUG(wifiTxBuffer,wifiRxBuffer);
	msgLength = sprintf(wifiTxBuffer,"P3=%s\r",ip);
	WIFI_SendATCommand(hwifi, wifiTxBuffer, msgLength+1, wifiRxBuffer, WIFI_RX_BUFFER_SIZE);
	WIFI_DEBUG(wifiTxBuffer,wifiRxBuffer);
	msgLength = sprintf(wifiTxBuffer,"P4=%s\r",port);
	WIFI_SendATCommand(hwifi, wifiTxBuffer, msgLength+1, wifiRxBuffer, WIFI_RX_BUFFER_SIZE);
	WIFI_DEBUG(wifiTxBuffer,wifiRxBuffer);
	msgLength = sprintf(wifiTxBuffer,"P6=1\r");
	WIFI_SendATCommand(hwifi, wifiTxBuffer, msgLength+1, wifiRxBuffer, WIFI_RX_BUFFER_SIZE);
	WIFI_DEBUG(wifiTxBuffer,wifiRxBuffer);
	return WIFI_OK;
}
WIFI_StatusTypeDef WIFI_DisconnectServer(WIFI_HandleTypeDef* hwifi)
{
	int msgLength = 0;
	msgLength = sprintf(wifiTxBuffer,"P6=0\r");
	WIFI_SendATCommand(hwifi, wifiTxBuffer, msgLength+1, wifiRxBuffer, WIFI_RX_BUFFER_SIZE);
	WIFI_DEBUG(wifiTxBuffer,wifiRxBuffer);
	return WIFI_OK;
}
WIFI_StatusTypeDef WIFI_SendData(WIFI_HandleTypeDef* hwifi,float data)
{
	strcpy(wifiTxBuffer,"S3=4\r");
	char *p_test = &data;
	wifiTxBuffer[5] = p_test[0];
	wifiTxBuffer[6] = p_test[1];
	wifiTxBuffer[7] = p_test[2];
	wifiTxBuffer[8] = p_test[3];
	wifiTxBuffer[9] = 0;
	WIFI_SendATCommand(hwifi, wifiTxBuffer, 10, wifiRxBuffer, WIFI_RX_BUFFER_SIZE);
	WIFI_DEBUG(wifiTxBuffer,wifiRxBuffer);
	return WIFI_OK;
}
WIFI_StatusTypeDef WIFI_SendStr(WIFI_HandleTypeDef* hwifi,char *data)
{
	int len = strlen(data);
	int msgLength = 0;
	msgLength = sprintf(wifiTxBuffer,"S1=4\r");
	WIFI_SendATCommand(hwifi, wifiTxBuffer, msgLength+1, wifiRxBuffer, WIFI_RX_BUFFER_SIZE);
	WIFI_DEBUG(wifiTxBuffer,wifiRxBuffer);
	msgLength = sprintf(wifiTxBuffer,"S0\r");
	wifiTxBuffer[3] = (len&0xff000000)>>24;
	wifiTxBuffer[4] = (len&0xff0000)>>16;
	wifiTxBuffer[5] = (len&0xff00)>>8;
	wifiTxBuffer[6] = (len&0xff);
	wifiTxBuffer[7] = '\0';
	WIFI_SendATData(hwifi, wifiTxBuffer,8, wifiRxBuffer, WIFI_RX_BUFFER_SIZE);
	WIFI_DEBUG(wifiTxBuffer,wifiRxBuffer);
	msgLength = sprintf(wifiTxBuffer,"S1=%d\r",len);
	WIFI_SendATCommand(hwifi, wifiTxBuffer, msgLength+1, wifiRxBuffer, WIFI_RX_BUFFER_SIZE);
	WIFI_DEBUG(wifiTxBuffer,wifiRxBuffer);
	msgLength = sprintf(wifiTxBuffer,"S0\r%s",data);
	WIFI_SendATCommand(hwifi, wifiTxBuffer, msgLength+1, wifiRxBuffer, WIFI_RX_BUFFER_SIZE);
	WIFI_DEBUG(wifiTxBuffer,wifiRxBuffer);
}
/**
  * @brief  Trims a given character from beginning and end of a c string.
  * @param  str: C string
  * @param  strSize: C string size
  * @param  size: Character to trim
  * @retval None
  */
void trimstr(char* str, uint32_t strSize, char c){

	uint32_t trimPos = 0;
	uint32_t endPos = 0;

	// Find end of string a.k.a. first occurrence of '\0'
	for(uint32_t i = 0; i < strSize; i++){
		if( str[i] != '\0' ) continue;
		else{
			endPos = i;
			break;
		}
	}

	/**
	 * If c is at the end of the string, replace it with '\0'.
	 * Repeat until a char emerges that is not c.
	 */
	for(uint32_t i = endPos -1; i > 0; i--){
		if( str[i] == c ){
			str[i] = '\0';
			endPos = i;
		}
		else break;
	}

	// Find the position of the first char in the string that is not c.
	for(uint32_t i = 0; i < strSize; i++){
		if(str[i] == c){
			trimPos = i + 1;
		}else break;
	}
	// Trim leading c
	char message[strSize];
	snprintf( message, endPos + 1 - trimPos, &str[trimPos] );
	strcpy(str,message);
}
