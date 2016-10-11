/*
===============================================================================
 Name        : FinalV2.0.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include <cr_section_macros.h>

#include <stdio.h>

//Registros GPIO0
unsigned int volatile * const FIO0DIR =(unsigned int *) 0x2009C000;
unsigned int volatile * const FIO0PIN =(unsigned int *) 0x2009C014;
unsigned int volatile * const FIO0SET =(unsigned int *) 0x2009C018;
unsigned int volatile * const FIO0CLR =(unsigned int *) 0x2009C01C;
//Registros GPIO1
unsigned int volatile * const FIO1DIR =(unsigned int *) 0x2009C020;
unsigned int volatile * const FIO1PIN =(unsigned int *) 0x2009C034;
unsigned int volatile * const FIO1SET =(unsigned int *) 0x2009C038;
unsigned int volatile * const FIO1CLR =(unsigned int *) 0x2009C03C;
// Registros GPIO2
unsigned int volatile * const FIO2DIR =(unsigned int *) 0x2009C040;
unsigned int volatile * const FIO2PIN =(unsigned int *) 0x2009C054;
unsigned int volatile * const FIO2SET =(unsigned int *) 0x2009C058;
unsigned int volatile * const FIO2CLR =(unsigned int *) 0x2009C05C;
//Registros TIMER0
unsigned int volatile * const T0IR     = (unsigned int *) 0x40004000;
unsigned int volatile * const T0TCR    = (unsigned int *) 0x40004004;
unsigned int volatile * const T0TC    = (unsigned int *) 0x40004008;
unsigned int volatile * const T0PR     = (unsigned int *) 0x4000400c;
unsigned int volatile * const T0PC    = (unsigned int *) 0x40004010;
unsigned int volatile * const T0MCR = (unsigned int *) 0x40004014;
unsigned int volatile * const T0MR0 = (unsigned int *) 0x40004018;
unsigned int volatile * const T0MR1    = (unsigned int *) 0x4000401C;
//Registros NVIC
unsigned int volatile * const ISER0 = (unsigned int *) 0xE000E100;
//Registros interrupción por GPIO2
unsigned int volatile * const IO2IntEnF = (unsigned int *) 0x400280B4; // Registro de habilitación de interrupción por flanco de bajada en GPIO2
unsigned int volatile * const IO2IntStatF = (unsigned int *) 0x400280A8; // Flags de interrupcion por flanco de bajada en GPIO2
unsigned int volatile * const IO2IntClr = (unsigned int *) 0x400280AC; // Registro para limpiar flags de interrupcion en GPIO2

uint32_t output[] = {
		0b00000000000001111000000000000011,
		0b00000000000001000000000000000010,
		0b00000000100000101000000000000011,
		0b00000000100001100000000000000011,
		0b00000000100001010000000000000010,
		0b00000000100001110000000000000001,
		0b00000000100001111000000000000001,
		0b00000000000001000000000000000011,
		0b00000000100001111000000000000011,
		0b00000000100001110000000000000011,
		0b00000000000000000000000000000000}; // la posicion 10 en el array es el display apagado
// "output" contiene los binarios de 0 a 9 y display apagado

int fraccion, periodo, mpx, retardo, teclaPres, flagReset, flagStart = 0;
int unidadAct = 10;
int decenaAct = 10;
int vueltaAct = 10;

int unidadSaved, decenaSaved, vueltaSaved;

int retardo_max = 2000000;

void displayInit(void); // habilitar display como salida
void keyboardInit(void); // settings para teclado matricial
void setVDU(void); // seteo de valores de vueltaAct, decenaAct y unidadAct
void tmr0Init(void); // inicializacion de timer0 para refresco de displays
void iniciarGiro(void); // inicia giro del motor (por ahora es un simulador que realiza conteo visible en displays)

int main(void) {
	displayInit(); // habilitar salidas hacia segmentos de displays y mpx
	keyboardInit(); // setear teclado matricial en GPIO2
	tmr0Init(); // inicializar tmr0 para barrido de displays

	while(1) {
		if(flagStart) {
			iniciarGiro();
		}
	}

    return 0 ;
}

void displayInit(void) {
	*FIO0DIR |= (1 << 0); // habilitar port0.0-pin9 como salida - segmento a
	*FIO0DIR |= (1 << 1); // habilitar port0.1-pin10 como salida - segmento b
	*FIO0DIR |= (1 << 15); // habilitar port0.15-pin11 como salida - segmento c
	*FIO0DIR |= (1 << 16); // habilitar port0.16-pin12 como salida - segmento d
	*FIO0DIR |= (1 << 17); // habilitar port0.17-pin13 como salida - segmento e
	*FIO0DIR |= (1 << 18); // habilitar port0.18-pin14 como salida - segmento f
	*FIO0DIR |= (1 << 23); // habilitar port0.23-pin15 como salida - segmento g
	*FIO0DIR |= (1 << 24); // habilitar port0.24-pin16 como salida - segmento dp

	*FIO0DIR |= (1 << 25); // habilitar port0.25-pin16 como salida - mpx0
	*FIO0DIR |= (1 << 26); // habilitar port0.26-pin17 como salida - mpx1
	*FIO1DIR |= (1 << 30); // habilitar port1.30-pin18 como salida - mpx2
	*FIO1DIR |= (1 << 31); // habilitar port1.31-pin19 como salida - mpx3
}

void keyboardInit(void) {
	*FIO2DIR |= (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3);
	/* se habilitan salidas en:
	 * GPIO2.0 (pin 42)
	 * GPIO2.1 (pin 43)
	 * GPIO2.2 (pin 44)
	 * GPIO2.3 (pin 45)
	 * se dejan por defecto entradas con pull-up en:
	 * GPIO2.4 (pin 46)
	 * GPIO2.5 (pin 47)
	 * GPIO2.6 (pin 48)
	 * GPIO2.7 (pin 49)
	 */
	*FIO2CLR |= (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3); // se pone '0' en todas las salidas
	*ISER0 |= (1 << 21); // se habilitan las interrupciones por GPIO2
	*IO2IntEnF |= (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7);
	/* se habilitan interrupciones por flanco de bajada en:
	 * GPIO2.4 (pin 46)
	 * GPIO2.5 (pin 47)
	 * GPIO2.6 (pin 48)
	 * GPIO2.7 (pin 49)
	 */
}

void tmr0Init(void) {
	*FIO1CLR |= (1 << 30) | (1 << 31);
    *T0TCR = 0;
    *T0MR0=25000;
    *T0MCR |= (1<<1)|(1<<0); //interrumpo y reseteo en Match0
    *ISER0 = (1<<1);
    *T0TCR |= 1;
    return;
}

void TIMER0_IRQHandler(void) {
    if(*T0IR & (1 << 0)) {
    	switch(mpx) {
    		case 0:
    			*FIO1CLR |= (1 << 31);
    			*FIO0PIN = output[periodo];
    			*FIO0SET |= (1 << 25);
    			mpx++;
    			break;
    		case 1:
    			*FIO0CLR |= (1 << 25);
    			*FIO0PIN = output[unidadAct];
    			*FIO0SET |= (1 << 26);
    			mpx++;
    			break;
    		case 2:
    			*FIO0CLR |= (1 << 26);
    			*FIO0PIN = output[decenaAct];
    			*FIO1SET |= (1 << 30);
    			mpx++;
    			break;
    		default:
    			*FIO1CLR |= (1 << 30);
    			*FIO0PIN = output[vueltaAct] | (1 << 24);
    			*FIO1SET |= (1 << 31);
    			mpx = 0;
    			break;
    	}
    	*T0IR |= (1 << 0); // Bajamos bandera
    }
    return;
}

void EINT3_IRQHandler(void) {
	for(int a = 0; a < 250000; a++) {} // retardo de 10 ms (antirrebote)
	if(*IO2IntStatF & (1 << 7)) { // interrupcion en fila 1 (numeros 1, 2 y 3)

		*FIO2SET |= (1 << 3) | (1 << 2) | (1 << 1); // se pónen en alto todas las salidas

		*FIO2CLR |= (1 << 3); // se pone en bajo salida a columna 1 (numero 1)
		if(!(*FIO2PIN & (1 << 7))) {
			teclaPres = 1;
			setVDU();
		}
		*FIO2SET |= (1 << 3); // se pónen en alto la salida a columna 1

		*FIO2CLR |= (1 << 2); // se pone en bajo salida a columna 2 (numero 2)
		if(!(*FIO2PIN & (1 << 7))) {
			teclaPres = 2;
			setVDU();
		}
		*FIO2SET |= (1 << 2); // se pónen en alto la salida a columna 2

		*FIO2CLR |= (1 << 1); // se pone en bajo salida a columna 3 (numero 3)
		if(!(*FIO2PIN & (1 << 7))) {
			teclaPres = 3;
			setVDU();
		}

		*FIO2CLR |= (1 << 3) | (1 << 2) | (1 << 1); // se ponen en bajo todas las salidas
	}
	else if(*IO2IntStatF & (1 << 6)) { // interrupcion en fila 2 (numeros 4,5 y 6)

		*FIO2SET |= (1 << 3) | (1 << 2) | (1 << 1); // se pónen en alto todas las salidas

		*FIO2CLR |= (1 << 3); // se pone en bajo salida a columna 1 (numero 4)
		if(!(*FIO2PIN & (1 << 6))) {
			teclaPres = 4;
			setVDU();
		}
		*FIO2SET |= (1 << 3); // se pónen en alto la salida a columna 1

		*FIO2CLR |= (1 << 2); // se pone en bajo salida a columna 2 (numero 5)
		if(!(*FIO2PIN & (1 << 6))) {
			teclaPres = 5;
			setVDU();
		}
		*FIO2SET |= (1 << 2); // se pónen en alto la salida a columna 2

		*FIO2CLR |= (1 << 1); // se pone en bajo salida a columna 3 (numero 6)
		if(!(*FIO2PIN & (1 << 6))) {
			teclaPres = 6;
			setVDU();
		}

		*FIO2CLR |= (1 << 3) | (1 << 2) | (1 << 1); // se ponen en bajo todas las salidas
	}
	else if(*IO2IntStatF & (1 << 5)) { // interrupcion en fila 3 (numeros 7, 8 y 9)

		*FIO2SET |= (1 << 3) | (1 << 2) | (1 << 1); // se pónen en alto todas las salidas

		*FIO2CLR |= (1 << 3); // se pone en bajo salida a columna 1 (numero 7)
		if(!(*FIO2PIN & (1 << 5))) {
			teclaPres = 7;
			setVDU();
		}
		*FIO2SET |= (1 << 3); // se pónen en alto la salida a columna 1

		*FIO2CLR |= (1 << 2); // se pone en bajo salida a columna 2 (numero 8)
		if(!(*FIO2PIN & (1 << 5))) {
			teclaPres = 8;
			setVDU();
		}
		*FIO2SET |= (1 << 2); // se pónen en alto la salida a columna 2

		*FIO2CLR |= (1 << 1); // se pone en bajo salida a columna 3 (numero 9)
		if(!(*FIO2PIN & (1 << 5))) {
			teclaPres = 9;
			setVDU();
		}

		*FIO2CLR |= (1 << 3) | (1 << 2) | (1 << 1); // se ponen en bajo todas las salidas
	}
	else if(*IO2IntStatF & (1 << 4)) { // interrupcion en fila 4 (RESET (*), numero 0 y START/STOP (#))

		*FIO2SET |= (1 << 3) | (1 << 2) | (1 << 1); // se pónen en alto todas las salidas

		*FIO2CLR |= (1 << 3); // se pone en bajo salida a columna 1 (*)
		if(!(*FIO2PIN & (1 << 4))) { // se presiono * que equivale a Reset
			if (!flagStart) {
				/*
				 * se chequea que no esté en alto la bandera de conteo:
				 * si la bandera de conteo está en alto, no se admite RESET
				 */
				flagReset = 0;
				decenaAct = 10;
				unidadAct = 10;
				vueltaAct = 10;
			}
		}
		*FIO2SET |= (1 << 3); // se ponen en alto la salida a columna 1

		*FIO2CLR |= (1 << 2); // se pone en bajo salida a columna 2 (numero 0)
		if(!(*FIO2PIN & (1 << 4))) {
			teclaPres = 0;
			setVDU();
		}
		*FIO2SET |= (1 << 2); // se pónen en alto la salida a columna 2

		*FIO2CLR |= (1 << 1); // se pone en bajo salida a columna 3 (#)
		if(!(*FIO2PIN & (1 << 4))) {
			flagStart = 1;
		}

		*FIO2CLR |= (1 << 3) | (1 << 2) | (1 << 1); // se ponen en bajo todas las salidas
	}

	*IO2IntClr |= (1 << 7) | (1 << 6) | (1 << 5) | (1 << 4); // limpia banderas de interrupcion

	return;
}

void setVDU(void) {
	switch(flagReset) {
	case 0:
		vueltaAct = teclaPres;
		if(vueltaAct == 9) {
			decenaAct = 0;
			unidadAct = 0;
			flagReset = 3;
		} else {
			flagReset++;
		}
		break;

	case 1:
		if(teclaPres > 4) {
			break;
		} else {
			decenaAct = teclaPres;
			flagReset++;
			break;
		}
	case 2:
		if(decenaAct == 4 && teclaPres > 7) {
			break;
		} else {
			unidadAct = teclaPres;
			flagReset++;
			break;
		}
	}
	return;
}

void iniciarGiro(void) {

	flagReset = 3; // flagReset se pone en 3 para evitar el ingreso de nuevos numeros durante el conteo

	for(vueltaAct = 8; vueltaAct <= 9; vueltaAct++) {
		if(vueltaAct != 9) {
			for(fraccion = 0; fraccion < 48; fraccion++) {
				unidadAct = fraccion % 10;
				decenaAct = fraccion / 10;
				for(retardo = 0; retardo < retardo_max; retardo++) {}
			}
		} else {
			unidadAct = 0;
			decenaAct = 0;
			for(retardo = 0; retardo < retardo_max; retardo++) {}
			break;
		}
	}

	flagStart = 0; // flagStart se vuelve a cero para admitir nuevamente el ingreso de números mediante teclado

	return;
}
