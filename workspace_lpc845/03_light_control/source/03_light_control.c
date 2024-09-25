#include "board.h"
#include "fsl_sctimer.h"
#include "fsl_swm.h"
#include "pin_mux.h"
#include "fsl_gpio.h"
#include "fsl_common.h"
#include "fsl_debug_console.h"
#include "fsl_i2c.h"

// Set PWM frequency
#define PWM_FREQ 1000
// BH1750 direction
#define BH1750_ADDR	0x5c

// Variable to store duty cycle
uint32_t duty_cycle;
// Variable to store PWM events
uint32_t event;

int main(void) {
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
    SysTick_Config(SystemCoreClock / 100000);

	while (true) {
		// Sensor reading
		if (I2C_MasterStart(I2C1, BH1750_ADDR, kI2C_Read) == kStatus_Success) {
			// Result
			uint8_t res[2] = {0};
			I2C_MasterReadBlocking(I2C1, res, 2, kI2C_TransferDefaultFlag);
			I2C_MasterStop(I2C1);
			// Return result
			float lux = ((res[0] << 8) + res[1]) / 1.2;
			PRINTF("LUX : %d \r\n",(uint16_t) lux);
			// Make the conversion
			uint32_t duty = (float) lux / 65535 * 100000;
			PRINTF("DUTY_CYCLE: %d\n", duty);

			if (duty <= 100 && duty >= 0){
				// Change duty cycle
				duty_cycle = duty;
			}
		}
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
		// Update PWM duty cycle
		SCTIMER_UpdatePwmDutycycle(SCT0, kSCTIMER_Out_4, duty_cycle, event);
	}
}
