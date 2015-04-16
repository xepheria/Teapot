varying vec3 ec_vnormal, ec_vposition, ec_reflect;


uniform sampler2D tex1;
uniform sampler2D tex2;
uniform sampler2D tex3;
uniform sampler2D tex4;

void main()
{
    vec2 d_irr_index, s_irr_index;
    vec3 N, R, d_irr, s_irr;
    vec4 diffuse_color = mix(texture2D(tex2, gl_TexCoord[0].st)
                             ,texture2D(tex3, gl_TexCoord[0].st)
                             ,.001);
    vec4 specular_color = vec4(.0,.1,.15,1.0);//gl_FrontMaterial.specular;
    //specular_color = vec4(.8,.8,.8,1.0);
    
    // Avoid map edges.
    N = 0.99*normalize(ec_vnormal);
    R = 0.99*normalize(ec_reflect);
    
    // Rescale coordinates to [0,1].
    d_irr_index = 0.5*(N.st + vec2(1.0,1.0));
    s_irr_index = 0.5*(R.st + vec2(1.0,1.0));
    
    // Look up texture values.
    d_irr = vec3(texture2D(tex4, d_irr_index));
    s_irr = vec3(texture2D(tex1, s_irr_index));
    
    specular_color *= vec4(s_irr,1.0);
    diffuse_color *= vec4(d_irr,1.0);
  

    gl_FragColor = diffuse_color+ 0.3*specular_color;
}
