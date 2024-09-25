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

int main(void) {
	signal_frequency = 1000;

	// Clock system to 30 MHz
	BOARD_BootClockFRO30M();
	// Configure DAC output to PIO0_29
	CLOCK_EnableClock(kCLOCK_Swm);
	SWM_SetFixedPinSelect(SWM0, kSWM_DAC_OUT0, true);
	CLOCK_DisableClock(kCLOCK_Swm);

	// Enable DAC function to PIO0_29
	CLOCK_EnableClock(kCLOCK_Iocon);
	IOCON_PinMuxSet(IOCON, 0, IOCON_PIO_DACMODE_MASK);
	CLOCK_DisableClock(kCLOCK_Iocon);

	// Turn DAC on
	POWER_DisablePD(kPDRUNCFG_PD_DAC0);

	// Configure DAC as default
	dac_config_t dac_config;
	DAC_GetDefaultConfig(&dac_config);
	DAC_Init(DAC0, &dac_config);
	DAC_SetBufferValue(DAC0, 0);

	// Configure SysTick to 1 us
	SysTick_Config(SystemCoreClock / 1000000);

    while (true);
    return 0;
}

void SysTick_Handler(void) {
	// Time reference
	static uint16_t time_ref_us = 0;
	// DAC value
	static uint16_t dac_value = 0;

	// Graph sine wave
	dac_value = (sin(2 * MPI * signal_frequency * time_ref_us * pow(10, -6)) + 1) / 2 * 1024;

	// Cargo el DAC
	DAC_SetBufferValue(DAC0, dac_value);

	// Increment time reference
	time_ref_us++;

	// Graph until the completion of one cycle
	if (time_ref_us == 1000){
		time_ref_us = 0;
	}
}
