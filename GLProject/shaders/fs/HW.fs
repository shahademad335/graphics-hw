#version 460 core
out vec4 FragColor;

in vec2 texCoord;

uniform vec3 objectColor;
uniform sampler2D texture_diffuse1;
uniform float alpha;   
uniform int torn;      

void main()
{
    if (torn == 1)
    {
        float edge = 0.86 + 0.035 * sin(texCoord.y * 47.0)
                          + 0.020 * sin(texCoord.y * 131.0 + 1.7)
                          + 0.010 * sin(texCoord.y * 263.0);
        if (texCoord.x > edge)
            discard;   
        float cut = 0.22 + 0.02 * sin(texCoord.x * 90.0);
        if (texCoord.x + texCoord.y < cut)
            discard;
    }

    vec4 tex = texture(texture_diffuse1, texCoord);
    FragColor = vec4(tex.rgb * objectColor, tex.a * alpha);
}
