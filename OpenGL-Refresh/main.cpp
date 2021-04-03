// OpenGL-Refresh.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string>
#include <fstream>
#include <vector>

#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

float vertices[] = {
    // front
    -0.5, -0.5,  0.5,
     0.5, -0.5,  0.5,
     0.5,  0.5,  0.5,
    -0.5,  0.5,  0.5,

    // back
    -0.5, -0.5, -0.5,
     0.5, -0.5, -0.5,
     0.5,  0.5, -0.5,
    -0.5,  0.5, -0.5
};

float colors[] = {
    // front colors
    1.0, 0.0, 0.0,
    0.0, 1.0, 0.0,
    0.0, 0.0, 1.0,
    1.0, 1.0, 1.0,
    // back colors
    1.0, 0.0, 0.0,
    0.0, 1.0, 0.0,
    0.0, 0.0, 1.0,
    1.0, 1.0, 1.0
  };

unsigned int indices[] = {
	// front
	0, 1, 2,
	2, 3, 0,
	// right
	1, 5, 6,
	6, 2, 1,
	// back
	7, 6, 5,
	5, 4, 7,
	// left
	4, 0, 3,
	3, 7, 4,
	// bottom
	4, 5, 1,
	1, 0, 4,
	// top
	3, 2, 6,
	6, 7, 3
};

unsigned int dvao;
unsigned int dibo;
unsigned int dicount;

using namespace std;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
  // tell opengl our screen coordinate ranges
  glViewport(0, 0, width, height);
}

unsigned int init_shaders() {
  // read the vertex shader from file
  ifstream vert_file("vertex.glsl");
  string vert_shader((istreambuf_iterator<char>(vert_file)), istreambuf_iterator<char>());
  const char *vert_c_str = vert_shader.c_str();

  unsigned int vshdr;
  vshdr = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vshdr, 1, &vert_c_str, NULL);
  glCompileShader(vshdr);

  // check if compilation was successful, and print error if not
  int success;
  char infoLog[512];
  glGetShaderiv(vshdr, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vshdr, 512, NULL, infoLog);
    cout << "Shader Compilation Failed\n" << infoLog << endl;
  }

  // read the fragment shader from file
  ifstream frag_file("fragment.glsl");
  string frag_shader((istreambuf_iterator<char>(frag_file)), istreambuf_iterator<char>());
  const char *frag_c_str = frag_shader.c_str();

  unsigned int fshdr;
  fshdr = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fshdr, 1, &frag_c_str, NULL);
  glCompileShader(fshdr);

  // check if compilation was successful, and print error if not
  glGetShaderiv(fshdr, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(fshdr, 512, NULL, infoLog);
    cout << "Shader Compilation Failed\n" << infoLog << endl;
  }

  // link the shader program
  unsigned int shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vshdr);
  glAttachShader(shaderProgram, fshdr);
  glLinkProgram(shaderProgram);

  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
    cout << "Shader Link Failed\n" << infoLog << endl;
  }

  // delete shader objects, unnecessary now
  glDeleteShader(vshdr);
  glDeleteShader(fshdr);

  return shaderProgram;
}

void load_model() {
  Assimp::Importer importer;
  const aiScene* scene = importer.ReadFile("dragon.obj", aiProcess_Triangulate);

  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
    cout << "Error Assimp loading..." << endl;
    return;
  }

  // for dragon, we have one node with one mesh
  aiMesh* mesh = scene->mMeshes[scene->mRootNode->mChildren[0]->mMeshes[0]];

  // we will put all data in single vbo object for ease
  unsigned int vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  // gross calculations can be refactored... basically just lays out data as {verticies, normals}
  glBufferData(GL_ARRAY_BUFFER, sizeof(mesh->mVertices[0]) * mesh->mNumVertices + sizeof(mesh->mNormals[0]) * mesh->mNumVertices, nullptr, GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(mesh->mVertices[0]) * mesh->mNumVertices, mesh->mVertices);
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(mesh->mVertices[0]) * mesh->mNumVertices, sizeof(mesh->mNormals[0]) * mesh->mNumVertices, mesh->mNormals);

  // configure the vao with vertex attributes
  glGenVertexArrays(1, &dvao);
  glBindVertexArray(dvao);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);
  
  // set attrib to start at offset after verticies
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)(sizeof(mesh->mVertices[0]) * mesh->mNumVertices));
  glEnableVertexAttribArray(1);

  // get indices out .... must do because they are in ptrs...
  vector<unsigned int> inds;
  for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
    for (unsigned int j = 0; j < 3; j++) {
      inds.push_back(mesh->mFaces[i].mIndices[j]);
    }
  }

  glGenBuffers(1, &dibo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dibo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, inds.size() * sizeof(unsigned int), inds.data(), GL_STATIC_DRAW);
  dicount = inds.size();

  return;
}

int main() {
  // initialize glfw
  glfwInit();

  // tell glfw the opengl context version
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // create window object
  GLFWwindow *window = glfwCreateWindow(800, 600, "Triangle", NULL, NULL);
  if (window == NULL) {
    cout << "Failed to create window..." << endl;
    glfwTerminate();
    return -1;
  }

  // tell opengl to use the current window context
  glfwMakeContextCurrent(window);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    cout << "Failed to init glad ..." << endl;
    return -1;
  }

  // ensure we cull the back face to prevent wacky artifacts
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  glEnable(GL_DEPTH_TEST);
  
  // tell opengl our screen coordinate ranges
  glViewport(0, 0, 800, 600);

  // register glfw callbacks
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  unsigned int shaderProgram = init_shaders();

  load_model();

  // create vertex array object
  unsigned int vao;
  glGenVertexArrays(1, &vao);

  // configure the vertex array object
  glBindVertexArray(vao);

  // create vbo for triangle vertices
  unsigned int vbo;
  glGenBuffers(1, &vbo);
  unsigned int cbo;
  glGenBuffers(1, &cbo);

  // bind it as array buffer
  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  // copy vertex data to GPU buffer
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // configure the vao with vertex attributes
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, cbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);
  
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  glEnableVertexAttribArray(1);

  unsigned int ibo;
  glGenBuffers(1, &ibo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);


  float degrees = 0.0f;
  float deg2 = 0.0f;
  while (!glfwWindowShouldClose(window)) {
    glClearColor(0.2f, 0.4f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgram);
    glBindVertexArray(dvao);

    glm::mat4 trans = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -5.0f, -10.f));
    glm::mat4 model = glm::rotate(trans, glm::radians(degrees), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 ortho = glm::ortho(-10.0f, 10.0f, -10.f, 10.0f, 0.1f, 100.0f);
    glm::mat4 mvp = model * ortho;
    // glm::mat4 model = glm::rotate(rot1, glm::radians(deg2), glm::vec3(1.0f, 0.0f, 0.0f));
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(ortho));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dibo);
    glDrawElements(GL_TRIANGLES, dicount, GL_UNSIGNED_INT, nullptr);
    //glDrawArrays(GL_TRIANGLES, 0, 3);

    // poll for input
    glfwPollEvents();
    // swap the framebuffers...
    glfwSwapBuffers(window);

    degrees += 0.05f;
    deg2 += 0.2f;
  }

  glfwTerminate();
}
