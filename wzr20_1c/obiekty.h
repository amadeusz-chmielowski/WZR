#include <stdio.h>
#include "kwaternion.h"

#define PI 3.1415926

struct StanObiektu
{
	Wektor3 wPol;             // polozenie obiektu (œrodka geometrycznego obiektu) 
	kwaternion qOrient;       // orientacja (polozenie katowe)
	Wektor3 wV, wA;            // predkosc, przyspiesznie liniowe
	Wektor3 wV_kat, wA_kat;   // predkosc i przyspieszenie liniowe
	float alfa;               // kat skretu kol w radianach (w lewo - dodatni)
};

// Klasa opisuj¹ca obiekty ruchome
class ObiektRuchomy
{
public:
	int iID;                  // identyfikator obiektu

	StanObiektu stan;

	float F, Fb;               // si³y dzia³aj¹ce na obiekt: F - pchajaca do przodu, Fb - w prawo
	float ham;                // stopieñ hamowania Fh_max = tarcie*Fy*ham
	float Fy;                 // si³a nacisku na podstawê pojazdu - gdy obiekt styka siê z pod³o¿em (od niej zale¿y si³a hamowania)

	float m;				  // masa obiektu	
	float dlugosc, szerokosc, wysokosc; // rozmiary w kierunku lokalnych osi x,y,z
	float przeswit;           // wysokoœæ na której znajduje siê podstawa obiektu
	float dl_przod;           // odleg³oœæ od przedniej osi do przedniego zderzaka 
	float dl_tyl;             // odleg³oœæ od tylniej osi do tylniego zderzaka
	float pr_kierownicy;      // prêdkoœæ krêcenia kierownic¹
	float pr_powrotu_kier;    // prêdkoœæ powrotu kierownicy po puszczeniu

public:
	ObiektRuchomy();          // konstruktor
	~ObiektRuchomy();
	void ZmienStan(StanObiektu stan);          // zmiana stanu obiektu
	StanObiektu Stan();        // metoda zwracajaca stan obiektu
	void Symulacja(float dt);  // symulacja ruchu obiektu w oparciu o biezacy stan, przylozone sily
	// oraz czas dzialania sil. Efektem symulacji jest nowy stan obiektu 
	void Rysuj();			   // odrysowanie obiektu					
};

// Klasa opisuj¹ca teren, po którym poruszaj¹ siê obiekty
class Teren
{
public:
	float **mapa;          // wysokoœci naro¿ników oraz œrodków pól
	float ***d;            // wartoœci wyrazu wolnego z równania p³aszczyzny dla ka¿dego trójk¹ta
	Wektor3 ***Norm;       // normalne do p³aszczyzn trójk¹tów
	float rozmiar_pola;    // dlugosc boku kwadratowego pola na mapie
	long lwierszy, lkolumn; // liczba wierszy i kolumn mapy (kwadratów na wysokoœæ i szerokoœæ)     
	Teren();
	~Teren();
	float Wysokosc(float x, float z);      // okreœlanie wysokoœci dla punktu o wsp. (x,z) 
	void Rysuj();	                      // odrysowywanie terenu   
	void PoczatekGrafiki();               // tworzenie listy wyœwietlania
};
