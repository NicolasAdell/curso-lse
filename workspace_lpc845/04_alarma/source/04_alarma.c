#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "FreeRTOS.h"
#include "task.h"

#define BUZZER 28
#define BTN_ON 16
#define BTN_OFF 25

void turn_off_buzzer(void *params);
void turn_on_buzzer(void *params);

int main(void) {
	// Clock del sistema de 30 MHz

	BOARD_BootClockFRO30M();

    // Data structure for output configuration
    gpio_pin_config_t out_config = {kGPIO_DigitalOutput, 0};
    gpio_pin_config_t in_config = {kGPIO_DigitalInput};

    // Enable GPIO 1 clock
    GPIO_PortInit(GPIO, 0);
    // Configure LED_RED as output
    GPIO_PinInit(GPIO, 0, BUZZER, &out_config);

    // Configure LED_RED as output
    GPIO_PinInit(GPIO, 0, BTN_ON, &in_config);
    GPIO_PinInit(GPIO, 0, BTN_OFF, &in_config);

	xTaskCreate(
		turn_on_buzzer,				// Callback de la tarea
		"TurnOnTask",				// Nombre
		configMINIMAL_STACK_SIZE,	// Stack reservado
		NULL,						// Sin parametros
		tskIDLE_PRIORITY + 1UL,		// Prioridad
		NULL						// Sin handler
	);

	xTaskCreate(
		turn_off_buzzer,			// Callback de la tarea
		"TurnOffTask",				// Nombre
		configMINIMAL_STACK_SIZE,	// Stack reservado
		NULL,						// Sin parametros
		tskIDLE_PRIORITY + 1UL,		// Prioridad
		NULL						// Sin handler
	);

	vTaskStartScheduler();
}

void turn_on_buzzer(void *params) {
	while (true) {
		if (!GPIO_PinRead(GPIO, 0, BTN_ON)){
			// Turn red LED on (common Anode)
			GPIO_PinWrite(GPIO, 0, BUZZER, true);
		}
		vTaskDelay(100);
	}
}

void turn_off_buzzer(void *params) {
	while (true) {
		if (!GPIO_PinRead(GPIO, 0, BTN_OFF)){
			// Turn red LED on (common Anode)
			GPIO_PinWrite(GPIO, 0, BUZZER, false);
		}
		vTaskDelay(100);
	}
}
