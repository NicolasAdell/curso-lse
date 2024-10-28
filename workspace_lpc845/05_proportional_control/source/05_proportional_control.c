#include "board.h"
#include "clock_config.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "FreeRTOS.h"
#include "fsl_debug_console.h"
#include "task.h"
#include "fsl_common.h"
#include "fsl_sctimer.h"
#include "fsl_power.h"
#include "fsl_adc.h"
#include "fsl_swm.h"
#include "peripherals.h"
#include "queue.h"
#include <math.h>

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
#define ADC_POT1_CH 0
#define ADC_POT2_CH 8
#define PWM_FREQ 1000

void adc_measurement(void *params);
void change_duty_led(void *params);
void adc_init();
void pwm_init();

// Variable to store PWM events
uint32_t event;
QueueHandle_t queue;

int main(void) {
	// Clock del sistema de 30 MHz
	BOARD_BootClockFRO30M();

    // Enable GPIO 1 clock
    GPIO_PortInit(GPIO, 0);

	adc_init();

    pwm_init();

    queue = xQueueCreate(1, sizeof(uint8_t));

	xTaskCreate(
		adc_measurement,				// Callback de la tarea
		"ReadADCValue",				// Nombre
		configMINIMAL_STACK_SIZE,	// Stack reservado
		NULL,						// Sin parametros
		tskIDLE_PRIORITY + 1UL,		// Prioridad
		NULL						// Sin handler
	);

	xTaskCreate(
		change_duty_led,			// Callback de la tarea
		"ChangeDutyCycleLed",				// Nombre
		configMINIMAL_STACK_SIZE,	// Stack reservado
		NULL,						// Sin parametros
		tskIDLE_PRIORITY + 1UL,		// Prioridad
		NULL						// Sin handler
	);


	vTaskStartScheduler();
}

void adc_measurement(void *params) {
	// Conversion result
	adc_result_info_t adc_info_ch0;
	adc_result_info_t adc_info_ch1;
	static uint8_t duty_cycle;
	while (true) {
    	// Initialize conversion
    	ADC_DoSoftwareTriggerConvSeqA(ADC0);
    	// Wait to the end of conversion
    	while (!ADC_GetChannelConversionResult(ADC0, ADC_POT1_CH, &adc_info_ch0));
    	while (!ADC_GetChannelConversionResult(ADC0, ADC_POT2_CH, &adc_info_ch1));
    	uint8_t new_duty_cycle = (float) abs(adc_info_ch0.result - adc_info_ch1.result) * 100 / 4095;
    	if (new_duty_cycle != duty_cycle) {
    		duty_cycle = new_duty_cycle;
    		xQueueSend(
				queue,
				&duty_cycle,
				portMAX_DELAY
			);
    	}
	}
}

void change_duty_led(void *params) {
	uint8_t data;
	while (true) {
		xQueueReceive(
				queue,
				&data,
				portMAX_DELAY
				);
		// Actualizo el duty
		SCTIMER_UpdatePwmDutycycle(SCT0, kSCTIMER_Out_4, data, event);
	}
}

void pwm_init() {
    CLOCK_EnableClock(kCLOCK_Swm);
    SWM_SetMovablePinSelect(SWM0, kSWM_SCT_OUT4, kSWM_PortPin_P0_29);
    CLOCK_DisableClock(kCLOCK_Swm);

    // Choose timer clock
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

    // Variable to store PWM events
    uint32_t event;
    // Initialize PWM
    SCTIMER_SetupPwm(
		SCT0,
		&pwm_config,
		kSCTIMER_CenterAlignedPwm,
		PWM_FREQ,
		sctimer_clock,
		&event
	);
    // Initialize Timer
    SCTIMER_StartTimer(SCT0, kSCTIMER_Counter_U);
}

void adc_init() {
	// Initialization
    BOARD_InitBootClocks();

    // Enable clock commutation matrix
    CLOCK_EnableClock(kCLOCK_Swm);
    // Configuro ADC function in the potentiometer channel
    SWM_SetFixedPinSelect(SWM0, kSWM_ADC_CHN0, true);
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
   		.channelMask = 1 << ADC_POT1_CH | 1 << ADC_POT2_CH,		// Elijo el canal del potenciometro
   		.triggerMask = 0,										// No hay trigger por hardware
   		.triggerPolarity = kADC_TriggerPolarityPositiveEdge,	// Flanco ascendente
   		.enableSyncBypass = false,								// Sin bypass de trigger
   		.interruptMode = kADC_InterruptForEachConversion		// Interrupciones para cada conversion
   	};

   	// Configuro and enable A sequence
   	ADC_SetConvSeqAConfig(ADC0, &adc_sequence);
   	ADC_EnableConvSeqA(ADC0, true);
}
