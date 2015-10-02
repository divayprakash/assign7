//**********Heaader Files**********//
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/power.h>
//*********************************// 


//**********Global variables**********//
volatile int flag = 0;   
volatile int switch_flag = 0; 
//************************************//


//**********Initialize WDT**********//
void wdt_init()
{
    if(MCUSR & _BV(WDRF))                           //if prev reset caused by WDT
    {                  
        MCUSR &= ~_BV(WDRF);                        //clear WDT reset flag
        WDTCSR |= (_BV(WDCE) | _BV(WDE));           //enable WDCE
        WDTCSR = 0x00;                              //disable WDT
    }
    WDTCSR |= (_BV(WDCE) | _BV(WDE));               //enable WDCE, set WDT to interrupt mode
    WDTCSR =  _BV(WDIE) |  _BV(WDP1) | _BV(WDP0);   //set WDT timeout to ~0.125 seconds
    WDTCSR |= _BV(WDIE);
}
//**********************************//


//**********Initialize output**********//
void ex_init()
{
    DDRB |= 0x20;                                   //set PORTB5 as output
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);            //set sleep mode  
}
//*************************************//


//**********Initialize ADC**********//
void adc_init()
{   
    ADCSRA |= 1<<ADPS2 | 1<<ADPS1 | 1<<ADPS0;       //Prescaler at 128 so we have an 125Khz clock source
    ADMUX |= 1<<REFS0;
    ADMUX &= ~(1<<REFS1);                           //Vcc as Vref
    ADMUX &= ~(1<<ADLAR);
}
//**********************************//


//**********Main function**********// 
int main(void) 
{
    cli();											//disable interrupts
    wdt_init();
    ex_init();
    adc_init();
    sei();                                  		//enables interrupts
    while(1) 										//main loop
    {                              		
        if(flag) 									//if flag is set
        {									
            cli();									//disable interrupts
            flag = 0;								//set flag to 0	
            sleep_bod_disable();					//diable brown-out detection
            sei();									//enable interrupts
            sleep_cpu();                 			//put to sleep with sleep mode as prviously defined
        }
    }
}
//*********************************//

   
//**********WDT Interrupt Service Routine**********//   
ISR(WDT_vect)
{
    sleep_disable();                    			//wake from sleep
    PRR &= ~(1<<PRADC);                 			//disable ADC power reduction
    if (switch_flag == 0) 
    {
    	//FOR LDR
    	ADMUX &= ~(1<<MUX0);
	    ADMUX &= ~(1<<MUX1);
	    ADMUX &= ~(1<<MUX2);
	    ADMUX &= ~(1<<MUX3);                		//ADC channel set to PC0
	    ADCSRA |= 1<<ADEN;                  		//enable ADC
	    ADCSRA |= 1<<ADSC;                  		//start conversion
	    while(!(ADCSRA & (1<<ADIF)));       		//loop until conversion completes
	    ADCSRA |= _BV(ADIF);                		//clear ADIF
	    int value = ADC;                    		//read converted value from ADC
	    if (value < 100) PORTB |= 1<<PB5;   		//led on
	    else PORTB &= ~(1<<PB5);					//led off
	    flag = 1;                           		//set flag to 1
	  	switch_flag = 1;
    }
    else if (switch_flag == 1) 
    {	
    	//FOR THERMISTOR
	    ADMUX |= 1<<MUX0;
	    ADMUX &= ~(1<<MUX1);
	    ADMUX &= ~(1<<MUX2);
	    ADMUX &= ~(1<<MUX3);                   		//ADC channel set to PC1
    	ADCSRA |= 1<<ADEN;                  		//enable ADC
	    ADCSRA |= 1<<ADSC;                  		//start conversion
	    while(!(ADCSRA & (1<<ADIF)));       		//loop until conversion completes
	    ADCSRA |= _BV(ADIF);                		//clear ADIF
	    int value = ADC;                    		//read converted value from ADC
	    if (value > 500) PORTB |= 1<<PB4;   		//led on
	    else PORTB &= ~(1<<PB4);					//led off
	    flag = 1;                           		//set flag to 1
	  	switch_flag = 0;
    }
    ADCSRA &= ~(1<<ADEN);               			//disable ADC before sleep
    PRR |= 1<<PRADC;                    			//shutdown ADC
    sleep_enable();                     			//enable sleep but not entering sleep yet
}
//*************************************************//