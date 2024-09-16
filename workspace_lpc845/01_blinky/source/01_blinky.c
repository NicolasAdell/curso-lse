#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "fsl_debug_console.h"
#include <stdbool.h>

#define LED_BLUE 1

/*
 * @brief   Application entry point.
*/
int main(void) {
	// Initialization
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();

    // Data structure for output configuration
    gpio_pin_config_t config = {kGPIO_DigitalOutput, 1};
    // Enable GPIO 1 clock
    GPIO_PortInit(GPIO, 1);
    // Configure LED_RED as output
    GPIO_PinInit(GPIO, 1, LED_BLUE, &config);

    while (true) {
    	// Change last LED_BLUE status
    	GPIO_PinWrite(GPIO, 1, LED_BLUE, !GPIO_PinRead(GPIO, 1, LED_BLUE));
    	// Waiting
    	for (uint32_t i = 0; i < 500000; i++);
    }
    return 0;
}
