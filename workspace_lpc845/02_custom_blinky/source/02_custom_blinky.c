#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "fsl_debug_console.h"
#include "fsl_swm.h"
#include "fsl_power.h"
#include "fsl_adc.h"

#define ADC_POT_CH 0
#define LED_RED 2

// Define variable which will allow to change the blinking frequency
uint16_t timelapse = 0;

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

    // Enable commutation matrix clock
    CLOCK_EnableClock(kCLOCK_Swm);
    // Configure ADC function in the potentiometer hannel
    SWM_SetFixedPinSelect(SWM0, kSWM_ADC_CHN0, true);
    // Deactivate commutation matrix clock
    CLOCK_DisableClock(kCLOCK_Swm);

    // Choose clock from FRO with 1 as divider (30MHz)
    CLOCK_Select(kADC_Clk_From_Fro);
    CLOCK_SetClkDivider(kCLOCK_DivAdcClk, 1);

    // Turn ADC on
    POWER_DisablePD(kPDRUNCFG_PD_ADC0);

    // Get desired frequency and calibrate ADC
   	uint32_t frequency = CLOCK_GetFreq(kCLOCK_Fro) / CLOCK_GetClkDivider(kCLOCK_DivAdcClk);
   	ADC_DoSelfCalibration(ADC0, frequency);
   	// Default configuration of the ADC (Synchronous Mode, Clk Divider 1, Low Power Mode true, High Voltage Range)
   	adc_config_t adc_config;
   	ADC_GetDefaultConfig(&adc_config);
    // Enable ADC
   	ADC_Init(ADC0, &adc_config);
   	// Conversion configuration
   	adc_conv_seq_config_t adc_sequence = {
   		.channelMask = 1 << ADC_POT_CH,							// Elijo el canal del potenciometro
   		.triggerMask = 0,										// No hay trigger por hardware
   		.triggerPolarity = kADC_TriggerPolarityPositiveEdge,	// Flanco ascendente
   		.enableSyncBypass = false,								// Sin bypass de trigger
   		.interruptMode = kADC_InterruptForEachConversion		// Interrupciones para cada conversion
   	};

   	// Configure and enable A sequence
   	ADC_SetConvSeqAConfig(ADC0, &adc_sequence);
   	ADC_EnableConvSeqA(ADC0, true);

   	// Define system interruption
    SysTick_Config(SystemCoreClock / 1000);

    while (true){
    	// Conversion result
    	adc_result_info_t adc_info;
    	// Start conversion
    	ADC_DoSoftwareTriggerConvSeqA(ADC0);
    	// Wait for the ending of the conversion
    	while (!ADC_GetChannelConversionResult(ADC0, ADC_POT_CH, &adc_info));
    	// Adjust the time lapse between cycles
    	timelapse = adc_info.result * 1900 / 4095 + 100;
    }

    return 0;
}

void SysTick_Handler(void) {
	// Variable to handle interruptions
	static uint16_t i = 0;

	// Counter increment
	i++;

	// Verify if SysTick has been shot the corresponding time lapse
	if(i == timelapse) {
		i = 0;
		// Change LED status
		GPIO_PinWrite(GPIO, 1, LED_RED, !GPIO_PinRead(GPIO, 1, LED_RED));
	} else if (timelapse < i) {
		i = 0;
	}
}

