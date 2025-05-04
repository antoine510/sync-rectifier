#include <avr/wdt.h>

enum Pins {
  PIN_GAN = A2, // PC2
  PIN_GAP = A3, // PC3
  PIN_GBN = A4, // PC4
  PIN_GBP = A5, // PC5
  PIN_GCN = A0, // PC0
  PIN_GCP = A1, // PC1

  PIN_S1 = 20,  // PB6
  PIN_S2 = 21,  // PB7

  PIN_COMP_LOW = 5,   // PD5, PCINT21
  PIN_COMP_HIGH = 6,  // PD6, PCINT22

  PIN_LED = 13  // PB5
};

enum Phase : uint8_t { A = 0, C = 1, B = 2 };
enum Event : uint8_t { Alow, Chigh, Blow, Ahigh, Clow, Bhigh };
constexpr uint8_t eventToPC[] = {0b100100, 0b000110, 0b010010, 0b011000, 0b001001, 0b100001};
volatile Event nextEvent = Alow;
volatile uint32_t event_us = 0;

extern "C" void __attribute__((naked, used, section (".init3"))) init3() {
  DDRB = _BV(DDB5) | _BV(DDB6) | _BV(DDB7);
  DDRC = _BV(DDC0) | _BV(DDC1) | _BV(DDC2) | _BV(DDC3) | _BV(DDC4) | _BV(DDC5);
}

ISR(PCINT2_vect) {
  PORTC = eventToPC[nextEvent]; // Update mosfet control
  event_us = micros();
  nextEvent = (nextEvent + 1) % 6;
  PORTB = PORTB & 0x3f | (nextEvent % 3) << 6;  // Select next phase
  PCMSK2 = (nextEvent % 2) ? _BV(PCINT22) : _BV(PCINT21); // Enable next comparator
  PORTB ^= _BV(PORTB5); // Toggle LED
  PCIFR = 0b0100; // Clear spurious interrupt on dirty high transitions
}

void setup() {
  ADCSRA &= ~_BV(ADEN);
  PCICR = _BV(PCIE2);  // Enable pin change interrup for PD0-7
  PCMSK2 = _BV(PCINT21);  // Enable low comparator
}

void loop() {
  static uint32_t lastEvent_us = 0, lastPeriod_us = 0;
  cli();
  uint32_t currentEvent_us = event_us;
  sei();
  if(uint32_t period_us = currentEvent_us - lastEvent_us; period_us) {
    lastPeriod_us = period_us;
    lastEvent_us = currentEvent_us;
  }
  if((micros() - lastEvent_us) * 10 > lastPeriod_us * 11) { // More than 20% longer than previous period
    PORTC = 0;  // Safe MOSFETs
  }
}
