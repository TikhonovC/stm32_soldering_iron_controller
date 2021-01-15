#include "ssd1306.h"
#include "settings.h"
#include "buzzer.h"
#include "iron.h"

// Need to be aligned to 32bit(4byte) boundary, as FillBuffer() uses 32bit tranfer for increased speed

oled_t oled = {
	ptr: 		&oled.buffer[0],
	status:		oled_idle,
	row:		0,
	device:		NULL,
	fillDMA:	NULL,
};




#if defined OLED_SOFT_I2C || defined OLED_SOFT_SPI

#ifdef OLED_SOFT_I2C
void Enable_Soft_I2C(void){
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	/*Configure GPIO pins : SCL_Pin */
	GPIO_InitStruct.Pin = 	OLED_SCL_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(OLED_SCL_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pins : SDA_Pin */
	GPIO_InitStruct.Pin = 	OLED_SDA_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(OLED_SDA_GPIO_Port, &GPIO_InitStruct);

	Oled_Clear_SDA();
	HAL_Delay(1);
	Oled_Clear_SCL();
	HAL_Delay(1);
	Oled_Set_SCL();			//Force stop condition on init
	HAL_Delay(1);
	Oled_Set_SDA();
	HAL_Delay(1);
}
#endif

#ifdef OLED_SOFT_SPI
void Enable_Soft_SPI(void){
	//HAL_SPI_MspDeInit(&OLED_DEVICE);
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	/*Configure GPIO pins : SCL_Pin */
	GPIO_InitStruct.Pin = 	OLED_SCL_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(OLED_SCL_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pins : SDA_Pin */
	GPIO_InitStruct.Pin = 	OLED_SDA_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(OLED_SDA_GPIO_Port, &GPIO_InitStruct);

}
#endif


#endif

#ifdef OLED_SOFT_SPI
__attribute__((section(".RamFunc")))
void spi_send(uint8_t* bf, uint16_t count){
	uint8_t shift,data;
	while(count--){
		data = *bf++;
		if(data==0){
			Oled_Clear_SDA();
			for(shift = 0; shift < 8; shift++){
				Oled_Set_SCL();
				Oled_Clear_SCL();
			}
		}

		else if(data==0xFF){
			Oled_Set_SDA();
			for(shift = 0; shift < 8; shift++){
				Oled_Set_SCL();
				Oled_Clear_SCL();
			}
		}

		else{
			for(shift = 0; shift < 8; shift++){
				if(data & 0x80){
					Oled_Set_SDA();
				}
				else{
					Oled_Clear_SDA();
				}

				Oled_Set_SCL();
				data <<= 1;
				Oled_Clear_SCL();
			}
		}
	}
}
#endif

#ifdef OLED_SOFT_I2C
void i2cWait(void){
	asm(	"nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n\
			 nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n\
			 nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
}
void i2cStart(void){						// 	Start condition, SDA transition to low with SCL high
	Oled_Set_SCL();
	i2cWait();
	Oled_Clear_SDA();
	i2cWait();
	Oled_Clear_SCL();
	i2cWait();
}
void i2cStop(void){						// 	Stop condition, SCL transition to high with SDA low
	Oled_Clear_SDA();
	i2cWait();
	Oled_Set_SCL();
	i2cWait();
	Oled_Set_SDA();
	i2cWait();
}
__attribute__((section(".RamFunc")))
void i2cBegin(bool isCmd){
	uint8_t bf[2]= { OLED_ADDRESS, 0x00 };
	if(!isCmd){
		bf[1] = 0x40;
	}
	for(uint8_t d=0;d<2;d++){
		uint8_t data = bf[d];
		for(uint8_t shift = 0; shift < 8; shift++){
			if(data & 0x80){
				Oled_Set_SDA();
				i2cWait();
			}
			else{
				Oled_Clear_SDA();
				i2cWait();
			}
			Oled_Set_SCL();
			i2cWait();
			Oled_Clear_SCL();
			data <<= 1;
		}
		i2cWait();
		Oled_Set_SCL();
		i2cWait();
		//Oled_Set_SDA();										// As we don't care the ACK, don't release SDA
		//i2cWait();
		//Get ACK here
		Oled_Clear_SCL();
	}
}
__attribute__((section(".RamFunc")))
void i2cSend(uint8_t* bf, uint16_t count, bool isCmd){
	volatile uint8_t shift,data;
	//bool ack=0;
	i2cStart();
	i2cBegin(isCmd);
	while(count--){
		data = *bf++;
		if(data==0){
			Oled_Clear_SDA();
			i2cWait();
			for(shift = 0; shift < 8; shift++){
				Oled_Set_SCL();
				i2cWait();
				Oled_Clear_SCL();
			}
			i2cWait();
			Oled_Set_SCL();
			i2cWait();
			//Oled_Set_SDA();										// As we don't care the ACK, don't release SDA
			//i2cWait();
			//Get ACK here
			Oled_Clear_SCL();
		}

		else if(data==0xFF){
			Oled_Set_SDA();
			i2cWait();
			for(shift = 0; shift < 8; shift++){
				Oled_Set_SCL();
				i2cWait();
				Oled_Clear_SCL();
			}
			i2cWait();
			Oled_Set_SCL();
			i2cWait();
			//Oled_Set_SDA();										// As we don't care the ACK, don't release SDA
			//i2cWait();
			//Get ACK here
			Oled_Clear_SCL();
		}

		else{
			for(shift = 0; shift < 8; shift++){
				if(data & 0x80){
					Oled_Set_SDA();
					i2cWait();
				}
				else{
					Oled_Clear_SDA();
					i2cWait();
				}
				Oled_Set_SCL();
				i2cWait();
				Oled_Clear_SCL();
				data <<= 1;
			}
			i2cWait();
			Oled_Set_SCL();
			i2cWait();
			//Oled_Set_SDA();										// As we don't care the ACK, don't release SDA
			//i2cWait();
			//Get ACK here
			Oled_Clear_SCL();
		}

	}
	i2cStop();
}
#endif


// Send command in blocking mode
void write_cmd(uint8_t cmd) {
	while(oled.status==oled_sending_data);	//Wait for DMA to finish
	// Now, else we are in idle (oled_idle) or DMA wants to send a cmd (oled_sending_cmd)
#if defined OLED_SOFT_I2C
	i2cSend(&cmd,1,i2cCmd);
#elif defined OLED_SPI || defined OLED_SOFT_SPI
	#ifdef USE_CS
	Oled_Clear_CS();
	#endif
	Oled_Clear_DC();
#ifdef OLED_SPI
	if(HAL_SPI_Transmit(oled.device, &cmd, 1, 1000)!=HAL_OK){
		Error_Handler();
	}
#elif defined OLED_SOFT_SPI
	spi_send(&cmd,1);
#endif
	Oled_Set_DC();
	#ifdef USE_CS
	Oled_Set_CS();
	#endif
#elif defined OLED_I2C
	if(HAL_I2C_Mem_Write(oled.device, OLED_ADDRESS, 0x00, 1, &cmd, 1, 100)){
		 Error_Handler();
	 }
#endif

}

void update_display( void ){
		if(oled.status!=oled_idle) { return; }	// If OLED busy, skip update

		#if defined OLED_SOFT_SPI || defined OLED_SOFT_I2C
		for(uint8_t row=0;row<8;row++){
			HAL_IWDG_Refresh(&HIWDG);
			setOledRow(row);

			#ifdef USE_CS
			Oled_Clear_CS();
			#endif

			#if defined OLED_SOFT_SPI
			spi_send((uint8_t *)&OledBuffer[128*row],128);

			#elif defined OLED_SOFT_I2C
			i2cSend((uint8_t *)&OledBuffer[128*row],128,i2cData);
			#endif

			#ifdef USE_CS
			Oled_Set_CS();
			#endif
		}
#elif defined OLED_SPI
		HAL_SPI_TxCpltCallback(oled.device); 	// Call the DMA callback function to send the frame

#elif defined OLED_I2C
		HAL_I2C_MemTxCpltCallback(oled.device);	// Call the DMA callback function to send the frame
#endif
}

// Function for drawing a pixel in display buffer
void pset(UG_U16 x, UG_U16 y, UG_COLOR c){
   unsigned int p;
   while(oled.status!=oled_idle);	// If OLED busy, wait

   if((x>127)||(y>63)) return;
   p = y>>3; // y/8
   p = p<<7; // *128
   p +=x;

   if(c){
      oled.buffer[p] |= 1<<(y%8);
   }
   else{
      oled.buffer[p] &= ~(1<<(y%8));
   }
}

void setOledRow(uint8_t row){
	write_cmd(0xB0|row);									// Set the OLED Row address
	write_cmd(systemSettings.settings.OledOffset);
	write_cmd(0x10);
}

void setContrast(uint8_t value) {
	write_cmd(0x81);         // Set Contrast Control
	write_cmd(value);         //   Default => 0xFF
}


#if defined OLED_SOFT_SPI
void ssd1306_init(DMA_HandleTypeDef *dma){
	Enable_Soft_SPI();

#elif defined OLED_SOFT_I2C
void ssd1306_init(DMA_HandleTypeDef *dma){
	Enable_Soft_I2C();

#elif defined OLED_SPI
void ssd1306_init(SPI_HandleTypeDef *device,DMA_HandleTypeDef *dma){
	oled.device	= device;

#elif defined OLED_I2C
void ssd1306_init(I2C_HandleTypeDef *device,DMA_HandleTypeDef *dma){
	oled.device	= device;

#endif


	oled.fillDMA= dma;

#if defined OLED_SPI || defined OLED_SOFT_SPI
	#ifndef USE_DC
	#error Mandatory OLED DC Pin not configured
	#endif

	#ifdef USE_CS
	Oled_Set_CS();				// De-select
	#endif

	#ifdef USE_RST
	Oled_Clear_RES();			// Set RST
	HAL_Delay(1);				// Delay
	Oled_Set_RES();				// Release RST
	#endif

#endif

#if defined OLED_I2C || defined OLED_SOFT_I2C
	#ifdef USE_CS
	Oled_Clear_CS();			// Unused in I2C mode, set low
	#endif

	#ifdef USE_DC
	Oled_Clear_DC();			// DC is the LSB address in I2C mode
	#endif

	#ifdef USE_RST
	Oled_Clear_RES();			// Set RST
	HAL_Delay(1);				//
	Oled_Set_RES();				// Release RST
	#endif

#endif

	HAL_IWDG_Refresh(&HIWDG);	// Clear watchdog
	HAL_Delay(100);				// 100mS wait for internal initialization
	write_cmd(0xAE);  			// Display Off
	write_cmd(0xD5);         	// Set Display Clock Divide Ratio / Oscillator Frequency
	write_cmd(0b11110000);      // Set Clock as 100 Frames/Sec
	write_cmd(0xA8);         	// Set Multiplex Ratio
	write_cmd(0x3F);         	// Default => 0x3F (1/64 Duty)
	write_cmd(0xD3);         	// Set Display Offset
	write_cmd(0x00);         	// Default => 0x00
	write_cmd(0x40|0x00);   	// Set Display Start Line
	write_cmd(0x8D);         	// Set Charge Pump
	write_cmd(0x10|0x04);   	// Default => 0x10
	write_cmd(0x20);         	// Set Memory Addressing Mode
	write_cmd(0x02);         	// Default => 0x02
	write_cmd(0xA0|0x01);   	// Set Segment Re-Map
	write_cmd(0xC0|0x08);   	// Set COM Output Scan Direction
	write_cmd(0xDA);         	// Set COM Pins Hardware Configuration
	write_cmd(0x02|0x10);   	// Default => 0x12 (0x10)
	setContrast(0xFF);			// Init in max contrast
	write_cmd(0xD9);         	// Set Pre-Charge Period
	write_cmd(0x22);         	// Default => 0x22 (2 Display Clocks [Phase 2] / 2 Display Clocks [Phase 1])
//  write_cmd_2(0xF1);         	// Default => 0x22 (2 Display Clocks [Phase 2] / 2 Display Clocks [Phase 1])
	write_cmd(0xDB);         	// Set VCOMH Deselect Level
	write_cmd(0x30);         	// Default => 0x20 (0.77*VCC)
	write_cmd(0xA4|0x00);   	// Set Entire Display On/Off
	write_cmd(0xA6|0x00);   	// Set Inverse Display On/Off
	FillBuffer(C_BLACK,fill_dma);	// Clear buffer
	systemSettings.settings.OledOffset = 2;		// Set by default while system settings are not loaded
	write_cmd(0xAF);   			// Set Display On
	update_display();			// Update display CGRAM
	while(oled.status!=oled_idle);	// Wait for DMA completion
	write_cmd(0xAF);   			// Set Display On
}

/*
*	Clear buffer with 32bit-transfer for fast filling (ensure that Oled buffer is 32-bit aligned!)
* 	128 * 8 = 1KB, / 4byte DMA txfer = 256 clock cycles (in theory)
* 	Args:
* 			color: 	0 = black, 1 = white
* 			mode: 	0 = Use software(fail-safe), 1= Use DMA (normal operation)
*/

void FillBuffer(bool color, bool mode){
	uint32_t fillVal;
	while(oled.status!=oled_idle);				// Don't write to buffer while screen buffer is being transfered

	if(color==C_WHITE){ fillVal=0xffffffff; }	// Fill color = white
	else{ fillVal=0; }						// Fill color = black

	if(mode==fill_dma){							// use DMA
		 HAL_DMA_Start(oled.fillDMA,(uint32_t)&fillVal,(uint32_t)oled.ptr,sizeof(oled.buffer)/sizeof(uint32_t));
		 HAL_DMA_PollForTransfer(oled.fillDMA, HAL_DMA_FULL_TRANSFER, 3000);
	}
	else{										// use software
		uint32_t* bf=(uint32_t*)oled.ptr;		// Pointer to oled buffer using 32bit data for faster operation
		for(uint16_t x=0;x<sizeof(oled.buffer)/sizeof(uint32_t);x++){	// Write to oled buffer
			bf[x]=fillVal;
		}
	}
}

#if defined OLED_I2C || defined OLED_SPI

// Abort DMA transfers and reset status
void display_abort(void){
#if defined OLED_SPI
	HAL_SPI_Abort(oled.device);

#elif defined OLED_I2C
	HAL_I2C_Abort(oled.device);
#endif

	HAL_DMA_PollForTransfer(oled.device->hdmatx, HAL_DMA_FULL_TRANSFER, 3000);	//Wait for DMA to finish
	oled.status=oled_idle;		// Force oled idle status
}


// Screen update for hard error handlers (crashes) not using DMA
void update_display_ErrorHandler(void){
	for(uint8_t row=0;row<8;row++){
		setOledRow(row);

		#if defined OLED_SPI
		#ifdef USE_CS
		Oled_Clear_CS();
		#endif
		Oled_Set_DC();
		if(HAL_SPI_Transmit(oled.device, (uint8_t*)oled.ptr + (row * 128), 128, 1000)!=HAL_OK){
			while(1);			// If error happens at this stage, just do nothing
		}
		#ifdef USE_CS
		Oled_Set_CS();
		#endif
		#elif defined OLED_I2C
		if(HAL_I2C_Mem_Write(oled.device, OLED_ADDRESS, 0x40, 1, (uint8_t*)oled.ptr + (row * 128), 128, 1000)!=HAL_OK){
			while(1);			// If error happens at this stage, just do nothing

		}
		#endif
	}
}


#ifdef OLED_SPI
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *device){

#elif defined OLED_I2C
void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *device){

#endif

	if(device == oled.device){

		#ifdef USE_CS
		Oled_Set_CS();
		#endif

		if(oled.row>7){
			oled.row=0;												// We sent the last row of the OLED buffer data
			oled.status=oled_idle;
			return;													// Return without retriggering DMA.
		}
		oled.status=oled_sending_cmd;

	#ifdef OLED_SPI

		setOledRow(oled.row);
		#ifdef USE_CS
		Oled_Clear_CS();
		#endif

		Oled_Set_DC();
		oled.status=oled_sending_data;								// Update status
		// Send next OLED row
		if(HAL_SPI_Transmit_DMA(oled.device,(uint8_t *) oled.ptr+(128*oled.row++), 128) != HAL_OK){
			Error_Handler();
		}
	}
	#elif defined OLED_I2C

		uint8_t cmd[3]={
			0xB0|oled.row,
			systemSettings.settings.OledOffset,
			0x10
		};

		if(HAL_I2C_Mem_Write(oled.device, OLED_ADDRESS, 0x00, 1, &cmd[0], 3, 50)){
			Error_Handler();
		}
		oled.status=oled_sending_data;								// Update status
		if( HAL_I2C_Mem_Write_DMA(oled.device, OLED_ADDRESS, 0x40, 1, oled.ptr+(128*oled.row++), 128)){
			Error_Handler();
		}

	}
	#endif
}

#endif
void FatalError(uint8_t type){

	#if defined OLED_I2C || defined OLED_SPI
	display_abort();
	#endif

	SetFailState(1);
	buzzer_fatal_beep();
	FillBuffer(C_BLACK,fill_soft);
	UG_FontSelect(&FONT_10X16_reduced);
	UG_SetForecolor(C_WHITE);
	UG_SetBackcolor(C_BLACK);
	switch(type){
		case error_NMI:
			UG_PutString(2,15,"NMI HANDLER");//10
			break;
		case error_HARDFAULT:
			UG_PutString(8,15,"HARD FAULT");//10
			break;
		case error_MEMMANAGE:
			UG_PutString(8,15,"MEM MANAGE");//10
			break;
		case error_BUSFAULT:
			UG_PutString(11,15,"BUS FAULT");//9
			break;
		case error_USAGEFAULT:
			UG_PutString(2,15,"USAGE FAULT");//11
			break;
		case error_RUNAWAY25:
			UG_PutString(0,0,"TEMP RUNAWAY");//12
			UG_PutString(38,15,">25*C");//5
			break;
		case error_RUNAWAY50:
			UG_PutString(0,0,"TEMP RUNAWAY");//12
			UG_PutString(38,17,">50*C");//5
			break;
		case error_RUNAWAY75:
			UG_PutString(0,0,"TEMP RUNAWAY");//12
			UG_PutString(38,17,">75*C");//5
			break;
		case error_RUNAWAY100:
			UG_PutString(0,0,"TEMP RUNAWAY");//12
			UG_PutString(33,17,">100*C");//6
			break;
		case error_RUNAWAY500:
			UG_PutString(23,0,"EXCEEDED");//8
			UG_PutString(33,17,"500*C!");//6
			break;
		case error_RUNAWAY_UNKNOWN:
			UG_PutString(0,0,"TEMP RUNAWAY");//12
			UG_PutString(23,17,">UNKNOWN");//8
			break;
		default:
			UG_PutString(2,15,"UNDEFINED");//11
			break;
	}
	UG_PutString(24,33,"ERROR!!");//7

	#if defined OLED_I2C || defined OLED_SPI
	update_display_ErrorHandler();

	#elif defined OLED_SOFT_I2C || defined OLED_SOFT_SPI
	update_display();
	#endif

	while(1){
		if(!BUTTON_input){
			for(uint16_t i=0;i<50000;i++);
			while(!BUTTON_input){
				HAL_IWDG_Refresh(&HIWDG);					// Clear watchdog
			}
			if(BUTTON_input){
				NVIC_SystemReset();
			}
		}
		HAL_IWDG_Refresh(&HIWDG);
	}

}
