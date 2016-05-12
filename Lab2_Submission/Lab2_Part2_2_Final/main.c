// EEC172 Lab2
// Weidong Wu
// Chen Chen

// Standard includes
#include <string.h>
// cstring = char array
//
// chat string[]; string would be referring to the string

// Driverlib includes
#include "hw_types.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "hw_ints.h"
#include "spi.h"
#include "gpio.h"
#include "rom.h"
#include "rom_map.h"
#include "utils.h"
#include "prcm.h"
#include "uart.h"
#include "interrupt.h"
#include "timer.h"
#include "pin.h"

// Common Interfaces and PinMux includes
#include "uart_if.h"
#include "i2c_if.h"
// Also need to copy i2c_if.c to the project or else will give MACRO definition error
// ASK TA Why is that?! What are the differences between the one from i2c_demo and include folder?
#include "pin_mux_config.h"
// PLEASE remember to set the pins to work with UART and I2C communication!

// OLED Specilized includes
#include "Adafruit_SSD1351.h"
#include "Adafruit_GFX.h"
#include "glcdfont.h"

///////////// Some Externs and Globals ////////////////
extern int cursor_x;
extern int cursor_y;

float p = 3.1415926;

char NEWLINE = '\n';
char RETURN = '\r';

//*****************************************************************************
//                      MACRO DEFINITIONS
//*****************************************************************************
#define APP_NAME                "Lab2: Remote Control Text Messanger"
#define FOREVER                 1
#define FAILURE                 -1
#define SUCCESS                 0
#define UART_PRINT              Report
#define SPI_IF_BIT_RATE			100000
#define TR_BUFF_SIZE			100

#define CONSOLE					UARTA0_BASE
#define THEOTHERCC3200			UARTA1_BASE
#define THEOTHERCC3200_PERIPH	PRCM_UARTA1

// Color Definitions
#define	BLACK           0x0000
#define	BLUE            0x001F
#define	GREEN           0x07E0
#define CYAN            0x07FF
#define	RED             0xF800
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF

// Timer Definitions
#define TIMER_FREQ				80000000

#define ZERO					10045623
#define ONE						10063983
#define TWO						10074183
#define THREE					10090503
#define FOUR					10072143
#define FIVE					10066023
#define SIX						10082343
#define SEVEN					10061943
#define EIGHT					10070103
#define NINE					10086423
#define ENTER					10031343
#define MUTE					10051743

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************

// Globals used to track the number of calls to the periodic and edge-timer interrupt handlers.
static volatile unsigned int g_ulA3A_IR_IntCount;
static volatile unsigned long g_ulA3B_Periodic_IntCount = 0;

// Global to store the current IR Transmission Code, each buttom has a unique IR Code to identify with
static volatile unsigned int g_uiIR_Code;

// Global duple to store the current editing/typing character INDEX,
// which can be used to retrieve the actual char from a 2-dimentional array
// Will be pushed to the buffer once it is confirmed
// Might be able to optimize
// [0] is the number of the last buttom pressed within the timeout cycle
// 0 - 9
// [1] is the number of the last character index referred to within the timeout cycle
// can be: 0; 0 - 1; 0 - 3; 0 - 4
// -1 would be a sign of timeout in both positions
static volatile int g_iCurrentChar[2] = {-1, -1};

// Char Array
// Should use an array of pointers to save space
// Not really significant though
// Ask: How to initialize in at global level instead of in main?
static unsigned char g_ucCharArray[10][5];

// Button Cycle Information
static int g_iButtonCycle[10] = {2, 1, 4, 4, 4, 4, 4, 5, 4, 5};

// String that holds the current Char being printed to OLED
static char g_cCurrentChar[];

// Globals for TimerA3A Sampling
static unsigned long g_ulSamples[2];
static unsigned long g_ulInterval;

// Global Text Buffer for UART Communication
// 21 x 7
static unsigned char ucTextBuff[147];
static int iTextBuffIndex = 0;

// cursor information for receiving text
static int cursor_x_r = 0;
static int cursor_y_r = 64;

#if defined(ccs) || defined(gcc)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************

//*****************************************************************************
//
//! Initialize Char Array
//
//*****************************************************************************
void InitCharArray (void) {
    g_ucCharArray[0][0] = ' ';
    g_ucCharArray[0][1] = '0';

    g_ucCharArray[1][0] = '1';

    g_ucCharArray[2][0] = 'a';
    g_ucCharArray[2][1] = 'b';
    g_ucCharArray[2][2] = 'c';
    g_ucCharArray[2][3] = '2';

    g_ucCharArray[3][0] = 'd';
    g_ucCharArray[3][1] = 'e';
    g_ucCharArray[3][2] = 'f';
    g_ucCharArray[3][3] = '3';

    g_ucCharArray[4][0] = 'g';
    g_ucCharArray[4][1] = 'h';
    g_ucCharArray[4][2] = 'i';
    g_ucCharArray[4][3] = '4';

    g_ucCharArray[5][0] = 'j';
    g_ucCharArray[5][1] = 'k';
    g_ucCharArray[5][2] = 'l';
    g_ucCharArray[5][3] = '5';

    g_ucCharArray[6][0] = 'm';
    g_ucCharArray[6][1] = 'n';
    g_ucCharArray[6][2] = 'o';
    g_ucCharArray[6][3] = '6';

    g_ucCharArray[7][0] = 'p';
    g_ucCharArray[7][1] = 'q';
    g_ucCharArray[7][2] = 'r';
    g_ucCharArray[7][3] = 's';
    g_ucCharArray[7][4] = '7';

    g_ucCharArray[8][0] = 't';
    g_ucCharArray[8][1] = 'u';
    g_ucCharArray[8][2] = 'v';
    g_ucCharArray[8][3] = '8';

    g_ucCharArray[9][0] = 'w';
    g_ucCharArray[9][1] = 'x';
    g_ucCharArray[9][2] = 'y';
    g_ucCharArray[9][3] = 'z';
    g_ucCharArray[9][4] = '9';
}

//*****************************************************************************
//
//! Push the Current/ Pop the last Char to/from the Text Buffer for sending
//
//*****************************************************************************

static void pushToTextBuff(void) {
	ucTextBuff[iTextBuffIndex] = g_cCurrentChar[0];
	iTextBuffIndex++;
}

static void popFromTextBuff(void) {	iTextBuffIndex--; }

static void sendMsgUART(void) {
	int i = 0;
	while (i < iTextBuffIndex) {
		while (UARTSpaceAvail(THEOTHERCC3200)) {
			MAP_UARTCharPutNonBlocking(CONSOLE, ucTextBuff[i]);
			MAP_UARTCharPutNonBlocking(THEOTHERCC3200, ucTextBuff[i]);
			i++;
			MAP_UtilsDelay(5000000/5);	// Software work around to RTS/CTS hardware?
										// To make sure that things are sent out in correct order!
			if (i == iTextBuffIndex) { break; }
		}
	}
	iTextBuffIndex = 0; // Flash Buffer

//	UART_PRINT("\n\r Message Sent \n\n\r");	This line of code will ALWAYS
//  throw away the next char printed to CONSOLE!!!!
}

//*****************************************************************************
//
//! Printing to and Deleting from OLED for Typing
//! Allow Maximum 7 rows of characters at the top of the screen
//
//*****************************************************************************
void OLED_PrintSingleChar(char s[]) {
	//
	// Cursor Edge Test
	//
	if (cursor_x >= 126){
		if (cursor_y >= 48) { return; }
		else { cursor_x = 0; cursor_y += 8; }
	}

	Outstr(s);
}

void OLED_Delete(void) {
	//
	// Cursor Backspace
	//
	if (cursor_x <= 0) {
		if (cursor_y <= 0) { return; }
		else { cursor_x = 120; cursor_y -= 8; }
	}
	else { cursor_x -= 6; }

	//
	// Delete Character
	//
	fillRect(cursor_x, cursor_y, 6, 8, BLACK);
}

//
// Not Used: Just in Case
//
void OLED_CursorNext(void) {
	if (cursor_x >= 126){
		if (cursor_y >= 48) { return; }
		else { cursor_x = 0; cursor_y += 8; }
	}

	cursor_x += 6;
}

//*****************************************************************************
//
//! Printing to OLED for Received Text (starts from row 64)
//! Allow Maximum 7 rows of characters at the bottom of the screen
//
//*****************************************************************************

void OLED_PrintSingleChar_Receive(char s[]) {
	int cursor_x_buffer;
	int cursor_y_buffer;

	//
	// Cursor Edge Test
	//
	if (cursor_x_r >= 126){
		if (cursor_y_r >= 112) {
			cursor_x_r = 0;
			cursor_y_r = 64;

			fillRect(cursor_x_r, cursor_y_r, 126, 56, BLACK);
		}
		else { cursor_x_r = 0; cursor_y_r += 8; }
	}

	//
	// Use cursor_x and cursor_y to finish the printing
	//
	cursor_x_buffer = cursor_x;
	cursor_y_buffer = cursor_y;

	cursor_x = cursor_x_r;
	cursor_y = cursor_y_r;

	fillRect(cursor_x, cursor_y, 6, 8, BLACK);
	Outstr(s);

	//
	// Recover cursor_x & cursor_y
	// Update cursor_x_r & cursor_y_r
	//
	cursor_x = cursor_x_buffer;
	cursor_y = cursor_y_buffer;

	cursor_x_r += 6;
}

//*****************************************************************************
//
//! CONSOLE Separation for better Readability
//
//*****************************************************************************

void
separation(void) {
	MAP_UARTCharPutNonBlocking(CONSOLE, NEWLINE);
	MAP_UARTCharPutNonBlocking(CONSOLE, NEWLINE);
	MAP_UARTCharPutNonBlocking(CONSOLE, RETURN);
}

//*****************************************************************************
//
//! Slave Signal Handlers to deal with typing and buffering
//! when different buttons are pressed
//
//*****************************************************************************

void signalHandlerForNum_helper(int buttonPressed) {
//	char c;	// for testing, echo to console

//	c = buttonPressed + '0';
//	UART_PRINT("%c\n\r", c);

	if (g_iCurrentChar[0] == buttonPressed) {
		//
		// Ask: Why Can't just g_iCurrentChar[1]++?
		//
		g_iCurrentChar[1] += 1;
		g_iCurrentChar[1] = g_iCurrentChar[1] % g_iButtonCycle[buttonPressed];
		//
		// Update OLED
		//
		OLED_Delete();
		g_cCurrentChar[0] = g_ucCharArray[g_iCurrentChar[0]][g_iCurrentChar[1]];
		OLED_PrintSingleChar(g_cCurrentChar);
	}
	else if (g_iCurrentChar[0] == -1) {
		g_iCurrentChar[0] = buttonPressed;
		g_iCurrentChar[1] = 0;
		//
		// Type the first Char in this cycle to OLED
		//
		g_cCurrentChar[0] = g_ucCharArray[g_iCurrentChar[0]][g_iCurrentChar[1]];
		OLED_PrintSingleChar(g_cCurrentChar);
	}
	else {
		pushToTextBuff();
		g_iCurrentChar[0] = buttonPressed;
		g_iCurrentChar[1] = 0;
		//
		// Type the first Char in this cycle to OLED
		//
		g_cCurrentChar[0] = g_ucCharArray[g_iCurrentChar[0]][g_iCurrentChar[1]];
		OLED_PrintSingleChar(g_cCurrentChar);
	}
}

void signalHandlerENTER(void) {
//	UART_PRINT("ENTER\n\r");
	if (g_iCurrentChar[0] != -1) {
		pushToTextBuff();
		g_iCurrentChar[0] = -1;
	}
	else {
		separation();
		sendMsgUART();
		separation();
		setCursor(0, 0);
		fillRect(cursor_x, cursor_y, 126, 56, BLACK);
	}
}

void signalHandlerMUTE(void) {
//	UART_PRINT("MUTE\n\r");
	OLED_Delete();
	popFromTextBuff();

}

//*****************************************************************************
//
//! Master Signal Handler to deal with typing and buffering
//
//*****************************************************************************
void signalHandler(unsigned int g_uiIR_Code) {
	switch (g_uiIR_Code) {
	case ZERO:
		signalHandlerForNum_helper(0);
		break;
	case ONE:
		signalHandlerForNum_helper(1);
		break;
	case TWO:
		signalHandlerForNum_helper(2);
		break;
	case THREE:
		signalHandlerForNum_helper(3);
		break;
	case FOUR:
		signalHandlerForNum_helper(4);
		break;
	case FIVE:
		signalHandlerForNum_helper(5);
		break;
	case SIX:
		signalHandlerForNum_helper(6);
		break;
	case SEVEN:
		signalHandlerForNum_helper(7);
		break;
	case EIGHT:
		signalHandlerForNum_helper(8);
		break;
	case NINE:
		signalHandlerForNum_helper(9);
		break;
	case ENTER:
		signalHandlerENTER();
		break;
	case MUTE:
		signalHandlerMUTE();
		break;
	default:
		UART_PRINT("!!!WRONG SIGNAL DETECTED!!!\n\r");
	}
}

//*****************************************************************************
//
//! Timer A interrupt handler for edge-timer and case handler
//
//*****************************************************************************
static void TimerAIntHandler()
{
    //
    // Clear the interrupt
    //
	MAP_TimerIntClear(TIMERA3_BASE,TIMER_CAPA_EVENT);

    //
    // Get the samples and compute the time elapsed
    //
    g_ulSamples[0] = g_ulSamples[1];
    g_ulSamples[1] = MAP_TimerValueGet(TIMERA3_BASE,TIMER_A);
    g_ulInterval = g_ulSamples[0] + 0x8fffff * g_ulA3B_Periodic_IntCount - g_ulSamples[1];

    //
    // Clear Periodic Count
    // Ask: Is there any other ways of doing this other than a global counter?
    //
    g_ulA3B_Periodic_IntCount = 0;

    //
    // Detect Starting Signal and Clear IR Transmission Count & Code
    //
    if (400000 < g_ulInterval && g_ulInterval < 410000) {
    	g_ulA3A_IR_IntCount = 0;
    	g_uiIR_Code = 0;
    }

    //
    // Case of 0
    //
    else if (80000 < g_ulInterval && g_ulInterval < 100000) {
    	g_ulA3A_IR_IntCount++;
    }

    //
    // Case of 1
    //
    else if (170000 < g_ulInterval && g_ulInterval < 190000) {
    	g_uiIR_Code += (1 << (31 - g_ulA3A_IR_IntCount));
    	g_ulA3A_IR_IntCount++;
    }

    //
    // Case of End Signal
    //
    else if (4100000 < g_ulInterval && g_ulInterval < 4120000) { signalHandler(g_uiIR_Code); }

    else {
    	//
    	// Other types of unimportant signals
        // UART_PRINT the Time Interval between 2 down signals
        //
    	// UART_PRINT("%lu ",g_ulInterval);
    }
}

//*****************************************************************************
//
//! Timer B interrupt handler for periodic timer
//
//*****************************************************************************
static void TimerBIntHandler()
{
//	unsigned long ulStatus;

    //
    // Clear the interrupt and get Int Status
    //
//	ulStatus = MAP_TimerIntStatus(TIMERA3_BASE, true);
    MAP_TimerIntClear(TIMERA3_BASE,TIMER_TIMB_TIMEOUT);

/*
    if (ulStatus & TIMER_CAPA_EVENT) {	// If TimerA is interrupted
        //
        // Previous time + 1; Effectively final time -1
        // Accomodate the wasted cycle of going into TimerB Int
    	//
    	g_ulSamples[1]++;

        //
        // UART_PRINT
        //
        UART_PRINT("Timer B Performs Accomodataion!!!!!");
    }
*/
    //
    // Increment Periodic Counter
    //
    g_ulA3B_Periodic_IntCount++;

    //
    // UART_PRINT the Time Interval between 2 down signals
    //
    // UART_PRINT("Timeout Miss: %d ",g_ulA3B_Periodic_IntCount);
}

//*****************************************************************************
//
//! interrupt handlers for CONSOLE (Keyboard Echo) and
//! THEOTHERCC3200 (Receiving Msg from the other board)
//
//*****************************************************************************

static void UARTIntHandler_CONSOLE(void) {
	unsigned long ulStatus;
	char string[1];

	ulStatus = MAP_UARTIntStatus(CONSOLE, true);
	MAP_UARTIntClear(CONSOLE, ulStatus);	// clear EVERY Int

	while (UARTCharsAvail(CONSOLE)) {
		string[0] = UARTCharGetNonBlocking(CONSOLE);
		MAP_UARTCharPut(CONSOLE, string[0]);	// Echo Terminal Input to Terminal
		OLED_PrintSingleChar_Receive(string);	// Update OLED like receiving msg
	}
}

static void UARTIntHandler_THEOTHERCC3200(void) {
	unsigned long ulStatus;
	char string[1];

	ulStatus = MAP_UARTIntStatus(THEOTHERCC3200, true);
	MAP_UARTIntClear(THEOTHERCC3200, ulStatus);	// clear EVERY Int

	while (MAP_UARTCharsAvail(THEOTHERCC3200)) {
		string[0] = MAP_UARTCharGetNonBlocking(THEOTHERCC3200);
		MAP_UARTCharPutNonBlocking(CONSOLE, string[0]);	// Display Msg on CONSOLE
		OLED_PrintSingleChar_Receive(string);	// Display Msg on OLED
	}
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
DisplayBanner(char * AppName)
{

    Report("\n\n\n\r");
    Report("\t\t *************************************************\n\r");
    Report("\t\t\t  CC3200 %s \n\r", AppName);
    Report("\t\t *************************************************\n\r");
    Report("\n\n\n\r");
}

//*****************************************************************************
//
//! Display the buffer contents over I2C
//!
//! \param  pucDataBuf is the pointer to the data store to be displayed
//! \param  ucLen is the length of the data to be displayed
//!
//! \return none
//!
//*****************************************************************************
void
DisplayBuffer(unsigned char *pucDataBuf, unsigned char ucLen)
{
    unsigned char ucBufIndx = 0;
    UART_PRINT("Read contents");
    UART_PRINT("\n\r");
    while(ucBufIndx < ucLen)
    {
        UART_PRINT(" 0x%x, ", pucDataBuf[ucBufIndx]);
        ucBufIndx++;
        if((ucBufIndx % 8) == 0)
        {
            UART_PRINT("\n\r");
        }
    }
    UART_PRINT("\n\r");
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
void
BoardInit(void)
{
/* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
  //
  // Set vector table base
  //
#if defined(ccs) || defined(gcc)
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


//*****************************************************************************
//
//! Main  Function
//
//*****************************************************************************
int main()
{
    //
    // Initialize Board configurations
    //
    BoardInit();

    //
    // PinMux for UART, SPI, I2C and Timer
    //
    PinMuxConfig();

    //
    // Enable the SPI module clock
    //
    MAP_PRCMPeripheralClkEnable(PRCM_GSPI,PRCM_RUN_MODE_CLK);

    //
    // Reset the SPI peripheral
    //
    MAP_PRCMPeripheralReset(PRCM_GSPI);

    //
    // Reset SPI
    //
    MAP_SPIReset(GSPI_BASE);

    //
    // Configure SPI interface
    //
    MAP_SPIConfigSetExpClk(GSPI_BASE,MAP_PRCMPeripheralClockGet(PRCM_GSPI),
                        SPI_IF_BIT_RATE,SPI_MODE_MASTER,SPI_SUB_MODE_3,	// Must be 3: Polarity = Phase = 1
                        (SPI_SW_CTRL_CS |
                        SPI_4PIN_MODE |
                        SPI_TURBO_OFF |
						SPI_CS_ACTIVELOW |	// LOW!!!!
                        SPI_WL_8));

    //
    // Enable SPI for communication
    //
    MAP_SPIEnable(GSPI_BASE);


    // initialize OLED
    Adafruit_Init();

    //
    // Register interrupt handler for UART CONSOLE
    //
    MAP_UARTIntRegister(CONSOLE,UARTIntHandler_CONSOLE);

    //
    // Register interrupt handler for UART THEOTHERCC3200
    //
    MAP_UARTIntRegister(THEOTHERCC3200,UARTIntHandler_THEOTHERCC3200);

    //
    // Enable RX and RT interrupts for uartA0
    //
    MAP_UARTIntEnable(CONSOLE,UART_INT_RX | UART_INT_RT);

    //
    // Enable RX and RT interrupts for uartA1
    //
    MAP_UARTIntEnable(THEOTHERCC3200,UART_INT_RX | UART_INT_RT);

    //
    // Configure the UART CONSOLE Tx and Rx FIFO level to 1/8 i.e 2 characters
    //
    MAP_UARTFIFOLevelSet(CONSOLE,UART_FIFO_TX4_8,UART_FIFO_RX4_8);

    //
    // Configure the UART THEOTHERCC3200 Tx and Rx FIFO level to 1/8 i.e 2 characters
    //
    MAP_UARTFIFOLevelSet(THEOTHERCC3200,UART_FIFO_TX4_8,UART_FIFO_RX4_8);

    MAP_UARTFIFOEnable(CONSOLE);
    MAP_UARTFIFOEnable(THEOTHERCC3200);

    MAP_UARTEnable(CONSOLE);
	MAP_UARTEnable(THEOTHERCC3200);

    //
    // Configuring UART CONSOLE
    //
    InitTerm();

    //
    // Initialize UART THEOTHERCC3200
    //
    MAP_UARTConfigSetExpClk(THEOTHERCC3200,MAP_PRCMPeripheralClockGet(THEOTHERCC3200_PERIPH),
                      UART_BAUD_RATE, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                       UART_CONFIG_PAR_NONE));

    //
    // Clearing the Terminal.
    //
    ClearTerm();

    //
    // I2C Init
    //
    I2C_IF_Open(I2C_MASTER_MODE_FST);

    //
    // Enable pull down
    //
    MAP_PinConfigSet(PIN_61,PIN_TYPE_STD_PD,PIN_STRENGTH_6MA);


    //
    // Register timer interrupt hander, both A and B
    //
    MAP_TimerIntRegister(TIMERA3_BASE,TIMER_A,TimerAIntHandler);
    MAP_TimerIntRegister(TIMERA3_BASE,TIMER_B,TimerBIntHandler);

    //
    // Set the appropriate interrupt priorities.
    //
//    MAP_IntPriorityGroupingSet(3);
//    MAP_IntPrioritySet(INT_TIMERA3A, INT_PRIORITY_LVL_0);
//    MAP_IntPrioritySet(INT_TIMERA3B, INT_PRIORITY_LVL_0);
//    MAP_IntPrioritySet(INT_UARTA1, INT_PRIORITY_LVL_0);
//    MAP_IntPrioritySet(INT_UARTA0, INT_PRIORITY_LVL_0);

    //
    // Configure the timer A in edge count mode
    // Configure the timer B in periodic mode
    //
    MAP_TimerConfigure(TIMERA3_BASE, (TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_CAP_TIME | TIMER_CFG_B_PERIODIC));

    //
    // Set the detection edge
    //
    MAP_TimerControlEvent(TIMERA3_BASE,TIMER_A,TIMER_EVENT_POS_EDGE);

    //
    // Set the reload value for both A and B
    //
    MAP_TimerLoadSet(TIMERA3_BASE,TIMER_A,0xffff);
    MAP_TimerLoadSet(TIMERA3_BASE,TIMER_B,0xffff);

    //
    // Set the prescale value for both A and B
    //
    MAP_TimerPrescaleSet(TIMERA3_BASE,TIMER_A,0x8f);
    MAP_TimerPrescaleSet(TIMERA3_BASE,TIMER_B,0x8f);

    //
    // Enable capture event interrupt for Timer A
    // Enable timeout event interrupt for Timer B
    //
    MAP_TimerIntEnable(TIMERA3_BASE,TIMER_CAPA_EVENT | TIMER_TIMB_TIMEOUT);


    //
    // Enable Timer A and B
    //
    MAP_TimerEnable(TIMERA3_BASE,TIMER_BOTH);

    //
    // Display Application Banner
    //
    DisplayBanner(APP_NAME);

    //
    // Initialize Cursor Position for Typing
    // Clear Screen
    //
    setCursor(0, 0);
    fillScreen(BLACK);

    //
    // Initialize Global Char Array for Typing
    //
    InitCharArray();

    while(1)
    {
        //
        // Infinite loop
        //
    }
}

///////////// Notes to Self ///////////////
// 1. Don't mess up & with &&, | with ||!

// 2. Proper way to display and modify
/*
        //
        // Print glcdfont.c Character-Set
        //
    	fillScreen(BLACK);
    	setCursor(0, 0);
        int i,j;
        int count = 0;
        for(i=0; i < 127; i = i + 8)
        {	for(j = 0; j < 125; j = j + 6)
        	{
        		drawChar(j,i,font[count], WHITE, WHITE, sizeof(font[count]));
        		count++;
        		// delay(5);
        	}
        }

        count = 0;
        for(i=0; i < 127; i = i + 8)
        {	for(j = 0; j < 125; j = j + 6)
        	{
        	fillRect(j, i, 6, 8, WHITE);
        		count++;
        		delay(10);
        	}
        }
*/

// 3. Need to change priority?
// 4. A ton of race conditions!
// 5. Can achieve better experience by adding one more timer(one-shot) when typing
// 6. Why this is not working? Probably with a little bit performance issue?
// Why have to check manually the availability of Tx FIFO?
/*
static void sendMsgUART(void) {
	int i = 0;
	while (i < iTextBuffIndex) {
		MAP_UARTCharPut(CONSOLE, ucTextBuff[i]);
		MAP_UARTCharPut(THEOTHERCC3200, ucTextBuff[i]);
//		UART_PRINT("%d", i);
		i++;
	}
	iTextBuffIndex = 0; // Flash Buffer
}
*/
// Shouldn't MAP_UARTCharPut does that already since it would wait?
//
// 7. Encounter RxFIFO overrun problem. Can be solved by using RTS/CTS hardward flow control
// Instead we used a software workaround with delay Transmission. Works well too.
