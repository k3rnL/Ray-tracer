#define MAX_RECURSION 3
#define SIZEX 1290
#define SIZEY 800

#define DEFAULT_COLOR (float3)(0.1,0.1,0.1)

#define GET_Y(id) (id/SIZEX)
#define DEBUG(id) (GET_Y(id) == SIZEY/2 && id-GET_Y(id)*SIZEX == SIZEX/2 ? 1 : 0)
#define PRINT_VECTOR(name,vector) (printf("%s: %f %f %f", name, vector.x, vector.y, vector.z))

#define SPHERE 0
#define PLAN 1

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

float3 reflect(float3 incendent, float3 normal)
{
	return  incendent - 2.f * dot(incendent, normal) * normal;
}

float3 refract( float3 i, float3 n, float eta )
{
  float cosi = dot(-i, n);
  float cost2 = 1.0f - eta * eta * (1.0f - cosi*cosi);
	if (cost2 < 0.0)
		return reflect(i, n);
  float3 t = eta*i + n * ((eta*cosi - sqrt(fabs(cost2))));
  return t;
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
	return -(ray->origin.y - 100) / ray->direction.y;
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
	if (k > 0.1)
		return k;
	else if (kk > 0.1)
		return kk;
	return -1;
}

float3 compute_normal(Ray *ray, __global Object *object)
{
	float3 normal;

	if (object->type == SPHERE) {
		normal = normalize(ray->position - object->position);
		float3 d = ray->origin - object->position;
		float dist = dot(d,d);
		if (dist < object->size*object->size + 1)
			normal = -normal;
	}
	else if (object->type == PLAN) {
		normal = (float3)(0,1,0);
	}

	vectorRotation(&normal, -object->rotation);
	return normal;
}

void cast_ray(Ray *rays, __global Object *objects, int nb_objects, int loop)
{
	Ray *ray = &rays[loop];
	__global Object *obj = 0;

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
		if (distance > 0 && (ray->distance < 0 || distance < ray->distance)) {
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
	float3 normal = compute_normal(ray, obj);

	if (DEBUG(get_global_id(0))) {
		printf("\tType touched: %d", obj->type);
		PRINT_VECTOR("\tnormal", normal);
		PRINT_VECTOR("\tposition", ray->position);
	}

	float3 light = (float3)(0,60,-160);
	float3 light_vector = normalize(light - ray->position);
	float angle = dot(light_vector, normal);
	float3 diffuse = (float3) obj->mat.color.xyz * angle;
	//ray->color = (float3) obj->mat.color.xyz;
	if (obj->mat.refraction > 0 && loop+1<MAX_RECURSION) {
		float n1 = 1;
		float n2 = obj->mat.refraction;
		if (loop > 0 && rays[loop-1].id_object == ray->id_object) {
			n1 = obj->mat.refraction;
			n2 = 1;
		}

		rays[loop+1].origin = ray->position;
		rays[loop+1].direction = refract(ray->direction, normal, n1/n2);
		rays[loop+1].distance = -1;
		rays[loop+1].color = DEFAULT_COLOR;
		cast_ray(rays, objects, nb_objects, loop+1);
		if (rays[loop+1].distance > 0)
			ray->color = rays[loop+1].color;
		//ray->color += diffuse * (1-obj->mat.reflexion);
	}
	else if (obj->mat.reflexion > 0 && loop+1<MAX_RECURSION && obj->type != PLAN) {
		rays[loop+1].origin = ray->position + normal;
		rays[loop+1].direction = reflect(ray->direction, normal);
		rays[loop+1].distance = -1;
		rays[loop+1].color = DEFAULT_COLOR;
		cast_ray(rays, objects, nb_objects, loop+1);
		if (rays[loop+1].distance > 0)
			ray->color = rays[loop+1].color * obj->mat.reflexion;
		ray->color += diffuse * (1-obj->mat.reflexion);

		int id = get_global_id(0);
		int y = id / SIZEX;
		int x = id - y * SIZEX;
		if (x == SIZEX/2 && y == SIZEY/4){
			//printf("normal=%f %f %f", normal.x,normal.y,normal.z);
			//printf("ray->position=%f %f %f", ray->position.x,ray->position.y,ray->position.z);
			//printf("rays[loop+1].direction=%f %f %f", rays[loop+1].direction.x,rays[loop+1].direction.y,rays[loop+1].direction.z);
		}
	}
	else {
		if (obj->type == PLAN) {
			int step = 20;
			if (fmod(fabs(ray->position.x), step) < step/2.f)
				if (fmod(fabs(ray->position.y), step) < step/2.f)
					diffuse = ray->position;
		}
		ray->color = diffuse;
	}
}

__kernel void draw_scene(__write_only image2d_t output, float3 origin, __global float *directions, __global Object *objects, int nb_objects)
{
	int id = get_global_id(0);
	int id3 = id*3;
	int id4 = id*4;

	Ray rays[MAX_RECURSION];

	if(DEBUG(id))
		printf("\n");

	rays->origin = origin;
	rays->distance = -1;
	rays->direction.x = directions[id4];
	rays->direction.y = directions[id4+1];
	rays->direction.z = directions[id4+2];
	rays->color = DEFAULT_COLOR;

	cast_ray(rays, objects, nb_objects, 0);

	int y = id / SIZEX;
	int x = id - y * SIZEX;
	write_imagef(output, (int2)(x,y), (float4)(rays->color,1));
	// write_imagef(output, (int2)(x,y), (float4)(rays->origin,1));
}
