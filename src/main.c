/** \file
 * Simple test file that is not linked into the library.
 * 
 * This file is normally rigged to run on the XC800 Starter Kit eval board
 * and used for testing whatever code is currently under development.
 *
 * @author kami
 * @version 2012-02-09
 */

#include <Infineon/XC878.h>

#include "config.h"

#include "hsk_boot/hsk_boot.h"
#include "hsk_timers/hsk_timer01.h"
#include "hsk_can/hsk_can.h"
#include "hsk_icm7228/hsk_icm7228.h"
#include "hsk_adc/hsk_adc.h"
#include "hsk_pwm/hsk_pwm.h"
#include "hsk_pwc/hsk_pwc.h"

HSK_ICM7228_FACTORY(xxx, P1, P3, 0, P3, 1)

void main(void);
void init(void);
void run(void);

/**
 * Call init functions and invoke the run routine.
 */
void main(void) {
	init();
	run();
}

/**
 * This structure is used to persist data between resets.
 */
struct {
	/**
	 * A prefix to detect whether this is a power on or boot from a reset.
	 */
	ulong prefix;

	/**
	 * Used for reset counting.
	 */
	ubyte count;

	/**
	 * For storing errors.
	 *
	 * Certain errors like a WDT can only be reported after a reboot.
	 */
	ubyte error;

	/**
	 * A postfix to detect whether this is a power on or boot from a
	 * reset.
	 */
	ulong postfix;
} xdata boot;

/**
 * A counter used to detecting that 250ms have passed.
 */
volatile uword tick0_count_250 = 0;

/**
 * A counter used to detecting that 20ms have passed.
 */
volatile ubyte tick0_count_20 = 10;

/**
 * A ticking function called back by the timer T0 ISR.
 */
#pragma save
#pragma nooverlay
void tick0(void) {
	tick0_count_250++;
	tick0_count_20++;
}
#pragma restore

/**
 * The storage variable for the potentiometer on the eval board.
 */
volatile uword adc7;

/**
 * Initialize ports, timers and ISRs.
 */
void init(void) {
	hsk_can_msg msgBoot;

	/* Activate xdata access. */
	hsk_boot_mem();

	/* Activate external clock. */
	hsk_boot_extClock(CLK);

	/*
	 * Boot/reset detection.
	 * This is an experiment that will likely become its own library.
	 */
	if (boot.prefix == 0x64821273 && boot.postfix == 0x09050785) {
		/* Just a reset. */
		boot.count++;
	} else {
		/* Power on boot. */
		memset(&boot, 0, sizeof(boot));
		boot.prefix = 0x64821273;
		boot.postfix = 0x09050785;
	}

	/* Activate timer 0. */
	hsk_timer0_setup(1000, &tick0);
	hsk_timer0_enable();

	/* Activate ADC */
	hsk_adc_init(ADC_RESOLUTION_10, 5);
	hsk_adc_open(7, &adc7);
	hsk_adc_enable();

	/* Activate CAN. */
	hsk_can_init(CAN1_IO, CAN1_BAUD);
	hsk_can_disable(CAN0);
	hsk_can_enable(CAN1);

	/* Activate PWM at 46.875kHz - exactly 10bit precision. */
//	hsk_pwm_init(468750);
//	hsk_pwm_init(PWM_60, 1200); /* 120Hz */
	hsk_pwm_init(PWM_63, 10); /* 1Hz */
	hsk_pwm_init(PWM_62, 500); /* 50Hz */
	hsk_pwm_enable();
	//hsk_pwm_port_open(PWM_OUT_60_P30);
	//hsk_pwm_port_open(PWM_OUT_60_P31);
	hsk_pwm_port_open(PWM_OUT_63_P37);
	//hsk_pwm_outChannel_dir(PWM_COUT60, 0);
	//hsk_pwm_outChannel_dir(PWM_COUT60, 0);
	hsk_pwm_port_open(PWM_OUT_62_P04);
	hsk_pwm_channel_set(PWM_62, 100, 5);

	/* Activate PWC with a 100ms window. */
	hsk_pwc_init(100);
	hsk_pwc_port_open(PWC_CC0_P40, 4);
	hsk_pwc_channel_edgeMode(PWC_CC0, PWC_EDGE_RISING);
	hsk_pwc_enable();

	//P3_DIR |= 0x30;
	//P3_DATA |= 0x10;
	P3_DIR = -1;
	EA = 1;

	hsk_adc_warmup();

	msgBoot = hsk_can_msg_create(0x7f0, 0, sizeof(ubyte));
	hsk_can_msg_connect(msgBoot, CAN1);
	hsk_can_msg_setData(msgBoot, &boot.count);
	hsk_can_msg_send(msgBoot);

}

/**
 * The main test code body.
 */
void run(void) {
	uword ticks250;
	ubyte ticks20;
	hsk_can_msg msg0;
	ubyte data0[2] = {0,0};
	ubyte xdata buffer[8];
	uword adc7_copy;

	hsk_can_data_setSignal(buffer, CAN_ENDIAN_MOTOROLA, 3, 16, 0x1234);
	adc7_copy = hsk_can_data_getSignal(buffer, CAN_ENDIAN_MOTOROLA, 3, 16);

	hsk_icm7228_writeHex(buffer, 123, -1, 0, 5);

	msg0 = hsk_can_msg_create(0x7ff, 0, 2);
	hsk_can_msg_connect(msg0, CAN1);

	while (1) {
		EA = 0;
		ticks20 = tick0_count_20;
		ticks250 = tick0_count_250;
		EA = 1;

		if (ticks20 >= 20) {
			EA = 0;
			tick0_count_20 -= 20;
			EA = 1;
			EADC = 0;
			adc7_copy = adc7;
			EADC = 1;

			hsk_pwm_channel_set(PWM_62, 100, adc7_copy * 5 / 1023 + 5);
			hsk_pwm_channel_set(PWM_63, 1023, adc7_copy);
			hsk_adc_service();
			P3_DATA = hsk_pwc_channel_getFreqHz(PWC_CC0);
		}		 

		if (ticks250 >= 250) {
			EA = 0;
			tick0_count_250 -= 250;
			EA = 1;
			EADC = 0;
			adc7_copy = adc7;
			EADC = 1;

			//P3_DATA ^= 0x30;
			hsk_can_data_setSignal(data0, CAN_ENDIAN_MOTOROLA, 7, 16, adc7_copy);
			hsk_can_msg_setData(msg0, data0);
			hsk_can_msg_send(msg0);
		}
	}
}

