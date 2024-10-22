#include "board.h"
#include "FreeRTOS.h"
#include "task.h"

#define COM_1 8
#define COM_2 9
#define SEG_A 10
#define SEG_B 11
#define SEG_C 6
#define SEG_D 14
#define SEG_E 0
#define SEG_F 13
#define SEG_G 15

void task_display(void *params);
void display_write(uint8_t number);
void counter_seconds(void *params);
void display_init(void);
void display_off(void);
void display_on(uint8_t com);
void display_segments_off(void);
void display_segment_on(uint8_t segment);

uint8_t counter = 0;

int main(void) {
	// Clock del sistema de 30 MHz
	BOARD_BootClockFRO30M();

	display_init();

	xTaskCreate(
		task_display,				// Callback de la tarea
		"DisplayCount",				// Nombre
		configMINIMAL_STACK_SIZE,	// Stack reservado
		NULL,						// Sin parametros
		tskIDLE_PRIORITY + 1UL,		// Prioridad
		NULL						// Sin handler
	);

	xTaskCreate(
		counter_seconds,			// Callback de la tarea
		"Counter",				// Nombre
		configMINIMAL_STACK_SIZE,	// Stack reservado
		NULL,						// Sin parametros
		tskIDLE_PRIORITY + 1UL,		// Prioridad
		NULL						// Sin handler
	);

	vTaskStartScheduler();
}

void task_display(void *params) {
	printf("Executing");

	while (true) {
		// Muestro el numero
		display_off();
		display_write((uint8_t)(counter / 10));
		display_on(COM_1);
		vTaskDelay(10);
		display_off();
		display_write((uint8_t)(counter % 10));
		display_on(COM_2);
		vTaskDelay(10);
	}
}

void counter_seconds(void *params) {
	while (true) {
		vTaskDelay(1000);

		if (counter  == 59) {
			counter = -1;
		}

		counter++;
	}
}

void display_write(uint8_t number) {
	// Array con valores para los pines
	uint8_t values[] = {~0x3f, ~0x6, ~0x5b, ~0x4f, ~0x66, ~0x6d, ~0x7d, ~0x7, ~0x7f, ~0x6f};
	// Array con los segmentos
	uint32_t pins[] = {SEG_A, SEG_B, SEG_C, SEG_D, SEG_E, SEG_F, SEG_G};

	for(uint8_t i = 0; i < sizeof(pins) / sizeof(uint32_t); i++) {
		// Escribo el valor del bit en el segmento que corresponda
		uint32_t val = (values[number] & (1 << i))? 1 : 0;
		GPIO_PinWrite(GPIO, 0, pins[i], val);
	}
}

void display_off(void) {
	// Pongo en uno ambos anodos
	GPIO_PinWrite(GPIO, 0, COM_1, true);
	GPIO_PinWrite(GPIO, 0, COM_2, true);
}

void display_on(uint8_t com) {
	// Pongo un cero en el anodo
	GPIO_PinWrite(GPIO, 0, com, false);
}

void display_segments_off(void) {
	// Pongo un uno en cada segmento
	uint8_t pins[] = {SEG_A, SEG_B, SEG_C, SEG_D, SEG_E, SEG_F, SEG_G};
	for(uint8_t i = 0; i < sizeof(pins) / sizeof(uint8_t); i++) {
		GPIO_PinWrite(GPIO, 0, pins[i], true);
	}
}

void display_segment_on(uint8_t segment) {
	// Pongo un cero en el segmento indicado
	GPIO_PinWrite(GPIO, 0, segment, false);
}

void display_init(void) {
	// Inicializo los pines como salidas
	gpio_pin_config_t out_config = {kGPIO_DigitalOutput, true};
	uint32_t pins[] = {SEG_A, SEG_B, SEG_C, SEG_D, SEG_E, SEG_F, SEG_G, COM_1, COM_2};
	for(uint8_t i = 0; i < sizeof(pins) / sizeof(uint32_t); i++) {
		GPIO_PinInit(GPIO, 0, pins[i], &out_config);
		GPIO_PinWrite(GPIO, 0, pins[i], true);
	}
}

/*
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "fsl_debug_console.h"

#define DIGIT_1 8
#define DIGIT_2 9
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
    GPIO_PinInit(GPIO, 0, DIGIT_1, &out_config);
    GPIO_PinInit(GPIO, 0, DIGIT_2, &out_config);
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
    	while (!GPIO_PinRead(GPIO, 0, USER_BUTTON)){
			GPIO_PinWrite(GPIO, 0, DIGIT_1, false);
			GPIO_PinWrite(GPIO, 0, DIGIT_2, false);
    		// Turn each segment on to create an animation (Common anode)
			GPIO_PinWrite(GPIO, 0, SEGMENT_A, false);
			for (uint32_t i = 0; i < 250000; i++);
			GPIO_PinWrite(GPIO, 0, SEGMENT_A, true);
			GPIO_PinWrite(GPIO, 0, SEGMENT_C, false);
			for (uint32_t i = 0; i < 250000; i++);
			GPIO_PinWrite(GPIO, 0, SEGMENT_C, true);
			GPIO_PinWrite(GPIO, 0, SEGMENT_D, false);
			for (uint32_t i = 0; i < 250000; i++);
			GPIO_PinWrite(GPIO, 0, SEGMENT_D, true);
			GPIO_PinWrite(GPIO, 0, SEGMENT_E, false);
			for (uint32_t i = 0; i < 250000; i++);
			GPIO_PinWrite(GPIO, 0, SEGMENT_E, true);
			GPIO_PinWrite(GPIO, 0, SEGMENT_F, false);
			for (uint32_t i = 0; i < 250000; i++);
			GPIO_PinWrite(GPIO, 0, SEGMENT_F, true);
			GPIO_PinWrite(GPIO, 0, SEGMENT_B, false);
			for (uint32_t i = 0; i < 250000; i++);
			GPIO_PinWrite(GPIO, 0, SEGMENT_B, true);
    	}

		GPIO_PinWrite(GPIO, 0, DIGIT_1, true);
		GPIO_PinWrite(GPIO, 0, DIGIT_2, true);
		// Turn all segments off (Common Anode)
		GPIO_PinWrite(GPIO, 0, SEGMENT_A, true);
		GPIO_PinWrite(GPIO, 0, SEGMENT_B, true);
		GPIO_PinWrite(GPIO, 0, SEGMENT_C, true);
		GPIO_PinWrite(GPIO, 0, SEGMENT_D, true);
		GPIO_PinWrite(GPIO, 0, SEGMENT_E, true);
		GPIO_PinWrite(GPIO, 0, SEGMENT_F, true);

    	// Waiting to deal with bounce
    	for (uint32_t i = 0; i < 250000; i++);
    }
    return 0;
}
*/
