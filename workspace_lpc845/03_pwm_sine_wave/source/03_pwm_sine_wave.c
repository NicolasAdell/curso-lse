#include "board.h"
#include "fsl_sctimer.h"
#include "fsl_swm.h"
#include "pin_mux.h"
#include "fsl_gpio.h"
#include "fsl_common.h"
#include "fsl_swm.h"
#include "fsl_debug_console.h"

// PWM frequency
#define PWM_FREQ 10000

int main(void) {
	BOARD_InitDebugConsole();
	// Start at 30 MHz
	BOARD_BootClockFRO30M();

    CLOCK_EnableClock(kCLOCK_Swm);
    SWM_SetMovablePinSelect(SWM0, kSWM_SCT_OUT4, kSWM_PortPin_P1_6);
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

    while (true);

    return 0;
}
