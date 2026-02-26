/*********************************************************************************
midi.c

This is a 7 segment display interface project. Display TS-5261ASR are connected
to port 1 of the MCU at89c51 and show number of pressed key and play music fragment
**********************************************************************************/

#include <mcs51\at89x51.h>

// timer count for note

// большая октава
#define f_C2	57892
#define f_Db2	58321
#define f_D2	58726
#define f_Eb2	59108
#define f_E2	59469
#define f_F2	59809
#define f_Gb2	60131
#define f_G2	60434
#define f_Ab2	60720
#define f_A2	60991
#define f_B2	61246
#define f_H2	61486

// малая октава
#define f_C3	61714
#define f_Db3	61928
#define f_D3	62131
#define f_Eb3	62322
#define f_E3	62502
#define f_F3	62673
#define f_Gb3	62833
#define f_G3	62985
#define f_Ab3	63128
#define f_A3	63263
#define f_B3	63391
#define f_H3	63511

// первая октава
#define f_C4	63625
#define f_Db4	63732
#define f_D4	63833
#define f_Eb4	63929
#define f_E4	64019
#define f_F4	64104
#define f_Gb4	64185
#define f_G4	64260
#define f_Ab4	64332
#define f_A4	64400
#define f_B4	64463
#define f_H4	64524

// вторая октава
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
#define f_B5	65000
#define f_H5	65030

// третья октава
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
#define f_B6	65268
#define f_H6	65283

#define PAUSE   0

// распределение пинов МК
#define dig1      P3_0
#define dig2	  P2_7
#define midiOut   P3_1
#define soundPin  P3_7

const unsigned int notes[60] = {
	57892,	58321,	58726,	59108,	59469,	59809,	60131,	60434,	60720,	60991,	61246,	61486,
	61714,	61928,	62131,	62322,	62502,	62673,	62833,	62985,	63128,	63263,	63391,	63511,
	63625,	63732,	63833,	63929,	64019,	64104,	64185,	64260,	64332,	64400,	64463,	64524,
	64580,	64634,	64685,	64732,	64778,	64820,	64860,	64898,	64934,	64968,	65000,	65030,
	65058,	65085,	65110,	65134,	65157,	65178,	65198,	65217,	65235,	65252,	65268,	65283
};

// длительности нот/пауз
unsigned int qtr, qtr_p, half, half_p, whole, whole_p, eighth, eighth_p, sixteen, sixteen_p, thirtyTwo, thirtyTwo_p, sixtyFour;

int segmarr[26] = { 0x03, 0x9F, 0x25, 0x0D, 0x99, 0x49, 0x41, 0x1F, 0x01, 0x09, 0xFF, 0xFE,  // 0, 1, 2, ..., 9, <пусто>, <точка> 
0x11, 0xC1, 0x63, 0x85, 0x61, 0x71, 0x43, 0x91, 0xC3, 0x83, 0x73, 0x13, 0x31, 0x89 };  // A, b, C, d, E, F, G, H, L, U, Г, П, Р, У

int keytime = 300, temp = 120;					
unsigned char x=0, y=0, z=0, dec0=10, dec1=10, j, k, flag, hbyte, lbyte, midi_note;

//------------------------------------------------------------------------------------
// динамическая индикация по прерыванию int 1
void Timer0_ISR(void) __interrupt (1) {
 if(flag == 0) {
    P1 = segmarr[dec0]; // 7-сегм.код младшего разряда числа --> P1
	dig1 = 0;			// dig1 = 0
    dig2 = 1;			// dig2 = 1  - зажигаем младший разряд дисплея
	flag = 1;
  }
  else { // flag = 1
    P1 = segmarr[dec1];
    dig1 = 1;			// dig1 = 1  - зажигаем старший разряд дисплея
    dig2 = 0;			// dig2 = 0
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

void StartSound(unsigned int p){
	hbyte = (p >> 8) & 0xFF; // ст.байт
	lbyte = p & 0xFF;		   // мл.байт
	TH1 = hbyte;
	TL1 = lbyte;
    ET1 = 1;  // разрешить прывание от таймера 1
	EA = 1;   // снять общую блокировку прерываний
	TR1 = 1;  // пуск таймера C/T1
}

void StopSound() {
    TR1 = 0;
	soundPin = 1;  
}

//----------------------------------------------
void PlayNote(unsigned int p, unsigned int t){
	if(p != 0) {
	  StartSound(p);
	}
	delay(t);
	StopSound();  
}

void sendMIDI(unsigned int status, unsigned int data) {
//  Serial.write(status);
//  Serial.write(data);
}

//-------------------------------------------------------------------
void VoPole(int bpm) { 
	Tempo(bpm);
	PlayNote(PAUSE, whole);
	PlayNote(f_G4, eighth);  // 1/8 g  Во поле береза стояла
	PlayNote(f_G4, eighth);  // 1/8 g
	PlayNote(f_G4, eighth);  // 1/8 g
	PlayNote(f_G4, eighth);  // 1/8 g
	PlayNote(f_F4, qtr);     // 1/4 f
	PlayNote(f_Eb4, eighth); // 1/8 e bem
	PlayNote(f_Eb4, eighth); // 1/8 e bem
	PlayNote(f_D4, qtr);     // 1/4 d
	PlayNote(f_C4, qtr);     // 1/4 c
	
	PlayNote(f_G4, eighth);  // 1/8 g  Во поле кудрявая стояла
	PlayNote(f_G4, eighth);  // 1/8 g
	PlayNote(f_B4, eighth);  // 1/8 g
	PlayNote(f_G4, eighth);  // 1/8 g
	PlayNote(f_F4, eighth);  // 1/8 f
	PlayNote(f_F4, eighth);  // 1/8 f
	PlayNote(f_Eb4, eighth); // 1/8 e bem
	PlayNote(f_Eb4, eighth); // 1/8 e bem
	PlayNote(f_D4, qtr);     // 1/4 d
	PlayNote(f_C4, qtr);     // 1/4 c   
	   
	PlayNote(f_D4, qtr_p);   // 1/4 d  Лю-ли, лю-ли стояла
	PlayNote(f_Eb4, eighth); // 1/8 e bem
	PlayNote(f_F4, qtr);     // 1/4 f
	PlayNote(f_Eb4, eighth); // 1/8 e bem
	PlayNote(f_Eb4, eighth); // 1/8 e bem
	PlayNote(f_D4, qtr);     // 1/4 d
	PlayNote(f_C4, qtr);     // 1/4 c
	
	PlayNote(f_D4, qtr_p);   // 1/4 d   Лю-ли, лю-ли стояла
	PlayNote(f_Eb4, eighth); // 1/8 e bem
	PlayNote(f_F4, qtr);     // 1/4 f
	PlayNote(f_Eb4, eighth); // 1/8 e bem
	PlayNote(f_Eb4, eighth); // 1/8 e bem
	PlayNote(f_D4, qtr);     // 1/4 d
	PlayNote(f_C4, qtr);     // 1/4 c
}

void Gentlemen(int bpm) { 
	Tempo(bpm);
	PlayNote(f_E4, eighth); 	// 1/8 e 
	PlayNote(f_F4, eighth_p); 	// 1/8p f
	PlayNote(f_E4, sixteen); 	// 1/16 e
	PlayNote(f_Eb4, sixteen);  	// 1/16 e bem
	PlayNote(f_E4, eighth); 	// 1/8 e
	PlayNote(f_C4, sixteen); 	// 1/16 c
	PlayNote(f_C4, qtr);  		// 1/4 c
	PlayNote(PAUSE, eighth); 	// 1/8
	
	PlayNote(f_E4, eighth); 	// 1/8 e 
	PlayNote(f_F4, eighth_p); 	// 1/8p f
	PlayNote(f_E4, sixteen); 	// 1/16 e--
	PlayNote(f_Eb4, sixteen);  	// 1/16 e bem
	PlayNote(f_E4, eighth); 	// 1/8 e
	PlayNote(f_C4, sixteen); 	// 1/16 c
	PlayNote(f_C4, qtr);  		// 1/4 c
	PlayNote(PAUSE, eighth); 	// 1/8
	
	PlayNote(f_E4, eighth); 	// 1/8 e 
	PlayNote(f_C5, sixteen); 	// 1/16 c
	PlayNote(f_H4, eighth); 	// 1/8 e 
	PlayNote(f_A4, sixteen); 	// 1/16 a
	PlayNote(f_A4, eighth); 	// 1/8 a 
	PlayNote(f_G4, eighth_p); 	// 1/8p g 
	PlayNote(f_F4, sixteen); 	// 1/16 f
	PlayNote(f_E4, sixteen); 	// 1/16 e
	PlayNote(f_F4, eighth_p); 	// 1/8p f
	PlayNote(f_E4, eighth); 	// 1/8 e
	PlayNote(f_D4, qtr_p); 		// 1/4p d
	PlayNote(PAUSE, qtr); 		// 1/4
	PlayNote(PAUSE, eighth); 	// 1/8
	
	PlayNote(f_D4, eighth); 	// 1/8 d
	PlayNote(f_G4, eighth_p); 	// 1/8p g 
	PlayNote(f_F4, sixteen); 	// 1/16 f
	PlayNote(f_E4, sixteen); 	// 1/16 e
	PlayNote(f_F4, eighth); 	// 1/8 f
	PlayNote(f_D4, sixteen); 	// 1/16 d
	PlayNote(f_D4, qtr);  		// 1/4 d
	PlayNote(PAUSE, eighth); 	// 1/8
	
	PlayNote(f_D4, eighth); 	// 1/8 d
	PlayNote(f_G4, eighth_p); 	// 1/8p g 
	PlayNote(f_F4, sixteen); 	// 1/16 f
	PlayNote(f_E4, sixteen); 	// 1/16 e
	PlayNote(f_F4, eighth); 	// 1/8 f
	PlayNote(f_D4, sixteen); 	// 1/16 d
	PlayNote(f_D4, qtr);  		// 1/4 d
	PlayNote(PAUSE, eighth); 	// 1/8
	
	PlayNote(f_D4, eighth); 	// 1/8 d
	PlayNote(f_G4, eighth_p); 	// 1/8p g 
	PlayNote(f_F4, sixteen); 	// 1/16 f
	PlayNote(f_E4, eighth); 	// 1/8 e
	PlayNote(f_D4, eighth); 	// 1/8 d
	PlayNote(f_F4, eighth_p); 	// 1/8p f
	PlayNote(f_E4, sixteen); 	// 1/16 e
	PlayNote(f_D4, eighth); 	// 1/8 d
	PlayNote(f_H3, eighth); 	// 1/8 h
	PlayNote(f_F4, eighth); 		// 1/8 f
	PlayNote(f_E4, qtr_p); 		// 1/4p e
	PlayNote(PAUSE, qtr); 		// 1/4
	PlayNote(PAUSE, eighth); 	// 1/8
	
	PlayNote(f_E4, eighth); 	// 1/8 e 
	PlayNote(f_F4, eighth_p); 	// 1/8p f
	PlayNote(f_E4, sixteen); 	// 1/16 e
	PlayNote(f_Eb4, sixteen);  	// 1/16 e bem
	PlayNote(f_E4, eighth); 	// 1/8 e
	PlayNote(f_C4, sixteen); 	// 1/16 c
	PlayNote(f_C4, qtr);  		// 1/4 c
	PlayNote(PAUSE, eighth); 	// 1/8
	
	PlayNote(f_E4, eighth); 	// 1/8 e 
	PlayNote(f_F4, eighth_p); 	// 1/8p f
	PlayNote(f_E4, sixteen); 	// 1/16 e
	PlayNote(f_Eb4, sixteen);  	// 1/16 e bem
	PlayNote(f_E4, eighth); 	// 1/8 e
	PlayNote(f_H3, sixteen); 	// 1/16 h
	PlayNote(f_H3, qtr);  		// 1/4 h
	PlayNote(PAUSE, eighth); 	// 1/8

	PlayNote(f_E4, eighth); 	// 1/8 e 
	PlayNote(f_F4, eighth_p); 	// 1/8p f
	PlayNote(f_E4, sixteen); 	// 1/16 e
	PlayNote(f_Eb4, eighth);  	// 1/8 e bem
	PlayNote(f_G4, eighth_p); 	// 1/8p g 
	PlayNote(f_F4, sixteen); 	// 1/16 f
	PlayNote(f_E4, eighth); 	// 1/8 e 
	PlayNote(f_F4, eighth); 	// 1/8 f
	PlayNote(f_E4, eighth); 	// 1/8 e
	PlayNote(f_D4, qtr_p); 		// 1/4p d
	PlayNote(PAUSE, qtr); 		// 1/4
	PlayNote(PAUSE, eighth); 	// 1/8
	
	PlayNote(f_A4, eighth); 	// 1/8 a
	PlayNote(f_D5, eighth_p); 	// 1/8p d
	PlayNote(f_C5, sixteen); 	// 1/16 c
	PlayNote(f_H4, sixteen); 	// 1/16 h
	PlayNote(f_C4, eighth); 	// 1/8 c
	PlayNote(f_H4, sixteen); 	// 1/16 h
	PlayNote(f_H4, qtr); 		// 1/4 h
	PlayNote(PAUSE, eighth); 	// 1/8
	
	PlayNote(f_A4, eighth); 	// 1/8 a
	PlayNote(f_D5, eighth_p); 	// 1/8p d
	PlayNote(f_C5, sixteen); 	// 1/16 c
	PlayNote(f_H4, sixteen); 	// 1/16 h
	PlayNote(f_C4, eighth); 	// 1/8 c
	PlayNote(f_H4, sixteen); 	// 1/16 h
	PlayNote(f_H4, qtr); 		// 1/4 h
	PlayNote(PAUSE, eighth); 	// 1/8
	
	PlayNote(f_A4, eighth); 	// 1/8 a
	PlayNote(f_D5, eighth_p); 	// 1/8p d
	PlayNote(f_C5, sixteen); 	// 1/16 c
	PlayNote(f_D5, eighth_p); 	// 1/8p d
	PlayNote(f_C5, sixteen); 	// 1/16 c
	PlayNote(f_C5, eighth_p); 	// 1/8p c
	PlayNote(f_H4, sixteen); 	// 1/16 h
	PlayNote(f_Ab4, eighth); 	// 1/8 a bem
	PlayNote(f_E4, eighth); 	// 1/8 e 
	PlayNote(f_H4, eighth); 	// 1/8 h
	PlayNote(f_A4, qtr_p); 		// 1/4p a
}

//-------------------------- Main program ---------------------------------
void main(void)
{
  soundPin = 1;
  EA = 0;	// блокировка всех прерываний
  ET0 = 0;  // запрет прерывания по таймеру 0
  
  // тест дисплея
  dig1 = 0;		
  dig2 = 1;		// тестируем младший разряд дисплея
  x = 0x80;		// сегмент_a = 0
  for(k=0; k < 8 ; k++) {
	P1 = ~x;
	delay(100);
	x = x >> 1; // след.сегмент
  }
  P1 = 0xFF;	// <пусто> --> дисплей
  delay(100);
  dig1 = 1;		// тестируем старший разряд дисплея
  dig2 = 0;
  x = 0x80;		// сегмент_a = 0
  for(k=0; k < 8 ; k++) {
	P1 = ~x;	// инверсия байта --> P1 (дисплей)
	delay(100);
	x = x >> 1;
  }
  P1 = 0xFF;	// очистить дисплей
  P2 = 0xFF;	// порт P2 переключаем на ввод
  P3 = 0x7C;
  
  // настройка таймеров
  TMOD = 0x11;  // Задание режимов работы C/T0, C/T1 в качестве 16-разрядных таймеров
  TL0 = 0xEF;   // Загрузка младшего байта C/T0
  TH0 = 0xD8;   // Загрузка старшего байта C/T0
  EA = 1;       // Снятие общей блокировки прерываний
  ET0 = 1;      // Разрешение прерывания от TF0
  TR0 = 1;      // Пуск таймера C/T0
  
  while(1) {
	x = 0x01;
    for(k=0; k < 8 ; k++) {  // сканирование муз.клавиш "бегущим 0"
	  P0 = ~x;	
	  y = P2;
	  if(y != 0xFF) {	// есть нажатие
		  delay(50);	
		  y = P2;		// повторное чтение
		  if(y != 0xFF) {	// нажатие подтверждено
		    y = (~y) & 0x7F;
			switch(y) {	// порядковый номер клавиши, начиная с 0
			  case 1:  j = 7 - k;
					break;
			  case 2:  j = 15 - k;
					break;
			  case 4:  j = 23 - k;
					break;
			  case 8:  j = 31 - k;
					break;
			  case 16: j = 39 - k;
					break;
			  case 32: j = 47 - k;
					break;
			  case 64: j = 55 - k;
			}
			midi_note = j + 36;	// код ноты по стандарту MIDI
			dec0 = j % 10;
			dec1 = j / 10;		
			StartSound(notes[j]);  	// стартуем ноту
			delay(keytime);
			while(1) {
			  y = P2;
			  if(y == 0xFF) {	// есть отжатие
			    delay(50);
			    y = P2;
			    if(y == 0xFF) {	// отжатие подтверждено
			      dec0 = 10;
			      dec1 = 10;
				  StopSound();	// выключаем ноту
				  break;
			    }
			  }
			}
		  }
	  }
	  else {  // сканирование функциональных кнопок "бегущим 0"
		y = P3;
		y = y | 0x83;
	    if(y != 0xFF) {	// есть нажатие кнопок
		  delay(30);
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
			z = z + k; // порядковый номер кнопки, начиная с 0 (0..34)
			dec0 = z % 10;
			dec1 = z / 10;
			P1 = segmarr[z];
			
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
				  
				  // анализ нажатой кнопки
				  switch(z) {
					case 2: VoPole(temp);
							break;
					case 6: if(temp > 120)
						      temp = temp - 60;
							break;
					case 7: if(temp < 500)
						      temp = temp + 60;
							break;
					case 9: Gentlemen(temp);
							break;
					case 11: if(keytime > 200)
					 	      keytime = keytime - 100;
							break;		
					case 14: if(keytime < 1000)
					 	       keytime = keytime + 100;
							break;
					default:  StopSound();
				  }
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