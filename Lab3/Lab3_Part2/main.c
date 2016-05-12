//*****************************************************************************

//Standard includes
#include <stdlib.h>
#include <string.h>

// Driverlib includes
#include "hw_common_reg.h"
#include "hw_memmap.h"
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_uart.h"
#include "hw_adc.h"
#include "uart.h"
#include "prcm.h"
#include "rom.h"
#include "rom_map.h"
#include "systick.h"
#include "utils.h"
#include "udma.h"
#include "interrupt.h"
#include "adc.h"
#include "gpio.h"
#include "gpio_if.h"

// Common interface includes
#include "udma_if.h"
#include "uart_if.h"
#include "pin_mux_config.h"

#define APPLICATION_VERSION     "1.1.1"
#define APP_NAME                "ADC uDMA Example"
#define UART_PRINT              Report

//*****************************************************************************
//
// The size of the data buffer.
//
//*****************************************************************************
#define BUFFER_SIZE         1024
#define VOLTAGE_ANALYSIS_PRECISION	1
#define TIMER_ANALYSIS_PRECISION	1
#define PING						0
#define PONG						1
#define ADC_TIMER_RATE				40000000

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************

// The destination buffers used for uDMA transfers.
volatile unsigned long g_ulPing[BUFFER_SIZE];
volatile unsigned long g_ulPong[BUFFER_SIZE];

volatile unsigned long g_ulPingCount;
volatile unsigned long g_ulPongCount;

volatile unsigned char g_ucPingflag;
volatile unsigned char g_ucPongflag;

//
// Voltage Update Globals
//
static float Period_Detection = 0.1;	// detect rising edge passing 0.6 V

volatile float g_fPing_CurrentReading;
volatile float g_fPong_CurrentReading;

volatile float g_fPing_LastReading;
volatile float g_fPong_LastReading;

volatile float g_fPing_LastReadingBeforeFinish;
volatile float g_fPong_LastReadingBeforeFinish;

//
// Time Update Globals
//
volatile double TimeoutCounter = 0;

volatile unsigned int g_uiPing_CurrentTimeStamp;
volatile unsigned int g_uiPong_CurrentTimeStamp;

volatile unsigned int g_uiPing_LastTimeStamp;
volatile unsigned int g_uiPong_LastTimeStamp;

volatile unsigned int g_uiPing_LastTimeStampBeforeFinish;
volatile unsigned int g_uiPong_LastTimeStampBeforeFinish;

volatile double g_ulpreviousTime;
volatile double g_ulcurrentTime;

volatile double g_ffrequency;

volatile float voltageDiff;

//
// Square Wave vs Sine Wave
//
#define ARRAY_SIZE		2048

volatile int scoringArray[ARRAY_SIZE];
volatile int Index = 0;
volatile int squareScore = 0;


// vector table entry
#if defined(ewarm)
    extern uVectorEntry __vector_table;
#endif

#if defined(ccs)
    extern void (* const g_pfnVectors[])(void);
#endif
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************



//*****************************************************************************
//!
//! The interrupt handler for ADC.  This interrupt will occur when a DMA
//! transfer is complete.
//!
//! \param None
//!
//! \return None
//*****************************************************************************
void ADCIntHandler(void)
{
	unsigned long ulMode;
	unsigned short Status;

	//
	// Clear ADC Interrupt
	//
	Status = ADCIntStatus(ADC_BASE, ADC_CH_3);
	ADCIntClear(ADC_BASE, ADC_CH_3, Status | ADC_DMA_DONE);

	//
	// Get Channel Mode from ADC_CH_3
	// Should return UDMA_MODE_PINGPONG if the current transfer is still ongoing
	//
	ulMode = MAP_uDMAChannelModeGet(UDMA_CH17_ADC_CH3 | UDMA_PRI_SELECT);
	if (ulMode == UDMA_MODE_STOP) {	// Stop if the transfer is complete
		//
		// To tell main that Primary Structure just finished a DMA transfer
		//
		g_ucPingflag=1;		// Let main do the printing
		g_ulPingCount++;	// Book keeping: Number of DMA Complete for Ping

		//
		// Setup the exact same transfer scheme for PRI Structure with Ping buffer
		//
		// When a uDMA transfer is completed, the channel is automatically disabled
		// by the uDMA controller. Therefore, this function should be called
		// prior to starting up any new transfer.
		//
		// Please also note that the transfer will NOT necessary start right away!
		// If the other Structure is still reading and transfering data from the pin
		// This Structure will wait until the other one completes.
		// Then it would start. Refer to TRM P102 for more detail.
		//
		UDMASetupTransfer(UDMA_CH17_ADC_CH3 | UDMA_PRI_SELECT, UDMA_MODE_PINGPONG,
				BUFFER_SIZE, UDMA_SIZE_32, UDMA_ARB_1,
				(void *)(ADC_BASE + ADC_O_channel6FIFODATA), UDMA_SRC_INC_NONE,
				(void *)&(g_ulPing[0]), UDMA_DST_INC_32);
	}

	//
	// Essentially the same scheme for ALT Structure with Pong buffer
	//
	ulMode = MAP_uDMAChannelModeGet(UDMA_CH17_ADC_CH3 | UDMA_ALT_SELECT);
	if	(ulMode == UDMA_MODE_STOP) {
		g_ucPongflag=1;
		g_ulPongCount++;

		UDMASetupTransfer(UDMA_CH17_ADC_CH3 | UDMA_ALT_SELECT, UDMA_MODE_PINGPONG,
					BUFFER_SIZE, UDMA_SIZE_32, UDMA_ARB_1,
					(void *)(ADC_BASE + ADC_O_channel6FIFODATA), UDMA_SRC_INC_NONE,
					(void *)&(g_ulPong[0]), UDMA_DST_INC_32);

	}

}
//*****************************************************************************

void InitAdcDma( void )
{
	unsigned short Status;

    //
    // Start Clock, Establish Interrupt, etc
    //
	UDMAInit();

    //
    // Assign ADC_CH3 to Channel 17, which has ADC6 and GPTimer A2-B
    //
	MAP_uDMAChannelAssign(UDMA_CH17_ADC_CH3);

    //
    // Set up both Primary and Alternate structure for pingpong mode!
	//
	//! - \b UDMA_MODE_PINGPONG to set up a transfer that switches between the
	//!   primary and alternate control structures for the channel.  This mode
	//!   allows use of ping-pong buffering for uDMA transfers.
	//
	// Need two UDMASetupTransfer for both pri and alt
	// Each with a different buffer: g_ulPing and g_ulPong
	// Need to use UDMA_SIZE_32 because we need time stamp information too!
    //
	// The ulTransferSize parameter is the number of data items,
	// not the number of bytes. The value of this parameter should not exceed 1024.
	// Here it equals BUFFER_SIZE which is 1024.
	//
	//! The arbitration size determines how many items are transferred before
	//! the uDMA controller re-arbitrates for the bus.  Choose the arbitration size
	//! from one of \b UDMA_ARB_1, \b UDMA_ARB_2, \b UDMA_ARB_4, \b UDMA_ARB_8,
	//! through \b UDMA_ARB_1024 to select the arbitration size from 1 to 1024
	//! items, in powers of 2.
	// Here it is 0 (UDMA_ARB_1), which means DON'T do the transfer
	//
	//! \param pvSrcAddr is the source address for the transfer.
	// Here it is the address of ADC6's FIFO
	// (void *)(ADC_BASE + ADC_O_channel6FIFODATA)
	//
	// address increment value for the source = 0 in this case because we
	// are always reading from the same address (ADC6)
	//
	//! \param pvDstBuf. Pointer to the destination buffer
	// Here it is the address of the buffer we declared globally
	// (void *)&(g_ulPing[0]) for Ping and
	// (void *)&(g_ulPong[0]) for Pong
	//
	// address increment value for the destination = UDMA_DST_INC_32
	// because we are saving the 32 bit data into the buffer
	// The DSTINC is in bit
	//
	UDMASetupTransfer(UDMA_CH17_ADC_CH3 | UDMA_PRI_SELECT, UDMA_MODE_PINGPONG,
			BUFFER_SIZE, UDMA_SIZE_32, UDMA_ARB_1,
			(void *)(ADC_BASE + ADC_O_channel6FIFODATA), UDMA_SRC_INC_NONE,
			(void *)&(g_ulPing[0]), UDMA_DST_INC_32);

	UDMASetupTransfer(UDMA_CH17_ADC_CH3 | UDMA_ALT_SELECT, UDMA_MODE_PINGPONG,
			BUFFER_SIZE, UDMA_SIZE_32, UDMA_ARB_1,
			(void *)(ADC_BASE + ADC_O_channel6FIFODATA), UDMA_SRC_INC_NONE,
			(void *)&(g_ulPong[0]), UDMA_DST_INC_32);
	//
	// Initialize counts and flags to 0
	//
	g_ulPingCount=0;
	g_ulPongCount=0;
	g_ucPingflag=0;
	g_ucPongflag=0;

	//
	// Enable DMA transfer for ADC_CH3, which is mapped to Pin 60
	// and has two individual channels, one for internal and one for external use
	// We are using CH6 for external data received from our circuit setup
	//
	ADCDMAEnable(ADC_BASE, ADC_CH_3);

	//
	// Handler registration for ADC CH 3
	//
	ADCIntRegister(ADC_BASE, ADC_CH_3, ADCIntHandler);

	//
	// Read and Clear Interrupt Status for ADC_CH_3
	//
	Status = ADCIntStatus(ADC_BASE, ADC_CH_3);
	ADCIntClear(ADC_BASE, ADC_CH_3, Status | ADC_DMA_DONE);

	//
	// Enable Interrupt for ADC_CH_3
	// This Interrupt will be triggered when the DMA of ADC finish the transfer
	// Keep in mind that the internal implementation is from ADC module
	//
	ADCIntEnable(ADC_BASE, ADC_CH_3, ADC_DMA_DONE);

	//
	// Enable ADC Module and CH3
	// From now on ADC will keep working to read Data
	//
	ADCEnable(ADC_BASE);
	ADCChannelEnable(ADC_BASE, ADC_CH_3);
}

//*****************************************************************************
//
//! Application startup display on UART
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void
DisplayBanner()
{
    UART_PRINT("\n\n\n\r");
    UART_PRINT("\t\t   *******************************************\n\r");
    UART_PRINT("\t\t        CC3200 %s Application        \n\r", APP_NAME);
    UART_PRINT("\t\t   *******************************************\n\r");
    UART_PRINT("\n\n\n\r");

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

void updatePeriod (int Ping_or_Pong) {
	//
	// Update TimeStamps
	//
	g_ulpreviousTime = g_ulcurrentTime;

	if (Ping_or_Pong == PING) {
		g_ulcurrentTime = g_uiPing_CurrentTimeStamp;
	}
	else {
		g_ulcurrentTime = g_uiPong_CurrentTimeStamp;
	}

	//
	// One cycle is 2^17
	//
	g_ffrequency = (ADC_TIMER_RATE / 1000000) / ( (g_ulpreviousTime + TimeoutCounter * 262144 - g_ulcurrentTime) / 400000000 );
	// UART_PRINT("%d %d %d \n\r", g_ulpreviousTime, g_ulcurrentTime, TimeoutCounter * 131072);

	TimeoutCounter = 0;
}

void updateTimeDiff(int Ping_or_Pong, unsigned int uiIndex) {
	if (Ping_or_Pong == PING) {
		if (uiIndex == 0) {
			voltageDiff = g_fPing_CurrentReading - g_fPong_LastReadingBeforeFinish;
		}
		else {
			voltageDiff = g_fPing_CurrentReading - g_fPing_LastReading;
		}
	}
	else {
		if (uiIndex == 0) {
			voltageDiff = g_fPong_CurrentReading - g_fPing_LastReadingBeforeFinish;
		}
		else {
			voltageDiff = g_fPong_CurrentReading - g_fPong_LastReading;
		}
	}

	// UART_PRINT("%f\n\r", voltageDiff);
	// UtilsDelay(5000000);

	if ((0.01 > voltageDiff) && (voltageDiff > -0.01)) {
		if (scoringArray[Index] == 0) {
			squareScore++;
			scoringArray[Index] = 1;
		}
		else { ; }
		// UART_PRINT("SQUARE WAVE!\n\r");
	}
	else if((voltageDiff > 0.5) || (voltageDiff < -0.5)) {
		if (scoringArray[Index] == 0) {
			squareScore++;
			scoringArray[Index] = 1;
		}
		else { ; }
		// UART_PRINT("SQUARE WAVE!\n\r");
	}
	else {
		if (scoringArray[Index] == 1) {
			squareScore--;
			scoringArray[Index] = 0;
		}
		else { ; }
		// UART_PRINT("Sine WAVE!\n\r");
	}

	Index = (Index + 1) % ARRAY_SIZE;

	// UART_PRINT("%d\n\r", Index);
}


//*****************************************************************************
//
//! Main function for uDMA Application 
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void main() {
	unsigned int uiIndex;
	unsigned char Inchar;

	//
	// Initialize LED
	// Switch all off
	//
	GPIOPinWrite(GPIOA1_BASE, 0x2, 0x0);
	GPIOPinWrite(GPIOA1_BASE, 0x4, 0x0);
	GPIOPinWrite(GPIOA1_BASE, 0x8, 0x0);

    //
    // Initailizing the board
    //
    BoardInit();

    //
    // Muxing for Enabling UART_TX and UART_RX and ADC Channel 3 (pin 60)
    //
    PinMuxConfig();
    
    //
    // Initialising the Terminal.
    //
    InitTerm();
    
    //
    // Display Welcome Message
    //
    DisplayBanner();
    UART_PRINT("Make sure analog signal in range 0 - 1.4V\n\r");
    UART_PRINT("Simple test program for Ping-Pong DMA;\r\n");
    UART_PRINT("Displays just a small portion of buffers\r\n");
    UART_PRINT("Press any key to start\r\n");
    UART_PRINT("Press 'q' key to stop\r\n");

    //
    // BARRIER, PRESS ANY KEY TO CONT.
    // Read the dummy Char
    //
    while(MAP_UARTCharsAvail(UARTA0_BASE) == false)  { ; }
    Inchar = MAP_UARTCharGetNonBlocking(UARTA0_BASE);

    //
    // Initialize DMA Transfer for ADC
    //
    InitAdcDma();

    //
    // Initialize scoreArray
    //
    int i = 0;

    for (i = 0; i < ARRAY_SIZE; i++) {
    	scoringArray[i] = 0;
    }

    //
    // Initialize Time Stamp for each ADC sample
    // Use the maximum 2^17 for wrap around value
    //
    ADCTimerConfig(ADC_BASE, 0x20000);

    //
    // Enables ADC Internal Timer
    //
    ADCTimerEnable(ADC_BASE);


    Inchar = ' ';
    do {
    	if (g_ucPingflag) {		// Ping Buffer Can now be printed!
    		g_ucPingflag=0;		// Reset Pingflag (just like clearing interrupt sig)

    		//
    		// Remember the last number in the buffer for next round of analysis
    		// Do it for both timer and voltage
    		//
    		g_fPing_LastReadingBeforeFinish = (((float)((g_ulPing[BUFFER_SIZE - 1] >> 2 ) & 0x0FFF))*1.467)/4096;
    		g_uiPing_LastTimeStampBeforeFinish = (unsigned int) g_ulPing[BUFFER_SIZE - 1] >> 14 & 0x3FFFF;

    		// UART_PRINT("Ping: \r\n");
    		uiIndex=0;			// Restart from the Start of the buffer for receiving data
/*    		while (uiIndex < 24) {	// Only Prints the first 24 items in the Ping buffer
    			UART_PRINT("%f\t",(((float)((g_ulPing[uiIndex] >> 2 ) & 0x0FFF))*1.467)/4096);

    			//
    			// [30:14]: time stamp
    			//
    			UART_PRINT("%05x\t",( ( g_ulPing[uiIndex] >> 14 ) & 0x3FFFF ) );

    			uiIndex++;
    			if ((uiIndex % 4)==0)	// Formatting, putting a \n\r for every 4 items
    				UART_PRINT("\n\r");
    		}
*/

    		//
    		// Frequency Analysis for Ping, starting from the last reading of pong
    		// Only Care about Rising Edge
   			//
   			for (uiIndex = 0; uiIndex < BUFFER_SIZE; uiIndex++ ) {
       			if (uiIndex % TIMER_ANALYSIS_PRECISION == 0) {
       				//
       				// Time Update
       				//
       				g_uiPing_LastTimeStamp = g_uiPing_CurrentTimeStamp;
       				g_uiPing_CurrentTimeStamp = (unsigned int) g_ulPing[uiIndex] >> 14 & 0x3FFFF;

       				//
       				// If there is a switch of buffer!
       				//
       				if ( uiIndex == 0 ) { g_uiPing_LastTimeStamp = g_uiPong_LastTimeStampBeforeFinish; }

       				//
       				// Increment Counter
       				//
       				if (g_uiPing_CurrentTimeStamp > g_uiPing_LastTimeStamp) { TimeoutCounter++; }
       			}

   				if (uiIndex % VOLTAGE_ANALYSIS_PRECISION == 0) {
   	   				//
   	   				// Voltage Update
   	   				//
   	   				g_fPing_LastReading = g_fPing_CurrentReading;
   	   				g_fPing_CurrentReading = (((float)((g_ulPing[uiIndex] >> 2 ) & 0x0FFF))*1.467)/4096;

   	   				updateTimeDiff(PING, uiIndex);

   	   				if (squareScore < 1500) {
   	   					// UART_PRINT("Sine Wave\n\r");
   	   					if ( GPIOPinRead(GPIOA1_BASE, 0x2) != 0 ) {
   	   						GPIOPinWrite(GPIOA1_BASE, 0x2, 0);
   	   					}

   	   					GPIOPinWrite(GPIOA1_BASE, 0x8, 0x8);
   	   				}
   	   				else {
   	   					// UART_PRINT("Square Wave\n\r");
   	   					if ( GPIOPinRead(GPIOA1_BASE, 0x8) != 0 ) {
   	   						GPIOPinWrite(GPIOA1_BASE, 0x8, 0);
   	   					}

   	   					GPIOPinWrite(GPIOA1_BASE, 0x2, 0x2);
   	   				}

   	   				// UART_PRINT("%d\n\r", squareScore);

   	   				if ( (uiIndex == 0) && (g_fPong_LastReadingBeforeFinish < Period_Detection) ) {
   	   					if ( g_fPing_CurrentReading > Period_Detection ) {
   	   						updatePeriod(PING);
   	   						// UART_PRINT("Period Detected!\n\r");
   	   						// UART_PRINT("Frequency: %f\n\r", g_ffrequency);
   	   					}
   	   				}

   	   				else if ( (uiIndex != 0) && (g_fPing_LastReading < Period_Detection) ) {
   	   					if ( g_fPing_CurrentReading > Period_Detection ) {
   	   						updatePeriod(PING);
   	   						// UART_PRINT("Period Detected!\n\r");
   	   						UART_PRINT("Frequency: %f\n\r", g_ffrequency);
   	   					}
   	   				}
   				}
   			}
    	}


    	//
    	// The functions for Pong are essentially the same as Ping
    	//
    	if (g_ucPongflag) {
    		g_ucPongflag=0;

    		//
    		// Remember the last number in the buffer for next round of analysis
    		// Do it for both timer and voltage
    		//
    		g_fPong_LastReadingBeforeFinish = (((float)((g_ulPong[BUFFER_SIZE - 1] >> 2 ) & 0x0FFF))*1.467)/4096;
    		g_uiPong_LastTimeStampBeforeFinish = (unsigned int) g_ulPong[BUFFER_SIZE - 1] >> 14 & 0x3FFFF;

    		// UART_PRINT("Pong: \r\n");
    		uiIndex=0;
/*    		while (uiIndex < 24) {
    			UART_PRINT("%f\t",(((float)((g_ulPong[uiIndex] >> 2 ) & 0x0FFF))*1.467)/4096);

				//
    			// [30:14]: time stamp
    			//
    			UART_PRINT("%05x\t",( ( g_ulPong[uiIndex] >> 14 ) & 0x3FFFF ) );

    			uiIndex++;
    			if ((uiIndex % 4)==0)
    				UART_PRINT("\n\r");
    		}
*/
        	//
        	// Frequency Analysis for Pong, starting from the last reading of ping
        	// Only Care about Rising Edge
       		//
       		for (uiIndex = 0; uiIndex < BUFFER_SIZE; uiIndex++) {
       			if (uiIndex % TIMER_ANALYSIS_PRECISION == 0) {
       				//
       				// Time Update
       				//
       				g_uiPong_LastTimeStamp = g_uiPong_CurrentTimeStamp;
       				g_uiPong_CurrentTimeStamp = (unsigned int) g_ulPong[uiIndex] >> 14 & 0x3FFFF;

       				//
       				// If there is a switch of buffer!
       				//
       				if ( uiIndex == 0 ) { g_uiPong_LastTimeStamp = g_uiPing_LastTimeStampBeforeFinish; }

       				//
       				// Increment Counter
       				//
       				if (g_uiPong_CurrentTimeStamp > g_uiPong_LastTimeStamp) { TimeoutCounter++; }
       			}

       			if (uiIndex % VOLTAGE_ANALYSIS_PRECISION == 0) {
       				//
       				// Voltage Update
       				//
       				g_fPong_LastReading = g_fPong_CurrentReading;
       				g_fPong_CurrentReading = (((float)((g_ulPong[uiIndex] >> 2 ) & 0x0FFF))*1.467)/4096;

       				updateTimeDiff(PONG, uiIndex);

       				if ( (uiIndex == 0) && (g_fPing_LastReadingBeforeFinish < Period_Detection) ) {
       					if ( g_fPong_CurrentReading > Period_Detection ) {
       						updatePeriod(PONG);
   	   						// UART_PRINT("Period Detected!\n\r");
   	   						// UART_PRINT("Frequency: %f\n\r", g_ffrequency);
       					}
       				}

       				else if ( (uiIndex != 0) && ( g_fPong_LastReading < Period_Detection ) ) {
       					if ( g_fPong_CurrentReading > Period_Detection ) {
       						updatePeriod(PONG);
   	   						// UART_PRINT("Period Detected!\n\r");
   	   						// UART_PRINT("Frequency: %f\n\r", g_ffrequency);
       					}
       				}
       			}
       		}
    	}

    	//
    	// Keep Reading from Console Terminal for quit signal
    	//
        if (MAP_UARTCharsAvail(UARTA0_BASE))  {
        	Inchar = MAP_UARTCharGetNonBlocking(UARTA0_BASE);
        }
   } while (Inchar != 'q');	// Input Char 'q' for Quitting

    //
    // End Message
    // ADC and DMA disable
    //
    UART_PRINT("Program stopped\r\n");
	ADCDMADisable(ADC_BASE, ADC_CH_3);
	ADCChannelDisable(ADC_BASE, ADC_CH_3);
}


//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************




