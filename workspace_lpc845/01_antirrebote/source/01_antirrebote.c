#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "fsl_debug_console.h"

#define LED_RED 2
#define USER_BUTTON 4

int main(void) {
	// Initialization
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();

    // Data structure for output configuration
    gpio_pin_config_t out_config = {kGPIO_DigitalOutput, 1};
    // Enable GPIO 1 clock
    GPIO_PortInit(GPIO, 1);
    // Configure LED_RED as output
    GPIO_PinInit(GPIO, 1, LED_RED, &out_config);

    // Data structure for input configuration
    gpio_pin_config_t in_config = {kGPIO_DigitalInput};
    // Enable clock for GPIO 0
    GPIO_PortInit(GPIO, 0);
    // Configure USER_BUTTON in GPIO 0 as input
    GPIO_PinInit(GPIO, 0, USER_BUTTON, &in_config);

    while (true) {
    	// Read USER button in Pull up
    	if (!GPIO_PinRead(GPIO, 0, USER_BUTTON)){
			// Turn red LED on (common Anode)
			GPIO_PinWrite(GPIO, 1, LED_RED, false);
    	} else {
    		// Turn red LED off (common Anode)
    		GPIO_PinWrite(GPIO, 1, LED_RED, true);
    	}

    	// Waiting to deal with bounce
    	for (uint32_t i = 0; i < 250000; i++);
    }
    return 0;
}
