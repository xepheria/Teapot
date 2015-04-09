varying vec3 ec_vnormal, ec_vposition;
void main(){
   vec3 P, N, L, V, H;
   vec4 diffuse_color = gl_FrontMaterial.diffuse; 
   vec4 specular_color = gl_FrontMaterial.specular; 
   float shininess = gl_FrontMaterial.shininess;

   P = ec_vposition;
   N = normalize(ec_vnormal);
   V = normalize(-P);

   vec4 final_color=vec4(0,0,0,1);
   //int i = 0;
   for(int i=0; i<3; i++){
      L = normalize(vec3(gl_LightSource[i].position)-P);
      float lambertTerm = dot(N,L);
      if (lambertTerm > 0.0){
         final_color += gl_LightSource[i].diffuse * diffuse_color* lambertTerm;	
         vec3 N = normalize(ec_vnormal);
         vec3 H = reflect(-L, N);
         float specular = pow(max(dot(H, N), 0.0), shininess);
         final_color += gl_LightSource[i].specular * shininess* specular;
      }	
   }
   gl_FragColor=final_color;

}
