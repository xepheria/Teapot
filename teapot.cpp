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

#define VPASSES 50
#define JITTER 0.01

const int xres = 512;
const int yres = 512;

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
int verticeSize=0, textureSize=0, normalSize=0, tangentSize=0;
int sides = 0;

GLfloat VBObuff[728448];
GLuint p; //program variable
GLuint surf; //surface shaders variable

string mtllib;
//used for the different materials pulled from mtllib
struct Material{
   int start;
   int end;
   string mat;
   Material(const int s, const int e, const string& m) : start(s), end(e), mat(m) {}
};
vector< Material* > mats;

float eye[3]={3.0, 2.2, 1.7};

//fake light used for shadow-mapping
GLfloat light_position[] = {-3.0, 3.2, 1.7, 1.0};
GLfloat light_direction[] = {3.0, -3.2, -1.7, 1.0};

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

//grab material from library
bool material(string mat){
   ifstream ifs(mtllib.c_str());
   if(!ifs){
      cout << "Error opening file: " << mtllib << endl;
      return false;
   }
   
   float mat_ambient[4];
   float mat_diffuse[4];
   float mat_specular[4];
   float mat_shininess[1];
   
   string lineIn;
   do{
      ifs >> lineIn;
      if(ifs.eof()){
         cout << "No such material\n";
         return false;
      }
   } while (lineIn.compare(mat) != 0);
   while(ifs >> lineIn && lineIn.compare("newmtl") != 0){
      if(lineIn.compare("Ka") == 0){
         ifs >> mat_ambient[0] >> mat_ambient[1] >> mat_ambient[2];
         mat_ambient[3] = 1.0;
      }
      else if(lineIn.compare("Kd") == 0){
         ifs >> mat_diffuse[0] >> mat_diffuse[1] >> mat_diffuse[2];
         mat_diffuse[3] = 1.0;
      }
      else if(lineIn.compare("Ks") == 0){
         ifs >> mat_specular[0] >> mat_specular[1] >> mat_specular[2];
         mat_specular[3] = 1.0;
      }
      else if(lineIn.compare("Ns") == 0){
         ifs >> mat_shininess[0];
      }
      else if(lineIn.compare("#") == 0){
         ifs.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      }
   }
   glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
   glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
   glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
   glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
   
   return true;
}

void build_shadowmap(){
   //set properties of shadow texture
   glBindTexture(GL_TEXTURE_2D, 8); //not using this one currently...
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   //clamp to edges
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
   
   //declare empty texture
   glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, xres, yres, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
   
   glBindTexture(GL_TEXTURE_2D, 0);
   glBindFramebufferEXT(GL_FRAMEBUFFER, 1); //do shadowmap offscreen
   glDrawBuffer(GL_NONE);
   
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 8, 0);
   
   glBindFramebufferEXT(GL_FRAMEBUFFER, 0);
}

GLuint mybuf = 1;
void initOGL(int argc, char **argv){
   glutInit(&argc, argv);
   glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH|GLUT_ACCUM);
   glutInitWindowSize(xres,yres);
   glutInitWindowPosition(100,50);
   glutCreateWindow("T-Pawt");
   build_shadowmap();
   glClearColor(.35,.35,.35,0);
   glClearAccum(0.0,0.0,0.0,0.0);

   viewVolume();
   jitter_view();
   
   //buffer stuff for VBO
   glBindBuffer(GL_ARRAY_BUFFER, mybuf);
   glBufferData(GL_ARRAY_BUFFER, sizeof(VBObuff), VBObuff, GL_STATIC_DRAW);

   glEnableClientState(GL_VERTEX_ARRAY);
   glVertexPointer(3, GL_FLOAT, 3*sizeof(GLfloat), BUFFER_OFFSET(0));

   glEnableClientState(GL_TEXTURE_COORD_ARRAY);
   glTexCoordPointer(2, GL_FLOAT, 2*sizeof(GLfloat), BUFFER_OFFSET(verticeSize*sizeof(GLfloat)));

   glEnableClientState(GL_NORMAL_ARRAY);
   glNormalPointer(GL_FLOAT, 3*sizeof(GLfloat),BUFFER_OFFSET((verticeSize+textureSize)*sizeof(GLfloat)));
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
         ifs >> textIndex[0]; ifs.ignore(1,'/');
         ifs >> normIndex[0]>> vertIndex[1]; ifs.ignore(1,'/');
         ifs >> textIndex[1]; ifs.ignore(1,'/');
         ifs >> normIndex[1]>> vertIndex[2];ifs.ignore(1,'/');
         ifs >> textIndex[2]; ifs.ignore(1,'/');
         ifs >> normIndex[2]>> vertIndex[3];ifs.ignore(1,'/');
         ifs >> textIndex[3]; ifs.ignore(1,'/');
         ifs >> normIndex[3];
         
         vertexIndices.insert(vertexIndices.end(), vertIndex, vertIndex+(sizeof(vertIndex)/sizeof(unsigned int)));
         normalIndices.insert(normalIndices.end(), normIndex, normIndex+(sizeof(normIndex)/sizeof(unsigned int)));
         textIndices.insert(textIndices.end(), textIndex, textIndex+(sizeof(textIndex)/sizeof(unsigned int)));
          
      }
      //library of materials referenced by usemtl
      else if(lineIn.compare("mtllib") == 0){
         ifs >> mtllib;
      }
      else if(lineIn.compare("usemtl") == 0){
         string matIn;
         ifs >> matIn;
         //update ending point of previous material
         if(mats.size() > 0)
            mats.back()->end = vertexIndices.size();
         mats.push_back(new Material(vertexIndices.size(), 0, matIn));
      }
      else if(lineIn.compare("#") == 0){
         ifs.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      }
   }
   
   ifs.close();
   
   //update ending point for final material
   if(mats.size() > 0)
      mats.back()->end = vertexIndices.size();

   //push the points onto the vectors in order
   for(unsigned int i=0; i<vertexIndices.size(); i++){
      unsigned int vertexIndex = vertexIndices[i];
      vec3 vertex = temp_vertices[vertexIndex-1];
      //T and B follow same indexing as vertices
      vec3 tangent = temp_tangents[vertexIndex-1];
      vec3 bitangent = temp_bitangents[vertexIndex-1];
      vertices.push_back(vertex);
      tangents.push_back(tangent);
      bitangents.push_back(bitangent);
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

void set_uniform_parameters(unsigned int p)
{
   int location;
   location = glGetUniformLocation(p,"eye_position");
   glUniform3fv(location,1,eye);
   
   location = glGetUniformLocation(p, "diffuse_irr_map");
   glUniform1i(location,4);
   
   location = glGetUniformLocation(surf, "diffuse_irr_map");
   glUniform1i(location,4);
   
   location = glGetUniformLocation(p, "specular_irr_map");
   glUniform1i(location,1);
   
   location = glGetUniformLocation(surf, "specular_irr_map");
   glUniform1i(location,1);
   
   location = glGetUniformLocation(p, "tex2");
   glUniform1i(location,2);
   
   location = glGetUniformLocation(p, "tex3");
   glUniform1i(location,3);
   
   location = glGetUniformLocation(p, "normal_map");
   glUniform1i(location,7);
   
   location = glGetUniformLocation(p, "shadow_map");
   glUniform1i(location,5);
   
   location = glGetUniformLocation(surf, "shadow_map");
   glUniform1i(location,5);
   
   GLuint index_tangent = glGetAttribLocation(p, "tangent");
   GLuint index_bitangent = glGetAttribLocation(p, "bitangent");
   
   //set up pointer to tangents
   glEnableVertexAttribArray(index_tangent);
   glVertexAttribPointer(index_tangent, 3, GL_FLOAT, GL_FALSE, 3*sizeof(GLfloat), BUFFER_OFFSET((verticeSize+textureSize+normalSize)*sizeof(GLfloat)));
   
   //set up pointer to bitangents
   glEnableVertexAttribArray(index_bitangent);
   glVertexAttribPointer(index_bitangent, 3, GL_FLOAT, GL_FALSE, 3*sizeof(GLfloat), BUFFER_OFFSET((verticeSize+textureSize+normalSize+tangentSize)*sizeof(GLfloat)));
}

void save_matrix(float *ep, float*vp){
   glMatrixMode(GL_TEXTURE); //texture matrix stack
   glActiveTexture(GL_TEXTURE5);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -0.005);
   glScalef(0.5, 0.5, 0.5);
   glTranslatef(1.0, 1.0, 1.0);
   gluPerspective(60.0, 1.0, 0.01, 1000.0);
   gluLookAt(ep[0], ep[1], ep[2], vp[0], vp[1], vp[2], 0.0, 1.0, 0.0);
}

void draw(){

   //render first from the point of view of the light source
   float eyepoint[3], viewpoint[3];
   glClearColor(.8, .6, .62, 1.0);
   glBindFramebufferEXT(GL_FRAMEBUFFER, 1);
   glUseProgram(0);
   for(int k = 0; k < 3; k++){
      eyepoint[k] = light_position[k];
      viewpoint[k] = light_direction[k] + light_position[k];
   }
   viewVolume();
   glLoadIdentity();
   gluLookAt(eyepoint[0],eyepoint[1],eyepoint[2],viewpoint[0],viewpoint[1],viewpoint[2],0.0,1.0,0.0);
   glLightfv(GL_LIGHT0,GL_POSITION,light_position);
   glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
   material("look");
   
   //draw teapot
   glDrawArrays(GL_QUADS, 0, sides);
   
   //draw flat surface
   glRotatef(63.6,0.0,1.0,0.0);
   glTranslatef(0,-1.65,1);
   glRotatef(90.0,1.0,0.0,0.0);
   glBegin(GL_QUADS);
   glTexCoord2f(1,0);
   glVertex3f(-5, -2, -1.5);
   glTexCoord2f(0,0);
   glVertex3f(4, -2, -1.5);
   glTexCoord2f(0,1);
   glVertex3f(4, 2, -1.5);
   glTexCoord2f(1,1);
   glVertex3f(-5, 2, -1.5);
   glEnd();
   glFlush();
   
   glBindFramebufferEXT(GL_FRAMEBUFFER, 0);
   save_matrix(eyepoint, viewpoint);

   glUseProgram(p);
   set_uniform_parameters(p);
   glActiveTexture(GL_TEXTURE5);
   glBindTexture(GL_TEXTURE_2D, 8);
   
   //set back to original eye point
   viewVolume();
   jitter_view();
   
   int view_pass;
   glClear(GL_ACCUM_BUFFER_BIT);
   
   //diff map
   glActiveTexture(GL_TEXTURE4);
   glBindTexture(GL_TEXTURE_2D,4);
   //spec map
   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D,1);
   //texture 1 for pot
   glActiveTexture(GL_TEXTURE2);
   glBindTexture(GL_TEXTURE_2D,2);
   //texture 2 for pot
   glActiveTexture(GL_TEXTURE3);
   glBindTexture(GL_TEXTURE_2D,3);
   //normal map
   glActiveTexture(GL_TEXTURE7);
   glBindTexture(GL_TEXTURE_2D,7);
   
   //shadow map
   glActiveTexture(GL_TEXTURE5);
   glBindTexture(GL_TEXTURE_2D,8);
   
   for(view_pass=0; view_pass < VPASSES; view_pass++){
      jitter_view();
      glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
      //this allows for multiple materials
      for(unsigned i=0; i<mats.size(); i++){
         if(!material(mats[i]->mat)){
            cout << "Couldn't load material\n";
            exit(-1);
         }
         //if we wanted to load textures from library file, we'd have to change the current texture(s) here.
         glDrawArrays(GL_QUADS, mats[i]->start, mats[i]->end-mats[i]->start);
      }
      glAccum(GL_ACCUM, 1.0/(float)(VPASSES));
   }
   glAccum(GL_RETURN, 1.0);
   glutSwapBuffers();
   
    //change to surface shading
    glUseProgram(surf);
    
    glActiveTexture(GL_TEXTURE2);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_MULTISAMPLE);
    
    GLuint location = glGetUniformLocation(surf, "tex");
    glUniform1i(location,2);
    
    //Vertical Wall
    glBindTexture(GL_TEXTURE_2D, 6);
    glRotatef(63.6,0.0,1.0,0.0);
    glBegin(GL_QUADS);
    glTexCoord2f(1,0);
    glVertex3f(-5, -2, -2.5);
    glTexCoord2f(0,0);
    glVertex3f(5, -2, -2.5);
    glTexCoord2f(0,1);
    glVertex3f(5, 2, -2.5);
    glTexCoord2f(1,1);
    glVertex3f(-5, 2, -2.5);
    glEnd();
    glFlush();
    
    //Horizontal Surface
    glBindTexture(GL_TEXTURE_2D, 5);
    glTranslatef(0,-1.65,1);
    glRotatef(90.0,1.0,0.0,0.0);
    glBegin(GL_QUADS);
    glTexCoord2f(1,0);
    glVertex3f(-5, -2, -1.5);
    glTexCoord2f(0,0);
    glVertex3f(4, -2, -1.5);
    glTexCoord2f(0,1);
    glVertex3f(4, 2, -1.5);
    glTexCoord2f(1,1);
    glVertex3f(-5, 2, -1.5);
    glEnd();
    glFlush();
    

    //Hemisphere
    /*GLUquadric* qptr=gluNewQuadric();
    gluQuadricTexture(qptr,1);
    gluQuadricOrientation(qptr,GLU_INSIDE);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,0);
    glEnable(GL_TEXTURE_2D);
    glRotatef(-90.0,0.0,0.0,1.0);
    glRotatef(60.0,0.0,1.0,0.0);
    glRotatef(90.0,0.0,0.0,1.0);
    gluSphere(qptr,40.0,64,64);
    */
}

GLuint readImage(const char * imagepath, GLuint id){
    unsigned char header[54];
    unsigned int dataPos;
    unsigned int width, height;
    unsigned int imageSize;
    unsigned char * data;
    
    FILE * file = fopen(imagepath,"rb");
    if (!file){
        printf("Image could not be opened\n"); return 0;
    }
    
    if ( fread(header, 1, 54, file)!=54 ){
        printf("Not a correct BMP file\n");
        return false;
    }
    
    dataPos    = *(int*)&(header[0x0A]);
    imageSize  = *(int*)&(header[0x22]);
    width      = *(int*)&(header[0x12]);
    height     = *(int*)&(header[0x16]);

    if (imageSize==0)imageSize=width*height*3;
    if (dataPos==0)dataPos=54;
    
    data = new unsigned char [imageSize];
    fread(data,1,imageSize,file);
    fclose(file);
    
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    if (id==1 || id==4){
      glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
    }
    delete [] data;

    return id;
}

void loadTextures(string files[]){
    GLuint t1 = readImage(files[0].c_str(), 1); //specular map
    GLuint t2 = readImage(files[1].c_str(), 2);
    GLuint t3 = readImage(files[2].c_str(), 3);
    GLuint t4 = readImage(files[3].c_str(), 4); //diffuse map
    GLuint t5 = readImage(files[4].c_str(), 5);
    GLuint t6 = readImage(files[5].c_str(), 6);
    GLuint t7 = readImage(files[6].c_str(), 7); //normal map
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

unsigned int set_shaders(int o, GLuint& program){
   GLint vertCompiled, fragCompiled;
   char *vs, *fs;
   GLuint v, f;

   v = glCreateShader(GL_VERTEX_SHADER);
   f = glCreateShader(GL_FRAGMENT_SHADER);
   if(o == 1){
      vs = read_shader_program("teapot.vert");
      fs = read_shader_program("teapot.frag");
   }
   else if(o == 0){
      vs = read_shader_program("surf.vert");
      fs = read_shader_program("surf.frag");      
   }
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

   program = glCreateProgram();
   glAttachShader(program,f);
   glAttachShader(program,v);
   glLinkProgram(program);
   //glUseProgram(program);
    
   return(p);
}

void keyboard(unsigned char key, int x, int y){
   switch(key){
      case 'q': 
        glDeleteBuffers(1, &mybuf);
           
           glActiveTexture(0);
           glDisable(GL_TEXTURE_2D);
           glActiveTexture(1);
           glDisable(GL_TEXTURE_2D);
           glActiveTexture(2);
           glDisable(GL_TEXTURE_2D);
           glActiveTexture(3);
           glDisable(GL_TEXTURE_2D);
           glActiveTexture(4);
           glDisable(GL_TEXTURE_2D);
           glActiveTexture(5);
           glDisable(GL_TEXTURE_2D);
           glActiveTexture(6);
           glDisable(GL_TEXTURE_2D);
           glActiveTexture(7);
           glDisable(GL_TEXTURE_2D);
           glActiveTexture(8);
           glDisable(GL_TEXTURE_2D);
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
   cout << ".OBJ file read.\n";

   //fill buffer
   int offset;
   for(unsigned int i = 0; i<vertices.size(); i++){
      VBObuff[(i*3)]=vertices[i].x;
      VBObuff[(i*3)+1]=vertices[i].y;
      VBObuff[(i*3)+2]=vertices[i].z;
      verticeSize+=3;
   } offset = verticeSize;
   for(unsigned int i = 0; i<uvs.size(); i++){
      VBObuff[offset+(i*2)]=uvs[i].x;
      VBObuff[offset+(i*2)+1]=uvs[i].y;
      textureSize+=2;
   } offset += textureSize;
   for(unsigned int i = 0; i<normals.size(); i++){
      VBObuff[offset+(i*3)]=normals[i].x;
      VBObuff[offset+(i*3)+1]=normals[i].y;
      VBObuff[offset+(i*3)+2]=normals[i].z;
      normalSize+=3;
   } offset += normalSize;
   for(unsigned int i = 0; i<tangents.size(); i++){
      VBObuff[offset+(i*3)]=tangents[i].x;
      VBObuff[offset+(i*3)+1]=tangents[i].y;
      VBObuff[offset+(i*3)+2]=tangents[i].z;
      tangentSize+=3;
   } offset += tangentSize;
   for(unsigned int i = 0; i<bitangents.size(); i++){
      VBObuff[offset+(i*3)]=bitangents[i].x;
      VBObuff[offset+(i*3)+1]=bitangents[i].y;
      VBObuff[offset+(i*3)+2]=bitangents[i].z;
   } sides=vertices.size();
    

   initOGL(argc, argv);
   glutKeyboardFunc(keyboard);
    
   set_shaders(1, p);
   set_shaders(0, surf);
   //spec map
   //teapot tex1
   //teapot tex2
   //diff map
   //table tex
   //background
   //normal map
   string textures[] = {"test3.bmp", "glaze_2.bmp", "metal.bmp", "diff_map_5.bmp", "test6.bmp", "kitchen_back.bmp", "normal_map.bmp"};
   loadTextures(textures);
   
   glutDisplayFunc(draw);
   glutMainLoop();
   
   for(unsigned i = 0; i < mats.size(); i++){
      delete mats[i];
   }
   
   return 0;
}

