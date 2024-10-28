#include "clock_config.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "board.h"
#include "FreeRTOS.h"
#include "fsl_debug_console.h"
#include "task.h"
#include "semphr.h"

#define BTN_1 16
#define BTN_2 25
#define BTN_USER 4

void task_display(void *params);
void display_write(uint8_t number);
void counter(void *params);
void display_init(void);
void display_off(void);
void display_on(uint8_t com);
void display_segments_off(void);
void display_segment_on(uint8_t segment);
void buttons_init();

SemaphoreHandle_t semphr;

int main(void) {
	// Clock del sistema de 30 MHz
	BOARD_BootClockFRO30M();

    // Enable GPIO 1 clock
    GPIO_PortInit(GPIO, 0);

    buttons_init();

	display_init();

	semphr = xSemaphoreCreateCounting(
			  100,
			  0
			 );

	xTaskCreate(
		task_display,				// Callback de la tarea
		"DisplayCount",				// Nombre
		configMINIMAL_STACK_SIZE,	// Stack reservado
		NULL,						// Sin parametros
		tskIDLE_PRIORITY + 1UL,		// Prioridad
		NULL						// Sin handler
	);

	xTaskCreate(
		counter,			// Callback de la tarea
		"Counter",				// Nombre
		configMINIMAL_STACK_SIZE,	// Stack reservado
		NULL,						// Sin parametros
		tskIDLE_PRIORITY + 2UL,		// Prioridad
		NULL						// Sin handler
	);


	vTaskStartScheduler();
}

void task_display(void *params) {
	while (true) {
		if ()
	}
}

void counter(void *params) {
	uint16_t frequency;
	while (true) {
		// Obtengo el valor del Semaphore Counting
		uint32_t count = uxSemaphoreGetCount(semphr);
		frequency = count / PORT_DELAY;
		printf("Frequency is %d\n", frequency);
		vTaskDelay(500);
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

void buttons_init() {
    gpio_pin_config_t in_config = {kGPIO_DigitalInput};
    // Configure LED_RED as output
    GPIO_PinInit(GPIO, 0, BTN_1, &in_config);
    GPIO_PinInit(GPIO, 0, BTN_2, &in_config);
    GPIO_PinInit(GPIO, 0, BTN_USER, &in_config);
}
