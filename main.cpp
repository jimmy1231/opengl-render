#include <math.h>
#include <vector>
#include <string.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <chrono>
#include <thread>

/*
 glad is the OpenGL library loader, basically, it discovers where all the
 openGL functions are in the process memory, then dynamically binds it
 to the invocation addresses (either through PLT or GOT).
 They do something clever like this:
 
 #define glCompileProgram glad_glCompileProgram
 So, when we call glCompileProgram(), it is actually invoking
 the glad_glCompileProgram function 
*/
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/geometric.hpp>

#include "helpers.h"

// Custom header files
#include "bind_shaders.h"
#include "ShaderProg.h"
#include "load_shader_prog.h"
#include "bind_vao.h"
#include "transformation_matrices.h"

#define _DEBUG_LOOP_LOGS_ 0
#define FPS 20
#define MAX_MS_PER_FRAME 1000 / FPS /* milliseconds per frame */
#define WIDTH_PIXELS 1400 
#define HEIGHT_PIXELS 900 
#define DEG_TO_RAD 0.0174533

#define time_p          std::chrono::high_resolution_clock::time_point
#define duration        std::chrono::duration
#define clock           std::chrono::high_resolution_clock
#define duration_cast   std::chrono::duration_cast

/*
 * g : gaze direction (normalized)
 * e : eye position
 * t : "up" vector (in world coordinates)
 */
glm::vec3 e(2, 2, 3);
glm::vec3 g;
glm::vec3 t(0,1,0);

glm::mat4 M_per, M_cam;
glm::vec2 *last_cursor_pos = NULL;
bool LEFT_MOUSE_BTN_PRESSED = false;


void window_callback_scroll(const double &xoffset, const double &yoffset) {
  // Get direction of gaze, and adjust towards/away that direction
  glm::vec3 _d = 0.5f * glm::normalize(g);
  
  if (yoffset > 0) {
    e = e+_d; 
  } else {
    e = e-_d;
  }
}

void window_callback_mouse_btn(int btn, int action, int mods) {
  if (btn == GLFW_MOUSE_BUTTON_LEFT) {
    if (action == GLFW_PRESS) {
      LEFT_MOUSE_BTN_PRESSED = true;
    } else {
      assert(action == GLFW_RELEASE);
      LEFT_MOUSE_BTN_PRESSED = false;
      free(last_cursor_pos);
      last_cursor_pos = NULL;
    }
  }
  /* Add more mouse events here (i.e. GLFW_MOUSE_BUTTON_RIGHT) */
}

void window_callback_cursor_pos(float xpos, float ypos) {
  if (!LEFT_MOUSE_BTN_PRESSED) {
    return;
  }

  glm::vec2 curr_cursor_pos = glm::vec2(xpos, ypos);
  if (last_cursor_pos == NULL) {
    last_cursor_pos = (glm::vec2 *)malloc(sizeof(glm::vec2));
    *last_cursor_pos = glm::vec2(xpos, ypos);
  }
  
  #define curr curr_cursor_pos
  #define prev last_cursor_pos
  glm::vec2 delta = curr-(*prev);
  if (delta.x == 0) {
    return;
  }

  /*
   * left (delta.x < 0) -> clockwise
   * right (delta.x > 0) -> counter-clockwise
   * Treat delta.x (in pixels) as degrees - see how it works out
   *  - add a little smoothing by capping the angle
   */
  float rad;
  rad = fmin((float)(DEG_TO_RAD * -delta.x)/20.0f, 0.5f);
  
  // Rotate about y-axis by rad
  glm::mat3 R;
  R = gcc_test::rot_about(rad, glm::vec3(0,1,0));
  
  e = R*e;
  g = glm::normalize(
    glm::vec3(0.0f,0.0f,0.0f)-e);

  // Finally, reset prev (i.e. last_cursor_pos)
  *prev = curr;
}

int main(int argc, char *argv[]) {
  g = glm::normalize(glm::vec3(0.0f,0.0f,0.0f)-e);

  glfwInit();
  
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

  GLFWwindow *window = glfwCreateWindow(WIDTH_PIXELS, HEIGHT_PIXELS,
    "OpenGL", NULL, NULL) ;
  glfwMakeContextCurrent(window);

  // glad dynamically discovers and binds all openGL functions
  // so we can call them (e.g. glUseProgram) 
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    printf("FAILED TO LOAD GLAD!\n"); 
    return EXIT_FAILURE;
  }

  GLuint VAO, prog_id;
  bind_vao(VAO);
  std::vector<ShaderProg *> shader_progs;
  ShaderProg vs, fs;
  GLuint shader_ids[2];
  {
    vs.shader_name = "../glp/passthrough.vs";
    fs.shader_name = "../glp/passthrough.fs";
    vs.type = GL_VERTEX_SHADER;
    fs.type = GL_FRAGMENT_SHADER;
    load_shader_prog(vs);
    load_shader_prog(fs);
    shader_progs.push_back(&vs);
    shader_progs.push_back(&fs);
  }
  bind_shaders(shader_progs, prog_id, shader_ids);

  /*
   * Note: we do not need the M_vp (i.e. viewport transformation)
   * because OpenGL does this automatically for us in the
   * vertex processing stage (right after the vertex shader).
   * It applies the viewport transformation since it knows 
   * the screen width, height, and depth of our vertices.
   *
   * Note: the perspective (i.e. projection) matrix does not
   * change in between frames, so we can create it once, and
   * reuse it.
  */
  GLint M_proj_id, M_per_id, M_cam_id;
  init_per_mat(-1,1,-1,1,0.1f, 100.0f,M_per);
  init_camera_mat(g, t, e, M_cam);
  std::cout << "Program: " << prog_id << std::endl;
  std::cout << "VS: " << shader_ids[0] << std::endl;
  std::cout << "FS: " << shader_ids[1] << std::endl;

  glfwSetScrollCallback(
    window,
    [](GLFWwindow *window, double xoffset, double yoffset) {
      window_callback_scroll(xoffset, yoffset);
    }
  );

  glfwSetMouseButtonCallback(
    window,
    [](GLFWwindow *window, int btn, int action, int mods) {
      window_callback_mouse_btn(btn, action, mods);
    }
  );

  glfwSetCursorPosCallback(window,
    [](GLFWwindow *window, double xpos, double ypos) {
      window_callback_cursor_pos((float)xpos, (float)ypos);
    }
  );
  
  time_p tic, toc;
  duration<int, std::milli> fps(MAX_MS_PER_FRAME);
  while (!glfwWindowShouldClose(window)) {
    tic = clock::now();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Bind the shaders
    glUseProgram(prog_id);
    
    GLuint vs_id = shader_ids[0];
    M_per_id = glGetUniformLocation(prog_id, "M_per");
    M_cam_id = glGetUniformLocation(prog_id, "M_cam");

    // Send uniform variables to device
    init_camera_mat(g, t, e, M_cam);
    glUniformMatrix4fv(M_per_id, 1, GL_FALSE, glm::value_ptr(M_per));
    glUniformMatrix4fv(M_cam_id, 1, GL_FALSE, glm::value_ptr(M_cam));

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    // Unbind the shaders
    glUseProgram(0);

    glfwSwapBuffers(window);
    glfwPollEvents();

    toc = clock::now();
    auto elapsed_ms = duration_cast<std::chrono::milliseconds>(toc-tic);
    if (elapsed_ms < fps) {
      auto left = fps-elapsed_ms;
      std::this_thread::sleep_for(left);
    } 
  }

  glfwTerminate();
}