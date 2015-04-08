#define GL_GLEXT_PROTOTYPES
#include <stdio.h>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <GLUT/glut.h>
#else
#include <GL/gl.h>
#include <GL/glut.h>
#endif

#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <math.h>

using namespace std;

#define VPASSES 10
#define JITTER 0.01

struct point{
   float x;
   float y;
   float z;
};

class vec2{
public:
   vec2() : x(0), y(0) {}
   vec2(const double inx, const double iny) : x(inx), y(iny) {}
   vec2(const vec2& in) : x(in.x), y(in.y) {}
   vec2& operator=(const vec2& in){
      if(this == &in) return *this;
      x = in.x;
      y = in.y;
      return *this;
   }
   double x, y;
};

class vec3{
public:
   vec3() : x(0), y(0), z(0) {}
   vec3(const double inx, const double iny, const double inz) : x(inx), y(iny), z(inz) {}
   vec3(const vec3& in) : x(in.x), y(in.y), z(in.z) {}
   vec3& operator=(const vec3& in){
      if(this == &in) return *this;
      x = in.x;
      y = in.y;
      z = in.z;
      return *this;
   }
   double x, y, z;
};

float cameraAngleX=0, cameraAngleY=0, cameraDistance=0;
GLfloat *myVertices=NULL;
GLfloat *myNormals=NULL;
int verticeSize=0, normalSize=0, sides=0;

GLfloat VBObuff[1253340];
float eye[3]={0,3.0,5.0};

bool loadObj(string filename,
             vector <vec3> &vertices,
             vector <vec2> &uvs,
             vector <vec3> &normals)
{
   vector<unsigned int> vertexIndices, uvIndices, normalIndices;
   vector<vec3> temp_vertices;
   vector<vec2> temp_uvs;
   vector<vec3> temp_normals;

   ifstream ifs(filename.c_str());
   if(!ifs){
      cout << "Error opening file: " << filename << endl;
      return false;
   }
   
   string lineIn;
   while(ifs >> lineIn){
      if(lineIn.compare("v") == 0){
         vec3 vertex;
         ifs >> vertex.x >> vertex.y >> vertex.z;
         temp_vertices.push_back(vertex);
      }
      else if(lineIn.compare("vt") == 0){
         vec2 uv;
         ifs >> uv.x >> uv.y;
         temp_uvs.push_back(uv);
      }
      else if(lineIn.compare("vn") == 0){
         vec3 normal;
         ifs >> normal.x >> normal.y >> normal.z;
         temp_normals.push_back(normal);
      }
      //scan vertex//normal
      else if(lineIn.compare("f") == 0){
         unsigned int vertIndex[3], normIndex[3];
         ifs >> vertIndex[0];
         ifs.ignore(1,'/'); ifs.ignore(1,'/');
         ifs >> normIndex[0] >> vertIndex[1];
         ifs.ignore(1,'/'); ifs.ignore(1,'/');
         ifs >> normIndex[1] >> vertIndex[2];
         ifs.ignore(1,'/'); ifs.ignore(1,'/');
         ifs >> normIndex[2];
         vertexIndices.push_back(vertIndex[0]);
         vertexIndices.push_back(vertIndex[1]);
         vertexIndices.push_back(vertIndex[2]);
         normalIndices.push_back(normIndex[0]);
         normalIndices.push_back(normIndex[1]);
         normalIndices.push_back(normIndex[2]);
      }
   }

   for(unsigned int i=0; i<vertexIndices.size(); i++){
      unsigned int vertexIndex = vertexIndices[i];
      vec3 vertex = temp_vertices[vertexIndex-1];
      vertices.push_back(vertex);
   }
   for(unsigned int i=0; i<uvIndices.size(); i++){
      unsigned int uvIndex = uvIndices[i];
      vec2 uv = temp_uvs[uvIndex-1];
      uvs.push_back(uv);
   }
   for(unsigned int i=0; i<normalIndices.size(); i++){
      unsigned int normalIndex = normalIndices[i];
      vec3 normal = temp_normals[normalIndex-1];
      normals.push_back(normal);
   }
   
   return 1;
}

int main(int argc, char **argv){
   srandom(123456789);
   string filename= "bunny.obj";
   vector <vec3> vertices2;
   vector <vec2> uvs2;
   vector <vec3> normals2;
   if(!loadObj(filename, vertices2, uvs2, normals2)){
      cout << "Could not read object.\n";
      return 0;
   }

   //fill buffer
   for(unsigned int i = 0; i<vertices2.size(); i++){
      VBObuff[(i*3)]=vertices2[i].x;
      VBObuff[(i*3)+1]=vertices2[i].y;
      VBObuff[(i*3)+2]=vertices2[i].z;
      verticeSize+=3;
   }
   for(unsigned int i = 0; i<normals2.size(); i++){
      VBObuff[verticeSize+(i*3)]=normals2[i].x;
      VBObuff[verticeSize+(i*3)+1]=normals2[i].y;
      VBObuff[verticeSize+(i*3)+2]=normals2[i].z;
      normalSize+=3;
   }sides=verticeSize/3;

   /*
   initOGL(argc, argv);

   //quit function
   glutKeyboardFunc(keyboard);

   set_shaders();
   glutDisplayFunc(draw);
   glutMainLoop();
   return 0;
   */
}

