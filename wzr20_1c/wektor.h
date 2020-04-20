

#define _WEKTOR__H

class Wektor3
{
public:      
  float x,y,z;
  Wektor3();
  Wektor3(float _x,float _y, float _z);
  Wektor3 operator* (float value);
  
  Wektor3 operator/ (float value);
  Wektor3 operator+=(Wektor3 v);    // dodanie vec+vec
  Wektor3 operator+(Wektor3 v);     // dodanie vec+vec
  Wektor3 operator+=(float stala);    // dodanie vec+liczba
  Wektor3 operator+(float stala);     // dodanie vec+liczba
  Wektor3 operator-=(Wektor3 v);    // dodanie vec+vec
  Wektor3 operator-(Wektor3 v);     // odejmowanie vec-vec
  Wektor3 operator*(Wektor3 v);     // iloczyn wektorowy
  Wektor3 operator- ();              // odejmowanie vec-vec
  bool operator==(Wektor3 v2); // porownanie
  Wektor3 obrot(float alfa,float vn0,float vn1,float vn2);
  Wektor3 znorm();
  Wektor3 znorm2D();
  float operator^(Wektor3 v);        // iloczyn scalarny
  float dlugosc(); // dlugosc aktualnego wektora    
};

Wektor3 normalna(Wektor3 A,Wektor3 B, Wektor3 C);  // normalna do trojkata ABC
float kat_pom_wekt2D(Wektor3 Wa, Wektor3 Wb);  // zwraca kat pomiedzy wektorami
bool punkt_przeciecia2D(float *x,float *y,float xA,float yA, float xB, float yB,
                        float xC,float yC, float xD, float yD);
