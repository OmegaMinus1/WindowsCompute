#include <math.h>
#include "vector3.h"

float mag(vector3 *v)
{
	return (float)sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
}

vector3 subtract(vector3 *v1, vector3 *v2)
{
	vector3 d;

	d.x = v1->x - v2->x;
	d.y = v1->y - v2->y;
	d.z = v1->z - v2->z;

	return d;
}

vector3 add(vector3 *v1, vector3 *v2)
{
	vector3 d;

	d.x = v1->x + v2->x;
	d.y = v1->y + v2->y;
	d.z = v1->z + v2->z;

	return d;
}

vector3 cross(vector3 *v1, vector3 *v2)
{
	vector3 c;

	c.x = v1->y * v2->z - v2->y * v1->z;
	c.y = v1->z * v2->x - v2->z * v1->x;
	c.z = v1->x * v2->y - v2->x * v1->y;

	return c;
}

vector3 divide(vector3 *v, float number)
{
	vector3 result;

	if (number != 0)
	{
		result.x = v->x / number;
		result.y = v->y / number;
		result.z = v->z / number;
	}

	return result;
}

void normalize(vector3 *v)
{
	float d = (float)sqrt(v->x * v->x + v->y * v->y + v->z * v->z);

	if (d != 0)
	{
		v->x = v->x / d;
		v->y = v->y / d;
		v->z = v->z / d;
	}

}



float dot(vector3 *v1, vector3 *v2)
{
	return v1->x * v2->x + v1->y * v2->y + v1->z * v2->z;
}

void RotateX(vector3 &v, float a)
{
	vector3 temp;
	temp.x = v.x;
	temp.y = (v.y * (float)cos(a)) + (v.z * (float)-sin(a));
	temp.z = (v.y * (float)sin(a)) + (v.z * (float)cos(a));
	v = temp;
}

void RotateY(vector3 &v, float a)
{
	vector3 temp;
	temp.x = (v.x * (float)cos(a)) + (v.z * (float)sin(a));
	temp.y = v.y;
	temp.z = (v.x * (float)-sin(a)) + (v.z * (float)cos(a));
	v = temp;
}

void RotateZ(vector3 &v, float a)
{
	vector3 temp;
	temp.x = (v.x * (float)cos(a)) + (v.y * (float)-sin(a));
	temp.y = (v.x * (float)sin(a)) + (v.y * (float)cos(a));
	temp.z = v.z;
	v = temp;
}
