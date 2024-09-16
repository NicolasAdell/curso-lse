#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "fsl_debug_console.h"
#include <stdbool.h>

#define SEGMENT_A 11
#define SEGMENT_B 10
#define SEGMENT_C 6
#define SEGMENT_D 14
#define SEGMENT_E 0
#define SEGMENT_F 13

#define USER_BUTTON 4

int main(void) {
	// Initialization
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();

    // Enable GPIO 0 clock
    GPIO_PortInit(GPIO, 0);

    // Data structure for output configuration
    gpio_pin_config_t out_config = {kGPIO_DigitalOutput, 1};

    // Configure segments as outputs
    GPIO_PinInit(GPIO, 0, SEGMENT_A, &out_config);
    GPIO_PinInit(GPIO, 0, SEGMENT_B, &out_config);
    GPIO_PinInit(GPIO, 0, SEGMENT_C, &out_config);
    GPIO_PinInit(GPIO, 0, SEGMENT_D, &out_config);
    GPIO_PinInit(GPIO, 0, SEGMENT_E, &out_config);
    GPIO_PinInit(GPIO, 0, SEGMENT_F, &out_config);

    // Data structure for input configuration
    gpio_pin_config_t in_config = {kGPIO_DigitalInput};

    // Configure USER_BUTTON in GPIO 0 as input
    GPIO_PinInit(GPIO, 0, USER_BUTTON, &in_config);

    while (true) {
    	// Read USER button in Pull up
    	if (!GPIO_PinRead(GPIO, 0, USER_BUTTON)){
    		// Turn each segment on to create an animation (Common anode)
			GPIO_PinWrite(GPIO, 0, SEGMENT_A, false);
			for (uint32_t i = 0; i < 250000; i++);
			GPIO_PinWrite(GPIO, 0, SEGMENT_B, false);
			for (uint32_t i = 0; i < 250000; i++);
			GPIO_PinWrite(GPIO, 0, SEGMENT_C, false);
			for (uint32_t i = 0; i < 250000; i++);
			GPIO_PinWrite(GPIO, 0, SEGMENT_D, false);
			for (uint32_t i = 0; i < 250000; i++);
			GPIO_PinWrite(GPIO, 0, SEGMENT_E, false);
			for (uint32_t i = 0; i < 250000; i++);
			GPIO_PinWrite(GPIO, 0, SEGMENT_F, false);
    	} else {
    		// Turn all segments off (Common Anode)
			GPIO_PinWrite(GPIO, 0, SEGMENT_A, true);
			GPIO_PinWrite(GPIO, 0, SEGMENT_B, true);
			GPIO_PinWrite(GPIO, 0, SEGMENT_C, true);
			GPIO_PinWrite(GPIO, 0, SEGMENT_D, true);
			GPIO_PinWrite(GPIO, 0, SEGMENT_E, true);
			GPIO_PinWrite(GPIO, 0, SEGMENT_F, true);
    	}

    	// Waiting to deal with bounce
    	for (uint32_t i = 0; i < 250000; i++);
    }
    return 0;
}
