//*****************************************************************************
//
// Application Name     - Lab0
// Group Member 1: Weidong Wu
// Group Member 2: Chen Chen
//
//*****************************************************************************

// Standard includes
#include <stdio.h>

// Driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "interrupt.h"
#include "hw_apps_rcm.h"
#include "prcm.h"
#include "rom.h"
#include "rom_map.h"
#include "prcm.h"
#include "gpio.h"
#include "utils.h"

// Common interface includes
#include "gpio_if.h"

// New PinMux Header
#include "pin_mux_config.h"

// uart headers for printing to console
 #include "uart.h"
 #include "uart_if.h"

#define APPLICATION_VERSION     "1.1.1"

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
#if defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif

// Globals for Moore State Machine
int state = 0;
long SW3State = 0;
long SW4State = 0;
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************


//*****************************************************************************
//                      LOCAL FUNCTION PROTOTYPES                           
//*****************************************************************************
void S0toS2();	// handles the while loop for state 0 to state 2
void S3();		// handles the goal state
void S4();		// handles the error state
void getSwitchStates();	// poll the switch and update global state variables
void S0StateChange();	// state switching at state 0
void S1StateChange();	// state switching at state 1
void S2StateChange();	// state switching at state 2
static void BoardInit(void);

//*****************************************************************************
//                      LOCAL FUNCTION DEFINITIONS                         
//*****************************************************************************

//*****************************************************************************
//
//! Configures the pins as GPIOs and peroidically toggles the lines
//!
//! \param None
//! 
//! This function  
//!    1. Configures 3 lines connected to LEDs as GPIO
//!    2. Sets up the GPIO pins as output
//!    3. Periodically toggles each LED one by one by toggling the GPIO line
//!
//! \return None
//
//*****************************************************************************
void S0toS2()
{
    // Deal with polling and states between 0 and 2
    //
    // Toggle the lines initially to turn off the LEDs.
    // The values driven are as required by the LEDs on the LP.
    //
    GPIO_IF_LedOff(MCU_ALL_LED_IND);
    while(state >= 0 && state < 3)
    {
    	getSwitchStates();
    	switch (state) {
    	case 0:
    		 GPIO_IF_LedOn(MCU_GREEN_LED_GPIO);
    		 S0StateChange();
    		 break;
    	case 1:
    		GPIO_IF_LedOn(MCU_ORANGE_LED_GPIO);
    		S1StateChange();
    		break;
    	default:	// case 2
    		GPIO_IF_LedOn(MCU_GREEN_LED_GPIO);
    		GPIO_IF_LedOn(MCU_ORANGE_LED_GPIO);
    		S2StateChange();
    	}
    }

}

void S3() {
    GPIO_IF_LedOn(MCU_ALL_LED_IND);
    while (1) {
    	GPIOPinWrite(GPIOA3_BASE, GPIO_PIN_4, 0);	// Pin 18 low
    	MAP_UtilsDelay(1350000);	// Delay by about 200 ms
    	GPIOPinWrite(GPIOA3_BASE, GPIO_PIN_4, 16);	// Pin 18 high
    	// the value for assignment must be a ucVal!
    	// Binary mask: Pin 4 the position 4 = 16(dec)
    	MAP_UtilsDelay(1350000);	// Delay by about 200 ms
    }
}

void S4() {
    GPIO_IF_LedOff(MCU_ALL_LED_IND);
    GPIO_IF_LedOn(MCU_RED_LED_GPIO);
}

void getSwitchStates() {
	SW3State = GPIOPinRead(GPIOA1_BASE, GPIO_PIN_5);
	SW4State = GPIOPinRead(GPIOA2_BASE, GPIO_PIN_6);
}

// cannot simply use == 1, because SWstates are actually masked long
// must use != 0
//
// The following 3 functions are basically determining the
// state changes by testing the different switch combinations.
//
// Only GPIO_IF_LedOff(MCU_ALL_LED_IND); if there is a change in state.
void S0StateChange() {
	if ( (SW3State == 0) && (SW4State == 0) ) state = 0;
	else if ( (SW3State == 0) && (SW4State != 0) ) {
		state = 1;
		GPIO_IF_LedOff(MCU_ALL_LED_IND);
	}
	else {
		state = 4;
		GPIO_IF_LedOff(MCU_ALL_LED_IND);
	}
}

void S1StateChange() {
	if ( (SW3State == 0) && (SW4State != 0) ) state = 1;
	else if ( (SW3State != 0 && SW4State != 0) ) {
		state = 2;
		GPIO_IF_LedOff(MCU_ALL_LED_IND);
	}
	else {
		state = 4;
		GPIO_IF_LedOff(MCU_ALL_LED_IND);
	}
}

void S2StateChange() {
	if ( (SW3State != 0) && (SW4State == 0) ) {
		state = 3;
		GPIO_IF_LedOff(MCU_ALL_LED_IND);
	}
	else if ( (SW3State != 0) && (SW4State != 0) ) state = 2;
	else {
		state = 4;
		GPIO_IF_LedOff(MCU_ALL_LED_IND);
	}
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
//****************************************************************************
//
//! Main function
//!
//! \param none
//! 
//! This function  
//!    1. Invokes the S0toS2 task
//!
//! \return None.
//
//****************************************************************************
int
main()
{
    //
    // Initialize Board configurations
    //
    BoardInit();
    //
    // Configurate pins according to the new header
    //
    PinMuxConfig();
    //
    // Initialising the Terminal.
    //
    InitTerm();
    //
    // Clearing the Terminal.
    //
    ClearTerm();
    // Print Welcome Messages: Usage instruction
    Message("\t\t****************************************************\n\r");
    Message("\t\t\t        EEC 172 Lab0        \n\r");
    Message("\t\t Please Follow the Moore State  \n\r");
    Message("\t\t Diagram in the Lab0 manual to   \n\r");
    Message("\t\t change between different states.  \n\r");
    Message("\t\t Press Reset if necessary.  \n\r");
    Message("\t\t ****************************************************\n\r");
    Message("\n\n\n\r");

    // Start of the Moore State Machine Part of the program
    state = 0;

    GPIO_IF_LedConfigure(LED1|LED2|LED3);

    GPIO_IF_LedOff(MCU_ALL_LED_IND);
    
	GPIOPinWrite(GPIOA3_BASE, GPIO_PIN_4, 0);	// Pin 18 low

    //
    // Start the S0toS2
    //
    S0toS2();
    // If state has been changed to 3 or 4
    // Stop polling and:
    if (state == 3) S3();
    if (state == 4) S4();
    return 0;
}

//*****************************************************************************
// End of Lab0
//
//*****************************************************************************

