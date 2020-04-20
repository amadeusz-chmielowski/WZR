/************************************************************
					 Grafika OpenGL
*************************************************************/
#include <windows.h>
#include <gl\gl.h>
#include <gl\glu.h>
#include <iterator> 
#include <map>

using namespace std;

#include "grafika.h"
//#include "wektor.h"
//#include "kwaternion.h"
#include "obiekty.h"

ParametryWidoku parwid;

extern ObiektRuchomy *MojObiekt;               // obiekt przypisany do tej aplikacji
extern map<int, ObiektRuchomy*> obiekty_ruchome;

extern Teren teren;

int g_GLPixelIndex = 0;
HGLRC g_hGLContext = NULL;
unsigned int font_base;


extern void TworzListyWyswietlania();		// definiujemy listy tworz¹ce labirynt
extern void RysujGlobalnyUkladWsp();


int InicjujGrafike(HDC g_context)
{

	if (SetWindowPixelFormat(g_context) == FALSE)
		return FALSE;

	if (CreateViewGLContext(g_context) == FALSE)
		return 0;
	BuildFont(g_context);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);


	TworzListyWyswietlania();		// definiujemy listy tworz¹ce ró¿ne elementy sceny
	teren.PoczatekGrafiki();

	// pocz¹tkowe ustawienia widoku:
	// Parametry widoku:
	parwid.kierunek_kamery_1 = Wektor3(10, -3, -14);   // kierunek patrzenia
	parwid.pol_kamery_1 = Wektor3(-35, 6, 10);         // po³o¿enie kamery
	parwid.pion_kamery_1 = Wektor3(0, 1, 0);           // kierunek pionu kamery        
	parwid.kierunek_kamery_2 = Wektor3(0, -1, 0.02);   // to samo dla widoku z góry
	parwid.pol_kamery_2 = Wektor3(0, 100, 0);
	parwid.pion_kamery_2 = Wektor3(0, 0, -1);
	parwid.kierunek_kamery = parwid.kierunek_kamery_1;
	parwid.pol_kamery = parwid.pol_kamery_1;
	parwid.pion_kamery = parwid.pion_kamery_1;
	parwid.sledzenie = 0;                             // tryb œledzenia obiektu przez kamerê
	parwid.widok_z_gory = 0;                          // tryb widoku z góry
	parwid.oddalenie = 18.0;                          // oddalenie widoku z kamery
	parwid.kat_kam_z = 0;                            // obrót kamery góra-dó³
	parwid.oddalenie_1 = parwid.oddalenie;
	parwid.kat_kam_z_1 = parwid.kat_kam_z;
	parwid.oddalenie_2 = parwid.oddalenie;
	parwid.kat_kam_z_2 = parwid.kat_kam_z;
	parwid.oddalenie_3 = parwid.oddalenie;
	parwid.kat_kam_z_3 = parwid.kat_kam_z;
}


void RysujScene()
{
	GLfloat OwnVehicleColor[] = { 0.4f, 0.0f, 0.8f, 0.5f };
	GLfloat AlienVehicleColor[] = { 0.2f, 0.8f, 0.25f, 0.7f };
	GLfloat GroundColor[] = { 0.5f, 0.8f, 0.1f, 1.0f };
	GLfloat SunColor[] = { 10.0f, 10.0f, 1.0f, 1.0f };

	GLfloat LightAmbient[] = { 0.1f, 0.1f, 0.1f, 0.1f };
	GLfloat LightDiffuse[] = { 0.2f, 0.7f, 0.7f, 0.7f };
	GLfloat LightPosition[] = { 5.0f, 5.0f, 5.0f, 0.0f };

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glLightfv(GL_LIGHT0, GL_AMBIENT, LightAmbient);		//1 sk³adowa: œwiat³o otaczaj¹ce (bezkierunkowe)
	glLightfv(GL_LIGHT0, GL_DIFFUSE, LightDiffuse);		//2 sk³adowa: œwiat³o rozproszone (kierunkowe)
	glLightfv(GL_LIGHT0, GL_POSITION, LightPosition);
	glEnable(GL_LIGHT0);

	glPushMatrix();

	glClearColor(0.05, 0.05, 0.3, 0.7);

	Wektor3 kierunek_k, pion_k, pol_k;
	if (parwid.sledzenie)
	{
		kierunek_k = MojObiekt->stan.qOrient.obroc_wektor(Wektor3(1, 0, 0));
		pion_k = MojObiekt->stan.qOrient.obroc_wektor(Wektor3(0, 1, 0));
		Wektor3 prawo_kamery = MojObiekt->stan.qOrient.obroc_wektor(Wektor3(0, 0, 1));

		pion_k = pion_k.obrot(parwid.kat_kam_z, prawo_kamery.x, prawo_kamery.y, prawo_kamery.z);
		kierunek_k = kierunek_k.obrot(parwid.kat_kam_z, prawo_kamery.x, prawo_kamery.y, prawo_kamery.z);
		pol_k = MojObiekt->stan.wPol - kierunek_k*MojObiekt->dlugosc * 0 +
			pion_k.znorm()*MojObiekt->wysokosc * 5;
		parwid.pion_kamery = pion_k;
		parwid.kierunek_kamery = kierunek_k;
		parwid.pol_kamery = pol_k;
	}
	else
	{
		pion_k = parwid.pion_kamery;
		kierunek_k = parwid.kierunek_kamery;
		pol_k = parwid.pol_kamery;
		Wektor3 prawo_kamery = (kierunek_k*pion_k).znorm();
		pion_k = pion_k.obrot(parwid.kat_kam_z / 20, prawo_kamery.x, prawo_kamery.y, prawo_kamery.z);
		kierunek_k = kierunek_k.obrot(parwid.kat_kam_z / 20, prawo_kamery.x, prawo_kamery.y, prawo_kamery.z);
	}

	// Ustawianie widoku sceny    
	gluLookAt(pol_k.x - parwid.oddalenie*kierunek_k.x,
		pol_k.y - parwid.oddalenie*kierunek_k.y, pol_k.z - parwid.oddalenie*kierunek_k.z,
		pol_k.x + kierunek_k.x, pol_k.y + kierunek_k.y, pol_k.z + kierunek_k.z,
		pion_k.x, pion_k.y, pion_k.z);

	//glRasterPos2f(0.30,-0.27);
	//glPrint("MojObiekt->iID = %d",MojObiekt->iID ); 

	RysujGlobalnyUkladWsp();

	for (int w = -1; w < 2; w++)
		for (int k = -1; k < 2; k++)
		{
			glPushMatrix();

			glTranslatef(teren.lkolumn*teren.rozmiar_pola*k, 0, teren.lwierszy*teren.rozmiar_pola*w);

			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, OwnVehicleColor);
			glEnable(GL_BLEND);

			MojObiekt->Rysuj();

			for (map<int, ObiektRuchomy*>::iterator it = obiekty_ruchome.begin(); it != obiekty_ruchome.end(); it++)
			{
				glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, AlienVehicleColor);
				it->second->Rysuj();
			}

			glDisable(GL_BLEND);
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, GroundColor);

			teren.Rysuj();
			glPopMatrix();
		}

	// s³oneczko:
	GLUquadricObj *Qdisk = gluNewQuadric();
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, SunColor);
	glTranslatef(MojObiekt->stan.wPol.x, MojObiekt->stan.wPol.y + 400, MojObiekt->stan.wPol.z + 6000);

	//glTranslatef(0,300,5000);
	gluDisk(Qdisk, 0, 200, 40, 40);
	gluDeleteQuadric(Qdisk);

	glPopMatrix();

	glFlush();

}

void ZmianaRozmiaruOkna(int cx, int cy)
{
	GLsizei width, height;
	GLdouble aspect;
	width = cx;
	height = cy;

	if (cy == 0)
		aspect = (GLdouble)width;
	else
		aspect = (GLdouble)width / (GLdouble)height;

	glViewport(0, 0, width, height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(35, aspect, 1, 10000.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glDrawBuffer(GL_BACK);

	glEnable(GL_LIGHTING);

	glEnable(GL_DEPTH_TEST);

}


void ZakonczenieGrafiki()
{
	if (wglGetCurrentContext() != NULL)
	{
		// dezaktualizacja kontekstu renderuj¹cego
		wglMakeCurrent(NULL, NULL);
	}
	if (g_hGLContext != NULL)
	{
		wglDeleteContext(g_hGLContext);
		g_hGLContext = NULL;
	}
	glDeleteLists(font_base, 96);
}

BOOL SetWindowPixelFormat(HDC hDC)
{
	PIXELFORMATDESCRIPTOR pixelDesc;

	pixelDesc.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pixelDesc.nVersion = 1;
	pixelDesc.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_STEREO_DONTCARE;
	pixelDesc.iPixelType = PFD_TYPE_RGBA;
	pixelDesc.cColorBits = 32;
	pixelDesc.cRedBits = 8;
	pixelDesc.cRedShift = 16;
	pixelDesc.cGreenBits = 8;
	pixelDesc.cGreenShift = 8;
	pixelDesc.cBlueBits = 8;
	pixelDesc.cBlueShift = 0;
	pixelDesc.cAlphaBits = 0;
	pixelDesc.cAlphaShift = 0;
	pixelDesc.cAccumBits = 64;
	pixelDesc.cAccumRedBits = 16;
	pixelDesc.cAccumGreenBits = 16;
	pixelDesc.cAccumBlueBits = 16;
	pixelDesc.cAccumAlphaBits = 0;
	pixelDesc.cDepthBits = 32;
	pixelDesc.cStencilBits = 8;
	pixelDesc.cAuxBuffers = 0;
	pixelDesc.iLayerType = PFD_MAIN_PLANE;
	pixelDesc.bReserved = 0;
	pixelDesc.dwLayerMask = 0;
	pixelDesc.dwVisibleMask = 0;
	pixelDesc.dwDamageMask = 0;
	g_GLPixelIndex = ChoosePixelFormat(hDC, &pixelDesc);

	if (g_GLPixelIndex == 0)
	{
		g_GLPixelIndex = 1;

		if (DescribePixelFormat(hDC, g_GLPixelIndex, sizeof(PIXELFORMATDESCRIPTOR), &pixelDesc) == 0)
		{
			return FALSE;
		}
	}

	if (SetPixelFormat(hDC, g_GLPixelIndex, &pixelDesc) == FALSE)
	{
		return FALSE;
	}

	return TRUE;
}
BOOL CreateViewGLContext(HDC hDC)
{
	g_hGLContext = wglCreateContext(hDC);

	if (g_hGLContext == NULL)
	{
		return FALSE;
	}

	if (wglMakeCurrent(hDC, g_hGLContext) == FALSE)
	{
		return FALSE;
	}

	return TRUE;
}

GLvoid BuildFont(HDC hDC)								// Build Our Bitmap Font
{
	HFONT	font;										// Windows Font ID
	HFONT	oldfont;									// Used For Good House Keeping

	font_base = glGenLists(96);								// Storage For 96 Characters

	font = CreateFont(-14,							// Height Of Font
		0,								// Width Of Font
		0,								// Angle Of Escapement
		0,								// Orientation Angle
		FW_NORMAL,						// Font Weight
		FALSE,							// Italic
		FALSE,							// Underline
		FALSE,							// Strikeout
		ANSI_CHARSET,					// Character Set Identifier
		OUT_TT_PRECIS,					// Output Precision
		CLIP_DEFAULT_PRECIS,			// Clipping Precision
		ANTIALIASED_QUALITY,			// Output Quality
		FF_DONTCARE | DEFAULT_PITCH,		// Family And Pitch
		"Courier New");					// Font Name

	oldfont = (HFONT)SelectObject(hDC, font);           // Selects The Font We Want
	wglUseFontBitmaps(hDC, 32, 96, font_base);				// Builds 96 Characters Starting At Character 32
	SelectObject(hDC, oldfont);							// Selects The Font We Want
	DeleteObject(font);									// Delete The Font
}

// Napisy w OpenGL
GLvoid glPrint(const char *fmt, ...)	// Custom GL "Print" Routine
{
	char		text[256];	// Holds Our String
	va_list		ap;		// Pointer To List Of Arguments

	if (fmt == NULL)		// If There's No Text
		return;			// Do Nothing

	va_start(ap, fmt);		// Parses The String For Variables
	vsprintf(text, fmt, ap);	// And Converts Symbols To Actual Numbers
	va_end(ap);			// Results Are Stored In Text

	glPushAttrib(GL_LIST_BIT);	// Pushes The Display List Bits
	glListBase(font_base - 32);		// Sets The Base Character to 32
	glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);	// Draws The Display List Text
	glPopAttrib();			// Pops The Display List Bits
}


void TworzListyWyswietlania()
{
	glNewList(Wall1, GL_COMPILE);	// GL_COMPILE - lista jest kompilowana, ale nie wykonywana

	glBegin(GL_QUADS);		// inne opcje: GL_POINTS, GL_LINES, GL_LINE_STRIP, GL_LINE_LOOP
	// GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, GL_QUAD_STRIP, GL_POLYGON
	glNormal3f(-1.0, 0.0, 0.0);
	glVertex3f(-1.0, -1.0, 1.0);
	glVertex3f(-1.0, 1.0, 1.0);
	glVertex3f(-1.0, 1.0, -1.0);
	glVertex3f(-1.0, -1.0, -1.0);
	glEnd();
	glEndList();

	glNewList(Wall2, GL_COMPILE);
	glBegin(GL_QUADS);
	glNormal3f(1.0, 0.0, 0.0);
	glVertex3f(1.0, -1.0, 1.0);
	glVertex3f(1.0, 1.0, 1.0);
	glVertex3f(1.0, 1.0, -1.0);
	glVertex3f(1.0, -1.0, -1.0);
	glEnd();
	glEndList();

	glNewList(Auto, GL_COMPILE);
	glBegin(GL_QUADS);
	// przod
	glNormal3f(0.0, 0.0, 1.0);

	glVertex3f(0, 0, 1);
	glVertex3f(0, 1, 1);
	glVertex3f(0.7, 1, 1);
	glVertex3f(0.7, 0, 1);

	glVertex3f(0.7, 0, 1);
	glVertex3f(0.7, 0.5, 1);
	glVertex3f(1.0, 0.5, 1);
	glVertex3f(1.0, 0, 1);
	// tyl
	glNormal3f(0.0, 0.0, -1.0);
	glVertex3f(0, 0, 0);
	glVertex3f(0.7, 0, 0);
	glVertex3f(0.7, 1, 0);
	glVertex3f(0, 1, 0);

	glVertex3f(0.7, 0, 0);
	glVertex3f(1.0, 0, 0);
	glVertex3f(1.0, 0.5, 0);
	glVertex3f(0.7, 0.5, 0);
	// gora
	glNormal3f(0.0, 1.0, 0.0);
	glVertex3f(0, 1, 0);
	glVertex3f(0, 1, 1);
	glVertex3f(0.7, 1, 1);
	glVertex3f(0.7, 1, 0);

	glVertex3f(0.7, 0.5, 0);
	glVertex3f(0.7, 0.5, 1);
	glVertex3f(1.0, 0.5, 1);
	glVertex3f(1.0, 0.5, 0);
	// dol
	glNormal3f(0.0, -1.0, 0.0);
	glVertex3f(0, 0, 0);
	glVertex3f(1, 0, 0);
	glVertex3f(1, 0, 1);
	glVertex3f(0, 0, 1);
	// prawo
	glNormal3f(1.0, 0.0, 0.0);
	glVertex3f(0.7, 0.5, 0);
	glVertex3f(0.7, 0.5, 1);
	glVertex3f(0.7, 1, 1);
	glVertex3f(0.7, 1, 0);

	glVertex3f(1.0, 0.0, 0);
	glVertex3f(1.0, 0.0, 1);
	glVertex3f(1.0, 0.5, 1);
	glVertex3f(1.0, 0.5, 0);
	// lewo
	glNormal3f(-1.0, 0.0, 0.0);
	glVertex3f(0, 0, 0);
	glVertex3f(0, 1, 0);
	glVertex3f(0, 1, 1);
	glVertex3f(0, 0, 1);

	glEnd();
	glEndList();

}


void RysujGlobalnyUkladWsp(void)
{

	glColor3f(1, 0, 0);
	glBegin(GL_LINES);
	glVertex3f(0, 0, 0);
	glVertex3f(2, 0, 0);
	glVertex3f(2, -0.25, 0.25);
	glVertex3f(2, 0.25, -0.25);
	glVertex3f(2, -0.25, -0.25);
	glVertex3f(2, 0.25, 0.25);

	glEnd();
	glColor3f(0, 1, 0);
	glBegin(GL_LINES);
	glVertex3f(0, 0, 0);
	glVertex3f(0, 2, 0);
	glVertex3f(0, 2, 0);
	glVertex3f(0.25, 2, 0);
	glVertex3f(0, 2, 0);
	glVertex3f(-0.25, 2, 0.25);
	glVertex3f(0, 2, 0);
	glVertex3f(-0.25, 2, -0.25);

	glEnd();
	glColor3f(0, 0, 1);
	glBegin(GL_LINES);
	glVertex3f(0, 0, 0);
	glVertex3f(0, 0, 2);
	glVertex3f(-0.25, -0.25, 2);
	glVertex3f(0.25, 0.25, 2);
	glVertex3f(-0.25, -0.25, 2);
	glVertex3f(0.25, -0.25, 2);
	glVertex3f(-0.25, 0.25, 2);
	glVertex3f(0.25, 0.25, 2);

	glEnd();

	glColor3f(1, 1, 1);
}

