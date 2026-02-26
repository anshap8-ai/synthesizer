/***************************************************************************
keyboard.c

This is a 7 segment display interface project. Display TS-5261ASR are connected
to port 1 of the MCU at89c51 and show the sequential number of key pressed.
***************************************************************************/

#include <mcs51\at89x51.h>
 
int segmarr[26] = { 0x03, 0x9F, 0x25, 0x0D, 0x99, 0x49, 0x41, 0x1F, 0x01, 0x09, 0xFF, 0xFE,  // 0, 1, 2, ..., 9, <пусто>, <точка> 
0x11, 0xC1, 0x63, 0x85, 0x61, 0x71, 0x43, 0x91, 0xC3, 0x83, 0x73, 0x13, 0x31, 0x89 };  // A, b, C, d, E, F, G, H, L, U, Г, П, Р, У
					
unsigned char x=0, y=0, z=0, dec0=10, dec1=10, j, k, flag;

//----------------------------------------------------------------------------------
void Timer0_ISR(void) __interrupt (1)
{
  if(flag == 0) {
    P1 = segmarr[dec0]; // 7-сегм.код младшего разряда числа --> P1
	P3_0 = 0;			// dig1 = 0
    P3_1 = 1;			// dig2 = 1  - зажигаем младший разряд дисплея
	flag = 1;
  }
  else { // flag = 1
    P1 = segmarr[dec1];
    P3_0 = 1;			// dig1 = 1  - зажигаем старший разряд дисплея
    P3_1 = 0;			// dig2 = 0
    flag = 0;
  }
  TL0 = 0xEF;	// TH0,TL0 = D8EFh  timer 0 in mode 1 (16 bit)
  TH0 = 0xD8;	//
  return;
}

//----------------------------------------------------------------------------------
void time1ms()  // 1 ms delay with XTAL 12.000 MHz
{
  int i;
  for(i=0; i < 150; i++) // the value 150 was calibrated for 1ms 
    ;					
}

//---------------------------------------------------------------------------------
void delay(int n) // do nothing n*1ms
{
  int i;
  for(i=0; i < n ; i++)
    time1ms();
}

//-------------------------- Main program ---------------------------------
void main(void)
{
  EA = 0;	// Блокировка всех прерываний
  ET0 = 0;  // запрет прерывания по таймеру 0
  P3_0 = 0;		
  P3_1 = 1;		// тестируем младший разряд дисплея
  x = 0x80;		// сегмент_a = 0
  for(k=0; k < 8 ; k++) {
	P1 = ~x;
	delay(100);
	x = x >> 1; // след.сегмент
  }
  P1 = 0xFF;	// <пусто> --> дисплей
  delay(100);
  P3_0 = 1;		// тестируем старший разряд дисплея
  P3_1 = 0;
  x = 0x80;		// сегмент_a = 0
  for(k=0; k < 8 ; k++) {
	P1 = ~x;	// инверсия байта --> P1 (дисплей)
	delay(100);
	x = x >> 1;
  }
  P1 = 0xFF;	// очистить дисплей
  P2 = 0xFF;	// порт P2 переключаем на ввод
  P3 = 0x7C;
  
  TMOD = 0x01;  // Задание режима работы C/T0
  TL0 = 0xEF;   // Загрузка младшего байта C/T0
  TH0 = 0xD8;   // Загрузка старшего байта C/T0
  EA = 1;       // Снятие общей блокировки прерываний
  ET0 = 1;      // Разрешение прерывания от TF0
  TR0 = 1;      // Пуск таймера C/T0
  
  while(1) {
	x = 0x01;
    for(k=0; k < 8 ; k++) {  // сканирование муз.клавиш "бегущим 0"
	  P0 = ~x;	
	  delay(50);
	  y = P2;
	  if(y != 0xFF) {	// есть нажатие
		  delay(50);	// защита от дребезга контактов
		  y = P2;		// повторное чтение
		  if(y != 0xFF) {	// нажатие подтверждено
		    y = ~y;
			y = y & 0x7F;
			switch(y) {
			  case 1:  z = 7;
					break;
			  case 2:  z = 15;
					break;
			  case 4:  z = 23;
					break;
			  case 8:  z = 31;
					break;
			  case 16: z = 39;
					break;
			  case 32: z = 47;
					break;
			  case 64: z = 55;
			}
			z = z - k; // порядковый номер клавиши, начиная с 0
			dec0 = z % 10;
			dec1 = z / 10;
			delay(50);
			while(1) {
			  y = P2;
			  if(y == 0xFF) {	// есть отжатие
			    delay(50);
			    y = P2;
			    if(y == 0xFF) {	// отжатие подтверждено
			      dec0 = 10;
			      dec1 = 10;
				  break;
			    }
			  }
			}
		  }
	  }
	  else {
		y = P3;
		y = y | 0x83;
	    if(y != 0xFF) {	// есть нажатие кнопок
		  delay(50);
		  y = P3;		// повторное чтение
		  y = y | 0x83;
		  if(y != 0xFF) {	// нажатие кнопки подтверждено
		    y = ~y;
			y = y & 0x7C;
			y = y >> 2;
			switch(y) {
			  case 1:  z = 0;
					break;
			  case 2:  z = 8;
					break;
			  case 4:  z = 16;
					break;
			  case 8:  z = 24;
					break;
			  case 16: z = 32;
					break;
			  case 32: z = 40;
					break;
			  case 64: z = 48;
			}
			z = z + k + 60; // порядковый номер клавиши, начиная с 60
			dec0 = z % 10;
			dec1 = z / 10;
			delay(50);
			while(1) {
			  y = P3;
			  y = y | 0x83;
			  if(y == 0xFF) {	// есть отжатие кнопки
			    delay(50);
			    y = P3;
				y = y | 0x83;
			    if(y == 0xFF) {	// отжатие кнопки подтверждено
			      dec0 = 10;
			      dec1 = 10;
				  break;
			    }
			  }
			}
		  } // end if
		} // end if 
	  } // end else

	  x = x << 1;	// сдвигаем влево бегущий 0
    } // end for
  } // end while
}