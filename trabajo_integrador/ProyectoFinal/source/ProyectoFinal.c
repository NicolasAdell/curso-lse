#include "clock_config.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "board.h"
#include "FreeRTOS.h"
#include "fsl_debug_console.h"
#include "task.h"
#include "semphr.h"
#include "fsl_sctimer.h"
#include "fsl_swm.h"
#include "fsl_gpio.h"
#include "fsl_common.h"
#include "fsl_debug_console.h"
#include "fsl_i2c.h"
#include "fsl_adc.h"
#include "queue.h"
#include "semphr.h"
#include "fsl_power.h"
#include <stdbool.h>
#include <stdio.h>

#define COM_1 8
#define COM_2 9
#define SEG_A 10
#define SEG_B 11
#define SEG_C 6
#define SEG_D 14
#define SEG_E 0
#define SEG_F 13
#define SEG_G 15
#define BTN_1 16
#define BTN_2 25
#define BTN_USER 4
// Set PWM frequency
#define PWM_FREQ 1000
// BH1750 direction
#define BH1750_ADDR	0x5c
#define BTN_1 16
#define BTN_2 25
#define ADC_POT_CH 8

// Variable to store PWM events
uint32_t event;

// Cola para datos del ADC
QueueHandle_t queue_setpoint;
// Cola para datos del ADC
QueueHandle_t queue_light_intensity;
QueueHandle_t queue_duty_cycle;
QueueHandle_t queue_show_setpoint;

SemaphoreHandle_t semphr;

void task_measure_light(void *params);
void task_display(void *params);
void task_change_setpoint(void *params);
void task_change_blue_led_intensity(void *params);
void task_print_information(void *params);
void task_button(void *params);
void project_init();
void display_init();
void buttons_init();
void i2c_init();
void adc_init();
void pwm_init();
void display_write(uint8_t number);
void display_off(void);
void display_on(uint8_t com);
void display_segments_off(void);
void display_segment_on(uint8_t segment);

SemaphoreHandle_t semphr;

int main(void) {
	// Clock del sistema de 30 MHz
	BOARD_BootClockFRO30M();

    project_init();

	semphr = xSemaphoreCreateCounting(
			  50,
			  0
			 );

	xTaskCreate(
		task_button,				// Callback de la tarea
		"ChangeDisplayShow",				// Nombre
		configMINIMAL_STACK_SIZE,	// Stack reservado
		NULL,						// Sin parametros
		tskIDLE_PRIORITY + 1UL,		// Prioridad
		NULL						// Sin handler
	);

	xTaskCreate(
		task_display,				// Callback de la tarea
		"DisplayShow",				// Nombre
		configMINIMAL_STACK_SIZE,	// Stack reservado
		NULL,						// Sin parametros
		tskIDLE_PRIORITY + 1UL,		// Prioridad
		NULL						// Sin handler
	);

	xTaskCreate(
		task_measure_light,			// Callback de la tarea
		"MeasureLight",				// Nombre
		configMINIMAL_STACK_SIZE,	// Stack reservado
		NULL,						// Sin parametros
		tskIDLE_PRIORITY + 1UL,		// Prioridad
		NULL						// Sin handler
	);

	xTaskCreate(
		task_change_setpoint,			// Callback de la tarea
		"SetpointMove",				// Nombre
		configMINIMAL_STACK_SIZE,	// Stack reservado
		NULL,						// Sin parametros
		tskIDLE_PRIORITY + 1UL,		// Prioridad
		NULL						// Sin handler
	);

	xTaskCreate(
		task_change_blue_led_intensity,			// Callback de la tarea
		"BlueLEDIntensity",				// Nombre
		configMINIMAL_STACK_SIZE,	// Stack reservado
		NULL,						// Sin parametros
		tskIDLE_PRIORITY + 1UL,		// Prioridad
		NULL						// Sin handler
	);

	xTaskCreate(
		task_print_information,			// Callback de la tarea
		"Print",				// Nombre
		3 * configMINIMAL_STACK_SIZE,	// Stack reservado
		NULL,						// Sin parametros
		tskIDLE_PRIORITY + 1UL,		// Prioridad
		NULL						// Sin handler
	);

	PRINTF("Time (ms) | Light | Setpoint | Bright\n");

	vTaskStartScheduler();
}

void task_button(void *params) {
	bool show_setpoint;

	while (true) {
		if (!GPIO_PinRead(GPIO, 0, BTN_USER)) {
			show_setpoint = !show_setpoint;
		}

		vTaskDelay(85);

		xQueueOverwrite(
						queue_show_setpoint,
						&show_setpoint
						);

	}
}
void task_display(void *params) {
	bool show_setpoint;
	uint32_t setpoint;
	uint8_t light_intensity;
	uint8_t value;

	while (true) {
		// Definir value
		xQueuePeek(
					queue_setpoint,
					&setpoint,
					100
					);

		xQueuePeek(
					queue_light_intensity,
					&light_intensity,
					100
					);

		xQueuePeek(
					queue_show_setpoint,
					&show_setpoint,
					100
					);

		if (show_setpoint) {
			value = setpoint;
		} else {
			value = light_intensity;
		}

		// Muestro el numero
		display_off();
		display_write((uint8_t)(value / 10));
		display_on(COM_1);
		vTaskDelay(5);
		display_off();
		display_write((uint8_t)(value % 10));
		display_on(COM_2);
		vTaskDelay(5);
	}
}

void task_measure_light(void *params) {
	while (true) {
		// Sensor reading
		if (I2C_MasterStart(I2C1, BH1750_ADDR, kI2C_Read) == kStatus_Success) {
			// Result
			uint8_t res[2] = {0};
			I2C_MasterReadBlocking(I2C1, res, 2, kI2C_TransferDefaultFlag);
			I2C_MasterStop(I2C1);
			// Return result
			float lux = ((res[0] << 8) + res[1]) / 1.2;
			//PRINTF("LUX : %d \r\n",(uint16_t) lux);
			uint8_t lux_percentage = lux * 100 / 20000;
			xQueueOverwrite(
							queue_light_intensity,
							&lux_percentage
							);
		}
		vTaskDelay(50);
    }
}

void task_change_setpoint(void *params) {
	while (true) {
		if (!GPIO_PinRead(GPIO, 0, BTN_1)) {
			xSemaphoreGive(semphr);
		}
		else if (!GPIO_PinRead(GPIO, 0, BTN_2)) {
			xSemaphoreTake(semphr, 10);
		}

		vTaskDelay(85);

		uint32_t setpoint = uxSemaphoreGetCount(semphr) + 25;

		xQueueOverwrite(
						queue_setpoint,
						&setpoint
						);
		vTaskDelay(50);
	}
}

void task_print_information(void *params) {
	uint32_t setpoint = 25;
	uint8_t light_intensity;
	uint8_t bright_led_intensity;

	while (true) {
		xQueuePeek(
						queue_setpoint,
						&setpoint,
						100
						);
		xQueuePeek(
						queue_light_intensity,
						&light_intensity,
						100
						);

		xQueuePeek(
						queue_duty_cycle,
						&bright_led_intensity,
						100
						);

		uint32_t time  = xTaskGetTickCount();

		PRINTF("%8d | %3d%% | %3d%% | %3d%%\n", time, light_intensity, setpoint, bright_led_intensity);
		vTaskDelay(1000);
	}
}

void task_change_blue_led_intensity(void *params) {
	adc_result_info_t adc_info;
	while (true) {
		// Initialize conversion
		ADC_DoSoftwareTriggerConvSeqA(ADC0);
		// Wait to the end of conversion
		while (!ADC_GetChannelConversionResult(ADC0, ADC_POT_CH, &adc_info));
		uint8_t duty_cycle = adc_info.result * 100 / 4095;
		SCTIMER_UpdatePwmDutycycle(SCT0, kSCTIMER_Out_4, duty_cycle, event);
		xQueueOverwrite(
						queue_duty_cycle,
						&duty_cycle
						);
		vTaskDelay(50);
	}
}

void project_init() {
	i2c_init();
	pwm_init();
	adc_init();
	display_init();
	buttons_init();
}

void i2c_init() {
	// Start at 30 MHz
	BOARD_BootClockFRO30M();

	// Initialize clock in I2C1
	CLOCK_Select(kI2C1_Clk_From_MainClk);
    // Assign functions to I2C1 and to 26 and 27 pins
	CLOCK_EnableClock(kCLOCK_Swm);
    SWM_SetMovablePinSelect(SWM0, kSWM_I2C1_SDA, kSWM_PortPin_P0_27);
    SWM_SetMovablePinSelect(SWM0, kSWM_I2C1_SCL, kSWM_PortPin_P0_26);
    CLOCK_DisableClock(kCLOCK_Swm);

    // Master configuration to I2C with 400 KHz clock
    i2c_master_config_t config;
    I2C_MasterGetDefaultConfig(&config);
    config.baudRate_Bps = 400000;
    // Use system clock to generate the communication
    I2C_MasterInit(I2C1, &config, SystemCoreClock);

	if (I2C_MasterStart(I2C1, BH1750_ADDR, kI2C_Write) == kStatus_Success) {
		// Power on command
		uint8_t cmd = 0x01;
		I2C_MasterWriteBlocking(I2C1, &cmd, 1, kI2C_TransferDefaultFlag);
		I2C_MasterStop(I2C1);
	}
	if (I2C_MasterStart(I2C1, BH1750_ADDR, kI2C_Write) == kStatus_Success) {
		// Continuous measures to 1 lux of resolution
		uint8_t cmd = 0x10;
		I2C_MasterWriteBlocking(I2C1, &cmd, 1, kI2C_TransferDefaultFlag);
		I2C_MasterStop(I2C1);
	}

	queue_light_intensity = xQueueCreate(1, sizeof(uint8_t));
}

void adc_init() {
	BOARD_InitDebugConsole();

	// Initialization
	BOARD_InitBootClocks();

	// Enable clock commutation matrix
	CLOCK_EnableClock(kCLOCK_Swm);
	// Configuro ADC function in the potentiometer channel
	SWM_SetFixedPinSelect(SWM0, kSWM_ADC_CHN8, true);
	// Deactivate commutation matrix clock
	CLOCK_DisableClock(kCLOCK_Swm);

	// Choose clock from FRO with 1 as divider (30MHz)
	CLOCK_Select(kADC_Clk_From_Fro);
	CLOCK_SetClkDivider(kCLOCK_DivAdcClk, 1);

    // Turn ADC on
    POWER_DisablePD(kPDRUNCFG_PD_ADC0);

	// Get frequncy and calibrate ADC
	uint32_t frequency = CLOCK_GetFreq(kCLOCK_Fro) / CLOCK_GetClkDivider(kCLOCK_DivAdcClk);
	ADC_DoSelfCalibration(ADC0, frequency);
	// Default configuration for ADC (Synchronous Mode, Clk Divider 1, Low Power Mode true, High Voltage Range)
	adc_config_t adc_config;
	ADC_GetDefaultConfig(&adc_config);
	// Enable ADC
	ADC_Init(ADC0, &adc_config);
	// Configure conversions
	adc_conv_seq_config_t adc_sequence = {
		.channelMask = 1 << ADC_POT_CH,							// Elijo el canal del potenciometro
		.triggerMask = 0,										// No hay trigger por hardware
		.triggerPolarity = kADC_TriggerPolarityPositiveEdge,	// Flanco ascendente
		.enableSyncBypass = false,								// Sin bypass de trigger
		.interruptMode = kADC_InterruptForEachConversion		// Interrupciones para cada conversion
	};

	// Configuro and enable A sequence
	ADC_SetConvSeqAConfig(ADC0, &adc_sequence);
	ADC_EnableConvSeqA(ADC0, true);
}

void display_init(void) {
    // Enable GPIO 1 clock
    GPIO_PortInit(GPIO, 0);

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
    queue_show_setpoint = xQueueCreate(1, sizeof(bool));
    queue_setpoint = xQueueCreate(1, sizeof(uint32_t));
	semphr = xSemaphoreCreateCounting(
			  100,
			  0
			 );
}

void pwm_init() {
	// Connect 4th output of SCT to blue LED
    CLOCK_EnableClock(kCLOCK_Swm);
    SWM_SetMovablePinSelect(SWM0, kSWM_SCT_OUT4, kSWM_PortPin_P0_29);
    CLOCK_DisableClock(kCLOCK_Swm);

    // Choose clock timer
    uint32_t sctimer_clock = CLOCK_GetFreq(kCLOCK_Fro);
    // SCT Timer configuration
    sctimer_config_t sctimer_config;
    SCTIMER_GetDefaultConfig(&sctimer_config);
    SCTIMER_Init(SCT0, &sctimer_config);

    // PWM configuration
    sctimer_pwm_signal_param_t pwm_config = {
		.output = kSCTIMER_Out_4,		// Timer output
		.level = kSCTIMER_HighTrue,		// Positive logic
		.dutyCyclePercent = 50			// 50% duty cycle
    };

    // Initialize PWM
    SCTIMER_SetupPwm(
		SCT0,
		&pwm_config,
		kSCTIMER_CenterAlignedPwm,
		PWM_FREQ,
		sctimer_clock,
		&event
	);

    // Timer initialize
    SCTIMER_StartTimer(SCT0, kSCTIMER_Counter_U);
    // Create system interruption
    //SysTick_Config(SystemCoreClock / 1000);

    queue_duty_cycle = xQueueCreate(1, sizeof(uint8_t));
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
