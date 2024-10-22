#include "board.h"
#include "fsl_sctimer.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "fsl_debug_console.h"
#include "fsl_swm.h"
#include "fsl_power.h"
#include "fsl_adc.h"

#define ADC_POT_CH 0
#define PWM_FREQ 1000


uint8_t servo_duty_signal = 50;
// Variable to store PWM value
uint32_t event;

void SysTick_Handler(void);

int main(void) {
	// Initialization
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();

    // Enable clock commutation matrix
    CLOCK_EnableClock(kCLOCK_Swm);
    // Configuro ADC function in the potentiometer channel
    SWM_SetFixedPinSelect(SWM0, kSWM_ADC_CHN0, true);
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

	// Connect 4th output of the SCT to the blue LED
    CLOCK_EnableClock(kCLOCK_Swm);
    SWM_SetMovablePinSelect(SWM0, kSWM_SCT_OUT4, kSWM_PortPin_P0_29);
    CLOCK_DisableClock(kCLOCK_Swm);

    // Choose clock for the timer
    uint32_t sctimer_clock = CLOCK_GetFreq(kCLOCK_Fro);
    // Configure SCT Timer
    sctimer_config_t sctimer_config;
    SCTIMER_GetDefaultConfig(&sctimer_config);
    SCTIMER_Init(SCT0, &sctimer_config);

    // Configure PWM
    sctimer_pwm_signal_param_t pwm_config = {
		.output = kSCTIMER_Out_4,		// Salida del Timer
		.level = kSCTIMER_HighTrue,		// Logica negativa
		.dutyCyclePercent = 50			// 50% de ancho de pulso
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

    // Initialize Timer
    SCTIMER_StartTimer(SCT0, kSCTIMER_Counter_U);

    SysTick_Config(SystemCoreClock / 100000);

    while (true){
    	// Conversion result
    	adc_result_info_t adc_info;
    	// Initialize conversion
    	ADC_DoSoftwareTriggerConvSeqA(ADC0);
    	// Wait to the end of conversion
    	while (!ADC_GetChannelConversionResult(ADC0, ADC_POT_CH, &adc_info));
    	uint8_t angle = adc_info.result * 360 / 4095;
    	servo_duty_signal = angle;
    }

    return 0;
}

void SysTick_Handler(void) {
	// Variable to count interruptions
	static uint16_t i = 0;

	// Counter increment
	i++;

	// Verify if SysTick has been shot 100 times
	if(i == 100) {
		i = 0;
		SCTIMER_UpdatePwmDutycycle(SCT0, kSCTIMER_Out_4, servo_duty_signal, event);
	}
}
