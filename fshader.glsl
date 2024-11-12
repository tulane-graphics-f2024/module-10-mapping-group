#version 150

in vec4 pos;              // Position in eye space
in vec4 N;                // Normal in eye space
in vec2 texCoord;         // Texture coordinates

uniform sampler2D textureEarth;    // Day texture (Blue Marble)
uniform sampler2D textureNight;    // Night texture (Black Marble)
uniform sampler2D textureCloud;    // Cloud texture
uniform vec4 ambient;              // Ambient light color
uniform vec4 LightPosition;        // Light position in eye coordinates

out vec4 fragColor;

void main() {
    // Normalize the normal vector
    vec3 normal = normalize(N.xyz);
    
    // Calculate the light direction and diffuse factor
    vec3 lightDir = normalize(vec3(LightPosition - pos));
    float diffuse = max(dot(lightDir, normal), 0.0);

    // Sample day and night textures
    vec4 dayColor = texture(textureEarth, texCoord);
    vec4 nightColor = texture(textureNight, texCoord);

    // Blend day and night textures based on the diffuse factor
    vec4 baseColor = mix(nightColor, dayColor, diffuse);

    // Sample cloud texture and overlay it on the base color
    vec4 cloudColor = texture(textureCloud, texCoord);
    baseColor = clamp(baseColor + cloudColor, 0.0, 1.0);

    // Add night lights based on how dark it is
    float nightFactor = smoothstep(0.0, 0.5, 0.3 - diffuse); // Transition from day to night
    vec4 nightLightsColor = nightFactor * nightColor;        // Night lights appear in dark areas

    // Final color, with ambient lighting added
    fragColor = ambient + baseColor + nightLightsColor;
    fragColor = clamp(fragColor, 0.0, 1.0);  // Ensure color values are within [0, 1]
    fragColor.a = 1.0;  // Fully opaque
}
