#include <avr/io.h>		// for the input/output registers
#include <util/delay.h>

// For the serial port

#define CPU_FREQ        16000000L       // Assume a CPU frequency of 16Mhz

void init_serial(int speed){
	/* Set baud rate */
	UBRR0 = CPU_FREQ/(((unsigned long int)speed)<<4)-1;
	/* Enable transmitter & receiver */
	UCSR0B = (1<<TXEN0 | 1<<RXEN0);
	/* Set 8 bits character and 1 stop bit */
	UCSR0C = (1<<UCSZ01 | 1<<UCSZ00);
	/* Set off UART baud doubler */
	UCSR0A &= ~(1 << U2X0); 			
}


void send_serial(unsigned char c){
	loop_until_bit_is_set(UCSR0A, UDRE0);
	UDR0 = c;
}


unsigned char get_serial(void) {
	loop_until_bit_is_set(UCSR0A, RXC0);
	return UDR0;
}


void output_set(unsigned char value){
	if (value==0) PORTB &= 0xfe; else PORTB |= 0x01;
}


// For I/O handling (examples for PIN 8 as output and PIN 2 as input)
void output_init(void){
	DDRB |= 0x20; // PIN 13 as output
}


void input_init(void){
	DDRD &= 0x83;  // PIN 2 to 6 as input :  10000011 
	PORTD |= 0x7C; // Pull-up activated on PIN 2 to 6 :  01111100
}

int input_get(void){
	int value;
	value = (((PIND&0x04)!=0)?0:1)+(((PIND&0x08)!=0)?0:2)+(((PIND&0x10)!=0)?0:4)+(((PIND&0x20)!=0)?0:8)+(((PIND&0x40)!=0)?0:16);
	return value;
}


int main(void){
	init_serial(9600);
	output_init();
	input_init();
	int value;
	unsigned msg;
	while(1){
		value = input_get();
		msg = 0x20 + value;
		if (value!=0) send_serial(msg);
		
	
		msg = get_serial();
		if (msg == 49){
			send_serial(65);
			PORTB = 0x20;
		}
	       	else if (msg == 48){
			send_serial(66);
			PORTB = 0;
		}
	}


//on scrute via get serial
//si on a recu un truc on allume led 13
	
	
	return 0;
}
