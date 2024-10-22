#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "fsl_debug_console.h"

#define LED_BLUE 1
#define LED_D1 29

void SysTick_Handler(void);

int main(void) {
	// Initialization
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();

    // Data structure for output configuration
    gpio_pin_config_t out_config_1 = {kGPIO_DigitalOutput, 1};
    // Enable GPIO 1 clock
    GPIO_PortInit(GPIO, 1);
    // Configure LED_RED as output
    GPIO_PinInit(GPIO, 1, LED_BLUE, &out_config_1);

    // Data structure for output configuration
    gpio_pin_config_t out_config_0 = {kGPIO_DigitalOutput, 0};
    GPIO_PortInit(GPIO, 0);
    // Configure LED_RED as output
    GPIO_PinInit(GPIO, 0, LED_D1, &out_config_0);

    // Change SysTick configuration
    SysTick_Config(SystemCoreClock / 1000);

    while (true);

    return 0;
}

void SysTick_Handler(void) {
	// Variable to count interruptions
	static uint16_t i = 0;

	// Counter increment
	i++;

	// Verify SysTick has been shot 500 times
	if(i % 500 == 0) {
		// Change LED status
		GPIO_PinWrite(GPIO, 1, LED_BLUE, !GPIO_PinRead(GPIO, 1, LED_BLUE));
	}

	// Verify SysTick has been shot 1500 times
	if (i == 1500) {
		i = 0;
		GPIO_PinWrite(GPIO, 0, LED_D1, !GPIO_PinRead(GPIO, 0, LED_D1));
	}
}
