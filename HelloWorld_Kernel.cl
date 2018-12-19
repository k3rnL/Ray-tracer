#define MAX_RECURSION 16
#define SIZEX (1290)
#define SIZEY (720)

#define DEFAULT_COLOR (float3)(0.1,0.1,0.1)

#define GET_Y(id) (id/SIZEX)
//#define DEBUG(id) (GET_Y(id) == SIZEY/2 && id-GET_Y(id)*SIZEX == SIZEX/2 ? 1 : 0)
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
		normal.x = cos(ray->position.x/16 + time/300.f) / 20;
		normal.z = cos(ray->position.z/16 + time/300.f) / 20;
		//normal.xz += (float2)(cos(ray->position.x + time/300.f) / 20, cos(ray->position.z + time/300.f) / 20);
		normal = normalize(normal);
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

void cast_ray(Ray *rays, __global Object *objects, int nb_objects, int loop, int time)
{
	Ray *ray = &rays[loop];
	__global Object *obj = 0;

	// ray->color = normalize((float3)(0.1,0.1,clamp(ray->direction.y,0.3f,1.f)))-0.5;
	ray->color = DEFAULT_COLOR;

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

	if (ray->distance < 0)
		return;

	obj = &objects[ray->id_object];
	ray->position = ray->distance * ray->direction + ray->origin;
	float3 normal = compute_normal(ray, obj, time);

	if (DEBUG(get_global_id(0))) {
		printf("\tType touched: %d", obj->type);
		printf("\tid touched: %d", ray->id_object);
		printf("\tmat: r=%f tr=%f", obj->mat.reflexion, obj->mat.refraction);
		PRINT_VECTOR("\tnormal", normal);
		PRINT_VECTOR("\tposition", ray->position);
	}

	float3 light = (float3)(0,60,-160);
	float3 light_vector = normalize(light - ray->position);
	float angle = dot(light_vector, normal);
	float3 diffuse = (float3) obj->mat.color.xyz * angle;
	//ray->color = (float3) obj->mat.color.xyz;

	////////////////
	// refraction
	if (obj->mat.refraction > 0. && loop+1<MAX_RECURSION) {
		float n1 = 1;
		float n2 = 1.5;
		if (loop > 0 && rays[loop-1].id_object == ray->id_object) {
			n1 = 1.5;
			n2 = 1;
		}

		rays[loop+1].origin = ray->position - normal  * 0.1;
		rays[loop+1].direction = refract(ray->direction, normal, n1/n2);
		rays[loop+1].distance = -1;
		if (DEBUG(get_global_id(0)))
			printf("--- transmitted ray ---");
		cast_ray(rays, objects, nb_objects, loop+1, time);

		float3 transmitted = rays[loop+1].color;

		float kr = 0;
		if (loop == 0 || (loop >= 0 && rays[loop-1].id_object != ray->id_object))
			fresnel(ray->direction, normal, n1, n2, &kr);
		//kr=0;
		rays[loop+1].origin = ray->position + normal * 0.1;
		rays[loop+1].direction = reflect(ray->direction, normal);
		rays[loop+1].distance = -1;
		if (kr > 0) {
			if (DEBUG(get_global_id(0)))
				printf("--- reflected ray ---");
			cast_ray(rays, objects, nb_objects, loop+1, time);
		}
		float3 reflected = rays[loop+1].color;
		if (DEBUG(get_global_id(0))){
			printf("kr=%f",kr);
			PRINT_VECTOR("tr", transmitted);
			PRINT_VECTOR("re", reflected);
		}
		ray->color = reflected * kr + transmitted * (1.f-kr);
	}
	else if (obj->mat.reflexion > 0 && loop+1<MAX_RECURSION) {
		if (DEBUG(get_global_id(0)))
			printf("--- reflected ray ---");
		rays[loop+1].origin = ray->position + normal * 0.1;
		rays[loop+1].direction = reflect(ray->direction, normal);
		rays[loop+1].distance = -1;
		cast_ray(rays, objects, nb_objects, loop+1, time);
		if (rays[loop+1].distance > 0)
			ray->color = rays[loop+1].color * obj->mat.reflexion;
		ray->color += diffuse * (1-obj->mat.reflexion);
	}
	else {
		if (obj->type == PLAN && ray->id_object == 3) {
			int step = 20;
			if (fmod(fabs(ray->position.x), step) < step/2.f)
				if (fmod(fabs(ray->position.y), step) < step/2.f)
					diffuse = (normalize(ray->position) + 1)/2;
		}
		ray->color = diffuse;
	}

	// specular
	float3 reflect_light = reflect(-light_vector, normal);
	angle = clamp(pow(dot(reflect_light, -ray->direction), 8),0.f,1.f);
	ray->color += (float3)(1) * angle;
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

	Ray rays[MAX_RECURSION];

	if(DEBUG(id))
		printf("\n");

	rays->origin = origin;rays->origin.x += 10;
	rays->distance = -1;
	rays->direction.x = directions[id4];
	rays->direction.y = directions[id4+1];
	rays->direction.z = directions[id4+2];

	//cast_ray(rays, objects, nb_objects, 0);
	cast_ray(rays, objects, nb_objects, 0, time);

	int y = id / SIZEX;
	int x = id - y * SIZEX;
	write_imagef(output, (int2)(x,y), (float4)(rays->color,1));
	if (DEBUG(id)) {
		write_imagef(output, (int2)(x,y), (float4)(1,0,0,1));
		PRINT_VECTOR("color", rays->color);
	}
	// write_imagef(output, (int2)(x,y), (float4)(rays->origin,1));
}
