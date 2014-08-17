/*
  Battery voltage monitor with ATTiny45/85
  Klaas De Craemer - 8/2014
  https://sites.google.com/site/klaasdc/tiny-battery-monitor
 */
#include <avr/io.h>
#include <avr/interrupt.h>

//PB1 Has a red led
//PB2 Has a green led
int redled = 1;
int greenled = 2;
//ADC2 Has input through voltage divider
int voltIn = A2;

//Offset to be applied on the ADC measurement
uint16_t offset = 36;

//To hide the voltage status until enough samples are taken
uint8_t startupComplete = 0;

//For averaging of the measurements
#define  NB_MEAS  4
#define  NB_MEAS_SHIFT  2
volatile uint8_t measIdx = 0;
volatile uint16_t measuredValues[NB_MEAS];

#define NOP __asm__ __volatile__ ("nop\n\t")

void initTimerCounter1(); // function prototype
void setup() {                
  // initialize the LED connections as outputs.
  pinMode(redled, OUTPUT);    
  pinMode(greenled, OUTPUT);
  
  initTimerCounter1();
}

void initTimerCounter1() {    // initialize Timer1
    TCCR1 = 0;                  //stop the timer
    TCNT1 = 0;                  //zero the timer
    GTCCR = _BV(PSR1);          //reset the prescaler
    OCR1A = 30;                //set the compare value
    OCR1C = 30;
    TIMSK = _BV(OCIE1A);        //interrupt on Compare Match A
    //start timer, ctc mode, prescaler clk/16384   
    TCCR1 = _BV(CTC1) | _BV(CS13) | _BV(CS12) | _BV(CS11) | _BV(CS10);
    sei();
}

ISR(TIM1_COMPA_vect) {
  //Store new measurement into array
  measuredValues[measIdx] = analogRead(voltIn);//Value between [0 and 1023]
  measIdx++;
  if (measIdx >= NB_MEAS){
    measIdx = 0;
    startupComplete = 1;
  }
}

enum ledState {
  BLANK,
  RED,
  GREEN,
  ORANGE,
  BLINK_RED_S,
  BLINK_GREEN_S,
  BLINK_ORANGE_S,
};

ledState curLedState = BLANK;

void loop() {
  //Take average of samples
  uint16_t avgVal = 0;
  for (int i=0; i<NB_MEAS; i++){
    avgVal += measuredValues[i];
  }
  avgVal = avgVal >> NB_MEAS_SHIFT;
  
  //Add offset
  avgVal += offset;
  
  /*  Decide on LED color
      0.01953125 V/bit
      1023 = 20 V
      768 = 15.0V
      620 = 12.1V
      605 = 11.8V
      573 = 11.2V
      563 = 11.0V
      548 = 10.8V
      512 = 10.0V
      256 =  5.0V
      0 = 0V
  */
  if (startupComplete == 0){
    curLedState = BLANK;//Keep LED dark until we have NB_MEAS samples
  } else if (avgVal>711){//Higher then 13.9V
    curLedState = BLINK_ORANGE_S;
  } else if (avgVal<=711 && avgVal>=620){//Between 13.9V and 12.1V
    curLedState = GREEN;
  } else if (avgVal<=620 && avgVal>=573){//Between 12.1V and 11.2V
    curLedState = ORANGE;
  } else if (avgVal<573 && avgVal>=547){//Between 11.2V and 10.7V
    curLedState = RED;
  } else {//Lower than 10.7V
    curLedState = BLINK_RED_S;
  }
  
  switch (curLedState){
    case ORANGE:
      digitalWrite(redled, HIGH);
      digitalWrite(greenled, HIGH);
      break;
    case RED:
      digitalWrite(redled, HIGH);
      digitalWrite(greenled, LOW);
      break;
    case GREEN:
      digitalWrite(redled, LOW);
      digitalWrite(greenled, HIGH);
      break;
    case BLINK_ORANGE_S:
      digitalWrite(redled, HIGH);
      digitalWrite(greenled, HIGH);
      delayOwn(400);
      digitalWrite(redled, LOW);
      digitalWrite(greenled, LOW);
      delayOwn(400);
      break;
    case BLINK_RED_S:
      digitalWrite(redled, HIGH);
      digitalWrite(greenled, LOW);
      delayOwn(400);
      digitalWrite(redled, LOW);
      digitalWrite(greenled, LOW);
      delayOwn(400);
      break;
   default:
      digitalWrite(redled, LOW);
      digitalWrite(greenled, LOW);
  }
}

//Simple delay routine to avoid the built-in one that messes with our timer
void delayOwn(int value) {
  for (int i=0; i<value; i++){
    for (uint8_t j=0; j<126; j++){
      NOP;
    }
  }
}
