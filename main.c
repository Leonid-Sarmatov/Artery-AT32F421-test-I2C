#include "at32f421.h"                   // Device header
#include "RTE_Components.h"             // Component selection
#include "at32f421_conf.h"              // ArteryTek::Device:at32f421_conf
#include "at32f421_gpio.h"




/*_____________________________________________________________________*/


/*DEFAULT_SETTINGS*/
#define VERSION 1
const char SHORT_ZUMMER_DELAY = 30;
const char LONG_ZUMMER_DELAY = 120;
const char FRIMWARE_VERSION_EEPROM_ADR = 0x01;
const unsigned AUTOROTATION_DAYS = 14; //???? ?? ???????? ?????
const char MOVING_WAIT_DELAY = 1;
const unsigned LOW_WATER_RESISTANSE = 20000; //????????????? ???????
const unsigned HIGH_WATER_RESISTANSE = 25000; //
const unsigned UP_RESISTANSE = 20000; //????????????? ????????
/*?????? ?? ????????*/
const char WSP_MEAS_COUNT = 4; //?????????? ????????? ???????
const char FUN_MEAS_COUNT = 10; //?????????? ????????? ?????????????
const char JUMP_MEAS_COUNT = 10; //?????????? ????????? ????????
/*????????*/
const char RELE_POWER_WORK_DELAY = 120; // sec
const char RELE_POWER_AUTOROTATION_DELAY = 25; // sec
const char RELE_GAP = 1; //sec
const char MELODY_REPEAT_DELAY = 30; //min
const uint32_t AUTOROTATION_DELAY = (AUTOROTATION_DAYS * 24 * 60 * 60); //D*H*M*S
/*voltages*/
uint16_t BAD_WSP_VOLTAGE = 0;
uint16_t GOOD_WSP_VOLTAGE = 0;
uint16_t adc_result;
char fun_result;
/*_____________________________________________________________________*/

/**MY DATA*/
uint8_t numbers[10] = {
		0b00111111,
		0b00000110,
		0b01011011,
		0b01001111,
		0b01100110,
		0b01101101,
		0b01111101,
		0b00000111,
		0b01111111,  
		0b01101111  
	};
/*________*/

/**FLAGS*/
static union {
    uint32_t value;
    struct {

        unsigned ALARM_ON : 1;
        unsigned ALARM_OFF : 1;
        unsigned FUN_HIGH : 1;
        unsigned FUN_LOW : 1;
        unsigned ALLOW_MEASURE : 1;
        unsigned ALLOW_FUN : 1;
        unsigned ALLOW_JUMP : 1;
        unsigned JUMP_LOW : 1;

        unsigned JUMP_HIGH : 1;
        unsigned OPENING : 1;
        unsigned OPENED : 1;
        unsigned CLOSING : 1;
        unsigned CLOSED : 1;
        unsigned RELE_POWER_ON : 1;
        unsigned RELE_CONTROL_ON : 1;
        unsigned TONE_ON : 1;

        unsigned TONE_OFF : 1;
        unsigned SIREN : 1;
        unsigned ZUM_BUSY : 1;
        unsigned MOVING_ALLOWED : 1;
        unsigned NORMAL_WORK_MODE_ON : 1;
        unsigned UNIVERSAL_VORK_MODE_ON : 1;
        unsigned LED_ON : 1;
        unsigned SEC_LOCK : 1;

        unsigned AUTOROTATION_WORK : 1;
        unsigned MELODY_ON : 1;
        unsigned  LAST_BEEP_LONG: 1;
        unsigned  : 1;
        unsigned  : 1;
        unsigned  : 1;
        unsigned  : 1;
        unsigned  : 1;
    } bits;
} ff;

/*_____________________________________________________________________*/




/*TIMES*/

/*sec_div*/
uint32_t time_rotation; //????? ?? ???????????? (???)
unsigned time_rele_power; //????? ?? ???????? ???? (???)
unsigned time_rele_control;
unsigned time_rele_gap;

uint64_t tone_gap_millis;
char sec_count = 0;
char time_melody; //minute
char time_moving_wait;


/*ms_div*/

uint64_t millis = 0 ;
unsigned ms_tone_delay = 0;
/*_____________________________________________________________________*/



/*counters*/
char beep_short_count;
char beep_long_count;
char beep_double_count;

/*���������������������������������������������������������������������*/
/*���������������������������������������������������������������������*/

/*SERVICE*/

/*sound*/
void start_tone() {
    ff.bits.ZUM_BUSY = 1;
    ff.bits.TONE_ON = 1;
    ff.bits.TONE_OFF = 0;
}

void stop_tone() {
    ff.bits.ZUM_BUSY = 0;
    ff.bits.TONE_ON = 0;
    ff.bits.TONE_OFF = 1;
}

void beep_short() {
   // if (!ff.bits.ZUM_BUSY) {
        if (beep_short_count > 0)	beep_short_count--;
        ms_tone_delay = SHORT_ZUMMER_DELAY;
        ff.bits.LAST_BEEP_LONG = 0;
        start_tone();
    }
//}

void beep_long() {
  //  if (!ff.bits.ZUM_BUSY) {
        if (beep_long_count > 0) 	beep_long_count--;
        ms_tone_delay = LONG_ZUMMER_DELAY;
        ff.bits.LAST_BEEP_LONG = 1;
        start_tone();
    }
//}

void beep_double() {
    if (ff.bits.LAST_BEEP_LONG) {
        beep_short();
    } else {
        beep_long();
    }
}

/*moving*/
void go_close() {

    if (!ff.bits.CLOSING && !ff.bits.CLOSED && ff.bits.MOVING_ALLOWED) {
        ff.bits.CLOSING = 1;
        ff.bits.OPENED = 0;
        ff.bits.OPENING = 0;

        ff.bits.RELE_POWER_ON = 0;
        ff.bits.RELE_CONTROL_ON = 1;

        time_rele_control = RELE_GAP + RELE_POWER_WORK_DELAY + RELE_GAP;
        time_rele_power = RELE_POWER_WORK_DELAY;
        time_rele_gap = RELE_GAP;

        time_rotation = 0;

    }
}

void go_close_short() {

    if (!ff.bits.CLOSING && !ff.bits.CLOSED && ff.bits.MOVING_ALLOWED) {
        ff.bits.CLOSING = 1;
        ff.bits.OPENED = 0;
        ff.bits.OPENING = 0;

        ff.bits.RELE_POWER_ON = 0;
        ff.bits.RELE_CONTROL_ON = 1;

        time_rele_control = RELE_GAP + RELE_POWER_AUTOROTATION_DELAY + RELE_GAP;
        time_rele_power = RELE_POWER_AUTOROTATION_DELAY;
        time_rele_gap = RELE_GAP;


    }
}


void go_open() {

    if (!ff.bits.OPENED && !ff.bits.OPENING && ff.bits.MOVING_ALLOWED) {
        ff.bits.OPENING = 1;
        ff.bits.CLOSED = 0;
        ff.bits.CLOSING = 0;


        ff.bits.RELE_CONTROL_ON = 0;
        ff.bits.RELE_POWER_ON = 1;

        time_rele_power = RELE_POWER_WORK_DELAY;
        return;
    }
}

void go_close_alt() {

    if ((!ff.bits.CLOSED && ff.bits.MOVING_ALLOWED) || ff.bits.ALARM_ON) {
        ff.bits.OPENED = 0;
        ff.bits.CLOSED = 1;

        ff.bits.RELE_CONTROL_ON = 0;
        ff.bits.RELE_POWER_ON = 1;
    }
}

void go_open_alt() {
    if (!ff.bits.OPENED && ff.bits.MOVING_ALLOWED) {
        ff.bits.CLOSED = 0;
        ff.bits.OPENED = 1;

        ff.bits.RELE_CONTROL_ON = 0;
        ff.bits.RELE_POWER_ON = 0;
    }
}

void rele_off() {
    ff.bits.RELE_CONTROL_ON = 0;
    ff.bits.RELE_POWER_ON = 0;
    ff.bits.CLOSING = 0;
    ff.bits.OPENING = 0;
    ff.bits.CLOSED = 0;
    if (ff.bits.UNIVERSAL_VORK_MODE_ON) {
        ff.bits.OPENED = 1;
    } else {
        ff.bits.OPENED = 0;
    }
    ff.bits.MOVING_ALLOWED = 0;
    time_moving_wait = MOVING_WAIT_DELAY;
}

void close() {
    if (ff.bits.OPENING) {
        rele_off();
    } else {
        if (ff.bits.NORMAL_WORK_MODE_ON) {
            go_close();
        } else if (ff.bits.UNIVERSAL_VORK_MODE_ON) {
            go_close_alt();
        }
    }
}

void open() {
    if (ff.bits.CLOSING) {
        rele_off();
    } else {
        if (ff.bits.NORMAL_WORK_MODE_ON) {
            go_open();
        } else if (ff.bits.UNIVERSAL_VORK_MODE_ON) {
            go_open_alt();
        }
    }
}

void rele_tick() {

    if (ff.bits.OPENING && ff.bits.CLOSING) {
        return;
    }


    if (ff.bits.OPENING) {
        if (time_rele_power > 0) {
            time_rele_power--;
            if (time_rele_power == 0) {
                ff.bits.RELE_POWER_ON = 0;
                ff.bits.OPENED = 1;
                ff.bits.OPENING = 0;
                ff.bits.AUTOROTATION_WORK = 0;
            }
        }
    }


    if (ff.bits.CLOSING) {

        if (time_rele_gap == 0) {
            if (time_rele_power > 0) {
                ff.bits.RELE_POWER_ON = 1;
                time_rele_power--;
            } else {
                ff.bits.RELE_POWER_ON = 0;
            }
        } else {
            time_rele_gap--;
        }

        if (time_rele_control > 0) {
            time_rele_control--;
            if (time_rele_control == 0) {
                ff.bits.RELE_CONTROL_ON = 0;
                ff.bits.CLOSED = 1;
                ff.bits.CLOSING = 0;
            }
        }
    }

}
/*logic*/
void start_alarm() {
    ff.bits.ALARM_ON = 1;
    ff.bits.ALARM_OFF = 0;
    ff.bits.MELODY_ON = 1;
    ff.bits.SIREN = 1;
    sec_count=0;
}

void clear_alarm() {
    ff.bits.ALARM_ON = 0;
    ff.bits.ALARM_OFF = 1;
}

void fun_work() {
    {
        if (
            ff.bits.FUN_LOW &&
            !ff.bits.FUN_HIGH &&
            ff.bits.ALARM_OFF &&
            ff.bits.MOVING_ALLOWED &&
            !ff.bits.OPENED &&
            !ff.bits.OPENING &&
            !ff.bits.AUTOROTATION_WORK) {
            beep_short_count = 1;
            open();
        };
        if (
            ff.bits.FUN_HIGH &&
            ff.bits.MOVING_ALLOWED &&
            !ff.bits.FUN_LOW &&
            !ff.bits.CLOSED &&
            !ff.bits.CLOSING &&
            !ff.bits.AUTOROTATION_WORK) {
            beep_short_count = 2;
            close();
        }
    }
}

void switch_wm() {
    if (ff.bits.JUMP_LOW) {//go_universal_mode
        if (!ff.bits.UNIVERSAL_VORK_MODE_ON) {
            ff.bits.NORMAL_WORK_MODE_ON = 0;
            ff.bits.UNIVERSAL_VORK_MODE_ON = 1;
            rele_off();
          //  beep_long_count = 2;
        }
    } else if (ff.bits.JUMP_HIGH) {//go_norm_mode
        if (!ff.bits.NORMAL_WORK_MODE_ON) {
            ff.bits.NORMAL_WORK_MODE_ON = 1;
            ff.bits.UNIVERSAL_VORK_MODE_ON = 0;
            rele_off();
          //  beep_long_count = 1;
        }
    }
}

void autorotation_work() {

    if ((time_rotation > (AUTOROTATION_DELAY + RELE_POWER_AUTOROTATION_DELAY + RELE_GAP * 2)) && //???????? ???? ????????? ?????
            !ff.bits.OPENED &&
            !ff.bits.OPENING &&
            ff.bits.ALARM_OFF &&
            ff.bits.NORMAL_WORK_MODE_ON
       ) {
        open();
        beep_short_count=1;

        time_rotation = 0;
    }
    if ((time_rotation > AUTOROTATION_DELAY) &&
            !ff.bits.CLOSED &&
            !ff.bits.CLOSING &&
            ff.bits.ALARM_OFF &&
            ff.bits.NORMAL_WORK_MODE_ON
       ) {
        go_close_short();
        ff.bits.AUTOROTATION_WORK = 1;
    }

}


/*���������������������������������������������������������������������*/

/*TIMES*/


void minute_tick() {

    if (time_melody > 0) {
        time_melody--;
    } else {
        if (time_melody == 0) {
            ff.bits.SIREN = 1;
            time_melody = MELODY_REPEAT_DELAY;
        }
    };

}


void sec_30_work() {
    if (ff.bits.MELODY_ON) {
        if (ff.bits.SIREN) {
            ff.bits.SIREN = 0;
        } else {
            beep_short_count = 3;  //connect to ms200
        }
    }
}



void sec_work() {

    ff.bits.SEC_LOCK = 1;
    sec_count++;

    //back-forward gap
    if (!ff.bits.MOVING_ALLOWED) {
        if (time_moving_wait > 0) {
            time_moving_wait--;
        } else {
            ff.bits.MOVING_ALLOWED = 1;
        }
    }

    //autorotation tick
    if (ff.bits.NORMAL_WORK_MODE_ON) {
        if (!ff.bits.CLOSED) {
            time_rotation++;
        }
        rele_tick();
    }

    //led tick
    if (ff.bits.ALARM_ON || ff.bits.CLOSING || ff.bits.OPENING) {
        ff.bits.LED_ON = !ff.bits.LED_ON;
    }
    else {
        static char iled;
        iled++;
        if (iled > 2) {
            ff.bits.LED_ON = !ff.bits.LED_ON;
            iled = 0;
        }

    }

    //melody tick
    if (ff.bits.ALARM_ON) {

        if (sec_count == 30|| sec_count==60) {
            sec_30_work();
        }

        if (sec_count == 60) {
            minute_tick();
            sec_count = 0;
        }

    }
}

void ms_200_work() {

    ff.bits.SEC_LOCK = 0;

    if (ff.bits.ALARM_ON) {
        if (ff.bits.SIREN) {
            beep_double();
        } else {
            if (beep_short_count > 0) {     //connect to sec30
                beep_short();
            }

        }
    } else if (ff.bits.ALARM_OFF) {


        if ((beep_short_count > 0) && (beep_long_count > 0)) {
            beep_double();
        } else {
            if (beep_short_count > 0) {
                beep_short();
            }
            if (beep_long_count > 0) {
                beep_long();
            }
        }

    }
}

void PIN_POWER_MEAS_SetHigh(void);

void ms_10_work() {

    static char f;

    if ( !ff.bits.ALARM_ON &&(ff.bits.NORMAL_WORK_MODE_ON || ff.bits.UNIVERSAL_VORK_MODE_ON)) {     //
        if (!f) {
            PIN_POWER_MEAS_SetHigh();
            f=1;
        } else {
            adc_ordinary_conversion_trigger_set(ADC1,ADC12_ORDINARY_TRIG_SOFTWARE,TRUE);
            f=0;
        }
    }
}

void ms_tick() {
	
 //  EventRecord2(0x01,millis,0);
       

    static uint64_t ms200_count = 0;
    static uint64_t ms10_count = 0;
    static uint64_t ms1000_count = 0;


    if (ms_tone_delay > 0) {
        ms_tone_delay--;
    }   else {
        stop_tone();
    }


    ff.bits.ALLOW_JUMP = 1;

    if (ms10_count <=millis) {
        ms10_count = millis+10;
        ms_10_work();
    }

    if (ms200_count <= millis) {
        ms200_count = millis + 200;
        ms_200_work();
    }


    if (ms1000_count <= millis) {
        ms1000_count = millis+1000;
      //  if (!ff.bits.SEC_LOCK)
					sec_work();
    }


    ++millis;
	//	 EventRecord2(0x02,millis,0);
}

/*���������������������������������������������������������������������*/

/*HARDWARE*/



void gpio_set(gpio_type *PORT, uint32_t PIN, gpio_drive_type DRIVE, gpio_mode_type MODE, gpio_output_type OUT_TYPE, gpio_pull_type PULL ) {


    gpio_init_type pinx;

    gpio_init_type *pina = &pinx;

    pinx.gpio_drive_strength= DRIVE;
    pinx.gpio_mode =MODE;
    pinx.gpio_out_type=OUT_TYPE;
    pinx.gpio_pins = PIN;
    pinx.gpio_pull = PULL;

    gpio_init( PORT,pina);

}

void timer_init() {




    nvic_irq_enable(TMR6_GLOBAL_IRQn,35,36);

    TMR6->iden_bit.ovfien =1;

    TMR6 ->ctrl1_bit.ocmen = 0;

    TMR6 ->ctrl1_bit.ovfen = 0;

    tmr_channel_buffer_enable(TMR6,TRUE);

    tmr_base_init(TMR6,1,500);

    tmr_counter_enable(TMR6,TRUE);



}


void hardware_init() {

    wdt_register_write_enable(TRUE);
    wdt_divider_set(WDT_CLK_DIV_8);
    wdt_register_write_enable(FALSE);
  //  wdt_enable();                  //todo
    //wdt_counter_reload();


    crm_hick_sclk_frequency_select(CRM_HICK_SCLK_8MHZ);
    crm_clock_source_enable (CRM_CLOCK_SOURCE_HICK,TRUE);

    crm_hick_divider_select(CRM_HICK48_NODIV);



    crm_ahb_div_set(CRM_AHB_DIV_1);


    crm_periph_clock_enable(CRM_GPIOA_PERIPH_CLOCK,TRUE);
  //  crm_periph_clock_enable(CRM_GPIOB_PERIPH_CLOCK,TRUE);
    //crm_periph_clock_enable(CRM_ADC1_PERIPH_CLOCK,TRUE);
    //crm_periph_clock_enable(CRM_TMR6_PERIPH_CLOCK,TRUE);


    gpio_set(GPIOA,
             GPIO_PINS_0,
             GPIO_DRIVE_STRENGTH_MODERATE,
             GPIO_MODE_OUTPUT,
             GPIO_OUTPUT_PUSH_PULL,
             GPIO_PULL_NONE);

    gpio_set(GPIOA,
             GPIO_PINS_1,
             GPIO_DRIVE_STRENGTH_MODERATE,
             GPIO_MODE_OUTPUT,
             GPIO_OUTPUT_PUSH_PULL,
             GPIO_PULL_NONE);

    gpio_set(GPIOA,
             GPIO_PINS_2,
             GPIO_DRIVE_STRENGTH_MODERATE,
             GPIO_MODE_OUTPUT,
             GPIO_OUTPUT_PUSH_PULL,
             GPIO_PULL_NONE);

    gpio_set(GPIOA,
             GPIO_PINS_3,
             GPIO_DRIVE_STRENGTH_MODERATE,
             GPIO_MODE_OUTPUT,
             GPIO_OUTPUT_PUSH_PULL,
             GPIO_PULL_NONE);

    gpio_set(GPIOA,
             GPIO_PINS_4,
             GPIO_DRIVE_STRENGTH_MODERATE,
             GPIO_MODE_OUTPUT,
             GPIO_OUTPUT_PUSH_PULL,
             GPIO_PULL_NONE);

    gpio_set(GPIOA,
             GPIO_PINS_5,
             GPIO_DRIVE_STRENGTH_MODERATE,
             GPIO_MODE_OUTPUT,
             GPIO_OUTPUT_PUSH_PULL,
             GPIO_PULL_NONE);

    gpio_set(GPIOA,
             GPIO_PINS_6,
             GPIO_DRIVE_STRENGTH_MODERATE,
             GPIO_MODE_OUTPUT,
             GPIO_OUTPUT_PUSH_PULL,
             GPIO_PULL_NONE);

    gpio_set(GPIOA,
             GPIO_PINS_7,
             GPIO_DRIVE_STRENGTH_MODERATE,
             GPIO_MODE_OUTPUT,
             GPIO_OUTPUT_PUSH_PULL,
             GPIO_PULL_NONE);

    gpio_set(GPIOA,
             GPIO_PINS_8,
             GPIO_DRIVE_STRENGTH_MODERATE,
             GPIO_MODE_OUTPUT,
             GPIO_OUTPUT_PUSH_PULL,
             GPIO_PULL_NONE);
		/* I2C pins */
		gpio_set(GPIOA,
             GPIO_PINS_9,
             GPIO_DRIVE_STRENGTH_MODERATE,
             GPIO_MODE_MUX,
             GPIO_OUTPUT_OPEN_DRAIN,
             GPIO_PULL_UP);
						 
		gpio_set(GPIOA,
             GPIO_PINS_10,
             GPIO_DRIVE_STRENGTH_MODERATE,
             GPIO_MODE_MUX,
             GPIO_OUTPUT_OPEN_DRAIN,
             GPIO_PULL_UP);
		/************/

//    timer_init();
/*
    nvic_irq_enable(ADC1_CMP_IRQn,37,38);

    adc_base_config_type *adc1;
    adc_base_default_para_init(adc1);
    adc1 ->repeat_mode = FALSE;
    adc_base_config(ADC1,adc1);

    adc_enable(ADC1,TRUE);
    adc_interrupt_enable(ADC1,ADC_CCE_INT,TRUE);
    adc_ordinary_channel_set(ADC1,ADC_CHANNEL_5,1,ADC_SAMPLETIME_239_5);
*/
}











void hardware_work() {
    gpio_bits_write(GPIOA,GPIO_PINS_6,(confirm_state) (ff.bits.ALARM_ON));
    gpio_bits_write(GPIOA,GPIO_PINS_2,(confirm_state) (ff.bits.RELE_CONTROL_ON));
    gpio_bits_write(GPIOA,GPIO_PINS_0,(confirm_state) (ff.bits.RELE_POWER_ON));
    gpio_bits_write(GPIOB,GPIO_PINS_1,(confirm_state) (ff.bits.LED_ON));
    if (ff.bits.TONE_OFF) {
        gpio_bits_reset(GPIOB,GPIO_PINS_0);
    };
}




void zummer_switch() {


    if(ff.bits.TONE_ON) gpio_bits_write(GPIOB,GPIO_PINS_0,(confirm_state) (!GPIOB ->odt_bit.odt0));
//    if(ff.bits.TONE_ON) gpio_bits_write(GPIOB,GPIO_PINS_0,!GPIOB ->odt_bit.odt0);   //todo
}

void PIN_POWER_MEAS_SetHigh() {
    gpio_bits_set(GPIOA,GPIO_PINS_4);
};

void	PIN_POWER_MEAS_SetLow   () {
    gpio_bits_reset(GPIOA,GPIO_PINS_4);
};

uint16_t   ADC_GetConversion() {
    return (adc_result);
};

void get_wsp() {

    if (ff.bits.ALLOW_MEASURE) {

        BAD_WSP_VOLTAGE = (LOW_WATER_RESISTANSE / ((UP_RESISTANSE + LOW_WATER_RESISTANSE) / 4096));
        GOOD_WSP_VOLTAGE =(HIGH_WATER_RESISTANSE / ((UP_RESISTANSE + HIGH_WATER_RESISTANSE) / 4096));

        static signed char bad_measures_counter = 0;
        uint16_t res = ADC_GetConversion();
        PIN_POWER_MEAS_SetLow();
        if (res < BAD_WSP_VOLTAGE) {
            bad_measures_counter++;
        } else {
            if (res > GOOD_WSP_VOLTAGE) {
                bad_measures_counter--;
            }
        }
        if (bad_measures_counter > WSP_MEAS_COUNT) {
            start_alarm();
            bad_measures_counter = WSP_MEAS_COUNT;
        }
        if (bad_measures_counter < -WSP_MEAS_COUNT) {
            clear_alarm();
            bad_measures_counter = -WSP_MEAS_COUNT;
        }
        ff.bits.ALLOW_MEASURE = 0;
    }
}

char PIN_FUN_STATE_GetValue() {
    return(fun_result);
}


void get_fun() {

    if(!(ff.bits.NORMAL_WORK_MODE_ON || ff.bits.UNIVERSAL_VORK_MODE_ON)) {
        PIN_POWER_MEAS_SetHigh();
        fun_result = gpio_input_data_bit_read(GPIOA,GPIO_PINS_3);
        PIN_POWER_MEAS_SetLow();
        ff.bits.ALLOW_FUN =1;
    }

    if (ff.bits.ALLOW_FUN) {


        static signed char fun_counter;

        if (PIN_FUN_STATE_GetValue()) fun_counter--;
        else fun_counter++;

        if (fun_counter > FUN_MEAS_COUNT) {
            fun_counter = FUN_MEAS_COUNT;
            ff.bits.FUN_LOW = 0;
            ff.bits.FUN_HIGH = 1;
        } else if (fun_counter<-FUN_MEAS_COUNT) {
            fun_counter = -FUN_MEAS_COUNT;
            ff.bits.FUN_LOW = 1;
            ff.bits.FUN_HIGH = 0;
        }
        ff.bits.ALLOW_FUN = 0;
    }
}


char PIN_JUMP_STATE_GetValue() {
    return(gpio_input_data_bit_read(GPIOA,GPIO_PINS_7));
}


void get_jump() {

    static signed char jump_counter;

    if (ff.bits.ALLOW_JUMP) {


        if (PIN_JUMP_STATE_GetValue() ) jump_counter++;
        else jump_counter--;

        if (jump_counter > JUMP_MEAS_COUNT) {
            jump_counter = JUMP_MEAS_COUNT;
            ff.bits.JUMP_LOW = 0;
            ff.bits.JUMP_HIGH = 1;
        } else if (jump_counter<-JUMP_MEAS_COUNT) {
            jump_counter = -JUMP_MEAS_COUNT;
            ff.bits.JUMP_LOW = 1;
            ff.bits.JUMP_HIGH = 0;
        }
        ff.bits.ALLOW_JUMP = 0;
    }

}


void TMR6_GLOBAL_IRQHandler(void) {


    static char i =0;
    ++i;

    if (i>=8) {
        ms_tick();
        i=0;
    }

    zummer_switch();
    tmr_period_value_set(TMR6,1);
    TMR6 ->ists_bit.ovfif =0;
}

void ADC1_CMP_IRQHandler(void) {
    wdt_counter_reload();
    adc_result = adc_ordinary_conversion_data_get(ADC1);
    fun_result = gpio_input_data_bit_read(GPIOA,GPIO_PINS_3);
    ff.bits.ALLOW_MEASURE = 1;
    ff.bits.ALLOW_FUN =1;
}


void PIN_RELE_POWER_SetLow() {
    gpio_bits_reset(GPIOA,GPIO_PINS_0);
};
void PIN_RELE_CONTROL_SetLow() {
    gpio_bits_reset(GPIOA,GPIO_PINS_2);
};
void PIN_ALARM_STATE_SetLow() {
    gpio_bits_reset(GPIOA,GPIO_PINS_6);
};
void PIN_ZUMMER_SetLow() {
    gpio_bits_reset(GPIOB,GPIO_PINS_0);
};
void PIN_LED_SetLow() {
    gpio_bits_reset(GPIOB,GPIO_PINS_1);
};

void start_setup() {
    hardware_init(); // initialize the device

    ff.value = 0;

    PIN_RELE_POWER_SetLow();
    PIN_RELE_CONTROL_SetLow();
    PIN_ALARM_STATE_SetLow();
    PIN_POWER_MEAS_SetLow();
    PIN_ZUMMER_SetLow();
    PIN_LED_SetLow();

    time_rotation = 0;
    time_rele_power = 0;
    time_rele_control = 0;
    time_rele_gap = 0;
    ms_tone_delay = 0;


    time_melody = 0;
   


}
/*���������������������������������������������������������������������*/

void printNumber(uint8_t array[], uint8_t number) {
	
	for (uint8_t i = 0; i < 8; i++) {
		if (array[number] & (1<<i)) {
			gpio_bits_set(GPIOA, 1<<i);
		} else {
			gpio_bits_reset(GPIOA, 1<<i);
		}
	}
}

void i2cInit() {
	//I2C1->oaddr1_bit.addr1mode=0;
	//I2C1->ctrl1_bit.genstart=1;
}

void i2cSetData() {
	// 7-bit option
	I2C1->oaddr1_bit.addr1mode=0;
	
	// Addres for target
	I2C1->oaddr1_bit.addr1=0x68;
	
	// Addres for target registor
	I2C1->dt_bit.dt=0x0F;
	
	// Run transmit
	I2C1->ctrl1_bit.genstart=1;
	
	// Waiting for readiness
	while(I2C1->sts1_bit.startf == 0) {
		printNumber(numbers, 0);
	}
	while(I2C1->sts1_bit.addr7f == 0) {
		printNumber(numbers, 1);
	}
	while(I2C1->sts1_bit.stopf == 0) {
		printNumber(numbers, 2);
	}
	
}

void i2cGetData() {
	
}
 

int main(void) {
	
  //start_setup();
	hardware_init();
	//i2cInit();
	i2cSetData();
	//gpio_bits_set(GPIOA, 0b00000010>>0);
	//printNumber(numbers, 9);
  //  EventRecorderInitialize(EventRecordAll,1);
	/*
		gpio_set(GPIOA,
             GPIO_PINS_0,
             GPIO_DRIVE_STRENGTH_MODERATE,
             GPIO_MODE_OUTPUT,
             GPIO_OUTPUT_PUSH_PULL,
             GPIO_PULL_NONE);
	*/

    while (1) {

     //   wdt_counter_reload();

        //hardware_work();
			//gpio_bits_set(GPIOA, GPIO_PINS_0);
			//gpio_bits_reset(GPIOA, GPIO_PINS_0);
    }
}
