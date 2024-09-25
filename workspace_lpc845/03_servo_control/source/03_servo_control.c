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
// Variable para guardar el evento al quese asigna el PWM
uint32_t event;

int main(void) {
	// Initialization
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();

    // Activo clock de matriz de conmutacion
    CLOCK_EnableClock(kCLOCK_Swm);
    // Configuro la funcion de ADC en el canal del potenciometro
    SWM_SetFixedPinSelect(SWM0, kSWM_ADC_CHN0, true);
    // Desactivo clock de matriz de conmutacion
    CLOCK_DisableClock(kCLOCK_Swm);

    // Elijo clock desde el FRO con divisor de 1 (30MHz)
    CLOCK_Select(kADC_Clk_From_Fro);
    CLOCK_SetClkDivider(kCLOCK_DivAdcClk, 1);

    // Prendo el ADC
    POWER_DisablePD(kPDRUNCFG_PD_ADC0);

    // Obtengo frecuencia deseada y calibro ADC
   	uint32_t frequency = CLOCK_GetFreq(kCLOCK_Fro) / CLOCK_GetClkDivider(kCLOCK_DivAdcClk);
   	ADC_DoSelfCalibration(ADC0, frequency);
   	// Configuracion por defecto del ADC (Synchronous Mode, Clk Divider 1, Low Power Mode true, High Voltage Range)
   	adc_config_t adc_config;
   	ADC_GetDefaultConfig(&adc_config);
       // Habilito el ADC
   	ADC_Init(ADC0, &adc_config);
   	// Configuracion para las conversiones
   	adc_conv_seq_config_t adc_sequence = {
   		.channelMask = 1 << ADC_POT_CH,							// Elijo el canal del potenciometro
   		.triggerMask = 0,										// No hay trigger por hardware
   		.triggerPolarity = kADC_TriggerPolarityPositiveEdge,	// Flanco ascendente
   		.enableSyncBypass = false,								// Sin bypass de trigger
   		.interruptMode = kADC_InterruptForEachConversion		// Interrupciones para cada conversion
   	};

   	// Configuro y habilito secuencia A
   	ADC_SetConvSeqAConfig(ADC0, &adc_sequence);
   	ADC_EnableConvSeqA(ADC0, true);

	// Conecto la salida 4 del SCT al LED azul
    CLOCK_EnableClock(kCLOCK_Swm);
    SWM_SetMovablePinSelect(SWM0, kSWM_SCT_OUT4, kSWM_PortPin_P0_29);
    CLOCK_DisableClock(kCLOCK_Swm);

    // Eligo el clock para el Timer
    uint32_t sctimer_clock = CLOCK_GetFreq(kCLOCK_Fro);
    // Configuracion del SCT Timer
    sctimer_config_t sctimer_config;
    SCTIMER_GetDefaultConfig(&sctimer_config);
    SCTIMER_Init(SCT0, &sctimer_config);

    // Configuro el PWM
    sctimer_pwm_signal_param_t pwm_config = {
		.output = kSCTIMER_Out_4,		// Salida del Timer
		.level = kSCTIMER_HighTrue,		// Logica negativa
		.dutyCyclePercent = 50			// 50% de ancho de pulso
    };

    // Inicializo el PWM
    SCTIMER_SetupPwm(
		SCT0,
		&pwm_config,
		kSCTIMER_CenterAlignedPwm,
		PWM_FREQ,
		sctimer_clock,
		&event
	);

    // Inicializo el Timer
    SCTIMER_StartTimer(SCT0, kSCTIMER_Counter_U);

    SysTick_Config(SystemCoreClock / 100000);

    while (true){
    	// Resultado de conversion
    	adc_result_info_t adc_info;
    	// Inicio conversion
    	ADC_DoSoftwareTriggerConvSeqA(ADC0);
    	// Espero a terminar la conversion
    	while (!ADC_GetChannelConversionResult(ADC0, ADC_POT_CH, &adc_info));
    	uint8_t angle = adc_info.result * 360 / 4095;
    	servo_duty_signal = angle;
    }

    return 0;
}

void SysTick_Handler(void) {
	// Variable para contar interrupciones
	static uint16_t i = 0;

	// Incremento contador
	i++;

	// Verifico si el SysTick se disparo
	if(i == 100) {
		i = 0;
		SCTIMER_UpdatePwmDutycycle(SCT0, kSCTIMER_Out_4, servo_duty_signal, event);
	}
}
