// assign2.cpp : Defines the entry point for the console application.
//

/*
	CSCI 480 Computer Graphics
	Assignment 2: Simulating a Roller Coaster
	C++ starter code
*/

#include "stdafx.h"
#include <pic.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <GL/glu.h>
#include <GL/glut.h>
#include <iostream>
#include <string>
#include <sstream> 
#include <vector>

using namespace std;

/* represents one control point along the spline */
struct point {
	double x;
	double y;
	double z;
};

/* spline struct which contains how many control points, and an array of control points */
struct spline {
	int numControlPoints;
	struct point *points;
};

/* the spline array */
struct spline *g_Splines;

/* total number of splines */
int g_iNumOfSplines;

bool save_image = false; 
int num_of_image = 0;

float s = 0.5; // Catmull-Rom matrix parimeter
float MAX_LINE_LEN = 0.075; 

point p0; 
point p1; 
point p2; 
point p3; 

Pic* ground_image; 
GLuint texture_id; 
Pic* sky_image; 
Pic* sky_top;

float box_left = -0.5;
float box_right = 0.5;
float box_up = -0.5;
float box_down = 0.5;

float square_max = 0; 
float max_len = 0; 
float lowest = 0; // The lowest value of any spline coordinate, in terms of y

float FLOOR_SUB = 0; // How far the ground plane will be from the floor relative to the lowest point of the splines
float MAX_DIST_MULT = 4.75; // How far the skybox will extend out from the farthest point in the spline(s)

GLuint display_list; 

int control_num = 1; 
int spline_num = 0;
float u_step = 0; // from 0 to 1
float STEP = 0.0175;

int WINDOW_WIDTH = 640;  
int WINDOW_HEIGHT = 480; 

point tangent_prev;
point norm_prev;
point biNorm_prev;
point tangent_current;
point norm_current;
point biNorm_current;

point arbitrary_vector;

point camera_current;
point camera_prev;

GLUquadricObj *quadratic;                    
#define PI 3.14159265

bool light_on = true;

/* Write a screenshot to the specified filename */ 
void saveScreenshot (char *filename)
{
  int i;//, j;
  Pic *in = NULL;

  if (filename == NULL)
    return;

  /* Allocate a picture buffer */
  in = pic_alloc(640, 480, 3, NULL);

  printf("File to save to: %s\n", filename);

  for (i=479; i>=0; i--) {
    glReadPixels(0, 479-i, 640, 1, GL_RGB, GL_UNSIGNED_BYTE,
                 &in->pix[i*in->nx*in->bpp]);
  }

  if (jpeg_write(filename, in))
    printf("File saved Successfully\n");
  else
    printf("Error in Saving\n");

  pic_free(in);
}

point normalization(point p) 
{
	float len = p.x*p.x + p.y*p.y + p.z*p.z;
	len = sqrt(len+0.0);
	if (len <= 0)
		return p;

	p.x /= len;
	p.y /= len;
	p.z /= len;

	return p;
}

point compute_pos(float u) 
{
	point result;

	float u0 = 1;
	float u1 = u;
	float u2 = u1*u1;
	float u3 = u2*u1;

	float temp1, temp2, temp3, temp4;
	temp1 = u3*(-s) + u2*2*s + u1*(-s);
	temp2 = u3*(2-s) + u2*(s-3) + u0*1;
	temp3 = u3*(s-2) + u2*(3-2*s) + u1*s;
	temp4 = u3*s + u2*(-s);

	result.x = temp1*p0.x + temp2*p1.x + temp3*p2.x + temp4*p3.x;
	result.y = temp1*p0.y + temp2*p1.y + temp3*p2.y + temp4*p3.y;
	result.z = temp1*p0.z + temp2*p1.z + temp3*p2.z + temp4*p3.z;

	return result;
}

point compute_tangent(float u) 
{
	point result;

	float u0 = 0;
	float u1 = 1;
	float u2 = 2*u;
	float u3 = u*u*3;

	float temp1, temp2, temp3, temp4;
	temp1 = u3*(-s) + u2*2*s + u1*(-s);
	temp2 = u3*(2-s) + u2*(s-3) + u0*1;
	temp3 = u3*(s-2) + u2*(3-2*s) + u1*s;
	temp4 = u3*s + u2*(-s);

	result.x = temp1*p0.x + temp2*p1.x + temp3*p2.x + temp4*p3.x;
	result.y = temp1*p0.y + temp2*p1.y + temp3*p2.y + temp4*p3.y;
	result.z = temp1*p0.z + temp2*p1.z + temp3*p2.z + temp4*p3.z;

	return result;
}

point cross_production(point a, point b) 
{
	point p;
	p.x = a.y*b.z - a.z*b.y;
	p.y = -1 * (a.x*b.z - a.z*b.x);
	p.z = a.x*b.y - a.y*b.x;

	return p;
}

void initialize_vector() {
	point temp;
	temp.x = 0;
	temp.y = 1;
	temp.z = 0;
	while (true) 
	{ 
		// Sloan method
		// tangent
		tangent_prev = normalization(compute_tangent(u_step)); // Initialize this value to the first tangent	
		tangent_current = tangent_prev;
		// normal
		// N0 = unit(T0 x V) and B0 = unit(T0 x N0)
		norm_prev = normalization(cross_production(normalization(tangent_prev), normalization(temp))); // Get the unit vector for T0 X (1,1,1)
		norm_current = norm_prev;
		biNorm_prev = normalization(cross_production(normalization(tangent_prev), normalization(norm_prev))); // Get the unit vector for T0 X N0
		biNorm_current = biNorm_prev;

		if ((norm_current.x == 0 && norm_current.y == 0 && norm_current.z == 0)
			|| (biNorm_current.x == 0 && biNorm_current.y == 0 && biNorm_current.z == 0)) 
		{ 
			temp.x = 0;
			temp.y = 0;
			temp.z = 1;
			continue;
		}
		else { 
			break;
		}
	}

	arbitrary_vector = temp;
}

void set_camera() {
	p0 = g_Splines[spline_num].points[control_num-1]; // Pi-1
	p1 = g_Splines[spline_num].points[control_num]; // Pi
	p2 = g_Splines[spline_num].points[control_num+1]; // Pi+1
	p3 = g_Splines[spline_num].points[control_num+2]; // Pi+2
	// initialization
	if (spline_num == 0 && control_num == 1 && u_step == 0.000) 
	{ 
		initialize_vector();
	}
	else 
	{	
		// Sloan's method decides each coordinate system using a function of the previous one, to ensure continuity.
		tangent_current = normalization(compute_tangent(u_step)); 
		// N1 = unit(B0 x T1) and B1 = unit(T1 x N1)
		norm_current = normalization(cross_production(normalization(biNorm_prev), normalization(tangent_current)));
		biNorm_current = normalization(cross_production(normalization(tangent_current), normalization(norm_current)));
	}

	// get camera position
	camera_current = compute_pos(u_step);
	camera_prev = tangent_current; 

	camera_prev.x *= 500;
	camera_prev.y *= 500;
	camera_prev.z *= 500;

	u_step += STEP;

	// pre is current
	tangent_prev = tangent_current;
	norm_prev = norm_current;
	biNorm_prev = biNorm_current;

	if (u_step >= 1) 
	{ 
		u_step = 0.000;
		control_num++;
		// dont compute p0 and pn-1. 
		if (control_num == g_Splines[spline_num].numControlPoints - 2) 
		{ 
			spline_num++;
			if (spline_num == g_iNumOfSplines) 
			{ 
				spline_num = 0;
			}			
		}
	}
}

void boundary()
{
	for (int i = 0; i < g_iNumOfSplines; i++)
	{
		for (int j = 0; j < g_Splines[i].numControlPoints; j++) 
		{
			float tempDistSquared = pow(g_Splines[i].points[j].x, 2) + pow(g_Splines[i].points[j].y, 2) + pow(g_Splines[i].points[j].z, 2); 
			if (tempDistSquared > square_max) {
				// find the furthest point
				square_max = tempDistSquared;
			}
			if (g_Splines[i].points[j].y < lowest || (i == 0 && j == 0)) 
			{
				// the lowest point in y axis
				lowest = g_Splines[i].points[j].y;
			}
		}	
	}
	max_len = sqrt(square_max);
	FLOOR_SUB = -max_len/8; 
}

void draw_cylinder(float radius, float height, int slice, float x0, float y0, float z0, float x1, float y1, float z1)
{
	// draw cylinder at position (x, y, z)
	glPushMatrix();
	glTranslatef(x0, y0, z0);
	/*
	// rotate the cylinder. default it is along z axis
	float x_angle, y_angle; // dont need to rotate along z axis
	// float unit_z_x = 0, unit_z_y = 0, unit_z_z = 1;
	float use_x = x1 - x0, use_y = y1 - y0, use_z = z1 - z0;
	float arc_cos_x, arc_cos_y;

	float dot_prod = use_z;
	float len = sqrt(use_x*use_x + use_y*use_y + use_z*use_z);
	float temp_dot1 = use_x*use_x + use_z*use_z;
	temp_dot1 = temp_dot1 / len / (sqrt(use_x*use_x + use_z*use_z));
	float angle_xz = acos(180/PI*temp_dot1);
	if (use_y>0)
		arc_cos_x = -1 * angle_xz;
	else
		arc_cos_x = angle_xz;

	float temp2 = use_z / sqrt(use_x*use_x + use_z*use_z);
	float angle_z = acos(180/PI*temp2);
	if (use_x>0)
		arc_cos_y = -1 * angle_z;
	else
		arc_cos_y = angle_z;

	glRotatef(arc_cos_x, 1, 0, 0);
	glRotatef(arc_cos_y, 0, 1, 0);*/

	gluCylinder(quadratic,radius,radius,height,slice,slice);

	glPopMatrix();

	glMatrixMode(GL_MODELVIEW); 
}

// recursion
void subdivide(float u0, float u1, float max_length_square) 
{
	float uMidPoint = (u0 + u1) / (float)2; 
	point c0 = compute_pos(u0); 
	point c1 = compute_pos(u1); 

	point distance;
	distance.x = c1.x - c0.x;
	distance.y = c1.y - c0.y;
	distance.z = c1.z - c0.z;

	float temp = pow(distance.x, 2) + pow(distance.y, 2) + pow(distance.z, 2);

	if (temp > max_length_square) 
	{	// if step is too big, then do recursion
		subdivide(u0, uMidPoint, max_length_square);
		subdivide(uMidPoint, u1, max_length_square);
	}	
	else // draw line
	{
		glBegin(GL_LINES);
		glLineWidth(5);
		glColor3f(1.0, 1.0, 1.0);
		glVertex3f(c0.x, c0.y, c0.z);		
		glVertex3f(c1.x, c1.y, c1.z);
		glLineWidth(2.5);
		glColor3f(0.0, 1, 0.5);
		glVertex3f(c0.x, c0.y, c0.z-0.5);
		glVertex3f(c0.x, c0.y, c0.z+0.5);
		glVertex3f(c1.x, c1.y, c1.z-0.5);						
		glVertex3f(c1.x, c1.y, c1.z+0.5);
		glEnd();
		glColor3f(1,1,0);
		float dx = c1.x - c0.x;
		float dy = c1.y - c0.y;
		float dz = c1.z - c0.z; 
		float cylinder_height = sqrt(dx*dx + dy*dy + dz*dz);

		// position: (c0.x, c0.y, c0.z)
		draw_cylinder(0.1, cylinder_height, 12, c0.x, c0.y, c0.z-0.5, c1.x, c1.y, c1.z-0.5);
		draw_cylinder(0.1, cylinder_height, 12, c0.x, c0.y, c0.z+0.5, c1.x, c1.y, c1.z+0.5);
	}
}

void draw_splines() {
	for (int i = 0; i < g_iNumOfSplines; i++) 
	{
		glLineWidth(2.5);
				
		for (int j = 1; j <= g_Splines[i].numControlPoints - 3; j++) 
		{			
			p0 = g_Splines[i].points[j-1]; // Pi-1
			p1 = g_Splines[i].points[j]; // Pi
			p2 = g_Splines[i].points[j+1]; // Pi+1
			p3 = g_Splines[i].points[j+2]; // Pi+2
			
			subdivide(0, 1, MAX_LINE_LEN*MAX_LINE_LEN);
		}
		
	}
}

void draw_ground_sky(float ground, float offset) {
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE); 
	glEnable(GL_TEXTURE_2D);
	float half_ground = ground/2;

	// load ground texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ground_image->nx,  ground_image->ny, 0, GL_RGB, GL_UNSIGNED_BYTE, ground_image->pix);

	// ground quad
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0); glVertex3f(box_left  * ground, ground + offset, box_up   * ground);
	glTexCoord2f(1.0, 0.0); glVertex3f(box_right * ground, ground + offset, box_up   * ground);
	glTexCoord2f(1.0, 1.0); glVertex3f(box_right * ground, ground + offset, box_down * ground);
	glTexCoord2f(0.0, 1.0); glVertex3f(box_left  * ground, ground + offset, box_down * ground);
	glEnd();
	
	// sky top texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, sky_top->nx, sky_top->ny, 0, GL_RGB, GL_UNSIGNED_BYTE, sky_top->pix);

	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0); glVertex3f(box_left  * ground, offset, box_up   * ground);
	glTexCoord2f(1.0, 0.0); glVertex3f(box_right * ground, offset, box_up   * ground);
	glTexCoord2f(1.0, 1.0); glVertex3f(box_right * ground, offset, box_down * ground);
	glTexCoord2f(0.0, 1.0); glVertex3f(box_left  * ground, offset, box_down * ground);
	glEnd();

	// load sky texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, sky_image->nx,  sky_image->ny, 0, GL_RGB, GL_UNSIGNED_BYTE, sky_image->pix); 

	// sky quad
	
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0); glVertex3f(-(half_ground), (box_down * ground) + (half_ground) + offset, box_left  * ground);
	glTexCoord2f(1.0, 0.0); glVertex3f(-(half_ground), (box_down * ground) + (half_ground) + offset, box_right * ground);
	glTexCoord2f(1.0, 1.0); glVertex3f(-(half_ground), (box_up   * ground) + (half_ground) + offset, box_right * ground);
	glTexCoord2f(0.0, 1.0); glVertex3f(-(half_ground), (box_up   * ground) + (half_ground) + offset, box_left  * ground);
	glEnd();

	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0); glVertex3f(box_left  * ground, (box_down * ground) + (half_ground) + offset, (half_ground));
	glTexCoord2f(1.0, 0.0); glVertex3f(box_right * ground, (box_down * ground) + (half_ground) + offset, (half_ground));
	glTexCoord2f(1.0, 1.0); glVertex3f(box_right * ground, (box_up   * ground) + (half_ground) + offset, (half_ground));
	glTexCoord2f(0.0, 1.0); glVertex3f(box_left  * ground, (box_up   * ground) + (half_ground) + offset, (half_ground));
	glEnd();

	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0); glVertex3f(box_left  * ground, (box_down * ground) + (half_ground) + offset, -(half_ground));
	glTexCoord2f(1.0, 0.0); glVertex3f(box_right * ground, (box_down * ground) + (half_ground) + offset, -(half_ground));
	glTexCoord2f(1.0, 1.0); glVertex3f(box_right * ground, (box_up   * ground) + (half_ground) + offset, -(half_ground));
	glTexCoord2f(0.0, 1.0); glVertex3f(box_left  * ground, (box_up   * ground) + (half_ground) + offset, -(half_ground));
	glEnd();

	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0); glVertex3f((half_ground), (box_down * ground) + (half_ground) + offset, box_left  * ground);
	glTexCoord2f(1.0, 0.0); glVertex3f((half_ground), (box_down * ground) + (half_ground) + offset, box_right * ground);
	glTexCoord2f(1.0, 1.0); glVertex3f((half_ground), (box_up   * ground) + (half_ground) + offset, box_right * ground);
	glTexCoord2f(0.0, 1.0); glVertex3f((half_ground), (box_up   * ground) + (half_ground) + offset, box_left  * ground);
	glEnd();

	glDisable(GL_TEXTURE_2D);
}

void reshape(int w, int h) 
{	// This function will project the contents of the program correctly
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity(); // Reset the matrix

	gluPerspective(60.0, (double)w/h, 0.01, 1000.0);
	
	gluLookAt(0,0,0, 0,0,1, 0,1,0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity(); 
}

void doIdle() {
	string file_name = "";
	string jpg = ".jpg";
	string a = "00";
	string b = "0";
	char p[10] = {};
	if (save_image)
	{
		if (num_of_image < 10)
		{
			sprintf(p, "%d", num_of_image);
			file_name = a  + p + jpg;		
			//cout << file_name << endl;
			saveScreenshot((char*)file_name.c_str());
		}
		else if (num_of_image < 100)
		{
			sprintf(p, "%d", num_of_image);
			file_name = b  + p + jpg;	
			saveScreenshot((char*)file_name.c_str());
		} else if (num_of_image < 300)
		{
			sprintf(p, "%d", num_of_image);
			file_name = p + jpg;	
			saveScreenshot((char*)file_name.c_str());
		} 
		else 
			save_image = false;

		num_of_image ++;
	}

	/* make the screen update */
	glutPostRedisplay();
}

void show_light()
{
	// code from CSCI 520
	// global ambient light
	GLfloat aGa[] = { 0.0, 0.0, 0.0, 0.0 };

	// light 's ambient, diffuse, specular
	GLfloat lKa0[] = { 1.0, 1.0, 1.0, 1.0 }; 
	GLfloat lKd0[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat lKs0[] = { 1.0, 1.0, 1.0, 1.0 };

	GLfloat lKa1[] = { 0.0, 0.0, 0.0, 1.0 };
	GLfloat lKd1[] = { 1.0, 0.0, 0.0, 1.0 };
	GLfloat lKs1[] = { 1.0, 0.0, 0.0, 1.0 };

	GLfloat lKa2[] = { 0.0, 0.0, 0.0, 1.0 };
	GLfloat lKd2[] = { 1.0, 1.0, 0.0, 1.0 };
	GLfloat lKs2[] = { 1.0, 1.0, 0.0, 1.0 };

	GLfloat lKa3[] = { 0.0, 0.0, 0.0, 1.0 };
	GLfloat lKd3[] = { 0.0, 1.0, 1.0, 1.0 };
	GLfloat lKs3[] = { 0.0, 1.0, 1.0, 1.0 };

	GLfloat lKa4[] = { 0.0, 0.0, 0.0, 1.0 };
	GLfloat lKd4[] = { 0.0, 0.0, 1.0, 1.0 };
	GLfloat lKs4[] = { 0.0, 0.0, 1.0, 1.0 };

	GLfloat lKa5[] = { 0.0, 0.0, 0.0, 1.0 };
	GLfloat lKd5[] = { 1.0, 0.0, 1.0, 1.0 };
	GLfloat lKs5[] = { 1.0, 0.0, 1.0, 1.0 };

	GLfloat lKa6[] = { 0.0, 0.0, 0.0, 1.0 };
	GLfloat lKd6[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat lKs6[] = { 1.0, 1.0, 1.0, 1.0 };

	GLfloat lKa7[] = { 0.0, 0.0, 0.0, 1.0 };
	GLfloat lKd7[] = { 0.0, 1.0, 1.0, 1.0 };
	GLfloat lKs7[] = { 0.0, 1.0, 1.0, 1.0 };

	// light positions and directions
	GLfloat lP0[] = { 0, 10, 0, 1.0 };
	GLfloat lP1[] = { 1.999, -1.999, -1.999, 1.0 };
	GLfloat lP2[] = { 1.999, 1.999, -1.999, 1.0 };
	GLfloat lP3[] = { -1.999, 1.999, -1.999, 1.0 };
	GLfloat lP4[] = { -1.999, -1.999, 1.999, 1.0 };
	GLfloat lP5[] = { 1.999, -1.999, 1.999, 1.0 };
	GLfloat lP6[] = { 1.999, 1.999, 1.999, 1.0 };
	GLfloat lP7[] = { -1.999, 1.999, 1.999, 1.0 };

	// material color

	GLfloat mKa[] = { 1.0, 0, 0, 1 };
	GLfloat mKd[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat mKs[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat mKe[] = { 0.0, 0.0, 0.0, 1.0 };

	/* set up lighting */
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, aGa);
	glLightModelf(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
	glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);

	// set up cube color
	glMaterialfv(GL_FRONT, GL_AMBIENT, mKa);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mKd);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mKs);
	glMaterialfv(GL_FRONT, GL_EMISSION, mKe);
	glMaterialf(GL_FRONT, GL_SHININESS, 120);

	// macro to set up light i
#define LIGHTSETUP(i)\
	glLightfv(GL_LIGHT##i, GL_POSITION, lP##i);\
	glLightfv(GL_LIGHT##i, GL_AMBIENT, lKa##i);\
	glLightfv(GL_LIGHT##i, GL_DIFFUSE, lKd##i);\
	glLightfv(GL_LIGHT##i, GL_SPECULAR, lKs##i);\
	glEnable(GL_LIGHT##i)

	LIGHTSETUP (0);
// 	LIGHTSETUP (1);
// 	LIGHTSETUP (2);
// 	LIGHTSETUP (3);
// 	LIGHTSETUP (4);
// 	LIGHTSETUP (5);
// 	LIGHTSETUP (6);
// 	LIGHTSETUP (7);

	
}

void render() 
{ 
	display_list = glGenLists(1); 

	glNewList(display_list, GL_COMPILE);

	draw_splines();

	glEndList();
}

void display() {

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
	glLoadIdentity(); 
	glMatrixMode(GL_MODELVIEW); 
	set_camera(); 

	gluLookAt(camera_current.x, camera_current.y, camera_current.z, 
			  -camera_prev.x, -camera_prev.y, -camera_prev.z, 
			  biNorm_current.x, biNorm_current.y, biNorm_current.z);

	if (light_on)
	{
		show_light();
		// enable lighting
		glEnable(GL_LIGHTING);    
	}
	
	draw_ground_sky(max_len * MAX_DIST_MULT, lowest + FLOOR_SUB);

	glCallList(display_list);

	if (light_on)
		glDisable(GL_LIGHTING);

	glutSwapBuffers();
}

void keyPressed(unsigned char key, int x, int y) {	
	if (key == ' ') 
	{	
		save_image = 1 - save_image;
	} else if (key == 'l' || key == 'L')
	{
		light_on = 1 - light_on;
	}
}

int loadSplines(char *argv) {
	char *cName = (char *)malloc(128 * sizeof(char));
	FILE *fileList;
	FILE *fileSpline;
	int iType, i = 0, j, iLength;

	/* load the track file */
	fileList = fopen(argv, "r");
	if (fileList == NULL) {
		printf ("can't open file\n");
		exit(1);
	}
  
	/* stores the number of splines in a global variable */
	fscanf(fileList, "%d", &g_iNumOfSplines);

	g_Splines = (struct spline *)malloc(g_iNumOfSplines * sizeof(struct spline));

	/* reads through the spline files */
	for (j = 0; j < g_iNumOfSplines; j++) {
		i = 0;
		fscanf(fileList, "%s", cName);
		fileSpline = fopen(cName, "r");

		if (fileSpline == NULL) {
			printf ("can't open file\n");
			exit(1);
		}

		/* gets length for spline file */
		fscanf(fileSpline, "%d %d", &iLength, &iType);

		/* allocate memory for all the points */
		g_Splines[j].points = (struct point *)malloc(iLength * sizeof(struct point));
		g_Splines[j].numControlPoints = iLength;

		/* saves the data to the struct */
		while (fscanf(fileSpline, "%lf %lf %lf", 
			&g_Splines[j].points[i].x, 
			&g_Splines[j].points[i].y, 
			&g_Splines[j].points[i].z) != EOF) {
			i++;
		}
	}

	free(cName);

	return 0;
}

void myinit()
{	
	glClearColor(0.0, 0.0, 0.0, 0.0);

	glShadeModel(GL_SMOOTH);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_POLYGON_OFFSET_LINE);
	glPointSize(10);
	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_BLEND);

	glLineWidth(5);

	// texture
	ground_image = jpeg_read("grass.jpg", NULL);
	sky_image = jpeg_read("sky.jpg", NULL); 
	sky_top = jpeg_read("sky_top.jpg", NULL);

	if (ground_image==NULL || sky_image==NULL || sky_top == NULL) {
		std::cout << "loading error" << std::endl;
		exit(1);
	}

	texture_id = 0; // Reference variable for the ground texture ID
	glGenTextures(1, &texture_id);
	glBindTexture(GL_TEXTURE_2D, texture_id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // Repeat the S texture coordinate
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // Repeat the T texture coordinate
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ground_image->nx, ground_image->ny, 0, GL_RGB, GL_UNSIGNED_BYTE, ground_image->pix);

	// draw rail
	quadratic=gluNewQuadric(); 

	control_num = 1; 
	spline_num = 0; 
	u_step = 0;
	boundary(); 
	render(); 
}

int main(int argc, char* argv[])
{
	if (argc<2)
	{  
		printf ("usage: %s <trackfile>\n", argv[0]);
		exit(0);
	}

	loadSplines(argv[1]);

	glutInit(&argc,argv);

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(WINDOW_WIDTH,WINDOW_HEIGHT);
	glutInitWindowPosition(200,200);
	glutCreateWindow(argv[0]);

	glutDisplayFunc(display);

	glutReshapeFunc(reshape);
  
	glutIdleFunc(doIdle);

	glutKeyboardFunc(keyPressed);

	myinit();

	glutMainLoop();

	return 0;
}