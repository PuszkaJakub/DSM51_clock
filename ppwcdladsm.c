/*
	Jakub Puszka 313272
    wtorek godz. 13:00
*/

/*
    OBSLUGA:
    - Enter: Wejdz w tryb edycji (zapala sie dioda LED)
    - Strzalka w prawo: Zwieksz sekundy
    - Strzalka w dol: Zwieksz minuty
    - Strzalka w lewo: Zwieksz godziny

*/

#include <8051.h>

#define TRUE 1
#define FALSE 0

#define TH_0 226

__code unsigned char WZOR[10] = {
    		0b00111111,  // 0
			0b00000110,	 // 1
            0b01011011,  // 2
			0b01001111,  // 3
			0b01100110,  // 4
			0b01101101,  // 5
			0b01111101,  // 6
			0b00000111,  // 7
			0b01111111,  // 8
			0b01101111,  // 9
			};


__bit __at (0x96) SEG_OFF;

//flagi bitowe
__bit __at (0x97) LED; // bit 7 portu P1 sterujcy LED
__bit t0_flag; // flaga przerwania licznika T0 

unsigned char timer_buf1; // czasomierz programowy
unsigned char timer_buf2;
unsigned char send_buf; // bufor bajtu nadawanego 

void incr_godz(char tablica[6]){
    tablica[4]++;
    if(tablica[4] == 10){
        tablica[4] = 0;
        tablica[5]++;
    }
    if((tablica[5] == 2) && (tablica[4] == 4)){
        tablica[4] = 0;
        tablica[5] = 0;
    }
}

void incr_min(char tablica[6], char incrflag){
    tablica[2]++;
    if(tablica[2] == 10){
        tablica[2] = 0;
        tablica[3]++;
    }
    if(tablica[3] == 6){
        tablica[3] = 0;
        if(incrflag){
            incr_godz(tablica);
        }
    }
}

void incr_sek(char tablica[6], char incrflag){
    tablica[0]++;
    if(tablica[0] == 10){
        tablica[0] = 0;
        tablica[1]++;
    }
    if(tablica[1] == 6){
        tablica[1] = 0;
        if(incrflag){
            incr_min(tablica, 1);
        }
    }
}


char tablica[6] = {0,5,9,5,3,2};
unsigned char klikniety = 0;
unsigned char aktualny_key = 0;
unsigned char stop = 0;


void t0_serv(void); 


void odczyt_klawiatury(){
    if(klikniety == FALSE){
        switch(aktualny_key){
            case 1: 
                LED = !LED; //zmien stan diody LED

                if(stop){
                    stop = 0;
                }
                else{
                    stop = 1;
                }
                klikniety = TRUE;
                break;

            case 4:
                if(stop){
                    incr_sek(tablica, 0);
                    klikniety = TRUE;
                }
                break;

            case 16:
                if(stop){
                    incr_min(tablica, 0);
                    klikniety = TRUE;
                }
                break;

            case 32:
                if(stop){
                    incr_godz(tablica);
                    klikniety = TRUE;
                }
                break;
        }

    }
}



void main(){

	//bufor wybierajacy bitowo aktywny wyswietlacz
    __xdata unsigned char * led_wyb =
                         (__xdata unsigned char *) 0xFF30; 
     //bufor wybierajacy aktywne segmenty wyswietlacza
     __xdata unsigned char * led_led = 
                         (__xdata unsigned char *) 0xFF38; 
 
     unsigned char led_p = 0, //indeks aktywnego wyswietlacza
                   led_b = 1; //aktywny wyswietlacz (bitowo)
                   
     
    PCON = 0x80; // zegar dla sio, T1 (19200 b/s)
    SCON = 0b01010000; 
    TMOD = 0b01110000; //ustaw T1 w tryb 2; T0 w tryb 1 
     
    TH0 = TH_0;

    TL1 = 0xFD;
    TH1 = 0xFD;

    timer_buf1 = 240; // laduj timeout T0 (100ms)
    timer_buf2 = 4;
    t0_flag = FALSE; // zeruj flage przerw. t0_int 

    ET0 = TRUE; // aktywuj przerwanie od licznika T0
    ES = TRUE; // aktywuj przerwanie od UART
    EA = TRUE; // aktywuj wszystkie przerwania
    TR0 = TRUE; // uruchom licznik T0
    TR1 = TRUE; // uruchom licznik T1 


    while (TRUE) { //petla nieskonczona

        if (t0_flag) { //przerwanie zegarowe

            SEG_OFF = TRUE;         //wylacza wysw. LED
            *led_wyb = led_b;       //wybiera wyswietlacz
            *led_led = WZOR[tablica[led_p]]; //wybiera segmenty
            SEG_OFF = FALSE;        //wlacza wysw. LED
                

            if(P3_5){
                aktualny_key = led_b;
                odczyt_klawiatury();
            }
            else{
                if(led_b == aktualny_key && klikniety){
                    klikniety = 0;
                }   
            }
            led_b += led_b;
            led_p++;

            if(led_p == 6){
                led_p = 0;
                led_b = 1;


            }
            t0_flag = FALSE;
            if(!stop){
                t0_serv();
            }
        }
    }
}

    void t0_serv(void){
        if (timer_buf1){
            timer_buf1--; //zmniejsz stan czasomierza
        }
        else{
            timer_buf2--;
            timer_buf1 = 240;
        }
        if(!timer_buf2){
            timer_buf1 = 240; 
            timer_buf2 = 4;
            
            incr_sek(tablica, 1);
        }
    }

    void t0_int(void) __interrupt(1){
        TH0 = TH_0; //ustawia flage sygnalizujaca
        t0_flag = TRUE; //fakt wystapienia przerwania
    } 