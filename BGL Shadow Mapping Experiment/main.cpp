/*
 * Copyright (c) 1993-2003, Silicon Graphics, Inc.
 * All Rights Reserved
 * VEREINFACHTE shadowmap.c
 */
#include "Windows.h"
#include "GLee.h"
#include <stdio.h>
#include <cmath>
#include <GL/glut.h>
#include <GL/glext.h>
#include "ShadowMap.h"

#define SHADOW_MAP_WIDTH      512
#define SHADOW_MAP_HEIGHT     512

#define PI 3.14159265359

GLdouble fovy      = 60.0;
GLdouble nearPlane = 2;
GLdouble farPlane  = 100.0;

GLfloat angle = 0.0;
GLfloat torusAngle = 0.0;

GLfloat lightPos[] = {25.0,25.0,25.0,0.0 }; // <=== Wichtig: w = 0,weil direktioanles Licht!
GLfloat lookat[] = { 0, 0.0, 0.0 };
GLfloat up[] = { 0.0, 0.0, 1.0 };

static GLfloat radius = 30;
static GLdouble projection_light[16];
static GLdouble projection_camera[16];
static GLdouble modelview_light[16];
static GLdouble modelview_camera[16];
static GLdouble texture[16];
static GLboolean showShadow = GL_FALSE;
static GLboolean showProjectiveShadow = GL_FALSE;
static GLboolean textureOn = GL_TRUE;

void drawObjects(GLboolean );
void transposeMatrix(double m[16]);
void transposeMatrix(float m[16]);

void reshape(int width,int height);
void idle(void);
void keyboard(unsigned char key,int x,int y);
void generateTextureMatrix(void);

/*
=================================================
=                                               =
=     Erstellt Projective Shadow Matrix         =
=                                               =
=================================================*/
static void myShadowMatrix(double ground[4], float light[4])
{
    double  dot;
    double  shadowMat[4][4];

    dot = ground[0] * light[0] +
          ground[1] * light[1] +
          ground[2] * light[2] +
          ground[3] * light[3];

    shadowMat[0][0] = dot - light[0] * ground[0];
    shadowMat[1][0] = 0.0 - light[0] * ground[1];
    shadowMat[2][0] = 0.0 - light[0] * ground[2];
    shadowMat[3][0] = 0.0 - light[0] * ground[3];

    shadowMat[0][1] = 0.0 - light[1] * ground[0];
    shadowMat[1][1] = dot - light[1] * ground[1];
    shadowMat[2][1] = 0.0 - light[1] * ground[2];
    shadowMat[3][1] = 0.0 - light[1] * ground[3];

    shadowMat[0][2] = 0.0 - light[2] * ground[0];
    shadowMat[1][2] = 0.0 - light[2] * ground[1];
    shadowMat[2][2] = dot - light[2] * ground[2];
    shadowMat[3][2] = 0.0 - light[2] * ground[3];

    shadowMat[0][3] = 0.0 - light[3] * ground[0];
    shadowMat[1][3] = 0.0 - light[3] * ground[1];
    shadowMat[2][3] = 0.0 - light[3] * ground[2];
    shadowMat[3][3] = dot - light[3] * ground[3];

    glMultMatrixd((const GLdouble*)shadowMat);
}

/*
=================================================
=                                               =
=           Berechnet alle Matritzen            =
=                                               =
=================================================*/
void CalculateMatrices(void)
{
    #ifdef __DEBUG__
        glPushAttrib(GL_TRANSFORM_BIT);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
    #endif

    /*
    ======================================
    = Kamera Matrix
    ======================================*/
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(fovy,1.0,nearPlane,100);
    glGetDoublev(GL_PROJECTION_MATRIX,projection_camera);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(radius*cos(angle), radius*sin(angle), 30,
			  lookat[0], lookat[1],lookat[2],
			  up[0],up[1],up[2]);


    glGetDoublev(GL_MODELVIEW_MATRIX,modelview_camera);

    /*
    ======================================
    = Lichtmatrix
    ======================================*/
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    #ifdef __ALIASING_DEMO__
    glOrtho(-105,50,-5,5,60,-60); // Aliasing, weil relevanter Teil auf Textur zu klein ist
    #else
    glOrtho(-15,5.5,-5,5,60,-60); // Maximaler Ausschnitt (Bounding Box)
    //gluPerspective(60,1,10,60); <= Würde auch gehen glOrtho ist baer einfacher
    #endif

    glGetDoublev(GL_PROJECTION_MATRIX,projection_light);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    gluLookAt(0,0,0, // Bei Ortho wurde Near Plane angepasst!
              lightPos[0],lightPos[1],lightPos[2], // Directional Light!
              up[0],up[1],up[2]);

    glGetDoublev(GL_MODELVIEW_MATRIX,modelview_light);

    /*
    ======================================
    = Texturmatrix
    ======================================*/
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    // T = B * M_l * P_l
    glTranslatef(0.5,0.5,0.5); glScalef(0.5,0.5,0.5); // Bias-Matrix
    glMultMatrixd(projection_light);
    glMultMatrixd(modelview_light);

    glGetDoublev(GL_MODELVIEW_MATRIX,texture);

    #ifdef __DEBUG__
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glPopAttrib();
    #endif
}

/*
=================================================
=                                               =
=                 Initialisierung               =
=                                               =
=================================================*/
void init( void )
{
    GLfloat  white[] = {1.0,1.0,1.0,1.0};

	/*
	=================================================
	Erstellt Tiefentextur
	=================================================*/
    glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT,
  		         SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT,0,
		         GL_DEPTH_COMPONENT, GL_DOUBLE,NULL);

	// Standardparameter
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE );

	// Einstellungen für Tiefenvergleich
    glTexParameteri( GL_TEXTURE_2D,GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL );
    glTexParameteri( GL_TEXTURE_2D,GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE );
    glTexParameteri( GL_TEXTURE_2D,GL_TEXTURE_COMPARE_MODE,GL_COMPARE_R_TO_TEXTURE );

	// Einstellungen für Projective Texture Mapping
	glTexGeni(GL_S,GL_TEXTURE_GEN_MODE,GL_OBJECT_LINEAR);
    glTexGeni(GL_T,GL_TEXTURE_GEN_MODE,GL_OBJECT_LINEAR);
    glTexGeni(GL_R,GL_TEXTURE_GEN_MODE,GL_OBJECT_LINEAR);
    glTexGeni(GL_Q,GL_TEXTURE_GEN_MODE,GL_OBJECT_LINEAR);

	/*
	=================================================
	Initialisiert Licht
	=================================================*/
    glLightfv(GL_LIGHT0,GL_POSITION,lightPos);
    glLightfv(GL_LIGHT0,GL_SPECULAR,white);
    glLightfv(GL_LIGHT0,GL_DIFFUSE,white);
    glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);

    glCullFace(GL_BACK);

	/*
	=================================================
	Aktiviert Einstellungen
	=================================================*/
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHT0 );
    glEnable(GL_LIGHTING );
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glEnable(GL_TEXTURE_GEN_R);
    glEnable(GL_TEXTURE_GEN_Q);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_CULL_FACE);
    glClearColor(0.2,0.2,0.2,1.0);

	/*
	=================================================
	Berechnet Matrizen
	=================================================*/
	CalculateMatrices();

	/*
	=================================================
	Projective Texturemapping
	=================================================*/
    generateTextureMatrix();
}


/*
=================================================
=                                               =
=              Shadowmap erstellen              =
=                                               =
=================================================*/
void generateShadowMap(void)
{
	// Viewportmaße auf Shadowmap-Maße setzen
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT,viewport );
    glViewport(0,0,SHADOW_MAP_WIDTH,SHADOW_MAP_HEIGHT );

	/*
	=================================================
	= Aus Lichtsperspektive rendern
	=================================================*/
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixd(projection_light);

    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixd(modelview_light);

	// Szene rendern
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    drawObjects(GL_TRUE);

	// Tiefenwerte speichern
    glCopyTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT,0,0,
		              SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT,0);

	// Vorherige Viewportmaße wiederherstellen
    glViewport(viewport[0],viewport[1],viewport[2],viewport[3] );

	/*
	=================================================
	= Shadowmap anzeigen (falls gewünscht)
	=================================================*/
    if(showShadow){
      GLfloat depthImage[SHADOW_MAP_WIDTH][SHADOW_MAP_HEIGHT];
      glReadPixels(0,0,SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT,
				   GL_DEPTH_COMPONENT,GL_FLOAT,depthImage);
      glWindowPos2f(0,0);
      glDrawPixels( SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT, GL_LUMINANCE,GL_FLOAT, depthImage );
      glutSwapBuffers();
    }
}

/*
=================================================
=                                               =
=           Erstellt Texturmatrix für           =
=           Projective Texture Mapping          =
=                                               =
=================================================*/
void generateTextureMatrix(void)
{
    GLfloat  tmpMatrix[16];

    glMatrixMode(GL_MODELVIEW);
	glLoadMatrixd(texture);

	// Matrix abspeichern
    glGetFloatv(GL_MODELVIEW_MATRIX,tmpMatrix);

	// Matrix transponieren
    transposeMatrix(tmpMatrix); // Inverse!


	// Ebenen aus Matrix extrahieren
    glTexGenfv(GL_S,GL_OBJECT_PLANE,&tmpMatrix[0] );
    glTexGenfv(GL_T,GL_OBJECT_PLANE,&tmpMatrix[4] );
    glTexGenfv(GL_R,GL_OBJECT_PLANE,&tmpMatrix[8] );
    glTexGenfv(GL_Q,GL_OBJECT_PLANE,&tmpMatrix[12] );
}

/*
=================================================
=                                               =
=              Haupt Renderfunktion             =
=                                               =
=================================================*/
void display(void)
{
  	/*
	=================================================
	Alles Initialisieren
	=================================================*/
    init();

	/*
	=================================================
	Shadowmap und Matrix berechnen (=> 1. Renderpass)
	=================================================*/
    generateShadowMap();
    if(showShadow) return;
    if(textureOn)
          glEnable(GL_TEXTURE_2D);
    else glDisable(GL_TEXTURE_2D);

	/*
	=================================================
	Eingetliche Szene rendern (=> 2. Renderpass)
	=================================================*/
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixd(projection_camera);

    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixd(modelview_camera);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    drawObjects(GL_FALSE);

    glutSwapBuffers();
}

/*
=================================================
=                                               =
=              Objekte zeichnen                 =
=                                               =
=================================================*/
void drawObjects(GLboolean shadowRender)
{
    GLboolean textureOn = glIsEnabled( GL_TEXTURE_2D );

    if(shadowRender)
       glDisable(GL_TEXTURE_2D); // Texturing Aus bei Shadowmap Erzeugung


	/*
	=================================================
	Zeichnet Schatten mit Projective Shadows
	=================================================*/
   if(!shadowRender && showProjectiveShadow)
   {
       glPushAttrib(GL_ALL_ATTRIB_BITS);
        glPushMatrix();
        glDisable(GL_LIGHTING);

        double plane[4] = {0,0,1,0};
        myShadowMatrix(plane,lightPos);

        glColor3f(1,1,0);
        glTranslated(0,0,-0.0);

        glPushMatrix();
        glTranslatef( 11, 11, 11 );
        glRotatef( 54.73, -5, 5, 0 );
        glRotatef( torusAngle, 1, 0, 0 );
        glutSolidTorus( 1, 4, 8, 36 );
        glPopMatrix();

        glPushMatrix();
        glTranslatef( -8, 9, 2 );
        glutSolidCube( 4 );
        glPopMatrix();

        glPushMatrix();
        glTranslatef( lightPos[0]/ 2, lightPos[1] / 2, lightPos[2]/ 2 );
        glColor3f( 1, 1, 1 );
        glutWireSphere( 0.5, 6, 6 );
        glPopMatrix();

        glPopMatrix();
        glPopAttrib();

   }

    if(!shadowRender){
        glNormal3f(0,0,1);
        glColor3f(1,1,1);
        glRectf( -20.0, -20.0, 20.0, 20.0 );

        // Zeichnet Lichstrahl
        glPushAttrib(GL_ALL_ATTRIB_BITS);
            glLineWidth(5);
            glBegin(GL_LINES);
                glColor3d(0.0,1.0,0.0);
                glVertex3f(lightPos[0],lightPos[1],lightPos[2]);
                glVertex3f(0,0,0);
            glEnd();
        glPopAttrib();
    }

    // Zeichnet Lichquelle als Kugel
    glPushMatrix();
    glTranslatef( lightPos[0]/ 2, lightPos[1]/ 2, lightPos[2] / 2);
    glColor3f( 1, 1, 1 );
    glutWireSphere( 0.5, 6, 6 );
    glPopMatrix();

    glPushMatrix();
    glTranslatef( 11, 11, 11 );
    glRotatef( 54.73, -5, 5, 0 );
    glRotatef( torusAngle, 1, 0, 0 );
    glColor3f( 1, 0, 0 );
    glutSolidTorus( 1, 4, 8, 36 );
    glPopMatrix();

    glPushMatrix();
    glTranslatef( -8, 9, 2 );
    glColor3f( 0, 0, 1 );
    glutSolidCube( 4 );
    glPopMatrix();

    if (shadowRender && textureOn)
        glEnable(GL_TEXTURE_2D);
}

/*
=================================================
=                                               =
=                    Main                       =
=                                               =
=================================================*/
int main( int argc, char** argv )
{
    glutInit(&argc,argv);
    glutInitDisplayMode( GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE );
    glutInitWindowSize(512,512);
    glutInitWindowPosition(100,100);
    glutCreateWindow("BGL Shadow Mapping Demo");
    glutDisplayFunc(display );
    glutReshapeFunc(reshape );
    glutKeyboardFunc(keyboard );
    glutIdleFunc(idle);
    glutMainLoop();
    glDepthFunc(GL_LEQUAL);
    return 0;
}

/*
=================================================
=                                               =
=               GLUT Funktionen                 =
=                                               =
=================================================*/
void reshape( int width, int height )
{
    glViewport(0,0,width, height );

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    gluPerspective(fovy, (GLdouble) width/height,nearPlane,farPlane);
    glMatrixMode( GL_MODELVIEW );
}

void idle( void )
{
    angle += PI / 10000;
    torusAngle += .1;
    glutPostRedisplay();
}

void keyboard( unsigned char key, int x, int y )
{
    switch( key ) {
    case 27:  /* Escape */
      exit( 0 );
      break;

    case 't': {
        textureOn = !textureOn;
        if ( textureOn )
	glEnable( GL_TEXTURE_2D );
        else
	glDisable( GL_TEXTURE_2D );
      }
      break;

    case 'm': {
        static GLboolean compareMode = GL_TRUE;
        compareMode = !compareMode;
        printf( "Compare mode %s\n", compareMode ? "On" : "Off" );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE,
		         compareMode ? GL_COMPARE_R_TO_TEXTURE : GL_NONE );
      }
      break;

    case 'f': {
        static GLboolean funcMode = GL_TRUE;
        funcMode = !funcMode;
        printf( "Operator %s\n", funcMode ? "GL_LEQUAL" : "GL_GEQUAL" );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC,
		         funcMode ? GL_LEQUAL : GL_GEQUAL );
      }
      break;

    case 's':
      showShadow = !showShadow;
      break;

    case 'p': {
        static GLboolean  animate = GL_TRUE;
        animate = !animate;
        glutIdleFunc( animate ? idle : NULL );
      }
      break;
    case 'q':
        showProjectiveShadow = (showProjectiveShadow) ? GL_FALSE : GL_TRUE;
        break;
    }

    glutPostRedisplay();
}

void transposeMatrix( double m[16] )
{
    double  tmp;
#define Swap( a, b )    tmp = a; a = b; b = tmp
    Swap( m[1],  m[4]  );
    Swap( m[2],  m[8]  );
    Swap( m[3],  m[12] );
    Swap( m[6],  m[9]  );
    Swap( m[7],  m[13] );
    Swap( m[11], m[14] );
#undef Swap
}

void transposeMatrix( float m[16] )
{
    float  tmp;
#define Swap( a, b )    tmp = a; a = b; b = tmp
    Swap( m[1],  m[4]  );
    Swap( m[2],  m[8]  );
    Swap( m[3],  m[12] );
    Swap( m[6],  m[9]  );
    Swap( m[7],  m[13] );
    Swap( m[11], m[14] );
#undef Swap
}
/*
============================================
BEi Fehlern mit near und far Plane spielen
=============================================*/
