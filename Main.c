#include "avr.h"
#include "avr/interrupt.h"
#include "stdlib.h"

#define MAINDDR DDRA
#define MAINPORT PORTA
#define MAINPIN PINA

#define  SPEAKER_PORT 0       
#define  RECORD_BUTTON_PORT 2   
#define  LED_BLUE_PORT 4           
#define  LED_GREEN_PORT 5         

#define  threshold 1          
#define  BEAT_OFF_LIMIT 25    
#define  BEAT_OFF_AVG 15 
#define  DEBOUNCING 150    

#define  MAX_BEATS 20       
#define  BEAT_WAIT 40000  

unsigned short storedPassword[MAX_BEATS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; 
unsigned short enteredPassword[MAX_BEATS];   
unsigned short beatSensor = 0;          
unsigned char recordButton = 0;  
unsigned short current_voltage = 0;

unsigned long systemTime = 0;

long map(long x, long in_min, long in_max, long out_min, long out_max)
{
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void initializeTimer()
{
	SET_BIT(TIMSK, TICIE1);
	TCCR1A = 0x00;
	TCCR1B = 0x1C;
	ICR1 = (unsigned short)((XTAL_FRQ / 256) * (1 / 100));
}

ISR(TIMER1_CAPT_vect)
{
	systemTime++;
}

void voltmeter_sample_voltage()
{
	SET_BIT(ADCSRA, 3);
	SET_BIT(ADCSRA, 7);
	SET_BIT(ADCSRA, 4);
	SET_BIT(ADCSRA, 6);
}

void initializeADC()
{
	SET_BIT(ADMUX, 6);
	SET_BIT(ADCSRA, 3);
	voltmeter_sample_voltage();
}

ISR(ADC_vect)
{
	current_voltage = ADC;
	voltmeter_sample_voltage();
}


unsigned char checkPassword(){
  unsigned short sizeEnteredPassword = 0;
  unsigned short sizeStoredPassword = 0;
  unsigned short intervalLimit = 0;          			
  
  unsigned short i=0;
  for (i=0;i<MAX_BEATS;i++){
    if (enteredPassword[i] > 0){
      sizeEnteredPassword++;
    }
    if (storedPassword[i] > 0){  				
      sizeStoredPassword++;
    }
    
    if (enteredPassword[i] > intervalLimit){ 
      intervalLimit = enteredPassword[i];
    }
  }
  

  if (recordButton==1){
      for (i=0;i<MAX_BEATS;i++){
        storedPassword[i]= map(enteredPassword[i],0, intervalLimit, 0, 100); 
      }
      CLR_BIT(MAINPORT,LED_GREEN_PORT);
      CLR_BIT(MAINPORT,LED_BLUE_PORT);
      wait_avr(1000);
      SET_BIT(MAINPORT, LED_GREEN_PORT);
      SET_BIT(MAINPORT, LED_BLUE_PORT);
      wait_avr(50);
      for (i = 0; i < MAX_BEATS ; i++){
        CLR_BIT(MAINPORT,LED_GREEN_PORT);
        CLR_BIT(MAINPORT,LED_BLUE_PORT);  

        if (storedPassword[i] > 0){                                   
          wait_avr(storedPassword[i]*6);
		  SET_BIT(MAINPORT, LED_GREEN_PORT);
          SET_BIT(MAINPORT, LED_BLUE_PORT);
        }
        wait_avr(50);
      }
	  return 0;
  }
  
  if (sizeEnteredPassword != sizeStoredPassword){
    return 0; 
  }
  
  int totaltimeDifferences=0;
  int timeDiff=0;
  for (i=0;i<MAX_BEATS;i++){ 
    enteredPassword[i]= map(enteredPassword[i],0, intervalLimit, 0, 100);      
    timeDiff = abs(enteredPassword[i] - storedPassword[i]);
    if (timeDiff > BEAT_OFF_LIMIT){ 
      return 0;
    }
    totaltimeDifferences += timeDiff;
  }

  if (totaltimeDifferences/sizeStoredPassword>BEAT_OFF_AVG){
    return 0; 
  }
  
  return 1;
}	

void passwordDetected(){
	unsigned short i=0;	
	wait_avr(100);            
	for (i=0; i < 5; i++){
		CLR_BIT(MAINPORT,LED_GREEN_PORT);
		wait_avr(100);
		SET_BIT(MAINPORT,LED_GREEN_PORT);
		wait_avr(100);
	}
}

void sampleBeats(){

	unsigned short i = 0;
	for (i=0;i<MAX_BEATS;i++){
		enteredPassword[i]=0;
	}
	
	unsigned short beatCounter = 0;         		
	unsigned short whenBeatStarted = systemTime;           			
	unsigned short tempTime = systemTime;
	
	CLR_BIT(MAINPORT, LED_GREEN_PORT);      		
	if (recordButton==1){
		CLR_BIT(MAINPORT,LED_BLUE_PORT);                     
	}
	
	wait_avr(DEBOUNCING);                       	      
	
	SET_BIT(MAINPORT,LED_GREEN_PORT);
	if (recordButton==1){
		SET_BIT(MAINPORT,LED_BLUE_PORT);
	}
	do {
		beatSensor = current_voltage;
		if (beatSensor >= threshold){   
			
			tempTime = systemTime;
			enteredPassword[beatCounter] = tempTime - whenBeatStarted;
			beatCounter ++;                   
			whenBeatStarted = tempTime;

			CLR_BIT(MAINPORT,LED_GREEN_PORT);
			if (recordButton == 1){
				CLR_BIT(MAINPORT,LED_BLUE_PORT);                      
			}
			
			wait_avr(DEBOUNCING);                          
			
			SET_BIT(MAINPORT,LED_GREEN_PORT);
			if (recordButton == 1){
				SET_BIT(MAINPORT,LED_BLUE_PORT);
			}
		}

		tempTime = systemTime;
		

	} while ((tempTime - whenBeatStarted < BEAT_WAIT) && ( beatCounter < MAX_BEATS ));
	

	if (recordButton == 0){            
		if (checkPassword() == 1){
			passwordDetected();
			} else {
			CLR_BIT(MAINPORT,LED_GREEN_PORT);  		
			for (i=0;i<4;i++){
				SET_BIT(MAINPORT,LED_BLUE_PORT);
				wait_avr(100);
				CLR_BIT(MAINPORT, LED_BLUE_PORT);
				wait_avr(100);
			}
			SET_BIT(MAINPORT,LED_GREEN_PORT);
		}
		} else { 
		checkPassword();
		
		CLR_BIT(MAINPORT,LED_BLUE_PORT);
		SET_BIT(MAINPORT, LED_GREEN_PORT);
		for (i=0;i<3;i++){
			wait_avr(100);
			SET_BIT(MAINPORT,LED_BLUE_PORT);
			CLR_BIT(MAINPORT,LED_GREEN_PORT);
			wait_avr(100);
			CLR_BIT(MAINPORT,LED_BLUE_PORT);
			SET_BIT(MAINPORT,LED_GREEN_PORT);
		}
	}
}

void ini() {
	SET_BIT(MAINDDR,LED_BLUE_PORT);
	SET_BIT(MAINDDR,LED_GREEN_PORT);
	CLR_BIT(MAINDDR,RECORD_BUTTON_PORT);
	
	
	SET_BIT(MAINPORT, LED_GREEN_PORT);     
	
	initializeTimer();
	initializeADC();
	sei();
}

int main()
{
	ini();
	for(;;)
	{
		beatSensor = current_voltage;
		
		if (GET_BIT(MAINPIN, RECORD_BUTTON_PORT)){ 
			recordButton = 1;         
			SET_BIT(MAINPORT,LED_BLUE_PORT);       
			} else {
			recordButton = 0;
			CLR_BIT(MAINPORT,LED_BLUE_PORT);
		}
		
		if (beatSensor >= threshold){
			sampleBeats();
		}	
	}
	return 0;
}