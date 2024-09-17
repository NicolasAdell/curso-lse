#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "fsl_debug_console.h"
#include <stdbool.h>

#define LED_BLUE 1
#define LED_D1 29

/*
 * @brief   Application entry point.
*/
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


    SysTick_Config(SystemCoreClock / 1000);

    while (true);

    return 0;
}

void SysTick_Handler(void) {
	// Variable para contar interrupciones
	static uint16_t i = 0;

	// Incremento contador
	i++;

	// Verifico si el SysTick se disparo 500 veces (medio segundo)
	if(i % 500 == 0) {
		// Conmuto el LED
		GPIO_PinWrite(GPIO, 1, LED_BLUE, !GPIO_PinRead(GPIO, 1, LED_BLUE));
	}

	if (i == 1500) {
		i = 0;
		GPIO_PinWrite(GPIO, 0, LED_D1, !GPIO_PinRead(GPIO, 0, LED_D1));
	}
}
