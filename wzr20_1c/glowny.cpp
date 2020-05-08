/****************************************************
	Wirtualne zespoly robocze - przykladowy projekt w C++
	Do zadañ dotycz¹cych wspó³pracy, ekstrapolacji i
	autonomicznych obiektów
	Amadeusz Chmielowski s165398
	****************************************************/

#include <windows.h>
#include <math.h>
#include <time.h>
#include <gl\gl.h>
#include <gl\glu.h>
#include <iterator> 
#include <map>
#include <vector>
#include "obiekty.h"
#include "grafika.h"
#include "siec.h"
using namespace std;

FILE *f = fopen("wzr_log.txt", "w");    // plik do zapisu informacji testowych

vector<ObiektRuchomy*> saved_obj;

ObiektRuchomy *MojObiekt;          // obiekt przypisany do tej aplikacji
Teren teren;

map<int, ObiektRuchomy*> obiekty_ruchome;

float fDt;                          // sredni czas pomiedzy dwoma kolejnymi cyklami symulacji i wyswietlania
long czas_cyklu_WS, licznik_sym;     // zmienne pomocnicze potrzebne do obliczania fDt

long czas_start = clock();          // czas uruchomienia aplikacji

multicast_net *multi_reciv;         // wsk do obiektu zajmujacego sie odbiorem komunikatow
multicast_net *multi_send;          //   -||-  wysylaniem komunikatow

HANDLE threadReciv;                 // uchwyt w¹tku odbioru komunikatów
extern HWND okno;
int SHIFTwcisniety = 0;
bool czy_rysowac_ID = 1;            // czy rysowac nr ID przy ka¿dym obiekcie
bool sterowanie_myszkowe = 0;       // sterowanie za pomoc¹ klawisza myszki
int kursor_x = 0, kursor_y = 0;     // po³o¿enie kursora myszy

extern ParametryWidoku parwid;      // ustawienia widoku zdefiniowane w grafice


struct Ramka                                   // g³ówna struktura s³u¿¹ca do przesy³ania informacji
{
	int typ;                                     // typ ramki: informacja o stanie, informacja o zamkniêciu, komunikat tekstowy, ... 
	int iID;                                      // identyfikator obiektu, którego 
	StanObiektu stan;                            // po³o¿enie, prêdkoœæ: œrodka masy + k¹towe, ...
	int pressed_key;
};


//******************************************
// Funkcja obs³ugi w¹tku odbioru komunikatów 
// UWAGA!  Odbierane s¹ te¿ komunikaty z w³asnej aplikacji by porównaæ obraz ekstrapolowany do rzeczywistego.
DWORD WINAPI WatekOdbioru(void *ptr)
{
	multicast_net *pmt_net = (multicast_net*)ptr;  // wskaŸnik do obiektu klasy multicast_net
	Ramka ramka;

	while (1)
	{
		int rozmiar = pmt_net->reciv((char*)&ramka, sizeof(Ramka));   // oczekiwanie na nadejœcie ramki 
		StanObiektu stan = ramka.stan;

		fprintf(f, "odebrano stan iID = %d, ID dla mojego obiektu = %d\n", ramka.iID, MojObiekt->iID);

		if (ramka.iID != MojObiekt->iID)          // jeœli to nie mój w³asny obiekt
		{
			if (obiekty_ruchome[ramka.iID] == NULL)        // nie ma jeszcze takiego obiektu w tablicy -> trzeba go
				// stworzyæ
			{
				ObiektRuchomy *ob = new ObiektRuchomy();
				ob->iID = ramka.iID;
				obiekty_ruchome[ramka.iID] = ob;

				//fprintf(f, "zarejestrowano %d obcy obiekt o ID = %d\n", iLiczbaCudzychOb - 1, CudzeObiekty[iLiczbaCudzychOb]->iID);
			}
			obiekty_ruchome[ramka.iID]->ZmienStan(stan);   // aktualizacja stanu obiektu obcego 	
			if (ramka.typ == 2) {
				if (saved_obj.size() == 0) {
					saved_obj.push_back(obiekty_ruchome[ramka.iID]);
				}
				else {
					for (int i = 0; i < saved_obj.size(); i++) {
						if (saved_obj[i]->iID == ramka.iID)
						{
							saved_obj[i] = obiekty_ruchome[ramka.iID];
						}
						else
						{
							saved_obj.push_back(obiekty_ruchome[ramka.iID]);
						}
					}
				}
			}
			if (ramka.typ == 3) {
				Ramka ramka2;
				for (int i = 0; i < saved_obj.size(); i++) {
					if (ramka.pressed_key == saved_obj[i]->iID % 10) {
						ramka2.stan = saved_obj[i]->Stan();               // stan w³asnego obiektu 
						ramka2.iID = ramka.iID;
						ramka2.typ = 4;

						multi_send->send((char*)&ramka2, sizeof(Ramka));  // wys³anie komunikatu do pozosta³ych aplikacji
					}
				}


			}
		}
		if (ramka.typ == 4) {
			if (ramka.iID == MojObiekt->iID) {
				MojObiekt->stan = ramka.stan;
			}
		}
	}  // while(1)
	return 1;
}

// *****************************************************************
// ****    Wszystko co trzeba zrobiæ podczas uruchamiania aplikacji
// ****    poza grafik¹   
void PoczatekInterakcji()
{
	DWORD dwThreadId;

	MojObiekt = new ObiektRuchomy();    // tworzenie wlasnego obiektu

	czas_cyklu_WS = clock();             // pomiar aktualnego czasu

	// obiekty sieciowe typu multicast (z podaniem adresu WZR oraz numeru portu)
	multi_reciv = new multicast_net("224.12.12.120", 200100);      // obiekt do odbioru ramek sieciowych
	multi_send = new multicast_net("224.12.12.120", 200100);       // obiekt do wysy³ania ramek


	// uruchomienie w¹tku obs³uguj¹cego odbiór komunikatów:
	threadReciv = CreateThread(
		NULL,                        // no security attributes
		0,                           // use default stack size
		WatekOdbioru,                // thread function
		(void *)multi_reciv,               // argument to thread function
		NULL,                        // use default creation flags
		&dwThreadId);                // returns the thread identifier

}


// *****************************************************************
// ****    Wszystko co trzeba zrobiæ w ka¿dym cyklu dzia³ania 
// ****    aplikacji poza grafik¹ 
void Cykl_Wirtualnego_Swiata()
{
	licznik_sym++;

	if (licznik_sym % 50 == 0)          // jeœli licznik cykli przekroczy³ pewn¹ wartoœæ, to
	{                              // nale¿y na nowo obliczyæ œredni czas cyklu fDt
		char text[256];
		long czas_pop = czas_cyklu_WS;
		czas_cyklu_WS = clock();
		float fFps = (50 * CLOCKS_PER_SEC) / (float)(czas_cyklu_WS - czas_pop);
		if (fFps != 0) fDt = 1.0 / fFps; else fDt = 1;

		sprintf(text, "WZR/WWC-lab 2019/20 temat 1, wersja c (%0.0f fps  %0.2fms) ", fFps, 1000.0 / fFps);

		SetWindowText(okno, text); // wyœwietlenie aktualnej iloœci klatek/s w pasku okna			
	}

	MojObiekt->Symulacja(fDt);                    // symulacja w³asnego obiektu

	Ramka ramka;
	ramka.stan = MojObiekt->Stan();               // stan w³asnego obiektu 
	ramka.iID = MojObiekt->iID;
	ramka.typ = 1;

	multi_send->send((char*)&ramka, sizeof(Ramka));  // wys³anie komunikatu do pozosta³ych aplikacji
}

// *****************************************************************
// ****    Wszystko co trzeba zrobiæ podczas zamykania aplikacji
// ****    poza grafik¹ 
void ZakonczenieInterakcji()
{
	fprintf(f, "Koniec interakcji\n");
	fclose(f);
}

//deklaracja funkcji obslugi okna
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);


HWND okno;                   // uchwyt do okna aplikacji
HDC g_context = NULL;        // uchwyt kontekstu graficznego



//funkcja Main - dla Windows
int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nCmdShow)
{
	MSG meldunek;		  //innymi slowy "komunikat"
	WNDCLASS nasza_klasa; //klasa g³ównego okna aplikacji

	static char nazwa_klasy[] = "Klasa_Podstawowa";

	//Definiujemy klase g³ównego okna aplikacji
	//Okreslamy tu wlasciwosci okna, szczegoly wygladu oraz
	//adres funkcji przetwarzajacej komunikaty
	nasza_klasa.style = CS_HREDRAW | CS_VREDRAW;
	nasza_klasa.lpfnWndProc = WndProc; //adres funkcji realizuj¹cej przetwarzanie meldunków 
	nasza_klasa.cbClsExtra = 0;
	nasza_klasa.cbWndExtra = 0;
	nasza_klasa.hInstance = hInstance; //identyfikator procesu przekazany przez MS Windows podczas uruchamiania programu
	nasza_klasa.hIcon = 0;
	nasza_klasa.hCursor = LoadCursor(0, IDC_ARROW);
	nasza_klasa.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
	nasza_klasa.lpszMenuName = "Menu";
	nasza_klasa.lpszClassName = nazwa_klasy;

	//teraz rejestrujemy klasê okna g³ównego
	RegisterClass(&nasza_klasa);

	okno = CreateWindow(nazwa_klasy, "WZR/WWC-lab 2019/20 temat 1 - Architektury sieciowe, wersja c", WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		100, 50, 750, 750, NULL, NULL, hInstance, NULL);


	ShowWindow(okno, nCmdShow);

	//odswiezamy zawartosc okna
	UpdateWindow(okno);

	// pobranie komunikatu z kolejki jeœli funkcja PeekMessage zwraca wartoœæ inn¹ ni¿ FALSE,
	// w przeciwnym wypadku symulacja wirtualnego œwiata wraz z wizualizacj¹
	ZeroMemory(&meldunek, sizeof(meldunek));
	while (meldunek.message != WM_QUIT)
	{
		if (PeekMessage(&meldunek, NULL, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&meldunek);
			DispatchMessage(&meldunek);
		}
		else
		{
			Cykl_Wirtualnego_Swiata();    // Cykl wirtualnego œwiata
			InvalidateRect(okno, NULL, FALSE);
		}
	}

	return (int)meldunek.wParam;
}

/********************************************************************
FUNKCJA OKNA realizujaca przetwarzanie meldunków kierowanych do okna aplikacji*/
LRESULT CALLBACK WndProc(HWND okno, UINT kod_meldunku, WPARAM wParam, LPARAM lParam)
{

	switch (kod_meldunku)
	{
	case WM_CREATE:  //meldunek wysy³any w momencie tworzenia okna
	{

		g_context = GetDC(okno);

		srand((unsigned)time(NULL));
		int wynik = InicjujGrafike(g_context);
		if (wynik == 0)
		{
			printf("nie udalo sie otworzyc okna graficznego\n");
			//exit(1);
		}

		PoczatekInterakcji();

		SetTimer(okno, 1, 10, NULL);

		return 0;
	}


	case WM_PAINT:
	{
		PAINTSTRUCT paint;
		HDC kontekst;
		kontekst = BeginPaint(okno, &paint);

		RysujScene();
		SwapBuffers(kontekst);

		EndPaint(okno, &paint);

		return 0;
	}

	case WM_TIMER:

		return 0;

	case WM_SIZE:
	{
		int cx = LOWORD(lParam);
		int cy = HIWORD(lParam);

		ZmianaRozmiaruOkna(cx, cy);

		return 0;
	}

	case WM_DESTROY: //obowi¹zkowa obs³uga meldunku o zamkniêciu okna

		ZakonczenieInterakcji();
		ZakonczenieGrafiki();

		ReleaseDC(okno, g_context);
		KillTimer(okno, 1);

		PostQuitMessage(0);
		return 0;

	case WM_LBUTTONDOWN: //reakcja na lewy przycisk myszki
	{
		int x = LOWORD(lParam);
		int y = HIWORD(lParam);
		if (sterowanie_myszkowe)
			MojObiekt->F = 30.0;        // si³a pchaj¹ca do przodu
		break;
	}
	case WM_RBUTTONDOWN: //reakcja na prawy przycisk myszki
	{
		int x = LOWORD(lParam);
		int y = HIWORD(lParam);
		if (sterowanie_myszkowe)
			MojObiekt->F = -5.0;        // si³a pchaj¹ca do tylu
		break;
	}
	case WM_MBUTTONDOWN: //reakcja na œrodkowy przycisk myszki : uaktywnienie/dezaktywacja sterwania myszkowego
	{
		sterowanie_myszkowe = 1 - sterowanie_myszkowe;
		kursor_x = LOWORD(lParam);
		kursor_y = HIWORD(lParam);
		break;
	}
	case WM_LBUTTONUP: //reakcja na puszczenie lewego przycisku myszki
	{
		if (sterowanie_myszkowe)
			MojObiekt->F = 0.0;        // si³a pchaj¹ca do przodu
		break;
	}
	case WM_RBUTTONUP: //reakcja na puszczenie lewy przycisk myszki
	{
		if (sterowanie_myszkowe)
			MojObiekt->F = 0.0;        // si³a pchaj¹ca do przodu
		break;
	}
	case WM_MOUSEMOVE:
	{
		int x = LOWORD(lParam);
		int y = HIWORD(lParam);
		if (sterowanie_myszkowe)
		{
			float kat_skretu = (float)(kursor_x - x) / 20;
			if (kat_skretu > 45) kat_skretu = 45;
			if (kat_skretu < -45) kat_skretu = -45;
			MojObiekt->stan.alfa = PI * kat_skretu / 180;
		}
		break;
	}
	case WM_KEYDOWN:
	{

		switch (LOWORD(wParam))
		{
		case VK_SHIFT:
		{
			SHIFTwcisniety = 1;
			break;
		}
		case VK_SPACE:
		{
			MojObiekt->ham = 10.0;       // stopieñ hamowania (reszta zale¿y od si³y docisku i wsp. tarcia)
			break;                       // 1.0 to maksymalny stopieñ (np. zablokowanie kó³)
		}
		case VK_UP:
		{

			MojObiekt->F = 50.0;        // si³a pchaj¹ca do przodu
			break;
		}
		case VK_DOWN:
		{
			MojObiekt->F = -20.0;
			break;
		}
		case VK_LEFT:
		{
			if (SHIFTwcisniety) MojObiekt->pr_kierownicy = 0.5;
			else MojObiekt->pr_kierownicy = 0.25 / 4;

			break;
		}
		case VK_RIGHT:
		{
			if (SHIFTwcisniety) MojObiekt->pr_kierownicy = -0.5;
			else MojObiekt->pr_kierownicy = -0.25 / 4;
			break;
		}
		case 'I':   // wypisywanie nr ID
		{
			czy_rysowac_ID = 1 - czy_rysowac_ID;
			break;
		}
		case 'W':   // oddalenie widoku
		{
			//pol_kamery = pol_kamery - kierunek_kamery*0.3;
			if (parwid.oddalenie > 0.5) parwid.oddalenie /= 1.2;
			else parwid.oddalenie = 0;
			break;
		}
		case 'S':   // przybli¿enie widoku
		{
			//pol_kamery = pol_kamery + kierunek_kamery*0.3; 
			if (parwid.oddalenie > 0) parwid.oddalenie *= 1.2;
			else parwid.oddalenie = 0.5;
			break;
		}
		case 'Q':   // widok z góry
		{
			if (parwid.sledzenie) break;
			parwid.widok_z_gory = 1 - parwid.widok_z_gory;
			if (parwid.widok_z_gory)
			{
				parwid.pol_kamery_1 = parwid.pol_kamery; parwid.kierunek_kamery_1 = parwid.kierunek_kamery; parwid.pion_kamery_1 = parwid.pion_kamery;
				parwid.oddalenie_1 = parwid.oddalenie; parwid.kat_kam_z_1 = parwid.kat_kam_z;
				parwid.pol_kamery = parwid.pol_kamery_2; parwid.kierunek_kamery = parwid.kierunek_kamery_2; parwid.pion_kamery = parwid.pion_kamery_2;
				parwid.oddalenie = parwid.oddalenie_2; parwid.kat_kam_z = parwid.kat_kam_z_2;
			}
			else
			{
				parwid.pol_kamery_2 = parwid.pol_kamery; parwid.kierunek_kamery_2 = parwid.kierunek_kamery; parwid.pion_kamery_2 = parwid.pion_kamery;
				parwid.oddalenie_2 = parwid.oddalenie; parwid.kat_kam_z_2 = parwid.kat_kam_z;
				parwid.pol_kamery = parwid.pol_kamery_1; parwid.kierunek_kamery = parwid.kierunek_kamery_1; parwid.pion_kamery = parwid.pion_kamery_1;
				parwid.oddalenie = parwid.oddalenie_1; parwid.kat_kam_z = parwid.kat_kam_z_1;
			}
			break;
		}
		case 'E':   // obrót kamery ku górze (wzglêdem lokalnej osi z)
		{
			parwid.kat_kam_z += PI * 5 / 180;
			break;
		}
		case 'D':   // obrót kamery ku do³owi (wzglêdem lokalnej osi z)
		{
			parwid.kat_kam_z -= PI * 5 / 180;
			break;
		}
		case 'A':   // w³¹czanie, wy³¹czanie trybu œledzenia obiektu
		{
			parwid.sledzenie = 1 - parwid.sledzenie;
			if (parwid.sledzenie)
			{
				parwid.oddalenie = parwid.oddalenie_3; parwid.kat_kam_z = parwid.kat_kam_z_3;
			}
			else
			{
				parwid.oddalenie_3 = parwid.oddalenie; parwid.kat_kam_z_3 = parwid.kat_kam_z;
				parwid.widok_z_gory = 0;
				parwid.pol_kamery = parwid.pol_kamery_1; parwid.kierunek_kamery = parwid.kierunek_kamery_1; parwid.pion_kamery = parwid.pion_kamery_1;
				parwid.oddalenie = parwid.oddalenie_1; parwid.kat_kam_z = parwid.kat_kam_z_1;
			}
			break;
		}
		case 'Z':
		{
			//zapisanie stanu
			Ramka ramka;
			ramka.stan = MojObiekt->Stan();               // stan w³asnego obiektu 
			ramka.iID = MojObiekt->iID;
			ramka.typ = 2;

			multi_send->send((char*)&ramka, sizeof(Ramka));  // wys³anie komunikatu do pozosta³ych aplikacji
			break;

		}
		case '0': {
			//zapisanie stanu
			Ramka ramka;
			ramka.stan = MojObiekt->Stan();               // stan w³asnego obiektu 
			ramka.iID = MojObiekt->iID;
			ramka.typ = 3;
			ramka.pressed_key = 0;

			multi_send->send((char*)&ramka, sizeof(Ramka));  // wys³anie komunikatu do pozosta³ych aplikacji
			break;
		}
		case '1': {
			//zapisanie stanu
			Ramka ramka;
			ramka.stan = MojObiekt->Stan();               // stan w³asnego obiektu 
			ramka.iID = MojObiekt->iID;
			ramka.typ = 3;
			ramka.pressed_key = 1;

			multi_send->send((char*)&ramka, sizeof(Ramka));  // wys³anie komunikatu do pozosta³ych aplikacji
			break;
		}
		case '2': {
			//zapisanie stanu
			Ramka ramka;
			ramka.stan = MojObiekt->Stan();               // stan w³asnego obiektu 
			ramka.iID = MojObiekt->iID;
			ramka.typ = 3;
			ramka.pressed_key = 2;

			multi_send->send((char*)&ramka, sizeof(Ramka));  // wys³anie komunikatu do pozosta³ych aplikacji
			break;
		}
		case '3': {
			//zapisanie stanu
			Ramka ramka;
			ramka.stan = MojObiekt->Stan();               // stan w³asnego obiektu 
			ramka.iID = MojObiekt->iID;
			ramka.typ = 3;
			ramka.pressed_key = 3;

			multi_send->send((char*)&ramka, sizeof(Ramka));  // wys³anie komunikatu do pozosta³ych aplikacji
			break;
		}
		case '4': {
			//zapisanie stanu
			Ramka ramka;
			ramka.stan = MojObiekt->Stan();               // stan w³asnego obiektu 
			ramka.iID = MojObiekt->iID;
			ramka.typ = 3;
			ramka.pressed_key = 4;

			multi_send->send((char*)&ramka, sizeof(Ramka));  // wys³anie komunikatu do pozosta³ych aplikacji
			break;
		}
		case '5': {
			//zapisanie stanu
			Ramka ramka;
			ramka.stan = MojObiekt->Stan();               // stan w³asnego obiektu 
			ramka.iID = MojObiekt->iID;
			ramka.typ = 3;
			ramka.pressed_key = 5;

			multi_send->send((char*)&ramka, sizeof(Ramka));  // wys³anie komunikatu do pozosta³ych aplikacji
			break;
		}
		case '6': {
			//zapisanie stanu
			Ramka ramka;
			ramka.stan = MojObiekt->Stan();               // stan w³asnego obiektu 
			ramka.iID = MojObiekt->iID;
			ramka.typ = 3;
			ramka.pressed_key = 6;

			multi_send->send((char*)&ramka, sizeof(Ramka));  // wys³anie komunikatu do pozosta³ych aplikacji
			break;
		}
		case '7': {
			//zapisanie stanu
			Ramka ramka;
			ramka.stan = MojObiekt->Stan();               // stan w³asnego obiektu 
			ramka.iID = MojObiekt->iID;
			ramka.typ = 3;
			ramka.pressed_key = 7;

			multi_send->send((char*)&ramka, sizeof(Ramka));  // wys³anie komunikatu do pozosta³ych aplikacji
			break;
		}
		case '8': {
			//zapisanie stanu
			Ramka ramka;
			ramka.stan = MojObiekt->Stan();               // stan w³asnego obiektu 
			ramka.iID = MojObiekt->iID;
			ramka.typ = 3;
			ramka.pressed_key = 8;

			multi_send->send((char*)&ramka, sizeof(Ramka));  // wys³anie komunikatu do pozosta³ych aplikacji
			break;
		}
		case '9': {
			//zapisanie stanu
			Ramka ramka;
			ramka.stan = MojObiekt->Stan();               // stan w³asnego obiektu 
			ramka.iID = MojObiekt->iID;
			ramka.typ = 3;
			ramka.pressed_key = 9;

			multi_send->send((char*)&ramka, sizeof(Ramka));  // wys³anie komunikatu do pozosta³ych aplikacji
			break;
		}
		case VK_F1:  // wywolanie systemu pomocy
		{
			char lan[1024], lan_bie[1024];
			//GetSystemDirectory(lan_sys,1024);
			GetCurrentDirectory(1024, lan_bie);
			strcpy(lan, "C:\\Program Files\\Internet Explorer\\iexplore ");
			strcat(lan, lan_bie);
			strcat(lan, "\\pomoc.htm");
			int wyni = WinExec(lan, SW_NORMAL);
			if (wyni < 32)  // proba uruchominia pomocy nie powiodla sie
			{
				strcpy(lan, "C:\\Program Files\\Mozilla Firefox\\firefox ");
				strcat(lan, lan_bie);
				strcat(lan, "\\pomoc.htm");
				wyni = WinExec(lan, SW_NORMAL);
				if (wyni < 32)
				{
					char lan_win[1024];
					GetWindowsDirectory(lan_win, 1024);
					strcat(lan_win, "\\notepad pomoc.txt ");
					wyni = WinExec(lan_win, SW_NORMAL);
				}
			}
			break;
		}
		case VK_ESCAPE:
		{
			SendMessage(okno, WM_DESTROY, 0, 0);
			break;
		}
		} // switch po klawiszach

		break;
	}
	case WM_KEYUP:
	{
		switch (LOWORD(wParam))
		{
		case VK_SHIFT:
		{
			SHIFTwcisniety = 0;
			break;
		}
		case VK_SPACE:
		{
			MojObiekt->ham = 0.0;
			break;
		}
		case VK_UP:
		{
			MojObiekt->F = 0.0;

			break;
		}
		case VK_DOWN:
		{
			MojObiekt->F = 0.0;
			break;
		}
		case VK_LEFT:
		{
			MojObiekt->Fb = 0.00;
			//MojObiekt->stan.alfa = 0;
			MojObiekt->pr_kierownicy = 0;
			break;
		}
		case VK_RIGHT:
		{
			MojObiekt->Fb = 0.00;
			//MojObiekt->stan.alfa = 0;
			MojObiekt->pr_kierownicy = 0;
			break;
		}

		}

		break;
	}

	default: //standardowa obs³uga pozosta³ych meldunków
		return DefWindowProc(okno, kod_meldunku, wParam, lParam);
	}


}

