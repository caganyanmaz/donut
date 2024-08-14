#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>

#define PI 3.14159265359
#define INV_ROOT_3 0.57735026919

#define GRID_SIZE 100
#define Y_OFFSET (GRID_SIZE >> 1)
#define SCALE GRID_SIZE
#define MAJOR_REVOLUTION_SEGMENT_COUNT 500
#define MINOR_REVOLUTION_SEGMENT_COUNT 200
#define MAJOR_RADIUS 2.0f
#define MINOR_RADIUS 1.0f
#define OBSERVER_DISTANCE 8
#define LIGHT_SCALE_SIZE 10

#define EPSILON 0.01f // For small addition
#define DELTA_TIME 0.05f
#define DELTA_ROT_X 0.037f
#define DELTA_ROT_Z 0.023f
#define SLEEP_COMMAND_BUF_SIZE 64

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

static inline __attribute__((always_inline)) void init();
static inline __attribute__((always_inline)) void init_y_offset_buf();
static inline __attribute__((always_inline)) void init_system_sleep_call_buf();

static inline __attribute__((always_inline)) void loop();
static inline __attribute__((always_inline)) void clear_grid();
static inline __attribute__((always_inline)) void clear_line(int i);
static inline __attribute__((always_inline)) void render_grid();
static inline __attribute__((always_inline)) void render_line(int i);
static inline __attribute__((always_inline)) void draw_donut(Angle rot_x, Angle rot_z);
static inline __attribute__((always_inline)) void draw_minor_revolution(Angle rot_x, Angle rot_z, Angle major_revolution);
static inline __attribute__((always_inline)) void draw_point(Angle rot_x, Angle rot_z, Angle major_revolution, Angle minor_revolution);

static inline __attribute__((always_inline)) PointData calculate_point_data(Angle rot_x, Angle rot_z, Angle major_revolution, Angle minor_revolution);
static inline __attribute__((always_inline)) float calculate_luminosity(Angle rot_x, Angle rot_z, Angle major_revolution, Angle minor_revolution);

static inline __attribute__((always_inline)) void do_rotations(float *x, float *y, float *z, Angle rot_x, Angle rot_z, Angle major_revolution);
static inline __attribute__((always_inline)) void apply_projection(float *x, float *y, float *z);

static inline __attribute__((always_inline)) void rotate_x(float *x, float *y, float *z, Angle rot_x);
static inline __attribute__((always_inline)) void rotate_y(float *x, float *y, float *z, Angle rot_y);
static inline __attribute__((always_inline)) void rotate_z(float *x, float *y, float *z, Angle rot_z);

static inline __attribute__((always_inline)) bool is_in_grid_range_xy(int x, int y);
static inline __attribute__((always_inline)) bool is_in_grid_range_i(int i);

static inline __attribute__((always_inline)) Angle construct_angle(float rad);

char y_offset_buf[Y_OFFSET+1];
char system_sleep_call_buf[SLEEP_COMMAND_BUF_SIZE];
Cell grid[GRID_SIZE][GRID_SIZE];

int main()
{
	init();
	for (int i = 0; ;i++)
	{
		loop(i);
	} 
}

void init()
{
	init_y_offset_buf();
	init_system_sleep_call_buf();
	puts(system_sleep_call_buf);
	system("sleep 1");
}

void loop(int i)
{
	Angle rot_x = construct_angle(i * DELTA_ROT_X);
	Angle rot_z = construct_angle(i * DELTA_ROT_Z);
	clear_grid();
	printf("\x1b[H");
	draw_donut(rot_x, rot_z);
	render_grid();
	//system(system_sleep_call_buf);
}

void init_y_offset_buf()
{
	for (int i = 0; i < Y_OFFSET; i++)
	{
		y_offset_buf[i] = ' ';
	}
	y_offset_buf[Y_OFFSET+1] = '\0';
}

void init_system_sleep_call_buf()
{
	sprintf(system_sleep_call_buf, "sleep %f", DELTA_TIME);
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
	fprintf(stdout, "%s", y_offset_buf);
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
		Angle major_revolution = construct_angle((2 * PI * (float)(i)) / (float)MAJOR_REVOLUTION_SEGMENT_COUNT);
		draw_minor_revolution(rot_x, rot_z, major_revolution);
	}
}

void draw_minor_revolution(Angle rot_x, Angle rot_z, Angle major_rad)
{
	for (int i = 0; i < MINOR_REVOLUTION_SEGMENT_COUNT; i++)
	{
		Angle minor_rad = construct_angle((2 * PI * (float)(i)) / ((float) MINOR_REVOLUTION_SEGMENT_COUNT));
		draw_point(rot_x, rot_z, major_rad, minor_rad);
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

