#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_uart.h"
#include "lpc17xx_dac.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_i2c.h"
#include "lpc17xx_ssp.h"
#include "lpc17xx_adc.h"
#include "joystick.h"
#include "oled.h"
#include "light.h"
#include "temp.h"
#include "acc.h"
#include "pca9532.h"

extern const unsigned char sound_8k[];
extern int sound_sz;

#define UART_DEV LPC_UART3
#define ACCEL_THRESHOLD 300

static uint32_t msTicks = 0;
static uint8_t buf[10];
static int displayState = 0; // Used to switch between sensor values
uint8_t btn1 = 0;
int32_t prev_x = 0, prev_y = 0, prev_z = 0;

// Forward declarations
void init_sensors_and_oled();
void display_sensor_value();
void engine_init();
//void engine_stop();
void speaker_init();

static void intToString(int value, uint8_t* pBuf, uint32_t len, uint32_t base)
{
    static const char* pAscii = "0123456789abcdefghijklmnopqrstuvwxyz";
    int pos = 0;
    int tmpValue = value;

    // the buffer must not be null and at least have a length of 2 to handle one
    // digit and null-terminator
    if (pBuf == NULL || len < 2)
    {
        return;
    }

    // a valid base cannot be less than 2 or larger than 36
    // a base value of 2 means binary representation. A value of 1 would mean only zeros
    // a base larger than 36 can only be used if a larger alphabet were used.
    if (base < 2 || base > 36)
    {
        return;
    }

    // negative value
    if (value < 0)
    {
        tmpValue = -tmpValue;
        value    = -value;
        pBuf[pos++] = '-';
    }

    // calculate the required length of the buffer
    do {
        pos++;
        tmpValue /= base;
    } while(tmpValue > 0);


    if (pos > len)
    {
        // the len parameter is invalid.
        return;
    }

    pBuf[pos] = '\0';

    do {
        pBuf[--pos] = pAscii[value % base];
        value /= base;
    } while(value > 0);

    return;

}

void SysTick_Handler(void) {
    msTicks++;
}

static uint32_t getTicks(void)
{
    return msTicks;
}

static void init_ssp(void)
{
	SSP_CFG_Type SSP_ConfigStruct;
	PINSEL_CFG_Type PinCfg;

	/*
	 * Initialize SPI pin connect
	 * P0.7 - SCK;
	 * P0.8 - MISO
	 * P0.9 - MOSI
	 * P2.2 - SSEL - used as GPIO
	 */
	PinCfg.Funcnum = 2;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Portnum = 0;
	PinCfg.Pinnum = 7;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 8;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 9;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Funcnum = 0;
	PinCfg.Portnum = 2;
	PinCfg.Pinnum = 2;
	PINSEL_ConfigPin(&PinCfg);

	SSP_ConfigStructInit(&SSP_ConfigStruct);

	// Initialize SSP peripheral with parameter given in structure above
	SSP_Init(LPC_SSP1, &SSP_ConfigStruct);

	// Enable SSP peripheral
	SSP_Cmd(LPC_SSP1, ENABLE);

}

static void init_i2c(void)
{
	PINSEL_CFG_Type PinCfg;

	/* Initialize I2C2 pin connect */
	PinCfg.Funcnum = 2;
	PinCfg.Pinnum = 10;
	PinCfg.Portnum = 0;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 11;
	PINSEL_ConfigPin(&PinCfg);

	// Initialize I2C peripheral
	I2C_Init(LPC_I2C2, 100000);

	/* Enable I2C1 operation */
	I2C_Cmd(LPC_I2C2, ENABLE);
}

static void init_adc(void)
{
	PINSEL_CFG_Type PinCfg;

	/*
	 * Init ADC pin connect
	 * AD0.0 on P0.23
	 */
	PinCfg.Funcnum = 1;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Pinnum = 23;
	PinCfg.Portnum = 0;
	PINSEL_ConfigPin(&PinCfg);

	/* Configuration for ADC :
	 * 	Frequency at 0.2Mhz
	 *  ADC channel 0, no Interrupt
	 */
	ADC_Init(LPC_ADC, 200000);
	ADC_IntConfig(LPC_ADC,ADC_CHANNEL_0,DISABLE);
	ADC_ChannelCmd(LPC_ADC,ADC_CHANNEL_0,ENABLE);

}

void init_uart(void) {
    PINSEL_CFG_Type PinCfg;
    UART_CFG_Type uartCfg;

    /* Initialize UART3 pin connect */
    PinCfg.Funcnum = 2;
    PinCfg.Pinnum = 0;
    PinCfg.Portnum = 0;
    PINSEL_ConfigPin(&PinCfg);
    PinCfg.Pinnum = 1;
    PINSEL_ConfigPin(&PinCfg);

    uartCfg.Baud_rate = 115200;
    uartCfg.Databits = UART_DATABIT_8;
    uartCfg.Parity = UART_PARITY_NONE;
    uartCfg.Stopbits = UART_STOPBIT_1;

    UART_Init(UART_DEV, &uartCfg);
    UART_TxCmd(UART_DEV, ENABLE);
}

void playSound() {
    uint32_t cnt = 0;
    uint32_t sampleRate = 0;
    uint32_t delay = 0;

    /*GPIO_SetDir(2, 1<<0, 1);
    GPIO_SetDir(2, 1<<1, 1);

    GPIO_SetDir(0, 1<<27, 1);
    GPIO_SetDir(0, 1<<28, 1);
    GPIO_SetDir(2, 1<<13, 1);
    GPIO_SetDir(0, 1<<26, 1);

    GPIO_ClearValue(0, 1<<27); //LM4811-clk
    GPIO_ClearValue(0, 1<<28); //LM4811-up/dn
    GPIO_ClearValue(2, 1<<13); //LM4811-shutdn*/

    /* Basic checks on WAV format - assuming sound_8k is already validated or static */
    sampleRate = (sound_8k[24] | (sound_8k[25] << 8) |
                  (sound_8k[26] << 16) | (sound_8k[27] << 24));
    delay = 1000000 / sampleRate;

    for (cnt = 44; cnt < sound_sz; cnt++) { // Start at first byte of data
        DAC_UpdateValue(LPC_DAC, (uint32_t)(sound_8k[cnt]));
        Timer0_us_Wait(delay);
    }
}


void check_light_and_control_leds() {
    uint32_t light = light_read();
    uint16_t ledOn = 0;
    uint16_t ledOff = 0xFFFF; // All LEDs off initially

    if (light < 300) {
        ledOn = 0xFFFF; // Turn on all LEDs
        ledOff = 0x0000; // Turn off no LEDs
    }

    pca9532_setLeds(ledOn, ledOff);
}

void check_accelerometer_and_trigger_alarm() {
    int32_t x, y, z;
    acc_read(&x, &y, &z);

    // Calculate the difference from the previous values
    int32_t diff_x = x - prev_x;
    int32_t diff_y = y - prev_y;
    int32_t diff_z = z - prev_z;

    // Update previous values
    prev_x = x;
    prev_y = y;
    prev_z = z;

    // Check if the difference exceeds the threshold
    if (abs(diff_x) + abs(diff_y) +abs(diff_z)> ACCEL_THRESHOLD ) {
        // Trigger the alarm sound
    	for (int i = 0; i < 100; i++) { // Play a louder and longer tone
    	            DAC_UpdateValue(LPC_DAC, 600); // Maximum DAC value
    	            Timer0_us_Wait(100); // Duration of each part of the tone
    	            DAC_UpdateValue(LPC_DAC, 0); // Reset the DAC value
    	            Timer0_us_Wait(100); // Pause between tones
    	        }
    }
}

int main(void) {
	PINSEL_CFG_Type PinCfg;
	speaker_init();
    joystick_init();
    init_sensors_and_oled();
    PinCfg.Funcnum = 2;
    PinCfg.OpenDrain = 0;
    PinCfg.Pinmode = 0;
    PinCfg.Pinnum = 26;
    PinCfg.Portnum = 0;
    PINSEL_ConfigPin(&PinCfg);
    DAC_Init(LPC_DAC);
    init_uart();
    pca9532_init();


    if (SysTick_Config(SystemCoreClock / 1000)) {
        while (1);  // Capture error
    }

    // Initialize previous accelerometer values
    acc_read(&prev_x, &prev_y, &prev_z);

    while (1) {
        uint8_t joyState = joystick_read();
        if ((joyState & JOYSTICK_LEFT) != 0 || (joyState & JOYSTICK_RIGHT) != 0) {
            playSound();
            if (joyState & JOYSTICK_RIGHT) {
                displayState++;
                if (displayState > 5) displayState = 0; // Wrap around
            } else if (joyState & JOYSTICK_LEFT) {
                if (displayState == 0) displayState = 5; // Wrap around
                else displayState--;
            }
            Timer0_Wait(200); // Debounce delay
        }
        display_sensor_value();
        check_light_and_control_leds();
        check_accelerometer_and_trigger_alarm();
        Timer0_Wait(200); // Debounce delay
    }

    return 0;
}

void engine_init() {
	// Initialize GPIO pins for engine control
	    PINSEL_CFG_Type PinCfg;

	    // Configure pins for engine control (EN, IN1, IN2)
	    PinCfg.Funcnum = 0;  // Configure as GPIO
	    PinCfg.OpenDrain = 0;  // Disable open-drain mode
	    PinCfg.Pinmode = 0;  // Disable pull-up/pull-down resistors
	    PinCfg.Portnum = 0;  // Port 0 for all engine control pins

	    // EN (Enable) pin configuration
	    PinCfg.Pinnum = 4;  // Pin 4 for EN
	    PINSEL_ConfigPin(&PinCfg);  // Configure pin as GPIO
	    GPIO_SetDir(0, (1 << 4), 1);  // Set pin 4 as output
	    GPIO_SetValue(0, (1 << 4));  // Set EN high initially

	    // IN1 and IN2 pins configuration
	    PinCfg.Pinnum = 8;  // Pin 8 for IN1
	    PINSEL_ConfigPin(&PinCfg);  // Configure pin as GPIO
	    GPIO_SetDir(0, (1 << 8), 1);  // Set pin 8 as output
	    GPIO_SetValue(0, (1 << 8));  // Set IN1 low initially

	    PinCfg.Pinnum = 9;  // Pin 9 for IN2
	    PINSEL_ConfigPin(&PinCfg);  // Configure pin as GPIO
	    GPIO_SetDir(0, (1 << 9), 1);  // Set pin 9 as output
	    GPIO_ClearValue(0, (1 << 9));  // Set IN2 low initially
}

void engine_stop() {
    // Clear all engine control pins (EN, IN1, IN2)
    GPIO_ClearValue(0, (1 << 4));  // Clear EN (Port 0, Pin 4)
    GPIO_ClearValue(0, (1 << 8));  // Clear IN1 (Port 0, Pin 8)
    GPIO_ClearValue(0, (1 << 9));  // Clear IN2 (Port 0, Pin 9)
}

void init_sensors_and_oled() {
    // Initialize SSP for OLED
    init_ssp();

    // Initialize I2C for temperature and light sensors
    init_i2c();

    // Initialize ADC for potentiometer readings
    init_adc();

    // Initialize OLED display
    oled_init();

    // Initialize temperature sensor
    temp_init(&getTicks);

    // Initialize light sensor
    light_enable();
    light_setRange(LIGHT_RANGE_4000);

    // Initialize accelerometer
    acc_init();
}


void display_sensor_value() {
    int32_t temp;
    uint32_t light, x, y, z;
    uint8_t str[40]; // Increased size for sensor name and value

    oled_clearScreen(OLED_COLOR_WHITE);

    switch(displayState) {
        case 0: // Temperature
            temp = temp_read();
            sprintf((char*)str, "Temp: %02d.%dC", temp / 10, temp % 10);

            oled_putString(1, 0, str, OLED_COLOR_BLACK, OLED_COLOR_WHITE);
            break;
        case 1: // Light
            light = light_read();
            sprintf((char*)str, "Light: %dLux", light);
            oled_putString(1, 0, str, OLED_COLOR_BLACK, OLED_COLOR_WHITE);
            break;
        case 2: // Accelerometer X
            acc_read(&x, &y, &z);
            sprintf((char*)str, "Acc X: %d", x);
            oled_putString(1, 0, str, OLED_COLOR_BLACK, OLED_COLOR_WHITE);
            break;
        case 3: // Accelerometer Y
            acc_read(&x, &y, &z);
            sprintf((char*)str, "Acc Y: %d", y);
            oled_putString(1, 0, str, OLED_COLOR_BLACK, OLED_COLOR_WHITE);
            break;
        case 4: // Accelerometer Z
            acc_read(&x, &y, &z);
            sprintf((char*)str, "Acc Z: %d", z);
            oled_putString(1, 0, str, OLED_COLOR_BLACK, OLED_COLOR_WHITE);
            break;
        case 5: //Turn on motor
        	//btn1 = ((GPIO_ReadValue(0) >> 4) & 0x01);
        	//if(btn1 == 0) {
        		//sprintf((char*)str, "Motor is ON");
        		//oled_putString(1, 0, str, OLED_COLOR_BLACK, OLED_COLOR_WHITE);
        		engine_init();
        	/*} else {
        		sprintf((char*)str, "Motor is ON");
        		oled_putString(1, 0, str, OLED_COLOR_BLACK, OLED_COLOR_WHITE);
        		engine_stop();
        	}*/
        	break;
        default:
            oled_putString(1, 0, (uint8_t*)"Select sensor", OLED_COLOR_BLACK, OLED_COLOR_WHITE);
            break;

        engine_stop();
    }
}

void speaker_init(){
	GPIO_SetDir(2, 1<<0, 1);
	GPIO_SetDir(2, 1<<1, 1);

	GPIO_SetDir(0, 1<<27, 1);
	GPIO_SetDir(0, 1<<28, 1);
	GPIO_SetDir(2, 1<<13, 1);
	GPIO_SetDir(0, 1<<26, 1);

	GPIO_ClearValue(0, 1<<27); //LM4811-clk
	GPIO_ClearValue(0, 1<<28); //LM4811-up/dn
	GPIO_ClearValue(2, 1<<13); //LM4811-shutdn
}