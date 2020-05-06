#ifndef GCC_TEST_LOAD_OBJ
#define GCC_TEST_LOAD_OBJ

#include <string>
#include "bind_vao.h"
#include <fstream>
#include <iostream>
#include <list>
#include "helpers.h"

#define SPACE_CHAR " "
#define SLASH_CHAR "/"

namespace ld_o {
  typedef struct V { /* Vertices */ 
    float x;
    float y;
    float z;
    float w;
  } V;

  typedef struct VT { /* Textures */ 
    float u;
    float v;
    float w;
  } VT;

  typedef struct VN { /* Vertex Normals */
    float x;
    float y;
    float z;
  } VN;

  typedef struct VP { /* Free-form Geometry Statement */
    float u;
    float v;
    float w;
  } VP;

  typedef struct F { /* One vertex of a Face - polygon */
    int v;
    int vt;
    int vn;
  } F;
  
  typedef struct VBO_STRUCT {
    glm::vec4 v;
    glm::vec3 n;
    glm::vec3 t;
  } VBO_STRUCT;

  typedef std::vector<F *> Face;
}

ld_o::V* handle_v(std::list<std::string> &chks) {
  ld_o::V *v;
  v = (ld_o::V *)malloc(sizeof(ld_o::V));
  if (v == NULL) {
    std::cout << "malloc failed" << std::endl;
    exit(1); 
  } 

  v->x = std::stof(chks.front());
  chks.pop_front();
  v->y = std::stof(chks.front());
  chks.pop_front();
  v->z = std::stof(chks.front());
  chks.pop_front();

  if (chks.size() > 0)
    v->w = std::stof(chks.front());
  else 
    v->w = 1.0f; /* default */

  return v;
}

ld_o::VN* handle_vn(std::list<std::string> &chks) {
  ld_o::VN *vn;
  vn = (ld_o::VN *)malloc(sizeof(ld_o::VN));
  if (vn == NULL) {
    std::cout << "malloc failed" << std::endl;
    exit(1); 
  } 

  vn->x = std::stof(chks.front());
  chks.pop_front();
  vn->y = std::stof(chks.front());
  chks.pop_front();
  vn->z = std::stof(chks.front());
  chks.pop_front();

  return vn;
}

ld_o::Face handle_f(std::list<std::string> &chks) {
  ld_o::Face face;

  std::list<int> v, vt, vn;
  std::string chk;
  int ind;
  while (chks.size() > 0) {
    chk = chks.front();
    chks.pop_front();

    ld_o::F *f;
    f = (ld_o::F *)malloc(sizeof(ld_o::F));
    if (f == NULL) {
      std::cout << "malloc failed" << std::endl;
      exit(1); 
    } 
  
    /* Split chk by delimiter */
    int i=0, j=0;
    int cnt=0;
    while (i < chk.length()) {
      j = chk.find(SLASH_CHAR, i);
      if (j == std::string::npos) {
        /*
         * Edge case: last element does not have a 
         * terminating space
         */
        j = chk.length();
      }

      int c;
      c = std::stof(chk.substr(i, j-i));

      int *ptr;
      if (cnt == 0)
        ptr = &f->v;
      else if (cnt == 1)
        ptr = &f->vt;
      else if (cnt == 2)
        ptr = &f->vn;

      *ptr = i != j ? c : -1.0f; /* Set to -1.0 if not applicable */

      cnt++;
      i = j+1;
    }

    face.push_back(f);
  }

  return face;
}

ld_o::VT* handle_vt(std::list<std::string> &chks) {
  ld_o::VT *vt;
  vt = (ld_o::VT *)malloc(sizeof(ld_o::VT));
  if (vt == NULL) {
    std::cout << "malloc failed" << std::endl;
    exit(1); 
  } 

  vt->u = std::stof(chks.front());
  chks.pop_front();
  if (chks.size() > 0) {
    vt->v = std::stof(chks.front());
    chks.pop_front();
    if (chks.size() > 0)
      vt->w = std::stof(chks.front());
  }

  return vt;  
}

ld_o::VP* handle_vp(std::list<std::string> &chks) {
  ld_o::VP *vp;
  vp = (ld_o::VP *)malloc(sizeof(ld_o::VP));
  if (vp == NULL) {
    std::cout << "malloc failed" << std::endl;
    exit(1); 
  } 

  vp->u = std::stof(chks.front());
  chks.pop_front();
  if (chks.size() > 0) {
    vp->v = std::stof(chks.front());
    chks.pop_front();
    if (chks.size() > 0)
      vp->w = std::stof(chks.front());
  }

  return vp;
}

void print_vbo(const std::vector<ld_o::VBO_STRUCT> &data) {
  int i;
  for (i=0; i<data.size(); i++) {
    const ld_o::VBO_STRUCT *v = &data[i];
    printf("[%d]\n", i);
    print_vec4("\tv:", v->v);
    print_vec3("\tn:", v->n);
    print_vec3("\tt:", v->t);
  }
}

void load_obj(std::string filepath, std::vector<ld_o::VBO_STRUCT> &data) {
  std::ifstream infile(filepath);
  if (!infile.is_open()) {
    std::cout << "File does not exist!" << std::endl;
    return;
  }
  
  using namespace ld_o;
  std::vector<V*> v_list;
  std::vector<VT*> vt_list;
  std::vector<VP*> vp_list;
  std::vector<VN*> vn_list;
  std::vector<Face> f_list;
  std::string line;
  while (std::getline(infile, line)) {
    /*
     * Split string up by spaces
     */
    std::list<std::string> chks;
    std::string _s = line;
    size_t i=0, j=0;
    while(i < _s.length()) {
      j = _s.find(SPACE_CHAR, i);
      if (j == std::string::npos) {
        /*
         * Edge case: last 'word' before endline does not have
         * a terminating space
         */
        j = _s.length();
      }

      std::string chk;
      chk = _s.substr(i, j-i);
      chks.push_back(chk);
      
      /* Skip the space */
      i = j+1;
    }

    std::string ELEM = chks.front(); 
    chks.pop_front();
    if (ELEM == "v")
      v_list.push_back(handle_v(chks));
    else if (ELEM == "vt")
      vt_list.push_back(handle_vt(chks));
    else if (ELEM == "vn")
      vn_list.push_back(handle_vn(chks));
    else if (ELEM == "f")
      f_list.push_back(handle_f(chks));
    else if (ELEM == "vp")
      vp_list.push_back(handle_vp(chks));
  }
  
  /*
   * Load in data one "Face" at a time. This way, primitives
   * should be contiguous in "data" - which is what we want.
   */
  Face f;
  int face_id;
  for (face_id=0; face_id<f_list.size(); face_id++) {
    f = f_list[face_id];
    F *face_v;

    bool is_wrap_around = false;
    int i=0, j;
    while (!is_wrap_around) {
      for (j=i; j<i+3; j++) { 
        if (j >= f.size()) {
          is_wrap_around = true;
        }
        int _j = j%f.size();
        VBO_STRUCT s_vbo;
        face_v = f[_j];
        /*
         * Need "-1" because .obj files indices begin at 1
         * (rather than 0, as is the vector)
         */
        V *v = v_list[face_v->v - 1];
        VT *vt = vt_list[face_v->vt - 1];
        VN *vn = vn_list[face_v->vn - 1];

        s_vbo.v = glm::vec4(v->x, v->y, v->z, v->w);
        s_vbo.n = glm::vec3(vn->x, vn->y, vn->z);
        s_vbo.t = glm::vec3(vt->u, vt->v, vt->w);
        
        /* Vector performs deep copy on T */
        data.push_back(s_vbo);
      }

      i += 2;
    }
  }
}

#endif /* GCC_TEST_LOAD_OBJ */