/****************************************************************************
melody.c

This is a project for musical tone generation using at89c51 microcontroller.
The basic idea behind melody is to produce the correct musical note. Display 
TS-5261ASR are connectedto port 1 of the MCU.

****************************************************************************/

#include <mcs51\at89x51.h>

// константы-делители частоты для нот
#define f_C5	64580
#define f_Db5	64634
#define f_D5	64685
#define f_Eb5	64732
#define f_E5	64778
#define f_F5	64820
#define f_Gb5	64860
#define f_G5	64898
#define f_Ab5	64934
#define f_A5	64968
#define f_Bb5	65000
#define f_B5	65030
#define f_C6	65058
#define f_Db6	65085
#define f_D6	65110
#define f_Eb6	65134
#define f_E6	65157
#define f_F6	65178
#define f_Gb6	65198
#define f_G6	65217
#define f_Ab6	65235
#define f_A6	65252
#define f_Bb6	65268
#define f_B6	65283
#define PAUSE   0

#define soundPin  P3_7

// длительности нот/пауз
unsigned int qtr, qtr_p, half, half_p, whole, whole_p, eighth, eighth_p, sixteen, sixteen_p, thirtyTwo, thirtyTwo_p;

struct musnote {
	unsigned int freq;
	unsigned int len;
};

static struct musnote VoPole[] = {
/*  frequency  length
  -----------  -------- */
  { PAUSE, whole }
  { f_G5, eighth },  // 1/8 g  Во поле береза стояла
  { f_G5, eighth },  // 1/8 g
  { f_G5, eighth },  // 1/8 g
  { f_G5, eighth },  // 1/8 g
  { f_F5, qtr },     // 1/4 f
  { f_Eb5, eighth }, // 1/8 e bem
  { f_Eb5, eighth }, // 1/8 e bem
  { f_D5, qtr },     // 1/4 d
  { f_C5, qtr },     // 1/4 c
  { f_G5, eighth },  // 1/8 g  Во поле кудрявая стояла
  { f_G5, eighth },  // 1/8 g
  { f_B5, eighth },  // 1/8 b 
  { f_G5, eighth },  // 1/8 g
  { f_F5, eighth },  // 1/8 f
  { f_F5, eighth },  // 1/8 f
  { f_Eb5, eighth }, // 1/8 e bem
  { f_Eb5, eighth }, // 1/8 e bem
  { f_D5, qtr },     // 1/4 d
  { f_C5, qtr },     // 1/4 c   
  { f_D5, qtr_p },   // 1/4 d  Лю-ли, лю-ли стояла
  { f_Eb5, eighth }, // 1/8 e bem
  { f_F5, qtr },     // 1/4 f
  { f_Eb5, eighth }, // 1/8 e bem
  { f_Eb5, eighth }, // 1/8 e bem
  { f_D5, qtr },     // 1/4 d
  { f_C5, qtr },     // 1/4 c
  { f_D5, qtr_p },   // 1/4 d   Лю-ли, лю-ли стояла
  { f_Eb5, eighth }, // 1/8 e bem
  { f_F5, qtr },     // 1/4 f
  { f_Eb5, eighth }, // 1/8 e bem
  { f_Eb5, eighth }, // 1/8 e bem
  { f_D5, qtr },     // 1/4 d
  { f_C5, qtr },     // 1/4 c
  { PAUSE, 0 }
};

// коды  символов 7-сегментного дисплея			 
unsigned char segmarr[26] = { 0x03, 0x9F, 0x25, 0x0D, 0x99, 0x49, 0x41, 0x1F, 0x01, 0x09, 0xFF, 0xFE,  // 0, 1, 2, ..., 9, <пусто>, <точка> 
0x11, 0xC1, 0x63, 0x85, 0x61, 0x71, 0x43, 0x91, 0xC3, 0x83, 0x73, 0x13, 0x31, 0x89 };  // A, b, C, d, E, F, G, H, L, U, Г, П, Р, У

unsigned char x=0, y=0, z=0, dec0=10, dec1=10, j, k, flag;

//------------------------------------------------------------------------------------
// динамическая индикация по прерыванию int 1
void Timer0_ISR(void) __interrupt (1) {
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

//-----------------------------------------------------------------------------------------------------
// генерация прямоугольных импульсов заданной звуковой частоты (делитель hbyte,lbyte) по прерыванию int3
void Timer1_ISR(void) __interrupt (3) {
	TF1 = 0;
	soundPin = soundPin ^ 1;
	TH1 = hbyte;
	TL1 = lbyte;
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

//---------------------------------------------
void Tempo(int bpm){
	qtr = 60000 / bpm;
	half = qtr * 2;
	whole = qtr * 4;
	eighth = qtr / 2;
	sixteen = qtr / 4;
	thirtyTwo = qtr / 8;
	sixtyFour = qtr / 16;
	whole_p = whole + half;
	half_p = half + qtr;
	qtr_p = qtr + eighth;
	eighth_p = eighth + sixteen;
	sixteen_p = sixteen + thirtyTwo;
	thirtyTwo_p = thirtyTwo + sixtyFour;
}

//----------------------------------------------
void PlayNote(unsigned int p, unsigned int t) {
	if(p != 0) {
	  hbyte = (p >> 8) & 0xFF; // ст.байт
	  lbyte = p & 0xFF;		   // мл.байт
	  TH1 = hbyte;
	  TL1 = lbyte;
      ET1 = 1;  // разрешить прывание от таймера 1
	  EA = 1;   // снять общую блокировку прерываний
	  TR1 = 1;  // пуск таймера C/T1
	}
	if(t != 0) {
	  delay(t);
	  TR1 = 0;
	  EA = 0;
	  soundPin = 1;  
	}
}

void PlayMelody(static struct musnote &melody, int bpm) {
	Tempo(bpm);
	while(melody.len != 0) {
		PlayNote(*melody.freq, *melody.len);
		melody++;
	}

//---------------------------------------------------------------------------------------

void main(void) {
	soundPin = 1;
	TMOD = 0x11;  // Задание режимов работы C/T0, C/T1 в качестве 16-разрядных таймеров
    EA = 0;		// запрет всех прерываний
    ET0 = 0;	// запрет прерываний по таймеру 0
	ET1 = 0;	// запрет прерываний по таймеру 1
    P3_0 = 0;		
    P3_1 = 1;	// тестируем младший разряд дисплея
    x = 0x80;	// a = 0
    for(k=0; k < 8 ; k++) {
	  P1 = ~x;
	  delay(100);
	  x = x >> 1;
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
	
	// 
	k = 0;
	while(1){
	  k++;
	  if(k > 99)
		k = 0;
	  P1 = segmarr[k]; // 7-сегм.код младшего разряда числа --> P1
	  PlayMelody(&VoPole, 120);
	}
}