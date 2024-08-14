#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>

#ifdef DEBUGGING
	#define MAX_ITER 1000
#endif
#ifdef NOINLINE
	#define FUNCTION_ATTRIBUTES __attribute((noinline))
#else
	#define FUNCTION_ATTRIBUTES static inline __attribute__((always_inline))
#endif

#define PI 3.14159265359
#define INV_ROOT_3 0.57735026919

#define GRID_SIZE 100
#define Y_OFFSET (GRID_SIZE >> 1)
#define SCALE GRID_SIZE
#define MAJOR_REVOLUTION_SEGMENT_COUNT 500
#define MINOR_REVOLUTION_SEGMENT_COUNT 200
#define TOTAL_REVOLUTION_SEGMENT_COUNT 1000 // Must be divisible by both major and minor revolutions
#define MAJOR_RADIUS 2.0f
#define MINOR_RADIUS 1.0f
#define OBSERVER_DISTANCE 8
#define LIGHT_SCALE_SIZE 10

#define EPSILON 0.01f // For small addition
#define FRAME_TIME 0.005 // (ms)
#define DELTA_ROT_X 0.037f
#define DELTA_ROT_Z 0.023f

const float LIGHT_VECTOR_X = -INV_ROOT_3;
const float LIGHT_VECTOR_Y = -INV_ROOT_3;
const float LIGHT_VECTOR_Z = -INV_ROOT_3;

const float FAR_AWAY = OBSERVER_DISTANCE + MAJOR_RADIUS + MINOR_RADIUS + 10;
const char light_scale[LIGHT_SCALE_SIZE] = " .:-=+*#%@";

typedef struct _angle
{
	float sin;
	float cos;
} Angle;

typedef struct _cell
{
	int light;
	float z;
} Cell;

typedef struct _point_data
{
	int grid_x;
	int grid_y;
	float z;
} PointData;

FUNCTION_ATTRIBUTES void init();
FUNCTION_ATTRIBUTES void init_x_offset_buf();
FUNCTION_ATTRIBUTES void precalculate_angles();

FUNCTION_ATTRIBUTES void time_loop(int i, double *delta_time);
FUNCTION_ATTRIBUTES void loop(int i, double delta_time);
FUNCTION_ATTRIBUTES void clear_grid();
FUNCTION_ATTRIBUTES void clear_line(int i);
FUNCTION_ATTRIBUTES void render_grid();
FUNCTION_ATTRIBUTES void render_line(int i);
FUNCTION_ATTRIBUTES void draw_donut(Angle rot_x, Angle rot_z);
FUNCTION_ATTRIBUTES void draw_minor_revolution(Angle rot_x, Angle rot_z, Angle major_revolution);
FUNCTION_ATTRIBUTES void draw_point(Angle rot_x, Angle rot_z, Angle major_revolution, Angle minor_revolution);

FUNCTION_ATTRIBUTES PointData calculate_point_data(Angle rot_x, Angle rot_z, Angle major_revolution, Angle minor_revolution);
FUNCTION_ATTRIBUTES float calculate_luminosity(Angle rot_x, Angle rot_z, Angle major_revolution, Angle minor_revolution);

FUNCTION_ATTRIBUTES void do_rotations(float *x, float *y, float *z, Angle rot_x, Angle rot_z, Angle major_revolution);
FUNCTION_ATTRIBUTES void apply_projection(float *x, float *y, float *z);
FUNCTION_ATTRIBUTES void rotate_xz(float *x, float *y, float *z, Angle rot_x, Angle rot_z);


FUNCTION_ATTRIBUTES void rotate_x(float *x, float *y, float *z, Angle rot_x);
FUNCTION_ATTRIBUTES void rotate_y(float *x, float *y, float *z, Angle rot_y);
FUNCTION_ATTRIBUTES void rotate_z(float *x, float *y, float *z, Angle rot_z);

FUNCTION_ATTRIBUTES bool is_in_grid_range_xy(int x, int y);
FUNCTION_ATTRIBUTES bool is_in_grid_range_i(int i);

FUNCTION_ATTRIBUTES Angle construct_angle(float rad);

char x_offset_buf[Y_OFFSET+1];
Cell grid[GRID_SIZE][GRID_SIZE];
Angle angle_segments[TOTAL_REVOLUTION_SEGMENT_COUNT];

int main()
{
	double delta_time = 0;
	init();
#ifdef DEBUGGING
	for (int i = 0; i < MAX_ITER;i++)
#else
	for (int i = 0; ; i++)
#endif
	{
		time_loop(i, &delta_time);
	} 
}

void init()
{
	init_x_offset_buf();
	precalculate_angles();
}

void precalculate_angles()
{
	for (int i = 0; i < TOTAL_REVOLUTION_SEGMENT_COUNT; i++)
	{
		angle_segments[i] = construct_angle(PI * 2.0f * (float)i / (float)TOTAL_REVOLUTION_SEGMENT_COUNT);
	}
}

void time_loop(int i, double *delta_time)
{
	clock_t start = clock();
	loop(i, *delta_time);
	clock_t end = clock();
	*delta_time = ((double)(end - start) / (CLOCKS_PER_SEC));
#ifndef DEBUGGING
	if (*delta_time < FRAME_TIME)
	{
		struct timespec tim, tim2;
		tim.tv_sec = 0;
		tim.tv_nsec = 1e9 * (FRAME_TIME - *delta_time);
		nanosleep(&tim, &tim2);
		*delta_time = FRAME_TIME;
	}
#endif
}


void loop(int i, double delta_time)
{
	Angle rot_x = construct_angle(i * DELTA_ROT_X);
	Angle rot_z = construct_angle(i * DELTA_ROT_Z);
	clear_grid();
	draw_donut(rot_x, rot_z);
	printf("\x1b[H");
	render_grid();
	printf("Fps: %d\n", (int)(1 / delta_time));
}

void init_x_offset_buf()
{
	for (int i = 0; i < Y_OFFSET; i++)
	{
		x_offset_buf[i] = ' ';
	}
	x_offset_buf[Y_OFFSET] = '\0';
}

void clear_grid()
{
	for (int i = 0; i < GRID_SIZE; i++)
	{
		clear_line(i);
	}
}

void clear_line(int i)
{
	for (int j = 0; j < GRID_SIZE; j++)
	{
		grid[i][j].light = 0;
		grid[i][j].z = FAR_AWAY;
	}
}

void render_grid()
{
	for (int i = 0; i < GRID_SIZE; i++)
	{
		render_line(i);
	}
}

void render_line(int i)
{
	fprintf(stdout, "%s", x_offset_buf);
	for (int j = 0; j < GRID_SIZE; j++)
	{
		putc(light_scale[grid[i][j].light], stdout);
	}
	putc('\n', stdout);
}

void draw_donut(Angle rot_x, Angle rot_z)
{
	for (int i = 0; i < MAJOR_REVOLUTION_SEGMENT_COUNT; i++)
	{
		Angle major_revolution = angle_segments[i * TOTAL_REVOLUTION_SEGMENT_COUNT / MAJOR_REVOLUTION_SEGMENT_COUNT];
		draw_minor_revolution(rot_x, rot_z, major_revolution);
	}
}

void draw_minor_revolution(Angle rot_x, Angle rot_z, Angle major_revolution)
{
	for (int i = 0; i < MINOR_REVOLUTION_SEGMENT_COUNT; i++)
	{

		Angle minor_revolution = angle_segments[i * TOTAL_REVOLUTION_SEGMENT_COUNT / MINOR_REVOLUTION_SEGMENT_COUNT];
		draw_point(rot_x, rot_z, major_revolution, minor_revolution);
	}
}

void draw_point(Angle rot_x, Angle rot_z, Angle major_revolution, Angle minor_revolution)
{
	PointData point_data = calculate_point_data(rot_x, rot_z, major_revolution, minor_revolution);
	if (!is_in_grid_range_xy(point_data.grid_x, point_data.grid_y) || point_data.z >= grid[point_data.grid_y][point_data.grid_x].z)
	{
		return;
	}
	float luminosity = calculate_luminosity(rot_x, rot_z, major_revolution, minor_revolution);
	grid[point_data.grid_y][point_data.grid_x].light = luminosity * LIGHT_SCALE_SIZE;
	grid[point_data.grid_y][point_data.grid_x].z = point_data.z;
}

PointData calculate_point_data(Angle rot_x, Angle rot_z, Angle major_revolution, Angle minor_revolution)
{
	float x = minor_revolution.cos * MINOR_RADIUS + MAJOR_RADIUS;
	float y = minor_revolution.sin * MINOR_RADIUS;
	float z = 0;
	do_rotations(&x, &y, &z, rot_x, rot_z, major_revolution);
	apply_projection(&x, &y, &z);
	int grid_x = round(x * SCALE) + (GRID_SIZE / 2) + EPSILON;
	int grid_y = round(y * SCALE) + (GRID_SIZE / 2) + EPSILON;
	return (PointData){ .grid_x = grid_x, .grid_y = grid_y, z = z };
}

float calculate_luminosity(Angle rot_x, Angle rot_z, Angle major_revolution, Angle minor_revolution)
{
	float x = minor_revolution.cos;
	float y = minor_revolution.sin;
	float z = 0;
	do_rotations(&x, &y, &z, rot_x, rot_z, major_revolution);
	return fmax(0.0f, x * LIGHT_VECTOR_X + y * LIGHT_VECTOR_Y + z * LIGHT_VECTOR_Z);
}

void do_rotations(float *x, float *y, float *z, Angle rot_x, Angle rot_z, Angle major_revolution)
{
	rotate_y(x, y, z, major_revolution);
	rotate_xz(x, y, z, rot_x, rot_z);
}

void rotate_xz(float *x, float *y, float *z, Angle rot_x, Angle rot_z)
{
	rotate_z(x, y, z, rot_z);
	rotate_x(x, y, z, rot_x);
}

void apply_projection(float *x, float *y, float *z)
{
	*z += OBSERVER_DISTANCE;
	float inv_z = 1 / *z;
	*x *= inv_z;
	*y *= inv_z;
}

void rotate_y(float *x, float *y, float *z, Angle rot_y)
{
	rotate_x(y, z, x, rot_y);
}

void rotate_z(float *x, float *y, float *z, Angle rot_z)
{
	rotate_x(z, x, y, rot_z);
}

void rotate_x(float *x, float *y, float *z, Angle rot_x)
{
	float new_y = rot_x.cos * (*y) + rot_x.sin * (*z);
	float new_z = rot_x.cos * (*z) - rot_x.sin * (*y);
	*y = new_y;
	*z = new_z;
}

Angle construct_angle(float rad)
{
	return (Angle) { .cos = cos(rad), .sin = sin(rad) };
}

bool is_in_grid_range_xy(int x, int y)
{
	return is_in_grid_range_i(x) && is_in_grid_range_i(y);
}

bool is_in_grid_range_i(int i)
{
	return 0 <= i && i < GRID_SIZE;
}

