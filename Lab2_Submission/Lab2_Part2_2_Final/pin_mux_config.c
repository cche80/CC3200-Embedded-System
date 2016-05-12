#include "pin_mux_config.h" 
#include "hw_types.h"
#include "hw_memmap.h"
#include "hw_gpio.h"
#include "pin.h"
#include "rom.h"		// added for terminal? is this necessary?
#include "rom_map.h"	// added for terminal? is this necessary?
#include "gpio.h"
#include "prcm.h"

//*****************************************************************************
void PinMuxConfig(void)
{
    //
    // Set unused pins to PIN_MODE_0 with the exception of JTAG pins 16,17,19,20
    //
    PinModeSet(PIN_03, PIN_MODE_0);
    PinModeSet(PIN_04, PIN_MODE_0);
    PinModeSet(PIN_21, PIN_MODE_0);
    PinModeSet(PIN_50, PIN_MODE_0);
    PinModeSet(PIN_52, PIN_MODE_0);
    PinModeSet(PIN_53, PIN_MODE_0);
    PinModeSet(PIN_59, PIN_MODE_0);
    PinModeSet(PIN_60, PIN_MODE_0);
    PinModeSet(PIN_62, PIN_MODE_0);
    PinModeSet(PIN_63, PIN_MODE_0);
    PinModeSet(PIN_64, PIN_MODE_0);

    /////////// Enabling output to UART and I2C////////////////
    //
    // Enable Peripheral Clocks
    //
    MAP_PRCMPeripheralClkEnable(PRCM_UARTA0, PRCM_RUN_MODE_CLK);	// UART0 for CONSOLE
    MAP_PRCMPeripheralClkEnable(PRCM_UARTA1, PRCM_RUN_MODE_CLK);	// UART1 for the other CC3200

    MAP_PRCMPeripheralClkEnable(PRCM_I2CA0, PRCM_RUN_MODE_CLK);		// I2C
    MAP_PRCMPeripheralClkEnable(PRCM_TIMERA3, PRCM_RUN_MODE_CLK);	// TimerA3

    //
    // Configure PIN_55 for UART0 UART0_TX
    //
    MAP_PinTypeUART(PIN_55, PIN_MODE_3);

    //
    // Configure PIN_57 for UART0 UART0_RX
    //
    MAP_PinTypeUART(PIN_57, PIN_MODE_3);

    //
    // Configure PIN_58 for UART1 UART1_TX
    //
    MAP_PinTypeUART(PIN_58, PIN_MODE_6);

    //
    // Configure PIN_45 for UART1 UART1_RX
    //
    MAP_PinTypeUART(PIN_45, PIN_MODE_2);

    //
    // Configure PIN_01 for I2C0 I2C_SCL
    //
    MAP_PinTypeI2C(PIN_01, PIN_MODE_1);

    //
    // Configure PIN_02 for I2C0 I2C_SDA
    //
    MAP_PinTypeI2C(PIN_02, PIN_MODE_1);

    //////////////////// OLED ///////////////////////////////

    //
    // Enable Peripheral Clocks 
    //
    PRCMPeripheralClkEnable(PRCM_GPIOA2, PRCM_RUN_MODE_CLK);	// GPIOA2:
    PRCMPeripheralClkEnable(PRCM_GPIOA3, PRCM_RUN_MODE_CLK);	// GPIOA3:
    PRCMPeripheralClkEnable(PRCM_GSPI, PRCM_RUN_MODE_CLK);		// SPI

    //
    // Configure PIN_15 for GPIO_22 Output
    //
    PinTypeGPIO(PIN_15, PIN_MODE_0, false);		// DC
    GPIODirModeSet(GPIOA2_BASE, 0x40, GPIO_DIR_MODE_OUT);	//2 0x40
    // GPIOA2; bit:6; output

    //
    // Configure PIN_18 for GPIO_28 Output
    //
    PinTypeGPIO(PIN_18, PIN_MODE_0, false);		// RESET (default)
    GPIODirModeSet(GPIOA3_BASE, 0x10, GPIO_DIR_MODE_OUT);	//3	0x10
    // GPIOA3; bit:4; output

    //
    // Configure PIN_08 for SPI0 GSPI_CS
    //
    PinTypeSPI(PIN_08, PIN_MODE_7);

    //
    // Configure PIN_05 for SPI0 GSPI_CLK
    //
    PinTypeSPI(PIN_05, PIN_MODE_7);

    //
    // Configure PIN_06 for SPI0 GSPI_MISO
    //
    PinTypeSPI(PIN_06, PIN_MODE_7);

    //
    // Configure PIN_07 for SPI0 GSPI_MOSI
    //
    PinTypeSPI(PIN_07, PIN_MODE_7);

    //
    // Configure PIN_61 for TimerCP6 GT_CCP06
    //
    MAP_PinTypeTimer(PIN_61, PIN_MODE_7);


}
