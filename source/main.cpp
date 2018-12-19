/**
 * @Author: danielb
 * @Date:   2017-07-22T23:35:22+02:00
 * @Last modified by:   daniel_b
 * @Last modified time: 2018-03-16T21:09:16+01:00
 */

#include <fse/Window.hpp>
#include <fse/Renderer/Renderer.hpp>
#include <fse/Renderer/ObjectPicker.hpp>
#include <fse/Scene/Object/Object.hpp>
#include <fse/Scene/Object/Wavefront.hpp>
#include <fse/Scene/DynamicScene.hpp>
#include <fse/Scene/CameraFPS.hpp>
#include <fse/Ui/Surface.hpp>
#include <fse/Ui/Text.hpp>
#include <fse/Ui/LayoutVertical.hpp>
#include <fse/Ui/LayoutHorizontal.hpp>
#include <fse/Ui/Button.hpp>

#include <Raytracer.h>

#include "Map.hpp"

#include <chrono>
#include <thread>

#include <CL/cl.h>

#include <CL/cl_gl.h>
#include <CL/opencl.h>
#include <GL/glew.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext.hpp>
#include <glm/gtx/string_cast.hpp>

using namespace fse::scene::object;

#if defined(_WIN32)

#include <direct.h>
#include <stdlib.h>
#include <stdio.h>

std::string getcwd_string(void) {
	char buff[256];
	_getcwd(buff, 256);
	std::string cwd(buff);
	return cwd;
}

#endif

int main(int argc, char **argv)
{

	Window                      window(1290, 720);
	window.captureMouse(true);

	Raytracer rt;
	rt.init();
	rt.setDirection(glm::vec3(0, 0, 0));
	rt.setOrigin(glm::vec3(0, 5, -150));
	rt.redraw();

	std::cout << "[OpenGL]\nVendor: " << glGetString(GL_VENDOR) << "\nRenderer: ";
	std::cout << glGetString(GL_RENDERER) << "\nVersion: ";
	std::cout << glGetString(GL_VERSION) << "\nGLSL Version: ";
	std::cout << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";

	std::shared_ptr<fse::gl_item::Shader> shader = std::make_shared<fse::gl_item::Shader>("shader/basic.vert", "shader/basic.frag");
	shader->useProgram();
	fse::gl_item::Buffer<glm::vec3>				vertex_buffer(fse::gl_item::Buffer<glm::vec3>::ArrayBuffer, fse::gl_item::Buffer<glm::vec3>::DynamicBuffer);

	std::vector<glm::vec3> vertexes;
	vertexes.push_back(glm::vec3(-1, -1, 0));
	vertexes.push_back(glm::vec3(1, -1, 0));
	vertexes.push_back(glm::vec3(1, 1, 0));
	vertexes.push_back(glm::vec3(-1, 1, 0));

	vertex_buffer.send(vertexes);

	glm::vec4 *image = rt.getImage();

	shader->setAttribute(vertex_buffer, 0, 3);
	std::cout << "[OpenCL] data[2]=" << image[0].z;
	std::shared_ptr<fse::gl_item::Texture>	texture = fse::gl_item::Texture::create(1290, 720,
																					fse::gl_item::Texture::InternalFormat::RGBA8,
																					fse::gl_item::Texture::Format::RGBA,
																					fse::gl_item::Texture::Type::UNSIGNED_BYTE);

	//texture = fse::gl_item::Texture::load("./Ressource/water.jpg");
	texture->activate(0);
	texture->bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	std::cout << "AFTER TEXTURE CREATION\n";

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1290, 720, 0, GL_RGBA,
		GL_UNSIGNED_BYTE, NULL); 

	std::cout << "AFTER IMAGE DATA\n";

	rt.setOpenGLOutputTexture(texture->getId(), 0, GL_TEXTURE_2D);


	//shader->setAttribute(gl_image, 1, 3);

	fse::gl_item::Shader::AttributeHolder attribute;

	attribute.addUniform("color_in", glm::vec3(0.9,0.3,0.3));
	attribute.apply(shader);
	std::map<int, bool> key;

	std::chrono::high_resolution_clock::time_point last_frame = std::chrono::high_resolution_clock::now();
	std::chrono::high_resolution_clock::time_point last_second = last_frame;
	int fps = 30;
	int fps_targetted = 30;
	int frame_counter = 0;

	bool mouse_capture = true;

	while (1)
	{

		rt.redraw();
		//texture->data(image);
		window.flipScreen();
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);


		GLenum err;
		while ((err = glGetError()) != GL_NO_ERROR)
		{
			std::cout << "[ERROR]  err = " << err << "\n";
		}

		std::chrono::high_resolution_clock::time_point actual_frame = std::chrono::high_resolution_clock::now();
		auto from_last_second = std::chrono::duration_cast<std::chrono::milliseconds>(actual_frame - last_second).count();
		auto from_last_frame = std::chrono::duration_cast<std::chrono::milliseconds>(actual_frame - last_frame).count();
		if (from_last_second >= 1000) {
			last_second = actual_frame;
			fps = frame_counter;
			frame_counter = 0;
			window.setTitle("FPS: " + std::to_string(fps));
		}
		frame_counter++;
		if (from_last_frame > 1000/fps_targetted && 0)
			std::this_thread::sleep_for(std::chrono::milliseconds(1000 / fps_targetted - from_last_frame));
		last_frame = actual_frame;

		float modifier = 1.f / fps * 20.f;

		SDL_Event event;

		float horizontal_angle = rt.getDirection().y;
		float vertical_angle = rt.getDirection().x;
		auto direction = glm::vec3(sin(horizontal_angle) * cos(vertical_angle),
			sin(-vertical_angle),
			cos(horizontal_angle) * cos(vertical_angle));
		direction = glm::normalize(direction);
		glm::vec3 right = glm::normalize(glm::vec3(sin(horizontal_angle - 3.14f / 2.0f),
			0,
			cos(horizontal_angle - 3.14f / 2.0f)));
		glm::vec3 up = glm::cross(direction, right);

		if (key[SDLK_LEFT]) {
			rt.setOrigin(rt.getOrigin() + right * modifier);
		}
		if (key[SDLK_RIGHT]) {
			rt.setOrigin(rt.getOrigin() + -right * modifier);
		}
		if (key[SDLK_UP]) {
			rt.setOrigin(rt.getOrigin() + direction * modifier);
		}
		if (key[SDLK_DOWN]) {
			rt.setOrigin(rt.getOrigin() + -direction * modifier);
		}
		if (key[SDLK_PAGEUP]) {
			rt.setOrigin(rt.getOrigin() + -up * modifier);
		}
		if (key[SDLK_PAGEDOWN]) {
			rt.setOrigin(rt.getOrigin() + up * modifier);
		}
		if (key[SDLK_w]) {
			rt.setDirection(rt.getDirection() + glm::vec3(0.05, 0, 0) * modifier);
		}
		if (key[SDLK_s]) {
			rt.setDirection(rt.getDirection() - glm::vec3(0.05, 0, 0) * modifier);
		}
		if (key[SDLK_a]) {
			rt.setDirection(rt.getDirection() - glm::vec3(0.0, 0.05, 0) * modifier);
		}
		if (key[SDLK_d]) {
			rt.setDirection(rt.getDirection() + glm::vec3(0.0, 0.05, 0) * modifier);
		}

		int max_poll = 5;
		while (window.pollEvent(event) && max_poll > 0)
		{
			max_poll--;
			if (event.type == SDL_MOUSEBUTTONDOWN) {
				if (event.button.button == SDL_BUTTON_LEFT) {
				}
				else if (event.button.button == SDL_BUTTON_RIGHT) {
				}
			}
			if (event.type == SDL_KEYDOWN)
				key[event.key.keysym.sym] = true;
			else if (event.type == SDL_KEYUP)
				key[event.key.keysym.sym] = false;


			if (event.type == SDL_KEYDOWN) {
/*				if (event.key.keysym.sym == SDLK_LEFT) {
					rt.setOrigin(rt.getOrigin() + glm::vec3(-1, 0, 0) * modifier);
				}
				else if (event.key.keysym.sym == SDLK_RIGHT) {
					rt.setOrigin(rt.getOrigin() + glm::vec3(1, 0, 0) * modifier);
				}
				else if (event.key.keysym.sym == SDLK_UP) {
					rt.setOrigin(rt.getOrigin() + glm::vec3(0, 0, 1) * modifier);
				}
				else if (event.key.keysym.sym == SDLK_DOWN) {
					rt.setOrigin(rt.getOrigin() + glm::vec3(0, 0, -1) * modifier);
				}
				else if (event.key.keysym.sym == SDLK_PAGEUP) {
					rt.setOrigin(rt.getOrigin() + glm::vec3(0, 1, 0) * modifier);
				}
				else if (event.key.keysym.sym == SDLK_PAGEDOWN) {
					rt.setOrigin(rt.getOrigin() + glm::vec3(0, -1, 0) * modifier);
				}
				else if (event.key.keysym.sym == SDLK_w) {
					rt.setDirection(rt.getDirection() + glm::vec3(0.05, 0, 0) * modifier);
				}
				else if (event.key.keysym.sym == SDLK_s) {
					rt.setDirection(rt.getDirection() - glm::vec3(0.05, 0, 0) * modifier);
				}
				else if (event.key.keysym.sym == SDLK_a) {
					rt.setDirection(rt.getDirection() - glm::vec3(0.0, 0.05, 0) * modifier);
				}
				else if (event.key.keysym.sym == SDLK_d) {
					rt.setDirection(rt.getDirection() + glm::vec3(0.0, 0.05, 0) * modifier);
				}
*/
				if (event.key.keysym.sym == SDLK_e) {
				}

				if (event.key.keysym.sym == SDLK_ESCAPE)
				{
					window.close();
					return (0);
				}
				if (event.key.keysym.sym == SDLK_SPACE) {
					shader->updateShader();
				}
			}

			if (event.type == SDL_MOUSEBUTTONDOWN) {
				mouse_capture = !mouse_capture;
				window.captureMouse(mouse_capture);
			}

			if (event.type == SDL_MOUSEMOTION && mouse_capture)
			{
				float mouse_speed = 0.01;
				int x_relative;
				int y_relative;
				SDL_GetRelativeMouseState(&x_relative, &y_relative);
				static float vertical_angle = 0;
				static float horizontal_angle = 0;
				vertical_angle += mouse_speed * y_relative * modifier;
				horizontal_angle += mouse_speed * x_relative * modifier;
				rt.setDirection(glm::vec3(vertical_angle, horizontal_angle, 0));
				/*
				std::cout << "vertical=" << vertical_angle << " hortiz=" << horizontal_angle << "\n";
				//vertical_angle = (float)glm::clamp((vertical_angle, -M_PI / 2., M_PI + M_PI / 2));

				auto direction = glm::vec3(sin(horizontal_angle) * cos(vertical_angle),
					sin(vertical_angle),
					sin(horizontal_angle) * sin(vertical_angle)); 
				direction = glm::normalize(direction);
				rt.setDirection(direction);*/
			}
		} // end event polling
	}

	return (0);
}

