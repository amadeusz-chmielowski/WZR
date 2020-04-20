/*********************************************************************************
    Operacje na wektorach 3- wymiarowych + kilka przydatnych funkcji graficznych
**********************************************************************************/
#include <math.h>
#include "wektor.h"

float pii = 3.14159265358979;

Wektor3::Wektor3(float x,float y,float z)
{
	this->x=x;
	this->y=y;
	this->z=z;
}

Wektor3::Wektor3()
{
	x=0;
	y=0;
	z=0;
}

Wektor3 Wektor3::operator* (float stala)  // mnozenie przez wartosc
{
  return Wektor3(x*stala,y*stala,z*stala);
}


Wektor3 Wektor3::operator/ (float stala)			// operator* is used to scale a Vector3D by a value. This value multiplies the Vector3D's x, y and z.
{
  if (stala!=0)
    return Wektor3(x/stala,y/stala,z/stala);
  else return Wektor3(x,y,z);
}

Wektor3 Wektor3::operator+ (float stala)  // dodanie vec+vec
{
  return Wektor3(x+stala,y+stala,z+stala);
};


Wektor3 Wektor3::operator+=(float stala)  // dodanie vec+vec
{
  x += stala;
  y += stala;
  z += stala;
  return *this;
};

Wektor3 Wektor3:: operator+= (Wektor3 ww)			// operator+= is used to add another Vector3D to this Vector3D.
{
  x += ww.x;
  y += ww.y;
  z += ww.z;
  return *this;
}



Wektor3 Wektor3::operator+ (Wektor3 ww)  // dodanie vec+vec
{
  return Wektor3(x+ww.x,y+ww.y,z+ww.z);
};


Wektor3 Wektor3::operator-=(Wektor3 ww)  // dodanie vec+vec
{
  x -= ww.x;
  y -= ww.y;
  z -= ww.z;
  return *this;
};

Wektor3 Wektor3::operator- (Wektor3 ww)  // dodanie vec+vec
{
  return Wektor3(x-ww.x,y-ww.y,z-ww.z);
};

Wektor3 Wektor3::operator- ()             // znak (-)
{
  return Wektor3(-x,-y,-z);
};

Wektor3 Wektor3::operator*(Wektor3 ww)  // iloczyn wektorowy
{
   Wektor3 w;
   w.x=y*ww.z-z*ww.y;
   w.y=z*ww.x-x*ww.z;
   w.z=x*ww.y-y*ww.x;
   return w;
}

bool Wektor3::operator==(Wektor3 v2)
{
  return (x==v2.x)&&(y==v2.y)&&(z==v2.z);
}

Wektor3 Wektor3::obrot(float alfa,float vn0,float vn1,float vn2)
{
  float s = sin(alfa), c = cos(alfa);

  Wektor3 w;
  w.x = x*(vn0*vn0+c*(1-vn0*vn0)) + y*(vn0*vn1*(1-c)-s*vn2) + z*(vn0*vn2*(1-c)+s*vn1);
  w.y = x*(vn1*vn0*(1-c)+s*vn2) + y*(vn1*vn1+c*(1-vn1*vn1)) + z*(vn1*vn2*(1-c)-s*vn0);
  w.z = x*(vn2*vn0*(1-c)-s*vn1) + y*(vn2*vn1*(1-c)+s*vn0) + z*(vn2*vn2+c*(1-vn2*vn2));

  return w;
}

Wektor3 Wektor3::znorm()
{
  float d = dlugosc();      
  if (d > 0)
    return Wektor3(x/d,y/d,z/d);       
  else return Wektor3(0,0,0);
}         

Wektor3 Wektor3::znorm2D()
{
  float d = sqrt(x*x+y*y);  
  if (d > 0)      
    return Wektor3(x/d,y/d,0); 
  else return Wektor3(0,0,0);        
}   

float Wektor3::operator^(Wektor3 w) // iloczyn skalarny
{
	return w.x*x+w.y*y+w.z*z;
}

float Wektor3::dlugosc() // dlugosc aktualnego wektora
{
	return sqrt(x*x+y*y+z*z);
}

// Funkcja obliczajaca normalna do plaszczyzny wyznaczanej przez trojkat ABC
// zgodnie z ruchem wskazowek zegara:
Wektor3 normalna(Wektor3 A,Wektor3 B, Wektor3 C)
{
  Wektor3 w1 = B-A, w2 = C-A;
  return (w1*w2).znorm();
}



// zwraca kat pomiedzy wektorami w zakresie <0,2pi) w kierunku przeciwnym
// do ruchu wskazowek zegara. Zakladam, ze Wa.z = Wb.z = 0
float kat_pom_wekt2D(Wektor3 Wa, Wektor3 Wb)  
{
    
  Wektor3 ilo = Wa.znorm2D() * Wb.znorm2D();  // iloczyn wektorowy wektorow o jednostkowej dlugosci
  float sin_kata = ilo.z;        // problem w tym, ze sin(alfa) == sin(pi-alfa)   
  if (sin_kata == 0)
  {
    if (Wa.znorm2D() == Wb.znorm2D()) return 0; 
    if (Wa.znorm2D() == -Wb.znorm2D()) return pii; 
  }  
  // dlatego trzeba  jeszcze sprawdzic czy to kat rozwarty
  Wektor3 wso_n = Wa.znorm2D() + Wb.znorm2D();          
  Wektor3 ilo1 = wso_n.znorm2D() * Wb.znorm2D();
  bool kat_rozwarty = (ilo1.dlugosc() > sqrt(2.0)/2);
  
  float kat = asin(fabs(sin_kata));
  if (kat_rozwarty) kat = pii-kat;
  if (sin_kata < 0) kat = 2*pii - kat;
  
  return kat;
}      

/*
   wyznaczanie punktu przeciecia sie 2 odcinkow AB i CD lub ich przedluzen
   zwraca 1 jesli odcinki sie przecinaja 
*/
bool punkt_przeciecia2D(float *x,float *y,float xA,float yA, float xB, float yB,
                        float xC,float yC, float xD, float yD)
{
  float a1,b1,c1,a2,b2,c2;  // rownanie prostej: ax+by+c=0 
  a1 = (yB-yA); b1 = -(xB-xA); c1 = (xB-xA)*yA - (yB-yA)*xA;               
  a2 = (yD-yC); b2 = -(xD-xC); c2 = (xD-xC)*yC - (yD-yC)*xC;
  float mian = a1*b2-a2*b1;
  if (mian == 0) {*x=0;*y=0;return 0;}  // odcinki rownolegle
  float xx = (c2*b1 - c1*b2)/mian,
        yy = (c1*a2 - c2*a1)/mian;
  *x = xx; *y = yy;
  if (  (((xx > xA)&&(xx < xB))||((xx < xA)&&(xx > xB))||
         ((yy > yA)&&(yy < yB))||((yy < yA)&&(yy > yB))) &&
        (((xx > xC)&&(xx < xD))||((xx < xC)&&(xx > xD))||
         ((yy > yC)&&(yy < yD))||((yy < yC)&&(yy > yD)))
     )    
     return 1;
  else
     return 0;      
}                        
