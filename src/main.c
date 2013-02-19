/** \file
 * Simple test file that is not linked into the library.
 *
 * This file is normally rigged to run on the XC800 Starter Kit eval board
 * and used for testing whatever code is currently under development.
 *
 * @author kami
 */

#include <Infineon/XC878.h>

/* Begin: Don't depend on non-public CAN DB. */
/* #include <dbc/Primary.h> */
	#define MSG_AFB_CHANNEL               0x403, 0, 3
	#define ID_AFB_CHANNEL                0x403
	#define SIG_AFB_CONFIG_CHAN_SELECT    CAN_ENDIAN_INTEL,   0, 0, 3
/* End. */

#include "config.h"

#include "hsk_boot/hsk_boot.h"
#include "hsk_timers/hsk_timer01.h"
#include "hsk_can/hsk_can.h"
#include "hsk_icm7228/hsk_icm7228.h"
#include "hsk_adc/hsk_adc.h"
#include "hsk_pwm/hsk_pwm.h"
#include "hsk_pwc/hsk_pwc.h"
#include "hsk_flash/hsk_flash.h"
#include "hsk_wdt/hsk_wdt.h"
#include "hsk_io/hsk_io.h"

ICM7228_FACTORY(p1, P1, P3, 0, P3, 1)

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
 * The version of the persist struct.
 */
#define PERSIST_VERSION	1

/** \var persist
 * This structure is used to persist data between resets.
 */
FLASH_STRUCT_FACTORY(
	/**
	 * Used for boot counting.
	 */
	ubyte boot;

	/**
	 * Used for reset counting.
	 */
	ubyte reset;

	/**
	 * For storing errors.
	 *
	 * Certain errors like a WDT can only be reported after a reboot.
	 */
	ubyte error;
) persist;

/**
 * A counter used to detecting that 250ms have passed.
 */
volatile uword pdata tick0_count_250 = 0;

/**
 * A counter used to detecting that 20ms have passed.
 */
volatile ubyte pdata tick0_count_20 = 10;

/**
 * A ticking function called back by the timer T0 ISR.
 */
#pragma save
#ifdef SDCC
#pragma nooverlay
#endif
void tick0(void) using 1 {
	tick0_count_250++;
	tick0_count_20++;
}
#pragma restore

/**
 * The storage variable for the potentiometer on the eval board.
 */
volatile uword pdata adc7;

/**
 * Initialize ports, timers and ISRs.
 */
void init(void) {
	hsk_can_msg msgBoot;

	/* Activate external clock. */
	hsk_boot_extClock(CLK);

	/*
	 * Boot/reset detection.
	 */
	IO_PORT_OUT_INIT(P3, -1, IO_PORT_STRENGTH_WEAK, IO_PORT_DRAIN_DISABLE, -1, 0);
	switch(hsk_flash_init(&persist, sizeof(persist), PERSIST_VERSION)) {
	case FLASH_PWR_FIRST:
		/* Init stuff if needed. */
		hsk_flash_write();
		break;
	case FLASH_PWR_RESET:
		persist.reset++;
		break;
	case FLASH_PWR_ON:
		persist.boot++;
		persist.reset = 0;
		hsk_flash_write();
		break;
	}
	IO_PORT_OUT_SET(P3, -1, -1, persist.boot);

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
	hsk_pwm_init(PWM_62, 505); /* 50Hz */
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
	//P3_DIR = -1;
	EA = 1;

	hsk_adc_warmup();

	/* Service watchdog every 20ms, so wait for 30ms. */
	hsk_wdt_init(3000);
	hsk_wdt_enable();

	msgBoot = hsk_can_msg_create(0x7f0, 0, 2);
	hsk_can_msg_connect(msgBoot, CAN1);
	hsk_can_msg_setData(msgBoot, &persist.boot);
	hsk_can_msg_send(msgBoot);
}

/**
 * The main test code body.
 */
void run(void) {
	uword ticks250;
	ubyte ticks20;
	hsk_can_msg msg0;
	hsk_can_fifo fifo0;
	ubyte data0[3] = {0,0,0};
	ubyte xdata buffer[8];
	uword adc7_copy;

	hsk_can_data_setSignal(buffer, CAN_ENDIAN_MOTOROLA, 0, 3, 16, 0x1234);
	adc7_copy = hsk_can_data_getSignal(buffer, CAN_ENDIAN_MOTOROLA, 0, 3, 16);

	hsk_icm7228_writeHex(buffer, 123, -1, 0, 5);

	msg0 = hsk_can_msg_create(0x7ff, 0, 2);
	hsk_can_msg_connect(msg0, CAN1);

	fifo0 = hsk_can_fifo_create(7);
	hsk_can_fifo_connect(fifo0, CAN1);
	hsk_can_fifo_setupRx(fifo0, MSG_AFB_CHANNEL);
	hsk_can_fifo_setRxMask(fifo0, 0x7f0); /* Take any message from the AFB */

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
			hsk_adc_request(7);
			hsk_adc_request(6);
			hsk_adc_request(5);
			hsk_adc_request(4);
			hsk_adc_request(3);
			hsk_adc_request(2);
			hsk_adc_request(1);
			hsk_adc_request(0);
			hsk_wdt_service();
			/*P3_DATA =*/ hsk_pwc_channel_getValue(PWC_CC0, PWC_UNIT_FREQ_S);
		}

		if (ticks250 >= 250) {
			EA = 0;
			tick0_count_250 -= 250;
			EA = 1;
			EADC = 0;
			adc7_copy = adc7;
			EADC = 1;

			//P3_DATA ^= 0x30;
			hsk_can_data_setSignal(data0, CAN_ENDIAN_MOTOROLA, 0, 7, 16, adc7_copy);
			hsk_can_msg_setData(msg0, data0);
			hsk_can_msg_send(msg0);
		}

		if (hsk_can_fifo_updated(fifo0)) {
			if (hsk_can_fifo_getId(fifo0) == ID_AFB_CHANNEL) {
				hsk_can_fifo_getData(fifo0, data0);
				/*P3_DATA ^= 1 <<*/ hsk_can_data_getSignal(data0, SIG_AFB_CONFIG_CHAN_SELECT);
			}
			hsk_can_fifo_next(fifo0);
		}
	}
}

