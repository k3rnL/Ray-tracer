#define PI  3.14159265359f

#define MAX_RECURSION 5
#define SIZEX (1290)
#define SIZEY (720)

#define DEFAULT_COLOR (float3)(0.1,0.1,0.1)

#define GET_Y(id) (id/SIZEX)
#define DEBUG(id) (GET_Y(id) == SIZEY/2 && id-GET_Y(id)*SIZEX == SIZEX/2 ? 1 : 0)
#define DEBUG(id) 0
#define PRINT_VECTOR(name,vector) (printf("%s: %f %f %f", name, vector.x, vector.y, vector.z))

#define SPHERE 0
#define PLAN 1
#define CYLINDER 2

typedef struct __attribute__((aligned(32))) s_Material {
	float4 	color;
	float 	reflexion;
	float 	refraction;
}		Material;

typedef struct __attribute__((aligned(64))) s_Object {
	float3 		position;
	float 		size;
	Material	mat;
	float4 		rotation;
	int 		type;
}				Object;

typedef struct s_Ray {
	float 	distance;
	int 		id_object;
	float3 	origin;
	float3 	direction;
	float3 	position;
	float3 	color;
}				Ray;

static float get_random(unsigned int *seed0, unsigned int *seed1) {

	/* hash the seeds using bitwise AND operations and bitshifts */
	*seed0 = 36969 * ((*seed0) & 65535) + ((*seed0) >> 16);
	*seed1 = 18000 * ((*seed1) & 65535) + ((*seed1) >> 16);

	unsigned int ires = ((*seed0) << 16) + (*seed1);

	/* use union struct to convert int to float */
	union {
		float f;
		unsigned int ui;
	} res;

	res.ui = (ires & 0x007fffff) | 0x40000000;  /* bitwise AND, bitwise OR */
	return (res.f - 2.0f) / 2.0f;
}

float3 reflect(float3 incident, float3 normal)
{
	return  incident - 2.f * dot(incident, normal) * normal;
}

float3 refract( float3 i, float3 n, float eta )
{
  float cosi = dot(-i, n);
  float cost2 = 1.0f - eta * eta * (1.0f - cosi*cosi);
	if (cost2 < 0.0)
		return reflect(i, n);
	if (DEBUG(get_global_id(0)))
		printf("cosi=%f cost=%f eta=%f", cosi, cost2, eta);
  float3 t = eta*i + n * ((eta*cosi - sqrt(fabs(cost2))));
  return t;
}

void fresnel(float3 direction, float3 normal, float n1, float n2, float *kr)
{
    float cosi = clamp(dot(direction, normal), -1.f, 1.f);
    float sint = (n1 / n2) * sqrt(max(0.f, 1 - cosi * cosi));
    // Total internal reflection
    if (sint >= 1) {
        *kr = 1;
    }
    else {
        float cost = sqrt(max(0.f, 1 - sint * sint));
        cosi = fabs(cosi);
        float Rs = ((n2 * cosi) - (n1 * cost)) / ((n2 * cosi) + (n1 * cost));
        float Rp = ((n1 * cosi) - (n2 * cost)) / ((n1 * cosi) + (n2 * cost));
        *kr = (Rs * Rs + Rp * Rp) / 2.f;
    }
    // As a consequence of the conservation of energy, transmittance is given by:
    // kt = 1 - kr;
}

void vectorRotation(float4* vector, const float4 angles)
{
    float4 __r = (*vector);
    /* X axis */
    __r.y = (*vector).y * half_cos(angles.x) - (*vector).z * half_sin(angles.x);
    __r.z = (*vector).y * half_sin(angles.x) + (*vector).z * half_cos(angles.x);
    (*vector) = __r;
    __r = (*vector);
    /* Y axis */
    __r.z = (*vector).z * half_cos(angles.y) - (*vector).x * half_sin(angles.y);
    __r.x = (*vector).z * half_sin(angles.y) + (*vector).x * half_cos(angles.y);
    (*vector) = __r;
}

float ray_plan(Ray *ray, __global Object *plan)
{
	float k = -(ray->origin.y) / ray->direction.y;

	float3 pos = ray->origin + k * ray->direction;
	if ((pos.x < -(plan->size / 2) || pos.x > plan->size / 2) ||
			(pos.z < -(plan->size / 2) || pos.z > plan->size / 2))
			return -1;
	return k;
}

float ray_sphere(Ray *ray, __global Object *sphere)
{
	float a = dot(ray->direction, ray->direction);
	float b = 2.f * dot(ray->direction, ray->origin);
	float c = dot(ray->origin, ray->origin) - sphere->size * sphere->size;
	float delta = b*b - 4.f * a * c;

	float k = (-b - sqrt(delta)) / (2.f * a);
	float kk = (-b + sqrt(delta)) / (2.f * a);

	if (k < 0.f && kk < 0.f)
		return -1;

	if (k > kk) { // k must be the lowest value
		a = k;
		k = kk;
		kk = a;
	}
	//if (k > 0. || kk > 0.)
	//printf("k=%f kk=%f", k, kk);
	if (k > 0.f)
		return k;
	else if (kk > 0.f)
		return kk;
	return -1;
}

float ray_cylinder(Ray *ray, __global Object *cylinder)
{
	float vy = ray->direction.y;
	float oy = ray->origin.y;
	ray->direction.y = 0;
	ray->origin.y = 0;

	float k = ray_sphere(ray, cylinder);

	ray->direction.y = vy;
	ray->origin.y = oy;

	// compute circles
	float3 pos = k * ray->direction + ray->origin;
	if (pos.y < 0 || pos.y > cylinder->size) {
		float k_pl_down = -ray->origin.y / ray->direction.y;
		float k_pl_up = (cylinder->size - ray->origin.y) / ray->direction.y;
		float k = -1;
		if (k_pl_down > 0.f && k_pl_down < k_pl_up)
    	k = k_pl_down;
  	else if (k_pl_up > 0.f && k_pl_up < k_pl_down)
    	k = k_pl_up;
  	else
    	return (-1);
		float x = ray->direction.x * k + ray->origin.x;
  	float z = ray->direction.z * k + ray->origin.z;
  	if (x * x + z * z > cylinder->size * cylinder->size)
    	return (-1);
		return (k);
	}
	return k;
}

float3 compute_normal(Ray *ray, __global Object *object, int time)
{
	float3 normal;

	if (object->type == SPHERE) {
		normal = normalize(ray->position - object->position);
	}
	else if (object->type == PLAN) {
		normal = (float3)(0,1,0);
	}
	else if (object->type == CYLINDER) {
		normal = ray->position - object->position;
		if (normal.y > object->size - 0.1)
			normal = (float3)(0,1,0);
		else if (normal.y < 0.1)
			normal = (float3)(0,-1,0);
		else {
			normal.y = 0;
			normal = normalize(normal);
		}
		float3 d = ray->origin - object->position;
		float dist = dot(d.xz,d.xz);
		if (dist < object->size*object->size + 1)
			normal = -normal;
	}
	vectorRotation(&normal, -object->rotation);
	float cosi = clamp(dot(-ray->direction,normal), -1.f, 1.f);
	if (cosi < 0)
		normal = -normal;
	return normal;
}

void cast_ray(float3 *color, float3 *color_mask, Ray *rays, __global Object *objects, int nb_objects, int loop, int time, unsigned int *seed0, unsigned int *seed1)
{
	Ray *ray = &rays[loop];
	__global Object *obj = 0;

	// ray->color = normalize((float3)(0.1,0.1,clamp(ray->direction.y,0.3f,1.f)))-0.5;
	ray->color = DEFAULT_COLOR;
	ray->distance = -1;
	ray->id_object = -1;

	if (DEBUG(get_global_id(0))) {
		printf("Ray %d", loop);
		PRINT_VECTOR("\torigin", ray->origin);
		PRINT_VECTOR("\tdirection", ray->direction);
	}

	for (int i = 0 ; i < nb_objects ; i++) {
		obj = &objects[i];
		ray->origin -= obj->position;
		vectorRotation(&ray->direction, obj->rotation);
		vectorRotation(&ray->origin, obj->rotation);

		float distance;
		if (obj->type == SPHERE)
			distance = ray_sphere(ray, obj);
		else if (obj->type == PLAN)
			distance = ray_plan(ray, obj);
		else if (obj->type == CYLINDER && 0)
			distance = ray_cylinder(ray, obj);
		if (distance > 0.0 && (ray->distance < 0.0 || distance < ray->distance)) {
			ray->distance = distance;
			ray->id_object = i;
		}

		vectorRotation(&ray->origin, -obj->rotation);
		vectorRotation(&ray->direction, -obj->rotation);
		ray->origin += obj->position;

	}

	if (ray->distance < 0) {
		*color = *color_mask * DEFAULT_COLOR;
		return;
	}

	obj = &objects[ray->id_object];
	ray->position = ray->distance * ray->direction + ray->origin;
	float3 normal = compute_normal(ray, obj, time);


	float rand1 = 2.0f * PI * get_random(seed0, seed1);
	float rand2 = get_random(seed0, seed1);
	float rand2s = sqrt(rand2);

	if (DEBUG(get_global_id(0))) {
		printf("\tType touched: %d", obj->type);
		printf("\tid touched: %d", ray->id_object);
		printf("\tmat: r=%f tr=%f", obj->mat.reflexion, obj->mat.refraction);
		PRINT_VECTOR("\tnormal", normal);
		PRINT_VECTOR("\tposition", ray->position);
		PRINT_VECTOR("\tmat color", objects[ray->id_object].mat.color.xyz);
	}
	/* create a local orthogonal coordinate frame centered at the hitpoint */
	float3 w = normal;
	float3 axis = fabs(w.x) > 0.1f ? (float3)(0.0f, 1.0f, 0.0f) : (float3)(1.0f, 0.0f, 0.0f);
	float3 u = normalize(cross(axis, w));
	float3 v = cross(w, u);

	float3 emission = (float3)(0);
	if (ray->id_object == 1)
		emission = (float3)(1);
	*color = *color + *color_mask * emission;
	*color_mask = *color_mask * objects[ray->id_object].mat.color.xyz;
	//*color_mask = *color_mask * dot(ray->direction, normal);

	//*color = objects[ray->id_object].mat.color.xyz;
	//*color = objects[ray->id_object].mat.color.xyz;
	// *color = (float3)(objects[0].mat.color.x);

	if (loop+1 != MAX_RECURSION && ray->distance > 0) {
		/* use the coordinte frame and random numbers to compute the next ray direction */
		rays[loop+1].direction = normalize(u * cos(rand1)*rand2s + v*sin(rand1)*rand2s + w*sqrt(1.0f - rand2));
		// rays[loop+1].direction = reflect(ray->direction, normal);
		rays[loop+1].origin = ray->position + normal * 0.01f;

		cast_ray(color, color_mask, rays, objects, nb_objects, loop+1, time, seed0, seed1);
	}

}

__kernel void generate_rays_directions(__global float *output, float3 rotation, float image_scale, float image_ratio) {
	int id = get_global_id(0);
	int y = id/SIZEX;
	int x = id - y*SIZEX;
	id *= 4;
	float4 dir;
	dir.x = (2 * (x + 0.5f) / SIZEX - 1) * image_scale * image_ratio;
	dir.y = -(1 - 2 * (y + 0.5) / SIZEY) * image_scale;
	dir.z = 1;
	dir = normalize(dir);
	vectorRotation(&dir, (float4)(rotation,1));
	output[id+0] = dir.x;
	output[id+1] = dir.y;
	output[id+2] = dir.z;
}

__kernel void draw_scene(__write_only image2d_t output, float3 origin, __global float *directions, __global Object *objects, int nb_objects, int time)
{
	int id = get_global_id(0);
	int id3 = id*3;
	int id4 = id*4;
	int y = id / SIZEX;
	int x = id - y * SIZEX;

	Ray rays[MAX_RECURSION];

	if(DEBUG(id))
		printf("\n");

	rays->origin = origin;rays->origin.z -= 50;
	rays->distance = -1;
	rays->direction.x = directions[id4];
	rays->direction.y = directions[id4+1];
	rays->direction.z = directions[id4+2];

	unsigned int seed0 = x;
	unsigned int seed1 = y;

	float3 final = (float3)(0);

	time = 0;

	float sample = 40;
	for (int i = 0 ; i < sample ; i++) {
		float3 color = (float3)(0);;
		float3 mask = (float3)(1);
		cast_ray(&color, &mask, rays, objects, nb_objects, 0, time, &seed0, &seed1);
		if (DEBUG(id))
			PRINT_VECTOR("color intermediaire", final);
		final += clamp(color * (1.f/sample), 0,1);
	}

	write_imagef(output, (int2)(x,y), (float4)(final,1));
	if (DEBUG(id)) {
		write_imagef(output, (int2)(x,y), (float4)(1,0,0,1));
		 PRINT_VECTOR("color final", final);
	}
	// write_imagef(output, (int2)(x,y), (float4)(rays->origin,1));
}
