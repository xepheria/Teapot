#define GL_GLEXT_PROTOTYPES
#define BUFFER_OFFSET(bytes) ((GLubyte*) NULL + (bytes))
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
#include <limits>
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

struct face{
public:
   int facei[4];
   int normali[4];
   int texti[4]; //uv
   string mat;
   face(int f1, int f2, int f3, int f4, int n1, int n2, int n3, int n4, int t1, int t2, int t3, int t4, string material);
};

float cameraAngleX=0, cameraAngleY=0, cameraDistance=0;

int sides = 0;

GLfloat VBObuff[416256];

string mtllib;
string currentMat;

float eye[3]={4.0,2.0,1.0};

//for anti-aliasing
double genRand(){
   return (((double)(random()+1))/2147483649.);
}

struct point cross(const struct point& u, const struct point& v){
   struct point w;
   w.x = u.y*v.z - u.z*v.y;
   w.y = -(u.x*v.z - u.z*v.x);
   w.z = u.x*v.y - u.y*v.x;
   return(w);
}

struct point unit_length(const struct point& u){
   double length;
   struct point v;
   length = sqrt(u.x*u.x+u.y*u.y+u.z*u.z);
   v.x = u.x/length;
   v.y = u.y/length;
   v.z = u.z/length;
   return(v);
}

void viewVolume(){
   glEnable(GL_DEPTH_TEST);
   
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluPerspective(60.0, 1.0, 0.01, 1000.0);
   glMatrixMode(GL_MODELVIEW);
}

//jitter view for anti-aliasing
void jitter_view(){
   glLoadIdentity();

   struct point jEye, view, up, vdir, utemp, vtemp;
   jEye.x = eye[0]; jEye.y = eye[1]; jEye.z = eye[2];
   view.x=JITTER*genRand(); view.y=JITTER*genRand(); view.z=JITTER*genRand();
   up.x=0.0; up.y=1.0; up.z=0.0;
   vdir.x = view.x - jEye.x; vdir.y = view.y - jEye.y; vdir.z = view.z - jEye.z;
   //up vector orthogonal to vdir, in plane of vdir and (0,1,0)
   vtemp = cross(vdir, up);
   utemp = cross(vtemp, vdir);
   up = unit_length(utemp);

   gluLookAt(jEye.x,jEye.y,jEye.z,view.x,view.y,view.z,up.x,up.y,up.z);
}

//3 point lighting
void lights(){
   const float diff1 = .5;
   const float spec1 = .5;
   const float diff2 = .2;
   const float spec2 = .2;
   const float diff3 = .2;
   const float spec3 = .2;

   float key_ambient[]={0,0,0,0};
   float key_diffuse[]={diff1,diff1,diff1,0};
   float key_specular[]={spec1,spec1,spec1,0};
   float key_position[]={-3,4,7.5,1};
   
   float fill_ambient[]={0,0,0,0};
   float fill_diffuse[]={diff2,diff2,diff2,0};
   float fill_specular[]={spec2,spec2,spec2,0};
   float fill_position[]={3.5,1,6.5,1};
   
   float back_ambient[]={0,0,0,0};
   float back_diffuse[]={diff3,diff3,diff3,0};
   float back_specular[]={spec3,spec3,spec3,0};
   float back_position[]={0,5,-6.5,1};

   glLightModelfv(GL_LIGHT_MODEL_AMBIENT,key_ambient);
   glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER,1);
   
   glLightfv(GL_LIGHT0,GL_AMBIENT,key_ambient);
   glLightfv(GL_LIGHT0,GL_DIFFUSE,key_diffuse);
   glLightfv(GL_LIGHT0,GL_SPECULAR,key_specular);
   glLightf(GL_LIGHT0,GL_SPOT_EXPONENT, 1.0);
   glLightf(GL_LIGHT0,GL_SPOT_CUTOFF, 180.0);
   glLightf(GL_LIGHT0,GL_CONSTANT_ATTENUATION, .5);
   glLightf(GL_LIGHT0,GL_LINEAR_ATTENUATION, .1);
   glLightf(GL_LIGHT0,GL_QUADRATIC_ATTENUATION, .01);
   glLightfv(GL_LIGHT0,GL_POSITION,key_position);
   
   glLightfv(GL_LIGHT1,GL_AMBIENT,fill_ambient);
   glLightfv(GL_LIGHT1,GL_DIFFUSE,fill_diffuse);
   glLightfv(GL_LIGHT1,GL_SPECULAR,fill_specular);
   glLightf(GL_LIGHT1,GL_SPOT_EXPONENT, 1.0);
   glLightf(GL_LIGHT1,GL_SPOT_CUTOFF, 180.0);
   glLightf(GL_LIGHT1,GL_CONSTANT_ATTENUATION, .5);
   glLightf(GL_LIGHT1,GL_LINEAR_ATTENUATION, .1);
   glLightf(GL_LIGHT1,GL_QUADRATIC_ATTENUATION, .01);
   glLightfv(GL_LIGHT1,GL_POSITION,fill_position);
   
   glLightfv(GL_LIGHT2,GL_AMBIENT,back_ambient);
   glLightfv(GL_LIGHT2,GL_DIFFUSE,back_diffuse);
   glLightfv(GL_LIGHT2,GL_SPECULAR,back_specular);
   glLightf(GL_LIGHT2,GL_SPOT_EXPONENT, 1.0);
   glLightf(GL_LIGHT2,GL_SPOT_CUTOFF, 180.0);
   glLightf(GL_LIGHT2,GL_CONSTANT_ATTENUATION, .5);
   glLightf(GL_LIGHT2,GL_LINEAR_ATTENUATION, .1);
   glLightf(GL_LIGHT2,GL_QUADRATIC_ATTENUATION, .01);
   glLightfv(GL_LIGHT2,GL_POSITION,back_position);
}

//PROBABLY NEED TO CHANGE THIS TO WORK WITH EXTERNAL MATERIAL LIBRARY
void material(){
   //make our bunny a nice shade of pink
   float mat_ambient[]={.2,0,0,1};
   float mat_diffuse[]={1,.2,.2,1};
   float mat_specular[] = {.5,.5,.5,1};
   float mat_shininess[]={1.0};

   glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
   glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
   glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
   glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
}

GLuint mybuf = 1;
void initOGL(int argc, char **argv){
   glutInit(&argc, argv);
   glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH|GLUT_ACCUM);
   glutInitWindowSize(512,512);
   glutInitWindowPosition(100,50);
   glutCreateWindow("T-Pawt");
   glClearColor(.35,.35,.35,0);
   glClearAccum(0.0,0.0,0.0,0.0);

   viewVolume();
   jitter_view();
   lights();
   material();
   
   //buffer stuff for VBO
   glBindBuffer(GL_ARRAY_BUFFER, mybuf);
   glBufferData(GL_ARRAY_BUFFER, sizeof(VBObuff), VBObuff, GL_STATIC_DRAW);

   glEnableClientState(GL_VERTEX_ARRAY);
   glVertexPointer(3, GL_FLOAT, 3*sizeof(GLfloat), BUFFER_OFFSET(0));

   //glEnableClientState(GL_TEXTURE_COORD_ARRAY);
   glTexCoordPointer(2, GL_FLOAT, 2*sizeof(GLfloat), BUFFER_OFFSET(156096*sizeof(GLfloat)));

   glEnableClientState(GL_NORMAL_ARRAY);
   glNormalPointer(GL_FLOAT, 3*sizeof(GLfloat), BUFFER_OFFSET(260160*sizeof(GLfloat)));

}

bool loadObj(string filename,
             vector <vec3> &vertices,
             vector <vec2> &uvs,
             vector <vec3> &normals,
             vector <vec3> &tangents,
             vector <vec3> &bitangents)
{
   //debugging purposes
   unsigned int l = 0;

   vector<unsigned int> vertexIndices, normalIndices, textIndices;
   vector<vec3> temp_vertices;
   vector<vec2> temp_uvs;
   vector<vec3> temp_normals;
   vector<vec3> temp_tangents;
   vector<vec3> temp_bitangents;

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
      //caught a tangent
      else if(lineIn.compare("vx") == 0){
         vec3 tangent;
         ifs >> tangent.x >> tangent.y >> tangent.z;
         temp_tangents.push_back(tangent);
      }
      //caught a bitangent
      else if(lineIn.compare("vy") == 0){
         vec3 bitangent;
         ifs >> bitangent.x >> bitangent.y >> bitangent.z;
         temp_bitangents.push_back(bitangent);
      }
      //scan vertex/normal/text face indexes
      else if(lineIn.compare("f") == 0){
         unsigned int vertIndex[4], normIndex[4], textIndex[4];
         ifs >> vertIndex[0]; ifs.ignore(1,'/');
         ifs >> normIndex[0]; ifs.ignore(1,'/');
         ifs >> textIndex[0] >> vertIndex[1]; ifs.ignore(1,'/');
         ifs >> normIndex[1]; ifs.ignore(1,'/');
         ifs >> textIndex[1] >> vertIndex[2];ifs.ignore(1,'/');
         ifs >> normIndex[2]; ifs.ignore(1,'/');
         ifs >> textIndex[2] >> vertIndex[3];ifs.ignore(1,'/');
         ifs >> normIndex[3]; ifs.ignore(1,'/');
         ifs >> textIndex[3];
         
         vertexIndices.insert(vertexIndices.end(), vertIndex, vertIndex+(sizeof(vertIndex)/sizeof(unsigned int)));
         normalIndices.insert(normalIndices.end(), normIndex, normIndex+(sizeof(normIndex)/sizeof(unsigned int)));
         textIndices.insert(textIndices.end(), textIndex, textIndex+(sizeof(textIndex)/sizeof(unsigned int)));
      }
      //library of materials referenced by usemtl
      else if(lineIn.compare("mtllib") == 0){
         ifs >> mtllib;
      }
      else if(lineIn.compare("usemtl") == 0){
         ifs >> currentMat;
      }
      else if(lineIn.compare("#") == 0){
         ifs.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      }
   }

   for(unsigned int i=0; i<vertexIndices.size(); i++){
      unsigned int vertexIndex = vertexIndices[i];
      vec3 vertex = temp_vertices[vertexIndex-1];
      vertices.push_back(vertex);
   }
   for(unsigned int i=0; i<textIndices.size(); i++){
      unsigned int uvIndex = textIndices[i];
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

void draw(){
   int view_pass;
   glClear(GL_ACCUM_BUFFER_BIT);
   for(view_pass=0; view_pass < VPASSES; view_pass++){
      jitter_view();
      glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
      glDrawArrays(GL_QUADS, 0, sides);
      glAccum(GL_ACCUM, 1.0/(float)(VPASSES));
   }
   glAccum(GL_RETURN, 1.0);
   glFlush();
}

char *read_shader_program(const char *filename) 
{
   FILE *fp;
   char *content = NULL;
   int fd, count;
   fd = open(filename,O_RDONLY);
   count = lseek(fd,0,SEEK_END);
   close(fd);
   content = (char *)calloc(1,(count+1));
   fp = fopen(filename,"r");
   count = fread(content,sizeof(char),count,fp);
   content[count] = '\0';
   fclose(fp);
   return content;
}

unsigned int set_shaders(){
   GLint vertCompiled, fragCompiled;
   char *vs, *fs;
   GLuint v, f, p;

   v = glCreateShader(GL_VERTEX_SHADER);
   f = glCreateShader(GL_FRAGMENT_SHADER);
   vs = read_shader_program("teapot.vert");
   fs = read_shader_program("teapot.frag");
   glShaderSource(v,1,(const char **)&vs,NULL);
   glShaderSource(f,1,(const char **)&fs,NULL);
   free(vs);
   free(fs); 

   glCompileShader(v);
   GLint shaderCompiled;
   glGetShaderiv(v, GL_COMPILE_STATUS, &shaderCompiled);
   if(shaderCompiled == GL_FALSE){
      cout<<"ShaderCasher.cpp"<<"loadShader(s)"<<"Shader did not compile"<<endl;
         int infologLength = 0;

         int charsWritten  = 0;
         char *infoLog;
         glGetShaderiv(v, GL_INFO_LOG_LENGTH,&infologLength);

         if (infologLength > 0){
            infoLog = (char *)malloc(infologLength);
            glGetShaderInfoLog(v, infologLength, &charsWritten, infoLog);
            string log = infoLog;
            cout<<log<<endl;
         }
   }

   glCompileShader(f);
   glGetShaderiv(f, GL_COMPILE_STATUS, &shaderCompiled);
   if(shaderCompiled == GL_FALSE){
      cout<<"ShaderCasher.cpp"<<"loadShader(f)"<<"Shader did not compile"<<endl;
         int infologLength = 0;

         int charsWritten  = 0;
         char *infoLog;
         glGetShaderiv(f, GL_INFO_LOG_LENGTH,&infologLength);

         if (infologLength > 0){
            infoLog = (char *)malloc(infologLength);
            glGetShaderInfoLog(f, infologLength, &charsWritten, infoLog);
            string log = infoLog;
            cout<<log<<endl;
         }
   }

   p = glCreateProgram();
   glAttachShader(p,f);
   glAttachShader(p,v);
   glLinkProgram(p);
   glUseProgram(p);
   return(p);
}

void set_uniform_parameters(unsigned int p)
{
   int location;
   location = glGetUniformLocation(p,"eye_position");
   glUniform3fv(location,1,eye);
}

void keyboard(unsigned char key, int x, int y){
   switch(key){
      case 'q': 
        glDeleteBuffers(1, &mybuf);
        exit(1);
      default: break;  
   }
}

int main(int argc, char **argv){
   srandom(123456789);
   string filename= "teapot.605.obj";
   vector <vec3> vertices;
   vector <vec2> uvs; //texture coords
   vector <vec3> normals;
   vector <vec3> tangents;
   vector <vec3> bitangents;
   
   if(!loadObj(filename, vertices, uvs, normals, tangents, bitangents)){
      cout << "Could not read object.\n";
      return 0;
   }

   int verticeSize=0, normalSize=0, uvSize=0;

   //fill buffer
   for(unsigned int i = 0; i<vertices.size(); i++){
      VBObuff[(i*3)]=vertices[i].x;
      VBObuff[(i*3)+1]=vertices[i].y;
      VBObuff[(i*3)+2]=vertices[i].z;
      verticeSize+=3;
   }
   for(unsigned int i = 0; i<uvs.size(); i++){
      VBObuff[verticeSize+(i*2)]=uvs[i].x;
      VBObuff[verticeSize+(i*2)+1]=uvs[i].y;
      uvSize+=2;
   }
   for(unsigned int i = 0; i<normals.size(); i++){
      VBObuff[verticeSize+uvSize+(i*3)]=normals[i].x;
      VBObuff[verticeSize+uvSize+(i*3)+1]=normals[i].y;
      VBObuff[verticeSize+uvSize+(i*3)+2]=normals[i].z;
      normalSize+=3;
   } sides=vertices.size();
   
   initOGL(argc, argv);

   //quit function
   glutKeyboardFunc(keyboard);

   set_shaders();
   glutDisplayFunc(draw);
   glutMainLoop();
   return 0;
}

