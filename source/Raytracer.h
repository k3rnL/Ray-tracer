#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include <string>
#include <vector>
#include <random>
#include <CL/cl.h>
#include <CL/cl_gl.h>
#include <GL/glew.h>
#include <CL/cl_gl_ext.h>
#include <glm/glm.hpp>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

class Raytracer
{
public:
			Raytracer();
	virtual ~Raytracer();

	virtual bool		init();
	virtual void		redraw();
	virtual glm::vec4	*getImage();
	
	void setOpenGLOutputTexture(GLuint texture_id, GLint mipmap_level, GLenum texture_type);

	virtual void		setDirection(const glm::vec3 &dir);
	virtual void		setOrigin(const glm::vec3 &origin);

	virtual const glm::vec3 &getOrigin();
	virtual const glm::vec3 &getDirection();

	virtual void drawSphere(const glm::vec3 &color, const glm::vec3 &position, float R);

private:

	__declspec(align(32)) struct Material
	{
		glm::vec4	color;		// 16 bytes
		float		reflexion;	// 4  bytes
		float		refraction;
	};

	__declspec(align(64)) struct Object
	{
		cl_float3	position; // 16 bytes
		cl_float	size;     // 4  bytes
		Material	mat;
		cl_float4	rotation;
		cl_int		type;
	};

	enum eObjectType
	{
		SPHERE = 0,
		PLAN = 1,
		CYLINDER = 2
	};

	std::vector<Object>	mObjects;

	int					convertToString(const char *filename, std::string& s);
	const char			*getErrorString(cl_int error);
	const char			*getEventStatusString(cl_int error);

	int					mSizeX;
	int					mSizeY;
	glm::vec3			mOrigin;
	glm::vec3			mDirection;
	float				mImageScale;
	float				mImageAspectRatio;

	cl_context				mContext;
	cl_mem					mDirectionsBuffer;
	cl_mem					mOutputBuffer;
	cl_mem					mPrimitivesBuffer;
	cl_kernel				mKernelDrawScene;
	cl_kernel				mKernelGenerateDirections;
	cl_program				mProgram;
	cl_command_queue		mCommandQueue;
	std::vector<cl_event>	mEventsToWait;
	glm::vec4				*mImage;
	glm::vec4				*mVectors;
	cl_mem					mGLTexture;
};

