#include "board.h"
#include "pin_mux.h"
#include "fsl_dac.h"
#include "fsl_swm.h"
#include "fsl_power.h"
#include "fsl_iocon.h"
#include <math.h>

#ifndef MPI
#define MPI 3.14159265358979323846
#endif

int signal_frequency;

/**
 * @brief Programa principal
 */
int main(void) {
	signal_frequency = 1000;

	// Clock del sistema a 30 MHz
	BOARD_BootClockFRO30M();
	// Configuro la salida del DAC al P0 17
	CLOCK_EnableClock(kCLOCK_Swm);
	SWM_SetFixedPinSelect(SWM0, kSWM_DAC_OUT0, true);
	CLOCK_DisableClock(kCLOCK_Swm);

	// Habilito la funcion de DAC en el P0 17
	CLOCK_EnableClock(kCLOCK_Iocon);
	IOCON_PinMuxSet(IOCON, 0, IOCON_PIO_DACMODE_MASK);
	CLOCK_DisableClock(kCLOCK_Iocon);

	// Prendo el DAC
	POWER_DisablePD(kPDRUNCFG_PD_DAC0);

	// Configuro por defecto el DAC
	dac_config_t dac_config;
	DAC_GetDefaultConfig(&dac_config);
	DAC_Init(DAC0, &dac_config);
	DAC_SetBufferValue(DAC0, 0);

	// Configuro el Systick para 1 us
	SysTick_Config(SystemCoreClock / 1000000);

    while (true);
    return 0;
}


/**
 ¿* @brief Handler para la interrupcion del Systick
 */
void SysTick_Handler(void) {
	static int time_ref_us = 0;
	// Valor del DAC
	static uint16_t dac_value = 0;

	dac_value = (sin(2 * MPI * signal_frequency * time_ref_us * pow(10, -6)) + 1) / 2 * 1024;

	// Cargo el DAC
	DAC_SetBufferValue(DAC0, dac_value);

	time_ref_us++;

	if (time_ref_us == 1000){
		time_ref_us = 0;
	}
}
