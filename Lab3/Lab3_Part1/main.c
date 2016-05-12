//*****************************************************************************
//
// Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/ 
// 
// 
//  Redistribution and use in source and binary forms, with or without 
//  modification, are permitted provided that the following conditions 
//  are met:
//
//    Redistributions of source code must retain the above copyright 
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the 
//    documentation and/or other materials provided with the   
//    distribution.
//
//    Neither the name of Texas Instruments Incorporated nor the names of
//    its contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
//  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
//  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
//  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************

//*****************************************************************************
//
// Application Name     - ADC
// Application Overview - The application is a reference to usage of ADC DriverLib 
//                        functions on CC3200. Developers/Users can refer to this 
//                        simple application and re-use the functions in 
//                        their applications.
//
// Application Details  -
// http://processors.wiki.ti.com/index.php/CC32xx_ADC
// or
// docs\examples\CC32xx_ADC.pdf
//
//*****************************************************************************

//*****************************************************************************
//
//! \addtogroup adc_example
//! @{
//
//*****************************************************************************

// Standard includes
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

// Driverlib includes
#include "utils.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "hw_types.h"
#include "hw_adc.h"
#include "hw_ints.h"
#include "hw_gprcm.h"
#include "rom.h"
#include "rom_map.h"
#include "interrupt.h"
#include "prcm.h"
#include "uart.h"
#include "pin_mux_config.h"
#include "pin.h"
#include "adc.h"
#include "spi.h"

#include "uart_if.h"

#define USER_INPUT 
#define UART_PRINT         Report
#define FOREVER            1
#define APP_NAME           "ADC Reference"
#define NO_OF_SAMPLES 		64

//*****************************************************************************
//
// Application Master/Slave mode selector macro
//
// MASTER_MODE = 1 : Application in master mode
// MASTER_MODE = 0 : Application in slave mode
//
//*****************************************************************************
#define MASTER_MODE      0

#define SPI_IF_BIT_RATE  100000
#define TR_BUFF_SIZE     100

//*****************************************************************************

unsigned long pulAdcSamples[4096];

//*****************************************************************************
//                      GLOBAL VARIABLES
//*****************************************************************************
#if defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif

/****************************************************************************/
/*                      LOCAL FUNCTION PROTOTYPES                           */
/****************************************************************************/
static void BoardInit(void);
static void DisplayBanner(char * AppName);

//*****************************************************************************
//
//! Application startup display on UART
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
static void
DisplayBanner(char * AppName)
{
    Report("\n\n\n\r");
    Report("\t\t *************************************************\n\r");
    Report("\t\t       CC3200 %s Application       \n\r", AppName);
    Report("\t\t *************************************************\n\r");
    Report("\n\n\n\r");
}

//*****************************************************************************
//
//! Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
static void
BoardInit(void)
{
/* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
    //
    // Set vector table base
    //
#if defined(ccs)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif
    //
    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}

void transmit(short sUserData) {
	unsigned long ulDummy;
    //
    // Push the character over SPI
    //
    MAP_SPIDataPut(GSPI_BASE,sUserData);

    UART_PRINT("%04x\n\r", sUserData);

    //
    // Clean up the receive register into a dummy
    // variable
    //
    MAP_SPIDataGet(GSPI_BASE,&ulDummy);
}

//*****************************************************************************
//
//! main - calls Crypt function after populating either from pre- defined vector 
//! or from User
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void 
main()
{
    unsigned int  uiIndex=0;
    unsigned long ulSample;

    short command = 0x0B00;
    // Command bits assigned: 0000 1011

    short dataMask = 0x00FF;
    // 0000 0000 1111 1111

    //
    // Initialize Board configurations
    //
    BoardInit();

    //adc
    // Configuring UART for Receiving input and displaying output
    // 1. PinMux setting
    // 2. Initialize UART
    // 3. Displaying Banner
    //
    PinMuxConfig();
    InitTerm();
    DisplayBanner(APP_NAME);

    //
    // Reset SPI
    //
    MAP_SPIReset(GSPI_BASE);

    //
    // Configure SPI interface
    //
    MAP_SPIConfigSetExpClk(GSPI_BASE,MAP_PRCMPeripheralClockGet(PRCM_GSPI),
                     SPI_IF_BIT_RATE,SPI_MODE_MASTER,SPI_SUB_MODE_0,
                     (SPI_SW_CTRL_CS |
                     SPI_4PIN_MODE |
                     SPI_TURBO_OFF |
                     SPI_CS_ACTIVELOW |
					 SPI_WL_16));

    //
    // Enable SPI for communication
    //
    MAP_SPIEnable(GSPI_BASE);

    while (1) {
        //
        // Initialize Array index for multiple execution
        //
        uiIndex=0;
      

        //
        // Configure ADC timer which is used to timestamp the ADC data samples
        //
 //       MAP_ADCTimerConfig(ADC_BASE,2^17);

        //
        // Enable ADC timer which is used to timestamp the ADC data samples
        //
//        MAP_ADCTimerEnable(ADC_BASE);

        //
        // Enable ADC module
        //
        MAP_ADCEnable(ADC_BASE);

        //
        // Enable ADC channel
        //

        MAP_ADCChannelEnable(ADC_BASE, ADC_CH_3);


        while(uiIndex < NO_OF_SAMPLES + 4)
        {
        	UtilsDelay(1000000);
            if(MAP_ADCFIFOLvlGet(ADC_BASE, ADC_CH_3))
            {
                ulSample = MAP_ADCFIFORead(ADC_BASE, ADC_CH_3);
                pulAdcSamples[uiIndex++] = ulSample;

                //
                // Enable Chip select
                //
                MAP_SPICSEnable(GSPI_BASE);

                transmit( ( (ulSample >> 6) & dataMask) | command );
                // According to P362 of TRM, [13:2] are ADC Sample Bits!
                // Need to shift 6 bits in order to get the most significant 8 bits
                // mask it with dataMask just in case, and than add command bits to it

            	//
            	// Disable chip select
            	//
            	MAP_SPICSDisable(GSPI_BASE);

            	// MAP_SPICSEnable and MAP_SPICSDisable are necessary
            	// because SW_CS selected and DAC needs a disable to read input
            }
        }

        MAP_ADCChannelDisable(ADC_BASE, ADC_CH_3);

        uiIndex = 0;


        //
        // Print out ADC samples
        //

        while(uiIndex < NO_OF_SAMPLES)
        {
            UART_PRINT("%f\t",(((float)((pulAdcSamples[4+uiIndex] >> 2 ) & 0x0FFF))*1.4)/4096);
            uiIndex++;
            if ((uiIndex % 4)==0)
            	UART_PRINT("\n\r");
        }

        UART_PRINT("\n\r");

    }
}
//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
