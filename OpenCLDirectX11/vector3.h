#pragma once

typedef struct
{
	float x, y, z;

} vector3;

float mag(vector3 *v);
vector3 subtract(vector3 *v1, vector3 *v2);
vector3 add(vector3 *v1, vector3 *v2);
vector3 cross(vector3 *v1, vector3 *v2);
vector3 divide(vector3 *v, float number);
void normalize(vector3 *v);
float dot(vector3 *v1, vector3 *v2);
void RotateX(vector3 &v, float a);
void RotateY(vector3 &v, float a);
void RotateZ(vector3 &v, float a);
