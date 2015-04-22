varying vec3 ec_vnormal, ec_vposition, ec_reflect;

varying vec4 tcoords;

uniform sampler2D diffuse_irr_map;
uniform sampler2D specular_irr_map;
uniform sampler2D tex;
uniform sampler2D shadow_map;

void main()
{
    vec2 d_irr_index, s_irr_index;
    vec3 N, R, d_irr, s_irr, mapN;
    vec4 diffuse_color = texture2D(tex, gl_TexCoord[0].st);
    vec4 specular_color = gl_FrontMaterial.specular;
    float shininess = gl_FrontMaterial.shininess;
    float depthsample, clarity;
    vec4 pccoords;
    
    // Avoid map edges.
    N = 0.99*normalize(ec_vnormal);
    R = 0.99*normalize(ec_reflect);
    
    // Rescale coordinates to [0,1].
    d_irr_index = 0.5*(N.st + vec2(1.0,1.0));
    s_irr_index = 0.5*(R.st + vec2(1.0,1.0));

    // Look up texture values.
    d_irr = vec3(texture2D(diffuse_irr_map, d_irr_index));
    s_irr = vec3(texture2D(specular_irr_map, s_irr_index));

    specular_color *= vec4(s_irr,1.0);
    diffuse_color *= vec4(d_irr,1.0);

    pccoords = tcoords/tcoords.w; //4D -> 3D
    depthsample = texture2D(shadow_map, pccoords.st).z;
    clarity = 1.0;
    //if simulated z is larger than value in depth map, point in shadow
    if(depthsample < pccoords.z) clarity = 0.5;
    diffuse_color *= clarity;
    specular_color *= clarity;

    gl_FragColor = diffuse_color + specular_color;
}
