/*
 * main.c
 *
 * Created: 11/27/2024 6:55:02 AM
 *  Author: Don Bosco Francis
 */ 
// Define CPU Clock
#define F_CPU 8000000UL  // 8 MHz internal clock
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>

// Define pins
#define DHT11_PIN PD0
#define FAN1_PIN PD1
#define FAN2_PIN PD2

// Function prototypes
uint8_t DHT11_ReadData(uint8_t *temperature, uint8_t *humidity);
void Fan_Control(uint8_t temperature);

int main(void) {
	// Initialize I/O
	DDRD &= ~(1 << DHT11_PIN); // PD0 as input
	DDRD |= (1 << FAN1_PIN) | (1 << FAN2_PIN); // PD1 and PD2 as output

	uint8_t temperature = 0, humidity = 0;
	uint8_t status;

	while(1) {
		status = DHT11_ReadData(&temperature, &humidity);
		if (status == 0) {
			Fan_Control(temperature);
			// Optional: Display temperature and humidity
		}
		_delay_ms(2000); // DHT11 requires at least 1-2 seconds between readings
	}
}

// Function to read data from DHT11
uint8_t DHT11_ReadData(uint8_t *temperature, uint8_t *humidity) {
	uint8_t bits[5] = {0, 0, 0, 0, 0};
	uint8_t i, j = 0;

	// Send start signal
	DDRD |= (1 << DHT11_PIN); // Set as output
	PORTD &= ~(1 << DHT11_PIN); // Pull low
	_delay_ms(20); // Minimum 18ms
	PORTD |= (1 << DHT11_PIN); // Pull high
	_delay_us(40);
	DDRD &= ~(1 << DHT11_PIN); // Set as input

	// Wait for DHT11 response
	// Check for the first low pulse
	for (i = 0; i < 80; i++) {
		if (!(PIND & (1 << DHT11_PIN))) break;
		_delay_us(1);
	}
	if (i == 80) return 1; // No response

	// Check for the second high pulse
	for (i = 0; i < 80; i++) {
		if (PIND & (1 << DHT11_PIN)) break;
		_delay_us(1);
	}
	if (i == 80) return 1; // No response

	// Read the 40 bits
	for (j = 0; j < 40; j++) {
		// Wait for the low pulse
		for (i = 0; i < 100; i++) {
			if (!(PIND & (1 << DHT11_PIN))) break;
			_delay_us(1);
		}
		if (i == 100) return 1; // Timeout

		// Wait for the high pulse and determine bit value
		uint8_t bit_length = 0;
		for (i = 0; i < 100; i++) {
			if (PIND & (1 << DHT11_PIN)) break;
			_delay_us(1);
			bit_length++;
		}
		if (i == 100) return 1; // Timeout

		if (bit_length > 30) { // Logical '1'
			bits[j / 8] |= (1 << (7 - (j % 8)));
		}
		// Else logical '0', do nothing
	}

	// Verify checksum
	uint8_t checksum = bits[0] + bits[1] + bits[2] + bits[3];
	if (bits[4] != checksum) return 1; // Checksum failed

	*humidity = bits[0];
	*temperature = bits[2];

	return 0; // Success
}

// Function to control fans based on temperature
void Fan_Control(uint8_t temperature) {
	if (temperature >= 30) { // Example threshold
		PORTD |= (1 << FAN1_PIN) | (1 << FAN2_PIN); // Turn ON both fans
	}
	else if (temperature >= 25) {
		PORTD |= (1 << FAN1_PIN);
		PORTD &= ~(1 << FAN2_PIN); // Turn ON Fan1 only
	}
	else {
		PORTD &= ~(1 << FAN1_PIN) & ~(1 << FAN2_PIN); // Turn OFF both fans
	}
}
