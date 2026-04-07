/*
2026-04-07

Testing deep sleep with Arduino Uno atmega328p

It is based on some old code that always would reset the mcu
after sleeping. I now realized it lacked an ISR handler, and
with that in place, it happily blinks the LED two times after
returning from sleep.

I used the Uno R3 to program it, then moved the chip to a standalone 
circuit with a 16 MHz crystal, plus an LED on pin 13, and nothing more.

Hooked up a multimeter in series to measure sleep power consumption:

5-6 uA

*/


void setup() {
  https://www.youtube.com/watch?v=urLSDi7SD8M

  disableWatchdogInterrupt();

  for (int i=0; i<20; i++) pinMode(i,OUTPUT);  // saves milliamps

  pinMode(LED_BUILTIN, OUTPUT);
  delay(2000);
}

ISR(WDT_vect) {
  cli();
  disableWatchdogInterrupt();
  sei();
}

void blink (int count) {
  for (int i=0; i<count; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
  }
  delay(600);
}


void disableWatchdogInterrupt() {
  WDTCSR &= ~(1<<WDIE);
}

void enableADC() {
  ADCSRA |= (1<<7);
}


// MUST CHECK THIS AS WELL: 
//
// https://forum.arduino.cc/t/watchdog-timer-interrupt-and-external-interrupt/194488/9


void doDeepSleep() {

  //blip(2);
  // https://www.youtube.com/watch?v=urLSDi7SD8M

  // disable ADC
  ADCSRA &= ~(1<<7);

  cli();

  // Setup watchdog timer to wake from sleep
  // 7     6   5    4    3   2    1    0
  // WDIF WDIE WDP3 WDCE WDE WDP1 WDP1 WDP0
  // WDP* are prescaler bits, note WDP3 separate from the others

  WDTCSR = (1<<WDCE) | (1<<WDE) ; // b00011000;  // (24); // change enable and WDE - also resets


  WDTCSR = (1<<WDP3) | (1<<WDP0) ; // b00100001;  // (33); // prescalers only, get rid of WDCE and WDE - 8 seconds
  //WDTCSR = (1<<WDP2) | (1<<WDP1) | (1<<WDP0) ; // 2 seconds
  //WDTCSR = (1<<WDP3) ; // 4 seconds
  
  WDTCSR |= (1<<WDIE)  ; // (1<<6); // enable interrupt mode: WDE=0, WDIE=1 (but also requires FUSE WDTON=1)
  //WDTCSR |= (1<<WDE) | (1<<WDIE)  ; // interrupt and system reset
  
    // WDE | WDIE = interrupt and system reset

    // Even when only settng WDIE we get system reset. Do I need to create an interrupt handler
 
  sei();
 
  // enable sleep
  SMCR |= (1<<2); // power down mode
  SMCR |= 1; // enable sleep

  
  // BOD disable - must go to sleep within 4 clock cycles, so inline here
  MCUCR |= (3<<5);  // set boths BODS and BODSE at the same time
  MCUCR = (MCUCR & ~(1<<5)) | (1<<6); // set BODS and clear BODSE
  __asm__ __volatile__("sleep");

  // back from sleep
  disableWatchdogInterrupt();
  enableADC();

}

void loop() {
  blink(1);

  doDeepSleep();

  blink(2);   // back from sleep

}
