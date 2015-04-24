varying vec3 ec_vnormal, ec_vposition, ec_reflect, ec_vtangent, ec_vbitangent;

varying vec4 tcoords;

uniform sampler2D diffuse_irr_map;
uniform sampler2D specular_irr_map;
uniform sampler2D tex2;
uniform sampler2D tex3;
uniform sampler2D normal_map;
uniform sampler2D shadow_map;

void main()
{
    mat3 tform;
    vec2 d_irr_index, s_irr_index;
    vec3 N, R, d_irr, s_irr, mapN;
    vec4 diffuse_color = mix(texture2D(tex2, gl_TexCoord[0].st)
                              ,texture2D(tex3, gl_TexCoord[0].st)
                             ,.01);
    vec4 specular_color = vec4(.2,.2,.2,1);//gl_FrontMaterial.specular;
 
    float pi = 3.14159;
    float depthsample, clarity;
    vec4 pccoords;
    
    //create a 3x3 matrix from T, B, and N as columns:
    tform = mat3(ec_vtangent, ec_vbitangent, ec_vnormal);
    
    //read map
    mapN = vec3(texture2D(normal_map,gl_TexCoord[0].st));
    //x, y, and z are in [0,1] but x and y should be in [-1,1].
    mapN.xy = 2.0*mapN.xy - vec2(1.0,1.0);
    
    // Avoid map edges.
    N = 0.99*normalize(tform*normalize(mapN));
    R = 0.99*normalize(ec_reflect);
    
    // Rescale coordinates to [0,1].
    d_irr_index = 0.5*(N.st + vec2(1.0,1.0));
    s_irr_index = 0.5*(R.st + vec2(1.0,1.0));

    // Look up texture values.
    d_irr = vec3(texture2D(diffuse_irr_map, d_irr_index));
    s_irr = vec3(texture2D(specular_irr_map, s_irr_index));

    diffuse_color *= vec4(d_irr,1.0);
    specular_color *= vec4(s_irr,1.0);


/*
    pccoords = tcoords/tcoords.w; //4D -> 3D
    depthsample = texture2D(shadow_map, pccoords.st).z;
    clarity = 1.0;
    //if simulated z is larger than value in depth map, point in shadow
    if(depthsample < pccoords.z) clarity = 0.5;
    diffuse_color *= clarity;
    specular_color *= clarity;
*/
    gl_FragColor = diffuse_color + .5*specular_color;
}
