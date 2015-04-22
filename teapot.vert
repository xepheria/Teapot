varying vec3 ec_vnormal, ec_vposition, ec_reflect, ec_vtangent, ec_vbitangent;
attribute vec3 tangent, bitangent;

varying vec4 tcoords;

void main(){
   ec_vnormal = gl_NormalMatrix*gl_Normal;
   ec_vtangent = gl_NormalMatrix*tangent;
   ec_vbitangent = gl_NormalMatrix*bitangent;
   ec_vposition= vec3(gl_ModelViewMatrix*gl_Vertex);
   gl_Position = gl_ProjectionMatrix*gl_ModelViewMatrix*gl_Vertex;
   gl_TexCoord[0] = gl_MultiTexCoord0;
    
   vec3 ec_eyedir = normalize(-ec_vposition.xyz);
   ec_reflect = -ec_eyedir + 2.0*(dot(ec_eyedir,ec_vnormal))*ec_vnormal;
   
   //from simulated shadow render
   tcoords = gl_TextureMatrix[5]*gl_Vertex;
}

