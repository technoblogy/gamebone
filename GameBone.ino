/* GameBone Handheld Electronic Game v2

   David Johnson-Davies - www.technoblogy.com - 7th December 2020
   ATtiny85 @ 1 MHz (internal oscillator; BOD disabled)
   
   CC BY 4.0
   Licensed under a Creative Commons Attribution 4.0 International license: 
   http://creativecommons.org/licenses/by/4.0/
*/

#include <avr/sleep.h>
#include <avr/power.h>

const int beat = 250;
const int maximum = 32;

// Buttons:    Blue   Orange Red    Green
int pins[] = { 2,     0,     3,     4};

// Notes:       E4  C#4 A4  E3
int notes[] = { 52, 49, 57, 40 };

int sequence[maximum];

// Simple Tones **********************************************

const uint8_t scale[] PROGMEM = {239,226,213,201,190,179,169,160,151,142,134,127};

void note (int n, int octave) {
  int prescaler = 8 - (octave + n/12);
  if (prescaler<1 || prescaler>7) prescaler = 0;
  DDRB = (DDRB & ~(1<<DDB1)) | (prescaler != 0)<<DDB1;
  OCR1C = pgm_read_byte(&scale[n % 12]) - 1;
  TCCR1 = 1<<CTC1 | 1<<COM1A0 | prescaler;
}

// Button routines **********************************************

// Pin change interrupt wakes us up
ISR (PCINT0_vect) {
}

// Turn on specified button
void button_on (int button) {
  int p = pins[button];
  pinMode(p, OUTPUT);
  digitalWrite(p, LOW);
}

// Turn off specified button
void button_off (int button) {
  int p = pins[button];
  pinMode(p, INPUT_PULLUP);
}
  
// Flash an LED and play the corresponding note of the correct frequency
void flashbeep (int button) {
  button_on(button);
  note(notes[button], 0);
  delay(beat);
  note(0, 0);
  button_off(button);
}

// Play a note and flash the wrong button
void misbeep (int button) {
  int wrong = (button + random(3) + 1) % 4;
  button_on(wrong);
  note(notes[button], 0);
  delay(beat);
  note(0, 0);
  button_off(wrong);
}

// Wait until a button is pressed and play it
int check () {
  GIMSK = GIMSK | 1<<PCIE;            // Enable pin change interrupt
  sleep();
  GIMSK = GIMSK & ~(1<<PCIE);         // Disable pin change interrupt
  int button = 0;
  do {
    button = (button+1) % 4;
  } while (digitalRead(pins[button]));
  flashbeep(button);
  return button;
}

void success_sound () {
  note(48, 0); delay(125);
  note(0, 0);  delay(125);
  note(52, 0); delay(125);
  note(0, 0);  delay(125);
  note(55, 0); delay(125);
  note(0, 0);  delay(125);
  note(60, 0); delay(375);
  note(0, 0);
}

void fail_sound () {
  note(51, 0); delay(125);
  note(0, 0);  delay(125);
  note(48, 0); delay(375);
  note(0, 0);
}

// Simon **********************************************

void simon () {
  int turn = 0;
  sequence[0] = random(4);
  do {
    for (int n=0; n<=turn; n++) {
      delay(beat); 
      flashbeep(sequence[n]);
    }
    for (int n=0; n<=turn; n++) {
      if (check() != sequence[n]) { fail_sound(); return; }
    }
    sequence[turn+1] = (sequence[turn] + random(3) + 1) % 4;
    turn++;
    delay(beat);
  } while (turn < maximum);
  success_sound();
}

// Echo **********************************************

void echo () {
  int turn = 0;
  sequence[turn] = check();
  do {
    for (int n=0; n<=turn; n++) {
      if (check() != sequence[n]) { fail_sound(); return; }
    }
    sequence[turn+1] = check();
    turn++;
    delay(beat);
  } while (turn < maximum);
  success_sound();
}

// Quiz **********************************************

void quiz () {
  do {
    int button = check();
    button_on(button);
    delay(3000);
    button_off(button);
  } while (1);
}

// Confusion **********************************************

void confusion () {
  int turn = 0;
  sequence[0] = random(4);
  do {
    for (int n=0; n<=turn; n++) {
      delay(beat);
      if (turn > 1 && n < turn) misbeep(sequence[n]);
      else flashbeep(sequence[n]);
    }
    for (int n=0; n<=turn; n++) {
      if (check() != sequence[n]) { fail_sound(); return; }
    }
    sequence[turn+1] = (sequence[turn] + random(3) + 1) % 4;
    turn++;
    delay(beat);
  } while (turn < maximum);
  success_sound();
}

// Setup and loop **********************************************

void sleep(void)
{
  sleep_enable();
  sleep_cpu();
}

void setup() {
  // Set up pin change interrupts for buttons
  PCMSK = 1<<PINB0 | 1<<PINB2 | 1<<PINB3 | 1<<PINB4;
  ADCSRA &= ~(1<<ADEN); // Disable ADC to save power
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  // After reset flash all four lights
  for (int button=0; button<4; button++) {
    button_on(button);
    delay(100);
    button_off(button);
  }
  // Wait for button to select game
  int game = 0;
  do {
    game = (game+1) % 4;
    if (millis() > 10000) sleep();
  } while (digitalRead(pins[game]));
  randomSeed(millis());
  delay(250);
  if (game == 3) simon();
  else if (game == 2) echo();
  else if (game == 1) quiz();
  else confusion();
}

// Only reset should wake us now
void loop() {
 sleep();
}