#include "Raytracer.h"

#include <iostream>
#include <fstream>
#include <chrono>

using std::cout;

#if defined(_WIN32)
#include <windows.h>

typedef HGLRC GLContext;
typedef HDC   GLDrawable;
typedef HWND  GLWindow;

GLContext getCurrentGLContext(void) { return wglGetCurrentContext(); }
GLDrawable getCurrentGLDrawable(void) { return wglGetCurrentDC(); }
GLWindow getCurrentGLWindow(void) { return WindowFromDC(wglGetCurrentDC()); }
#elif defined(__unix__)
typedef GLXContext GLContext;
typedef GLXDrawable GLDrawable;
typedef Window GLWindow;

GLContext getCurrentGLContext(void) { return glXGetCurrentContext(); }
GLDrawable getCurrentGLDrawable(void) { return glXGetCurrentDrawable(); }
GLWindow getCurrentGLWindow(void) { return glXGetCurrentDrawable(); }
#endif


/* convert the kernel file into a string */
int Raytracer::convertToString(const char *filename, std::string& s)
{
	size_t size;
	char*  str;
	std::fstream f(filename, (std::fstream::in | std::fstream::binary));

	if (f.is_open())
	{
		size_t fileSize;
		f.seekg(0, std::fstream::end);
		size = fileSize = (size_t)f.tellg();
		f.seekg(0, std::fstream::beg);
		str = new char[size + 1];
		if (!str)
		{
			f.close();
			return 0;
		}

		f.read(str, fileSize);
		f.close();
		str[size] = '\0';
		s = str;
		delete[] str;
		return 0;
	}
	std::cout << "Error: failed to open file\n:" << filename << std::endl;
	return -1;
}

Raytracer::Raytracer()
{
	mSizeX = 1290;
	mSizeY = 720;

	mVectors = new glm::vec4[mSizeX * mSizeY];
	mOrigin = glm::vec3(0, 0, -100);
	mGLTexture = NULL;

	cl_float4 null_rot = {0,0,0,0};

	Material mat;
	mat.color = glm::vec4(1, 0, 0, 0);
	mat.reflexion = 0.85;
	mat.reflexion = 0.85;
	mat.refraction = 1.01;
	mObjects.push_back({ {-50,0,-120}, 20, mat, null_rot, SPHERE });
	mat.color = glm::vec4(1, 1, 0, 0);
	mat.refraction = 1.07;
	mObjects.push_back({ {0,0,-0}, 20, mat, {0,0,0,0} , SPHERE });
	mat.color = glm::vec4(1, 1, 1, 0);
	mat.refraction = 1.3;
	mObjects.push_back({ {50,0,-120}, 20, mat, null_rot , SPHERE});
	mat.reflexion = 0;
	mat.refraction = 0;
	mObjects.push_back({ {-50,0,25}, 500, mat, {3.14/2,0,0,1}, (cl_int)PLAN });
	mat.refraction = 1.5;
	mat.reflexion = 0.6;
	mObjects.push_back({ {-50,-20,0}, 500, mat, {0,0,0,1}, (cl_int)PLAN });

	mat.refraction = 1.5;
	mat.reflexion = 1;
	//{-100, 0, -70};
	mObjects.push_back({ {-70,0,-50}, 20, mat, {0,0,0,1}, (cl_int)CYLINDER });

	srand(12);

	for (int i = 30; i > 0; i--) {
		Material mat;

		mat.color.x = rand() / (RAND_MAX + 1.);
		mat.color.y = rand() / (RAND_MAX + 1.);
		mat.color.z = rand() / (RAND_MAX + 1.);
		mat.refraction = 0;
		mat.reflexion = 0;
		int type = rand() % 9;
		if (type >= 5 && type < 8) // reflexion
			mat.reflexion = rand() / (RAND_MAX + 1.f);
		if (type >= 8 && type < 10)
			mat.refraction = 1.5;
//		mat.reflexion = rand() / (RAND_MAX + 1.) * 0;


		cl_float3 pos;
		pos.x = rand() / (RAND_MAX / (400)) - 200;
		pos.y = rand() / (RAND_MAX / 300) - 320;
		pos.z = rand() / (RAND_MAX / (400)) - 400;

		float size = rand() / (RAND_MAX / (30-10)) + 10;

		mObjects.push_back({ pos, size, mat, SPHERE });
	}

	float fov = 36;
	mImageScale = std::tan(CL_M_PI/180*(fov * 0.5));
	mImageAspectRatio = mSizeX / static_cast<float>(mSizeY);

}


Raytracer::~Raytracer()
{

}

bool	Raytracer::init()
{
	/*Step1: Getting platforms and choose an available one.*/
	cl_uint numPlatforms; //the NO. of platforms
	cl_platform_id platform = NULL; //the chosen platform
	cl_int status = clGetPlatformIDs(0, NULL, &numPlatforms);
	if (status != CL_SUCCESS)
	{
		std::cout << "Error: Getting platforms!" << std::endl;
		return 0;
	}

	// Choosing platform
	std::cout << "[OpenCL] Available platforms :" << std::endl;
	if (numPlatforms > 0)
	{
		cl_platform_id* platforms =
			(cl_platform_id*)malloc(numPlatforms * sizeof(cl_platform_id));
		status = clGetPlatformIDs(numPlatforms, platforms, NULL);

		size_t i_platform = 0;
		platform = platforms[i_platform];

		size_t size;
		for (i_platform = 0; i_platform < numPlatforms; i_platform++) {
			status = clGetPlatformInfo(platforms[i_platform], CL_PLATFORM_NAME, 0, NULL, &size);
			char *name = (char *) malloc(size);
			status = clGetPlatformInfo(platforms[i_platform], CL_PLATFORM_NAME, size, name, NULL);

			status = clGetPlatformInfo(platforms[i_platform], CL_PLATFORM_VENDOR, 0, NULL, &size);
			char *vendor = (char *)malloc(size);
			status = clGetPlatformInfo(platforms[i_platform], CL_PLATFORM_VENDOR, size, vendor, NULL);

			status = clGetPlatformInfo(platforms[i_platform], CL_PLATFORM_EXTENSIONS, 0, NULL, &size);
			char *exts = (char *)malloc(size);
			status = clGetPlatformInfo(platforms[i_platform], CL_PLATFORM_EXTENSIONS, size, exts, NULL);

			std::cout << " Platform " << i_platform << ":\n";
			std::cout << "	Name : " << name << "\n";
			std::cout << "	Vendor : " << vendor << "\n";
			std::cout << "	Extentions : " << exts << "\n";
		}
		int tmp;
		std::cin >> tmp;
		if (tmp >= 0 || tmp < numPlatforms)
			platform = platforms[tmp];
		free(platforms);

	}

	clGetGLContextInfoKHR_fn clGetGLContextInfoKHR = (clGetGLContextInfoKHR_fn) clGetExtensionFunctionAddressForPlatform(platform, "clGetGLContextInfoKHR");

	// Interop CL-GL //
	cl_context_properties props[] =
	{
	//OpenCL platform
		CL_CONTEXT_PLATFORM, (cl_context_properties)platform,
	//OpenGL context
		CL_GL_CONTEXT_KHR,   (cl_context_properties)getCurrentGLContext(),
	//HDC used to create the OpenGL context
		CL_WGL_HDC_KHR,     (cl_context_properties) getCurrentGLDrawable(),
		0
	};

	size_t bytes = 0;
	// Notice that extension functions are accessed via pointers
	// initialized with clGetExtensionFunctionAddressForPlatform.

	// queuring how much bytes we need to read
	clGetGLContextInfoKHR(props, CL_DEVICES_FOR_GL_CONTEXT_KHR, 0, NULL, &bytes);
	// allocating the mem
	size_t devNum = bytes / sizeof(cl_device_id);
	std::vector<cl_device_id> devs(devNum);
	//reading the info
	clGetGLContextInfoKHR(props, CL_DEVICES_FOR_GL_CONTEXT_KHR, bytes, devs.data(), NULL);

	// device id's index //
	cl_device_id device_cl_gl = 0;
	clGetGLContextInfoKHR(props, CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR, sizeof(cl_device_id), &device_cl_gl, NULL);

	//looping over all devices
//	for (size_t i = 0; i < devNum; i++)
//	{
		//enumerating the devices for the type, names, CL_DEVICE_EXTENSIONS, etc
		//clGetDeviceInfo(devs[i], CL_DEVICE_TYPE, …);
		//clGetDeviceInfo(devs[i], CL_DEVICE_EXTENSIONS, …);

		//size_t valueSize;
		//clGetDeviceInfo(devs[device_id_index], CL_DEVICE_NAME, 0, NULL, &valueSize);
		//char *interop_device = (char*)malloc(valueSize);
		//clGetDeviceInfo(devs[device_id_index], CL_DEVICE_NAME, valueSize, interop_device, NULL);
		//std::cout << "[OpenCL] Interop OpenGL compatible device name : " << value << "\n";

	//}


	/*Step 2:Query the platform and choose the first GPU device if has one.Otherwise use the CPU as device.*/
	cl_uint numDevices = 0;
	cl_device_id        *devices;
	status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, NULL, &numDevices);
	std::cout << "Devices available :" << std::endl;
	if (numDevices == 0) //no GPU available.
	{
		std::cout << "No GPU device available." << std::endl;
		return false;
	}
	else
	{
		devices = (cl_device_id*)malloc(numDevices * sizeof(cl_device_id));
		status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, numDevices, devices, NULL);
	}
	for (int i = 0; i < numDevices; i++) {
		size_t valueSize;
		clGetDeviceInfo(devices[i], CL_DEVICE_NAME, 0, NULL, &valueSize);
		char *value = (char*)malloc(valueSize);
		clGetDeviceInfo(devices[i], CL_DEVICE_NAME, valueSize, value, NULL);
		std::cout << "Device " << i << ":\n";
		std::cout << " Name " << value << ":\n";
		if (device_cl_gl == devices[i])
			std::cout << "Compatible OpenGL buffer sharing" << std::endl;
	}
	std::cout << std::endl << "Choose device:";
	int device_id = 0;
	std::cin >> (device_id);
//	std::cout << "[OpenCL] Interop opengl compatible device name : " << interop_device << "\n";
	/*Step 3: Create context.*/
	mContext = clCreateContext(props, 1, devices, NULL, NULL, NULL);
	/*Step 4: Creating command queue associate with the context.*/
	mCommandQueue = clCreateCommandQueue(mContext, devices[device_id], 0, NULL);

	/*Step 5: Create program object */
	const char *filename = "HelloWorld_Kernel.cl";
	std::string sourceStr;
	status = convertToString(filename, sourceStr);
	const char *source = sourceStr.c_str();
	size_t sourceSize[] = { strlen(source) };
	mProgram = clCreateProgramWithSource(mContext, 1, &source, sourceSize, NULL);

	/*Step 6: Build program. */
	status = clBuildProgram(mProgram, 1, devices, NULL, NULL, NULL);

	if (status == CL_BUILD_PROGRAM_FAILURE) {
		// Determine the size of the log
		size_t log_size;
		clGetProgramBuildInfo(mProgram, devices[0], CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);

		// Allocate memory for the log
		char *log = (char *)malloc(log_size);

		// Get the log
		clGetProgramBuildInfo(mProgram, devices[0], CL_PROGRAM_BUILD_LOG, log_size, log, NULL);

		// Print the log
		printf("%s\n", log);
	}

	mImage = new glm::vec4[mSizeX * mSizeY];

	// Buffer Creation
	int err;
	mDirectionsBuffer = clCreateBuffer(mContext, CL_MEM_READ_WRITE,
		mSizeX * mSizeY * sizeof(cl_float) * 4, NULL, &err);
	if (err != CL_SUCCESS)
		std::cout << "Error createbuffer1" << getErrorString(err) << "\n";

	mOutputBuffer = clCreateBuffer(mContext, CL_MEM_READ_WRITE,
		mSizeX * mSizeY * sizeof(float) * 4, NULL, &err);
	if (err != CL_SUCCESS)
		std::cout << "Error createbuffer2" << getErrorString(err) << "\n";

	mPrimitivesBuffer = clCreateBuffer(mContext, CL_MEM_READ_WRITE,
		sizeof(Object) * mObjects.size(), 0, &err);
	if (err != CL_SUCCESS)
		std::cout << "Error createbuffer primitives" << getErrorString(err) << "\n";


	setDirection(glm::vec3(0));
	setOrigin(glm::vec3(0));

	/* Drawer kernel & args */

	// main kernel
	mKernelDrawScene = clCreateKernel(mProgram, "draw_scene", NULL);

	status = clSetKernelArg(mKernelDrawScene, 1, sizeof(cl_float3), (void *)&mOrigin);
	if (status != CL_SUCCESS)
		std::cout << "Error setarg origin" << getErrorString(status) << "\n";
	status = clSetKernelArg(mKernelDrawScene, 2, sizeof(cl_mem), (void *)&mDirectionsBuffer);
	if (status != CL_SUCCESS)
		std::cout << "Error setarg directions buffer" << getErrorString(status) << "\n";
	status = clSetKernelArg(mKernelDrawScene, 3, sizeof(cl_mem), (void *)&mPrimitivesBuffer);
	if (status != CL_SUCCESS)
		std::cout << "Error setarg primitives" << getErrorString(status) << "\n";
	int size = mObjects.size();
	status = clSetKernelArg(mKernelDrawScene, 4, sizeof(cl_int), (void *)&size); // 4th parameter : int nb_objects
	if (status != CL_SUCCESS)
		std::cout << "Error setarg nb_objects" << getErrorString(status) << "\n";

	// copy scene to gpu
	status = clEnqueueWriteBuffer(mCommandQueue, mPrimitivesBuffer, CL_TRUE, 0, sizeof(Object) * mObjects.size(), mObjects.data(), 0, NULL, NULL);
	if (status != CL_SUCCESS)
		std::cout << "Error writebuffer primitives" << getErrorString(status) << "\n";


	// kernel for generate first rays directions
	mKernelGenerateDirections = clCreateKernel(mProgram, "generate_rays_directions", NULL);

	// Arguments
	status = clSetKernelArg(mKernelGenerateDirections, 0, sizeof(cl_mem), (void *)&mDirectionsBuffer);
	if (status != CL_SUCCESS)
		std::cout << "Error setarg1" << getErrorString(status) << "\n";
	glm::vec4 tmp(mDirection,1);
	status = clSetKernelArg(mKernelGenerateDirections, 1, sizeof(cl_float) * 4, (void *)&tmp);
	if (status != CL_SUCCESS)
		std::cout << "Error setarg direction" << getErrorString(status) << "\n";
	status = clSetKernelArg(mKernelGenerateDirections, 2, sizeof(cl_float), (void *)&mImageScale);
	if (status != CL_SUCCESS)
		std::cout << "Error setarg scale" << getErrorString(status) << "\n";
	status = clSetKernelArg(mKernelGenerateDirections, 3, sizeof(cl_float), (void *)&mImageAspectRatio);
	if (status != CL_SUCCESS)
		std::cout << "Error setarg ratio" << getErrorString(status) << "\n";

	return mImage;
}

void Raytracer::redraw()
{
	if (mGLTexture == NULL)
		return;

	size_t global_work_size[1] = { mSizeX * mSizeY };
	cl_event event;
	int status;

	// Generate rays
	glm::vec4 tmp(mDirection, 1);
	status = clSetKernelArg(mKernelGenerateDirections, 1, sizeof(cl_float) * 4, (void *)&tmp);
	if (status != CL_SUCCESS)
		std::cout << "Error setarg direction" << getErrorString(status) << "\n";
	status = clEnqueueNDRangeKernel(mCommandQueue, mKernelGenerateDirections, 1, NULL,
		global_work_size, NULL, 0, NULL, &event);
	if (status != CL_SUCCESS)
		std::cout << "Error filling output buffer " << getErrorString(status) << "\n";
	//mEventsToWait.push_back(event);
	//mOutputBuffer;

	drawSphere(glm::vec3(1, 0, 0), glm::vec3(), 20);

	// Do nothing if we dont use opengl buffer
	if (mGLTexture != NULL)
		return;
}

void Raytracer::drawSphere(const glm::vec3 &color, const glm::vec3 & position, float R)
{
	cl_event event;
	size_t global_work_size[1] = { mSizeX * mSizeY };

	int status = clSetKernelArg(mKernelDrawScene, 1, sizeof(cl_float3), (void *)&mOrigin);
	if (status != CL_SUCCESS)
		std::cout << "Error setarg origin" << getErrorString(status) << "\n";
	static auto from_beginning = std::chrono::high_resolution_clock::now();
	auto time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - from_beginning).count();
	status = clSetKernelArg(mKernelDrawScene, 5, sizeof(cl_int), (void *)&time);
	if (status != CL_SUCCESS)
		std::cout << "Error setarg origin" << getErrorString(status) << "\n";

	clEnqueueAcquireGLObjects(mCommandQueue, 1, &mGLTexture, 0, NULL, NULL);

	status = clEnqueueNDRangeKernel(mCommandQueue, mKernelDrawScene, 1, NULL,
		global_work_size, NULL, mEventsToWait.size(), mEventsToWait.data(), &event);
	if (status != CL_SUCCESS)
		std::cout << "Error draw_scene" << getErrorString(status) << "\n";
	mEventsToWait.clear();
	mEventsToWait.erase(mEventsToWait.begin(), mEventsToWait.end());

	clEnqueueReleaseGLObjects(mCommandQueue, 1, &mGLTexture, 0, NULL, NULL);

	clFinish(mCommandQueue);
}

glm::vec4 * Raytracer::getImage()
{
	return mImage;
}

void Raytracer::setOpenGLOutputTexture(GLuint texture_id, GLint mipmap_level, GLenum texture_type)
{
	int status;
	mGLTexture = clCreateFromGLTexture2D(mContext, CL_MEM_READ_WRITE, texture_type, mipmap_level, texture_id, &status);
	if (status != CL_SUCCESS) {
		std::cout << "Error create opencl buffer from opengl texture: " << getErrorString(status) << "\n";
		mGLTexture = NULL;
		return;
	}

	std::cout << "AFTER CREATE FROM GL\n";

	status = clSetKernelArg(mKernelDrawScene, 0, sizeof(cl_mem), (void *)&mGLTexture);
	if (status != CL_SUCCESS)
		std::cout << "Error setarg output" << getErrorString(status) << "\n";

	std::cout << "AFTER SET ARG\n";

}

void Raytracer::setDirection(const glm::vec3 & rot)
{
	mDirection = rot;
	return;

	/*Step 7: Initial input,output for the host and create memory objects for the kernel*/
	int img_x = mSizeX;
	int img_y = mSizeY;
	// Generate vectors
	for (int x = 0; x < img_x; x++) {
		for (int y = 0; y < img_y; y++) {
			mVectors[y * img_x + x] = glm::vec4(glm::normalize(glm::vec3(x - img_x / 2, y - img_y / 2, 2000)),0);
			mVectors[y * img_x + x] = glm::quat(rot) * mVectors[y * img_x + x];
			//if (y >798)//(x == 1290 / 2 && y == 400)
				//std::cout << mVectors[y * img_x + x].x << " " << mVectors[y * img_x + x].y << " " << mVectors[y * img_x + x].z << "\n";
			//if (y < 100)
				//mVectors[y * img_x + x] = glm::vec3(1, 0, 0);
		}
	}
	return;/*
	static cl_event *event;
	if (event == NULL) {
		event = new cl_event;
	} else {
		size_t info;
		clGetEventInfo(*event, CL_EVENT_COMMAND_EXECUTION_STATUS, sizeof(size_t), &info, NULL);
		if (info != CL_COMPLETE)
			return;
	}


	size_t info;
	clGetEventInfo(*event, CL_EVENT_COMMAND_EXECUTION_STATUS, sizeof(size_t), &info, NULL);
	std::cout << "Copy status:" << getEventStatusString(info) << "\n";*/
}

void Raytracer::setOrigin(const glm::vec3 & origin)
{
	mOrigin = origin;
	cl_event event;
	//int status = clEnqueueWriteBuffer(mCommandQueue, mOriginBuffer, CL_TRUE, 0, sizeof(glm::vec3), &mOrigin, 0, NULL, &event);
	//if (status != CL_SUCCESS)
		//std::cout << "Error readbuffer" << getErrorString(status) << "\n";
	//mEventsToWait.push_back(event);
}

const glm::vec3 & Raytracer::getOrigin()
{
	return mOrigin;
}

const glm::vec3 & Raytracer::getDirection()
{
	return mDirection;
}

const char *Raytracer::getEventStatusString(cl_int error)
{
	switch (error)
	{
	case CL_QUEUED: return "CL_QUEUED";
	case CL_SUBMITTED: return "CL_SUBMITTED";
	case CL_RUNNING: return "CL_RUNNING";
	case CL_COMPLETE: return "CL_COMPLETE";
	default:
		break;
	}
}

const char *Raytracer::getErrorString(cl_int error)
{
	switch (error) {
		// run-time and JIT compiler errors
	case 0: return "CL_SUCCESS";
	case -1: return "CL_DEVICE_NOT_FOUND";
	case -2: return "CL_DEVICE_NOT_AVAILABLE";
	case -3: return "CL_COMPILER_NOT_AVAILABLE";
	case -4: return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
	case -5: return "CL_OUT_OF_RESOURCES";
	case -6: return "CL_OUT_OF_HOST_MEMORY";
	case -7: return "CL_PROFILING_INFO_NOT_AVAILABLE";
	case -8: return "CL_MEM_COPY_OVERLAP";
	case -9: return "CL_IMAGE_FORMAT_MISMATCH";
	case -10: return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
	case -11: return "CL_BUILD_PROGRAM_FAILURE";
	case -12: return "CL_MAP_FAILURE";
	case -13: return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
	case -14: return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
	case -15: return "CL_COMPILE_PROGRAM_FAILURE";
	case -16: return "CL_LINKER_NOT_AVAILABLE";
	case -17: return "CL_LINK_PROGRAM_FAILURE";
	case -18: return "CL_DEVICE_PARTITION_FAILED";
	case -19: return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";

		// compile-time errors
	case -30: return "CL_INVALID_VALUE";
	case -31: return "CL_INVALID_DEVICE_TYPE";
	case -32: return "CL_INVALID_PLATFORM";
	case -33: return "CL_INVALID_DEVICE";
	case -34: return "CL_INVALID_CONTEXT";
	case -35: return "CL_INVALID_QUEUE_PROPERTIES";
	case -36: return "CL_INVALID_COMMAND_QUEUE";
	case -37: return "CL_INVALID_HOST_PTR";
	case -38: return "CL_INVALID_MEM_OBJECT";
	case -39: return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
	case -40: return "CL_INVALID_IMAGE_SIZE";
	case -41: return "CL_INVALID_SAMPLER";
	case -42: return "CL_INVALID_BINARY";
	case -43: return "CL_INVALID_BUILD_OPTIONS";
	case -44: return "CL_INVALID_PROGRAM";
	case -45: return "CL_INVALID_PROGRAM_EXECUTABLE";
	case -46: return "CL_INVALID_KERNEL_NAME";
	case -47: return "CL_INVALID_KERNEL_DEFINITION";
	case -48: return "CL_INVALID_KERNEL";
	case -49: return "CL_INVALID_ARG_INDEX";
	case -50: return "CL_INVALID_ARG_VALUE";
	case -51: return "CL_INVALID_ARG_SIZE";
	case -52: return "CL_INVALID_KERNEL_ARGS";
	case -53: return "CL_INVALID_WORK_DIMENSION";
	case -54: return "CL_INVALID_WORK_GROUP_SIZE";
	case -55: return "CL_INVALID_WORK_ITEM_SIZE";
	case -56: return "CL_INVALID_GLOBAL_OFFSET";
	case -57: return "CL_INVALID_EVENT_WAIT_LIST";
	case -58: return "CL_INVALID_EVENT";
	case -59: return "CL_INVALID_OPERATION";
	case -60: return "CL_INVALID_GL_OBJECT";
	case -61: return "CL_INVALID_BUFFER_SIZE";
	case -62: return "CL_INVALID_MIP_LEVEL";
	case -63: return "CL_INVALID_GLOBAL_WORK_SIZE";
	case -64: return "CL_INVALID_PROPERTY";
	case -65: return "CL_INVALID_IMAGE_DESCRIPTOR";
	case -66: return "CL_INVALID_COMPILER_OPTIONS";
	case -67: return "CL_INVALID_LINKER_OPTIONS";
	case -68: return "CL_INVALID_DEVICE_PARTITION_COUNT";

		// extension errors
	case -1000: return "CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR";
	case -1001: return "CL_PLATFORM_NOT_FOUND_KHR";
	case -1002: return "CL_INVALID_D3D10_DEVICE_KHR";
	case -1003: return "CL_INVALID_D3D10_RESOURCE_KHR";
	case -1004: return "CL_D3D10_RESOURCE_ALREADY_ACQUIRED_KHR";
	case -1005: return "CL_D3D10_RESOURCE_NOT_ACQUIRED_KHR";
	default: return "Unknown OpenCL error";
	}
}
