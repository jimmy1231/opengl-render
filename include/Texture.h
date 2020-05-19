#ifndef __RENDER_TEXTURE_H__
#define __RENDER_TEXTURE_H__

#include <glad/glad.h>
#include <string>
#include "Scene.h"
#include "transformation_matrices.h"
#include "_v1/load_tex.h"

static bool check_fbo_status(GLuint fbo, GLenum target) {
  GLenum fboStatus = glCheckFramebufferStatus(target);
  bool complete = false;
  std::string str;
  switch (fboStatus) {
    case GL_FRAMEBUFFER_COMPLETE:
      str = "COMPLETE";
      complete = true;
      break;
    case GL_FRAMEBUFFER_UNDEFINED:
      str = "UNDEFINED";
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
      str = "INCOMPLETE_ATTACHMENT";
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
      str = "tINCOMPLETE_MISSING_ATTACHMENT";
      break;
    case GL_FRAMEBUFFER_UNSUPPORTED:
      str = "FRAMEBUFFER_UNSUPPORTED";
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
      str = "FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
      break;
    default:
      break;
  }

  printf("Framebuffer %d: %s\n", fbo, 
    str.c_str());
  return complete;
}

class Texture {
  public:
    /* Which slot to attach the shadow texture to in FBO */
    GLenum mode;

    unsigned char *data = NULL;
    std::string file;
    int width;
    int height;
    glm::mat4 per, view;
    GLuint fbo_id;
    GLuint id;

    Texture(std::string f) : file(f) {
      glGenTextures(1, &id);
      data = load_tex(file, width, height);
      glBindTexture(GL_TEXTURE_2D, id);

      glTexParameteri(GL_TEXTURE_2D, 
                      GL_TEXTURE_WRAP_S, 
                      GL_CLAMP);
      glTexParameteri(GL_TEXTURE_2D, 
                      GL_TEXTURE_WRAP_T, 
                      GL_CLAMP);
      glTexParameteri(GL_TEXTURE_2D, 
                      GL_TEXTURE_MAG_FILTER, 
                      GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, 
                      GL_TEXTURE_MIN_FILTER, 
                      GL_LINEAR);

      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 
        height, width, 0, 
        GL_RGB, GL_UNSIGNED_BYTE, data);

      glActiveTexture(0);
      glBindTexture(GL_TEXTURE_2D, 0);
    }
    Texture(unsigned char *d, GLenum m, int w, int h) 
      : data(d), mode(m), width(w), height(h) 
    {
      glGenTextures(1, &id);
      glGenFramebuffers(1, &fbo_id);
      glBindFramebuffer(GL_FRAMEBUFFER, fbo_id);
      glBindTexture(GL_TEXTURE_2D, id);

      glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32, w, h);

      glTexParameteri(GL_TEXTURE_2D,
                      GL_TEXTURE_WRAP_T,
                      GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, 
                      GL_TEXTURE_WRAP_S, 
                      GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, 
                      GL_TEXTURE_MIN_FILTER, 
                      GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, 
                      GL_TEXTURE_MAG_FILTER, 
                      GL_LINEAR);

      /* 
       * Set up a texture evaluation function:
       * REF (r): current interpolated texture coordinate.
       * TEXTURE (Dt): depth texture value sampled from the 
       *               currently bound depth texture.
       * Says:
       * Every fragment 'r' that has a depth value <= the
       * the bound depth texture will have a value of 1.0,
       *
       * Formally:
       *    result = r <= Dt ? 1.0 : 0
       */
      glTexParameteri(GL_TEXTURE_2D, 
                      GL_TEXTURE_COMPARE_MODE,
                      GL_COMPARE_REF_TO_TEXTURE);
      glTexParameteri(GL_TEXTURE_2D, 
                      GL_TEXTURE_COMPARE_FUNC, 
                      GL_LEQUAL);

      glFramebufferTexture(GL_FRAMEBUFFER,
                           GL_DEPTH_ATTACHMENT, 
                           id, 0);

      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      glBindTexture(GL_TEXTURE_2D, 0);
    }
    Texture() {}
    ~Texture() {
      if (data != NULL) {
        free(data);
      }
      glDeleteTextures(1, &id);
    }
    void bind_to_unit(GLuint i) {
      glActiveTexture(GL_TEXTURE0 + i);
      {
        GLint _i;
        glGetIntegerv(GL_ACTIVE_TEXTURE, &_i);
        assert(_i == (GL_TEXTURE0 + i));        
      }
      glBindTexture(GL_TEXTURE_2D, id);
      {
        GLint _i;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &_i);
        assert(_i == id);        
      }
    }
    void shadow_map(const GLuint prog_id,
                    Scene &scene,
                    const glm::vec3 &g, 
                    const glm::vec3 &t,
                    const glm::vec3 &e) {
      {
        init_camera_mat(g, t, e, view);
        per = glm::perspective(
          glm::radians(30.0f),
          1400.0f/900.0f,
          0.1f, 100.0f 
        );
      }

      // Bind the shaders
      glUseProgram(prog_id);
      glBindFramebuffer(GL_FRAMEBUFFER, fbo_id);
      static const GLenum draw_buffers[] = { GL_DEPTH_ATTACHMENT };
      glDrawBuffers(1, draw_buffers);
      if (!check_fbo_status(fbo_id, GL_DRAW_FRAMEBUFFER)) {
        exit(1); 
      }

      glEnable(GL_DEPTH_TEST);
      glEnable(GL_CULL_FACE);
      glCullFace(GL_FRONT);
      glClearColor(1.0f,1.0f,1.0f,1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


      GLuint per_id, view_id;
      GLuint model_id;
      GLuint e_id;
      per_id = glGetUniformLocation(prog_id, "M_per");
      view_id = glGetUniformLocation(prog_id, "M_cam");
      glUniformMatrix4fv(per_id, 1, false,
        scene.orient.perspective(1400, 900));
      glUniformMatrix4fv(view_id, 1, false, 
        (const GLfloat *)&view);

      Data *data;
      GLint M_model_id;
      for (Model &model : scene.models) {
        data = model.data_;
        std::cout << data->filename << std::endl;
        data->bind_VAO();
        glBindVertexArray(data->vao);

        M_model_id = glGetUniformLocation(prog_id, "M_model");
        glUniformMatrix4fv(M_model_id, 1, false,
          model.model());
        glDrawArrays(GL_TRIANGLES, 0, data->size());
      } 

      // Resetting GL to defaults
      glBindVertexArray(0);
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      glUseProgram(0);
      glDisable(GL_DEPTH_TEST);
      glDisable(GL_CULL_FACE);
    }
};

#endif /* __RENDER_TEXTURE_H__ */