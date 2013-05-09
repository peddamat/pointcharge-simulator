/*********************************************************************/
/* Point Charge Simulator                                            */
/* (C) Sumanth Peddamatham, 2007                                     */
/* peddamat ~at~ purdue.edu                                          */
/*                                                                   */
/* This simulator lets you investigate the field lines between point */
/* charges in a 2d plane.                                            */
/*                                                                   */
/* The program window is divided into two parts:                     */
/*   a. Simulation window (Top)                                      */
/*   b. Point charge menu (Bottom)                                   */
/*                                                                   */
/* To begin the simulation, click and drag a point charge from the   */
/* point charge menu into the simulation window.  You can arrange    */
/* and rearrange charges at any point during the simulation.         */
/*                                                                   */
/* Enable the field line display by pressing 'f'.                    */
/* Enable a field line grid by pressing 'l'.                         */
/*                                                                   */
/*********************************************************************/
#include <GLUT/glut.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <vector>
#include <iostream>
using namespace std;

#define CHAI3D 1

#ifdef CHAI3D

#include "CMesh.h"
#include "CTriangle.h"
#include "CVertex.h"
#include "CMaterial.h"
#include "CTexture2D.h"
#include "CMatrix3d.h"
#include "CVector3d.h"
#include "CPrecisionClock.h"
#include "CPrecisionTimer.h"
#include "CMeta3dofPointer.h"
#include "CShapeSphere.h"
#include "CBitmap.h"

#endif

class CPointCharge;

// CONSTANTS
#define PI 3.14159265

#define MENU_H 50
#define VIEWPORT_W 800
#define VIEWPORT_H 600
#define CHARGE_RAD 10

bool showFieldVector;
bool showFieldLines;
bool enableHaptics;
bool enableDragging;

void Dragging(int x, int y);
CPointCharge *selectedCharge;

std::vector<CPointCharge*> m_simcharges;     // Sim. point charges
std::vector<CPointCharge*> m_menucharges;    // Menu point charges
std::vector<cVector3d*> m_fieldlines;        // Field line clicks

/** CHAI3d Stuff *****************************************************/
#ifdef CHAI3D
cWorld* world;
cCamera* camera;
cLight *light;
cShapeSphere* object;
cMeta3dofPointer* cursor;

cVector3d lastCursorPos;   // Last pos of the cursor
cVector3d cursorVel;       // Velocity of the cursor
cVector3d fieldPos;

cPrecisionClock g_clock;
double timeCounter;

// Haptic timer callback
cPrecisionTimer timer;
#endif
/*********************************************************************/

//class CPointCharge : public cGenericObject {
class CPointCharge {   
public:
   
   CPointCharge(int x, int y, int charge);
   virtual ~CPointCharge();
   
   void Draw();   
   void DrawChar(int x, int y, int c);
   
   bool Clicked(float x, float y);
   float Distance(float x, float y);
   
   int m_x, m_y;   
   cVector3d pos;
   int m_charge;
   float m_radius;   
};

CPointCharge::CPointCharge(int x, int y, int charge)
{
   m_radius = CHARGE_RAD;
   m_charge = charge;
   m_x = x; m_y = y;
   pos = cVector3d(x, y, 0);
}

/*********************************************************************/
/* Paint the point charge to the viewport.                           */
/*********************************************************************/
void CPointCharge::Draw()
{
   glColor3f(1.0f, 0.0f, 0.0f);
   
   // Solid circles for positive charges
   if (m_charge >= 0)
   {
#define STEP 2*PI/20.0
      glBegin(GL_TRIANGLE_FAN);
      glVertex3f(m_x, m_y, 0);
      
      for (GLfloat angle = 0; angle<=2*PI+STEP; angle+=STEP)
         glVertex3f(m_x+m_radius*sin(angle), m_y+m_radius*cos(angle), 0);
      glEnd();
   }
   // Hollow circles for negative charges
   else 
   {
      glBegin(GL_LINE_STRIP);
      for(int j=0; j<=360; j = j + 10){
         float th=PI * j / 180.0;
         float x_rad = m_radius * cos(th);
         float y_rad = m_radius * sin(th);
         glVertex2f(m_x+x_rad, m_y+y_rad);
      }
      glEnd();      
   }
   
   // Label circle with charge magnitude
   DrawChar(m_x, m_y, m_charge);
}

/* Right now we don't do any cleanup, *sigh* */
CPointCharge::~CPointCharge()
{
}

/*********************************************************************/
/* Draw a character at the given coordinate.                         */
/*********************************************************************/
void CPointCharge::DrawChar(int x, int y, int c)
{ 
   // Positive charges
   if (c >= 0) {
      glColor3f(1.0f, 1.0f, 1.0f);
      glRasterPos3f(x-6, y-4 ,0);
      glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, '+');
      glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, c+48);
      
   }
   // Negative charges
   else {
      c = -c; // Negate sign
      glRasterPos3f(x-6, y-4 ,0);
      glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, '-');
      glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, c+48);
   }
}

/*********************************************************************/
/* Check to see if the mouse click is on menu charge.                */
/*********************************************************************/
bool CPointCharge::Clicked(float x, float y)
{
   if (Distance(x, y) <= m_radius)
      return true;
   else
      return false;
}

/*********************************************************************/
/* Get the distance from the charge to the passed point.             */
/*********************************************************************/
float CPointCharge::Distance(float x, float y)
{
   float dx = m_x - x;
   float dy = m_y - y;
   
   return sqrt(pow(dx,2)+pow(dy,2));
}

/*********************************************************************/
/* Has the user clicked on a point in the point charge window?       */
/*********************************************************************/
CPointCharge* CheckMenuClick(int x, int y)
{
   std::vector<CPointCharge*>::iterator i1;
   for (i1 = m_menucharges.begin(); i1 != m_menucharges.end(); i1++)
   {
      CPointCharge* c = (*i1);
      bool clicked = c->Clicked(x, y);
      
      if (clicked == true)
         return c;
   }
   
   return NULL;
}

/*********************************************************************/
/* Has the user clicked on a point in the simulation window?         */
/*********************************************************************/
CPointCharge* CheckSimClick(float x, float y)
{
   std::vector<CPointCharge*>::iterator i1;
   for (i1 = m_simcharges.begin(); i1 != m_simcharges.end(); i1++)
   {
      CPointCharge* c = (*i1);
      bool clicked = c->Clicked(x, y);
      
      if (clicked == true)
         return c;
   }
   
   return NULL;
}

/*********************************************************************/
/* Iterate through all the charges, and draw them to the screen.     */
/*********************************************************************/
void DrawSimCharges()
{
   std::vector<CPointCharge*>::iterator i1;
   for (i1 = m_simcharges.begin(); i1 != m_simcharges.end(); i1++)
   {
      CPointCharge* c = (*i1);
      c->Draw();
   }
}

/*********************************************************************/
/* Iterate through all the charges, and draw them to the screen      */
/*********************************************************************/
void DrawMenuCharges()
{
   std::vector<CPointCharge*>::iterator i1;
   for (i1 = m_menucharges.begin(); i1 != m_menucharges.end(); i1++)
   {
      CPointCharge* c = (*i1);
      c->Draw();
   }
}

/*********************************************************************/
/* Return the E-field vector at the given point.                     */
/*********************************************************************/
cVector3d GetForce(float x, float y, float z)
{
   cVector3d totVecForce(0.0, 0.0, 0.0);
   
   // For each of the charges in the simulation window
   std::vector<CPointCharge*>::iterator i1;
   for (i1 = m_simcharges.begin(); i1 != m_simcharges.end(); i1++)
   {
      // Determine 1/distance^2 between point charge and probe
      CPointCharge* c = (*i1);
      
      // Get the distance vector
      cVector3d r = cVector3d(x, y, 0) - c->pos;
      
      // Find inverse of r^2
      float rr = r.lengthsq();
      if (rr <= 225.0) rr = 225.0;
      float irr = 1/rr;
      float sirr = c->m_charge * irr;
      //printf("total dist: %f %f\n", r.x, r.y );
      r.normalize();
      
      // Convert scalar back into vector
      cVector3d forceVec = r * sirr;
      
      // Scale and add to force accumulator
      totVecForce += 1000 * forceVec;
   }
   
   // TODO: Attach a spring to keep the cursor in the z-plane
   totVecForce += cVector3d(0, 0, -z);
   
   /* Clamp forces before sending to haptice device */
   if (totVecForce.x > 4)  totVecForce.x = 4;
   if (totVecForce.x < -4) totVecForce.x = -4;
   if (totVecForce.y > 4)  totVecForce.y = 4;
   if (totVecForce.y < -4) totVecForce.y = -4;
   if (totVecForce.z > 10)  totVecForce.z = 10;
   if (totVecForce.z < -10) totVecForce.z = -10;
   
   return totVecForce;
}

/*********************************************************************/
/* Subdivide window into a main window and point charge menu.        */
/*********************************************************************/
void DrawMenu(void)
{
   glColor3f(0.0f, 0.0f, 0.0f);
   
   glBegin(GL_LINES);
   glVertex2i(0, MENU_H);
   glVertex2i(VIEWPORT_W, MENU_H);
   glEnd();
   
   // Place sphere charges
   DrawMenuCharges();
}

/*********************************************************************/
/* Draw an arrow from the given start to end point.                  */
/*********************************************************************/
void DrawArrow(float sx, float sy, float ex, float ey)
{   
   glColor3f(0.0f, 0.0f, 0.0f);
   
   glBegin(GL_LINES);
      glVertex2f(sx, sy);
      glVertex2f(ex, ey);
   glEnd();
   
   // Draw a point at the arrow head
   glPointSize(1.8);
      glBegin(GL_POINTS);
      glVertex2f(ex, ey);
   glEnd();
}

/*********************************************************************/
/* Iterate over the entire simulation window and draw field vectors  */
/*    at regular intervals.                                          */
/*********************************************************************/
void DrawFieldVectors()
{
#define VEC_STEP 10
   // Over the entire window
   for (int x=0; x < VIEWPORT_W-VEC_STEP; x+=VEC_STEP)
   {
      for (int y=MENU_H+10; y < VIEWPORT_H-VEC_STEP; y+=VEC_STEP)
      {
         // Get field vector at current coordinate
         cVector3d forceVec = GetForce(x, y, 0.0);
         
         // TODO: Make sure arrow doesn't overlap with interior of point charge
         if(CheckSimClick(x, y) != NULL) continue;
         if(CheckSimClick(x+10*forceVec.x, y+10*forceVec.y) != NULL) continue;
         
         // Draw arrow pointing in direction of field vector
         DrawArrow(x, y, x+10*forceVec.x, y+10*forceVec.y);
      }
   }
}

/*********************************************************************/
/* Draw field line that passes through the given x, y.               */
/*     Euler's method is used to numerically solve the IVP           */
/*********************************************************************/
void DrawFieldLine(float x, float y)
{
   std::vector<cVector3d*>::iterator i1;
   for (i1 = m_fieldlines.begin(); i1 != m_fieldlines.end(); i1++)
   {
      cVector3d* f = (*i1);
      
      // Get field vector at current coordinate
      cVector3d w0(f->x, f->y, 0.0);
      
      glColor3f(0.0f, 0.0f, 0.0f);
      
      // Place a small dot at the IVP
      glPointSize(1.8);
      glBegin(GL_POINTS);
      glVertex2f(f->x, f->y);
      glEnd();
      
      float color = 0;
      glBegin(GL_LINE_STRIP);
      
      // Start at where the user clicks
      glVertex2f(f->x, f->y);
      
      // Draw until we go off the screen
      for(int i = 0; i < 1000; i++)
      {
         cVector3d force = GetForce(w0.x, w0.y, 0.0);
         force.normalize();
         w0 += force;
         glColor3f(color+=0.001, 0, 0);
         glVertex2f(w0.x, w0.y);
         
         if ((w0.x < 0) || (w0.x > VIEWPORT_W)) break;
         if ((w0.y < MENU_H) || (w0.y > VIEWPORT_H)) break;
         if (CheckSimClick(w0.x, w0.y) != NULL) break;
      }
      glEnd();
      
      // Get field vector at current coordinate
      w0 = cVector3d(f->x, f->y, 0.0);
      
      glColor3f(0.0f, 0.0f, 0.0f);
      glBegin(GL_LINE_STRIP);
      
      // Start at where the user clicks and go the other way
      glVertex2f(f->x, f->y);
      
      // Draw until we go off the screen
      for(i = 0; i < 1000; i++)
      {
         cVector3d force = GetForce(w0.x, w0.y, 0.0);
         force.normalize();
         w0 -= force;
         //printf("w1.x: %f, w1.y: %f\n", w1.x, w1.y);
         glVertex2f(w0.x, w0.y);
         
         if ((w0.x < 0) || (w0.x > VIEWPORT_W)) break;
         if ((w0.y < MENU_H) || (w0.y > VIEWPORT_H)) break;
         if (CheckSimClick(w0.x, w0.y) != NULL) break;
      }
      glEnd();
   }
}

/*********************************************************************/
/* Draw a circle representing the haptic device.                     */
/*********************************************************************/
void DrawHapticDevice()
{
   cVector3d cursorPos = cursor->m_deviceGlobalPos;
   
   // Convert coordinates from Chai3d to GLUT
   float x = (cursorPos.x+0.5)*VIEWPORT_W;
   float y = (0.5-cursorPos.z)*VIEWPORT_H;
   
   glColor3f(0.0f, 0.0f, 0.0f);
   
   glBegin(GL_LINE_STRIP);
   for(int j=0; j<=360; j = j + 10){
      float th=PI * j / 180.0;
      float x_rad = 5 * cos(th);
      float y_rad = 5 * sin(th);
      glVertex2f(x+x_rad, y+y_rad);
   }
   glEnd();      
}

void Reshape(int w, int h)
{
   glViewport(0, 0, w, h);       
   glMatrixMode(GL_PROJECTION);  
   glLoadIdentity();             
   gluOrtho2D(0.0, VIEWPORT_W, 0.0, VIEWPORT_H);
}

/*********************************************************************/
/* This is the main GLUT rendering loop.                             */
/*********************************************************************/
void Idle(void)
{
   glClear(GL_COLOR_BUFFER_BIT);
   
   DrawMenu();
   DrawSimCharges();
   DrawHapticDevice();
   if (showFieldVector == true) DrawFieldVectors();
   if (showFieldLines == true) DrawFieldLine(fieldPos.x, fieldPos.y);
   
   // Draw field lines
   
   //	for (j=0;j<100;j++)
   //		for (i=0;i<100000;i++);
   
   //  glFlush();
   glutSwapBuffers();  
}

void Display(void)
{
   //intentionally empty - everything is displayed in the Idle() routine
   //this procedure is required anyway
}

/*********************************************************************/
/* Keyboard callback handler.                                        */
/*********************************************************************/
void Kbd(unsigned char a, int x, int y)
{
   if (a == 'v') showFieldVector = !showFieldVector;
   if (a == 'l') showFieldLines = !showFieldLines;
   if (a == 'h') enableHaptics = !enableHaptics;
   
}

/*********************************************************************/
/* Mouse callback handler.                                           */
/*********************************************************************/
void Mouse(int button, int state, int x, int y)
{
   // Fix coordinates
   y = VIEWPORT_H - y;
   
   switch(button)
   {
      // Register start of left click
      case GLUT_LEFT_BUTTON: 
      {
         if (state == GLUT_DOWN) 
         {
            CPointCharge *c = CheckMenuClick(x, y);
            
            // Check to see if user clicked on a menu point charge            
            if (c != NULL)
            {               
               // Duplicate the menu charge that the user clicked on            
               CPointCharge *d = new CPointCharge(c->m_x, c->m_y, c->m_charge);
               m_simcharges.push_back(d);
               
               selectedCharge = d;
               
               // Turn on motionfunc to allow dragging of charge
               glutMotionFunc(Dragging);
            }
            
            c = CheckSimClick(x, y);
            
            // Check to see if clicked on sim charge
            if (c != NULL)
            {
               // Set selectedCharge to the clicked on charge
               selectedCharge = c;
               
               // Enable motionfunc to allow dragging of charge
               enableDragging = true;
               glutMotionFunc(Dragging);
               
               break;
            }
            
            // If we are here, let's assume the user wanted to draw
            //     a field line through the selected point
            cVector3d *f = new cVector3d(x, y, 0.0);
            m_fieldlines.push_back(f);
         }
         
         if (state == GLUT_UP)
         {
            if (enableDragging == true)
            {
               enableDragging = false;
               
               // Are we dragging a new charge into main window?
               glutMotionFunc(NULL);
               
               // If the cursor is in the menu bar, ignore
               //               if (y >= VIEWPORT_H-MENU_H-CHARGE_RAD) return;
            }
         }
         break;
      }
   }
   
   glutPostRedisplay();   
}

/*********************************************************************/
/* General program-related initializations.                          */
/*********************************************************************/
void MyInit(void)
{
   glClearColor(1.0, 1.0, 1.0, 0.0);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluOrtho2D(0.0, VIEWPORT_W, 0.0, VIEWPORT_H);
   
   // Enable anti-aliasing
   glEnable( GL_LINE_SMOOTH );
   glEnable (GL_BLEND);
   glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glHint (GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
   glLineWidth (0.5);
   glClearColor(1.0, 1.0, 1.0, 0.0);
   
   enableDragging = false;
   showFieldVector = false;
   showFieldLines = false;
   enableHaptics = false;
}

/*********************************************************************/
/* Draw the point charge selection menu.                             */
/*********************************************************************/
void InitMenu(void)
{
   int j = -9;
   // Create a few charges
   for (int i=0; i < 19; i++)
   {
      if (j == 0) {
         j++; 
         continue;
      }
      CPointCharge *c = new CPointCharge(((i+1)*VIEWPORT_W/20), MENU_H/2, j++);
      m_menucharges.push_back(c);
   }   
}

/*********************************************************************/
/* Callback-handler to allow live 'dragging' of the point charges.   */
/*********************************************************************/
void Dragging(int x, int y)
{
   //printf("Motionfunc! X: %i Y: %i\n", x, y);
   selectedCharge->m_x = x; selectedCharge->m_y = VIEWPORT_H - y;
   selectedCharge->pos.x = x; selectedCharge->pos.y = VIEWPORT_H - y;
}

/*********************************************************************/
/* Get the device coordinates into GLUT coordinates.                 */
/*********************************************************************/
cVector3d GetDevicePos()
{
   // Get position of cursor in global coordinates
   cVector3d cursorPos = cursor->m_deviceGlobalPos;
   
   // Convert the device coordinates into GLUT coordinates
   cVector3d devPos((cursorPos.x+0.5)*VIEWPORT_W, 
                    (0.5-cursorPos.z)*VIEWPORT_H, 
                    cursorPos.y);
   
   //printf("Cursor: x: %f, y: %f, z: %f\n", (cursorPos.x+0.5)*VIEWPORT_W, 
   //                                        cursorPos.y, 
   //                                        (0.5-cursorPos.z)*VIEWPORT_H);
   return devPos;
}

/*********************************************************************/
/* Do all the haptic calculations... at the speed of touch!          */
/*********************************************************************/
void hapticsLoop(void* a_pUserData)
{
   // Quit if haptics isn't enabled
   if(enableHaptics == false) return;
   
   // Read the position of the haptic device
   cursor->updatePose();
   
   // Stop the simulation clock
   g_clock.stop();
   
   // Read the time increment in seconds
   double increment = g_clock.getCurrentTime() / 1000000.0;
   
   // Restart the simulation clock
   g_clock.initialize();
   g_clock.start();
   
   cVector3d devpos = GetDevicePos();
   //printf("Cursor: x: %f, y: %f, z: %f\n", devpos.x, devpos.y, devpos.z);
   cVector3d devforce = GetForce(devpos.x, devpos.y, devpos.z);
   /* Rotate axes */
   cVector3d rotdevforce = cVector3d(devforce.x, devforce.z, devforce.y);
   cursor->m_lastComputedGlobalForce = rotdevforce;
   
   //printf("Cursor: x: %f, y: %f, z: %f\n", rotdevforce.x, rotdevforce.y, rotdevforce.z);
   
   // compute velocity of cursor;
   //timeCounter = timeCounter + increment;
   //if (timeCounter > 0.01)
   //{
   //	cursorVel = (cursorPos - lastCursorPos) / timeCounter;
   //	lastCursorPos = cursorPos;
   //	timeCounter = 0;
   //}
   
   // Get the last force applied to the cursor in global coordinates
   //cVector3d cursorForce = cursor->m_lastComputedGlobalForce;
   
   // Send forces to haptic device
   cursor->applyForces();
}

/*********************************************************************/
/* The magic starts here! ********************************************/
/*********************************************************************/
int main(int argc, char **argv)
{
   glutInit(&argc, argv);
   glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
   
   // Initialize viewport 
   glutInitWindowSize(VIEWPORT_W, VIEWPORT_H);
   glutCreateWindow("Point Charge Simulator");
   
   // Initialize haptic stuff
   world = new cWorld();
   world->setBackgroundColor(1.0f,1.0f,1.0f);
   
   // Create a cursor and add it to the world.
   cursor = new cMeta3dofPointer(world, 0);
   world->addChild(cursor);
   cursor->setPos(0.0, 0.0, 0.0);
   cursor->setWorkspace(1.0,1.0,1.0);
   cursor->setRadius(0.01);
   cursor->initialize();
   cursor->start();
   
   // Start haptic timer callback
   timer.set(0, hapticsLoop, NULL);
   
   // Callbacks
   glutDisplayFunc(Display);
   glutReshapeFunc(Reshape);
   glutIdleFunc(Idle);
   glutMouseFunc(Mouse);   
   glutKeyboardFunc(Kbd);   
   glutMotionFunc(NULL);
   
   // General initializations
   MyInit();
   InitMenu();
   
   // GO!!!
   glutMainLoop();
   return 0;        
}
