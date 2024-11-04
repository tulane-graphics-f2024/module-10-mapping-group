#include "common.h"
#include "SourcePath.h"


using namespace Angel;

typedef vec4 color4;

// Initialize shader lighting parameters
vec4 light(   0.0, 0.0, 10.0, 1.0 );
color4 light_ambient(  0.1, 0.1, 0.1, 1.0 );
color4 light_diffuse(  1.0, 1.0, 1.0, 1.0 );
color4 light_specular( 1.0, 1.0, 1.0, 1.0 );

// Initialize shader material parameters
color4 material_ambient( 0.1, 0.1, 0.1, 1.0 );
color4 material_diffuse( 1.0, 0.8, 0.0, 1.0 );
color4 material_specular( 0.8, 0.8, 0.8, 1.0 );
float  material_shininess = 10;

GLuint program;


enum{_SPHERE, _TEAPOT, _BUNNY, _DRAGON, _TOTAL_MODELS};
std::string files[_TOTAL_MODELS] = {"/models/sphere.obj",
                                    "/models/wt_teapot.obj",
                                    "/models/bunny.obj",
                                    "/models/dragon.obj"};


std::vector < Mesh > mesh;
std::vector < GLuint > buffer;
std::vector < GLuint > vao;
CubeMap *cube;
GLuint ModelView_loc, NormalMatrix_loc, Projection_loc;
bool wireframe;
int current_draw;

//==========Trackball Variables==========
static float curquat[4],lastquat[4];
/* current transformation matrix */
static float curmat[4][4];
mat4 curmat_a;
/* actual operation  */
bool scaling;
bool moving;
bool panning;
/* starting "moving" coordinates */
static int beginx, beginy;
/* ortho */
float ortho_x, ortho_y;
/* current scale factor */
static float scalefactor;
bool lbutton_down;


static void error_callback(int error, const char* description)
{
  fprintf(stderr, "Error: %s\n", description);
}

//User interaction handler
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  if (key == GLFW_KEY_SPACE && action == GLFW_PRESS){
    current_draw = (current_draw+1)%_TOTAL_MODELS;
  }
  if (key == GLFW_KEY_W && action == GLFW_PRESS){
    wireframe = !wireframe;
  }
}

//User interaction handler
static void mouse_click(GLFWwindow* window, int button, int action, int mods){
  
  if (GLFW_RELEASE == action){
    moving=scaling=panning=false;
    return;
  }
  
  if( mods & GLFW_MOD_SHIFT){
    scaling=true;
  }else if( mods & GLFW_MOD_ALT ){
    panning=true;
  }else{
    moving=true;
    trackball(lastquat, 0, 0, 0, 0);
  }
  
  double xpos, ypos;
  glfwGetCursorPos(window, &xpos, &ypos);
  beginx = xpos; beginy = ypos;
}

//User interaction handler
void mouse_move(GLFWwindow* window, double x, double y){
  
  int W, H;
  glfwGetFramebufferSize(window, &W, &H);

  
  float dx=(x-beginx)/(float)W;
  float dy=(beginy-y)/(float)H;
  
  if (panning)
    {
    ortho_x  +=dx;
    ortho_y  +=dy;
    
    beginx = x; beginy = y;
    return;
    }
  else if (scaling)
    {
    scalefactor *= (1.0f+dx);
    
    beginx = x;beginy = y;
    return;
    }
  else if (moving)
    {
    trackball(lastquat,
              (2.0f * beginx - W) / W,
              (H - 2.0f * beginy) / H,
              (2.0f * x - W) / W,
              (H - 2.0f * y) / H
              );
    
    add_quats(lastquat, curquat, curquat);
    build_rotmatrix(curmat, curquat);
    
    beginx = x;beginy = y;
    return;
    }
}


void init(){
  
  std::string vshader = source_path + "/shaders/vshader.glsl";
  std::string fshader = source_path + "/shaders/fshader.glsl";
  
  GLchar* vertex_shader_source = readShaderSource(vshader.c_str());
  GLchar* fragment_shader_source = readShaderSource(fshader.c_str());

  GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, (const GLchar**) &vertex_shader_source, NULL);
  glCompileShader(vertex_shader);
  check_shader_compilation(vshader, vertex_shader);
  
  GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, (const GLchar**) &fragment_shader_source, NULL);
  glCompileShader(fragment_shader);
  check_shader_compilation(fshader, fragment_shader);
  
  program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  
  glLinkProgram(program);
  check_program_link(program);
  
  glUseProgram(program);

  glBindFragDataLocation(program, 0, "fragColor");

  //Per vertex attributes
  GLuint vPosition = glGetAttribLocation( program, "vPosition" );
  GLuint vNormal = glGetAttribLocation( program, "vNormal" );

  //Compute ambient, diffuse, and specular terms
  color4 ambient_product  = light_ambient * material_ambient;
  color4 diffuse_product  = light_diffuse * material_diffuse;
  color4 specular_product = light_specular * material_specular;

  //Retrieve and set uniform variables
  glUniform4fv( glGetUniformLocation(program, "Light"), 1, light);
  glUniform4fv( glGetUniformLocation(program, "AmbientProduct"), 1, ambient_product );
  glUniform4fv( glGetUniformLocation(program, "DiffuseProduct"), 1, diffuse_product );
  glUniform4fv( glGetUniformLocation(program, "SpecularProduct"), 1, specular_product );
  glUniform1f(  glGetUniformLocation(program, "Shininess"), material_shininess );

  //Matrix uniform variable locations
  ModelView_loc = glGetUniformLocation( program, "ModelView" );
  NormalMatrix_loc = glGetUniformLocation( program, "NormalMatrix" );
  Projection_loc = glGetUniformLocation( program, "Projection" );

  //===== Send data to GPU ======
  vao.resize(_TOTAL_MODELS);
  glGenVertexArrays( _TOTAL_MODELS, &vao[0] );

  buffer.resize(_TOTAL_MODELS);
  glGenBuffers( _TOTAL_MODELS, &buffer[0] );

  for(unsigned int i=0; i < _TOTAL_MODELS; i++){
    mesh.push_back((source_path + files[i]).c_str());

    glBindVertexArray( vao[i] );
    glBindBuffer( GL_ARRAY_BUFFER, buffer[i] );
    unsigned int vertices_bytes = mesh[i].vertices.size()*sizeof(vec4);
    unsigned int normals_bytes  = mesh[i].normals.size()*sizeof(vec3);

    glBufferData( GL_ARRAY_BUFFER, vertices_bytes + normals_bytes, NULL, GL_STATIC_DRAW );
    unsigned int offset = 0;
    glBufferSubData( GL_ARRAY_BUFFER, offset, vertices_bytes, &mesh[i].vertices[0] );
    offset += vertices_bytes;
    glBufferSubData( GL_ARRAY_BUFFER, offset, normals_bytes,  &mesh[i].normals[0] );

    glEnableVertexAttribArray( vNormal );
    glEnableVertexAttribArray( vPosition );

    glVertexAttribPointer( vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0) );
    glVertexAttribPointer( vNormal, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(mesh[i].vertices.size()*sizeof(vec4)) );

  }
  
  glUseProgram(0);

  //===== End: Send data to GPU ======
  
  vector<std::string> faces(6);
  faces[0] = source_path + "/skybox/2/right.png";
  faces[1] = source_path + "/skybox/2/left.png";
  faces[2] = source_path + "/skybox/2/top.png";
  faces[3] = source_path + "/skybox/2/bottom.png";
  faces[4] = source_path + "/skybox/2/front.png";
  faces[5] = source_path + "/skybox/2/back.png";

  cube = new CubeMap();
  cube->loadImages(faces);
  cube->glInit();


  // ====== Enable some opengl capabilitions ======
  glEnable( GL_DEPTH_TEST );
  glShadeModel(GL_SMOOTH);

  glClearColor( 0.8, 0.8, 1.0, 1.0 );
  
  //===== Initalize some program state variables ======

  //Quaternion trackball variables, you can ignore
  scaling  = 0;
  moving   = 0;
  panning  = 0;
  beginx   = 0;
  beginy   = 0;
  
  matident(curmat);
  trackball(curquat , 0.0f, 0.0f, 0.0f, 0.0f);
  trackball(lastquat, 0.0f, 0.0f, 0.0f, 0.0f);
  add_quats(lastquat, curquat, curquat);
  build_rotmatrix(curmat, curquat);
  
  scalefactor = 1.0;
  
  wireframe = false;
  current_draw = 0;
  
  lbutton_down = false;


  //===== End: Initalize some program state variables ======

}


int main(void){
  
  GLFWwindow* window;
  
  glfwSetErrorCallback(error_callback);
  
  if (!glfwInit())
    exit(EXIT_FAILURE);
  
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  
  glfwWindowHint(GLFW_SAMPLES, 4);
  
  window = glfwCreateWindow(512, 512, "Assignment 3 - 3D Shading", NULL, NULL);
  if (!window){
    glfwTerminate();
    exit(EXIT_FAILURE);
  }
  
  //Set key and mouse callback functions
  glfwSetKeyCallback(window, key_callback);
  glfwSetMouseButtonCallback(window, mouse_click);
  glfwSetCursorPosCallback(window, mouse_move);

  
  glfwMakeContextCurrent(window);
  gladLoadGL(glfwGetProcAddress);
  glfwSwapInterval(1);
  
  init();
  

  
  while (!glfwWindowShouldClose(window)){
    
    //Display as wirframe, boolean tied to keystoke 'w'
    if(wireframe){
      glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    }else{
      glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    }
    
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    
    GLfloat aspect = GLfloat(width)/height;
    
    //Projection matrix
    mat4  projection = Perspective( 45.0, aspect, 0.5, 5.0 );
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //"Camera" position
    const vec3 viewer_pos( 0.0, 0.0, 3.0 );
    
    //Track_ball rotation matrix
    mat4 track_ball =  mat4(curmat[0][0], curmat[1][0], curmat[2][0], curmat[3][0],
                            curmat[0][1], curmat[1][1], curmat[2][1], curmat[3][1],
                            curmat[0][2], curmat[1][2], curmat[2][2], curmat[3][2],
                            curmat[0][3], curmat[1][3], curmat[2][3], curmat[3][3]);
 
    //Modelview based on user interaction
    mat4 user_MV  =  Translate( -viewer_pos ) *                    //Move Camera Back to -viewer_pos
                     Translate(ortho_x, ortho_y, 0.0) *            //Pan Camera
                     track_ball *                                  //Rotate Camera
                     Scale(scalefactor,scalefactor,scalefactor);   //User Scale
    

    // ====== Draw ======
    glUseProgram(program);
    glBindVertexArray(vao[current_draw]);
    glUniformMatrix4fv( ModelView_loc, 1, GL_TRUE, user_MV*mesh[current_draw].model_view);
    glUniformMatrix4fv( Projection_loc, 1, GL_TRUE, projection );
    glUniformMatrix4fv( NormalMatrix_loc, 1, GL_TRUE, transpose(invert(user_MV*mesh[current_draw].model_view)));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cube->cubemapTexture);
    glDrawArrays( GL_TRIANGLES, 0, mesh[current_draw].vertices.size() );
    // ====== End: Draw ======
    glBindVertexArray(0);
    glUseProgram(0);
    
    cube->draw(user_MV,projection);

    
    glfwSwapBuffers(window);
    glfwPollEvents();
    
  }
  
  glfwDestroyWindow(window);
  
  glfwTerminate();
  exit(EXIT_SUCCESS);
}
