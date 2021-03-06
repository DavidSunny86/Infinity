#version 430

layout(std140, column_major) uniform;

#define ATTR_PATCH_BASE 0
#define ATTR_LEVEL      1

#define UNI_TERRAIN_BINDING  0
#define UNI_VIEW_BINDING     1

layout(location = ATTR_PATCH_BASE)  in vec2  aPatchBase;
layout(location = ATTR_LEVEL)       in int   aLevel;

#define MAX_LOD_COUNT 8

layout(binding = UNI_TERRAIN_BINDING) uniform uniTerrain
{
    vec4    uAABB;
    vec4    uUVXform;
    vec2    uHeightXform;

    float   uPatchScales[MAX_LOD_COUNT];
    // Morph parameter are evaluated as follows:
    // uMorphParam[...].x = 1.0/(morphEnd-morphStart)
    // uMorphParam[...].y = -morphStart/(morphEnd-morphStart)
    vec2    uMorphParams[MAX_LOD_COUNT];
    vec4    uColors[MAX_LOD_COUNT];
};

layout(binding = UNI_VIEW_BINDING) uniform uniView
{
    mat4    au_MV;
    vec4    au_Proj;
    vec4    uLODViewK;
};

layout(binding = 0) uniform sampler2D uHeightmap;

out float vHeight;
out vec4  vColor;
out vec3  vPos;
out vec2  vUV;

void main()
{
    ivec2 ivertex = ivec2(gl_VertexID % 9, gl_VertexID / 9);
    vec2  vertex  = vec2(ivertex);

    float patchScale = uPatchScales[aLevel];
    vec2  morphDir   = fract(vertex*0.5)*2*(vec2(2)-fract(vertex*0.25)*4);

    vec3  vertexPos;

    vertexPos.xz = aPatchBase + patchScale*vertex;
    vertexPos.xz = clamp(vertexPos.xz, uAABB.xy, uAABB.zw);
    vertexPos.y  = 0.0;

    float distance = length(vertexPos + uLODViewK.xyz);
    float morphK   = clamp(distance*uMorphParams[aLevel].x + uMorphParams[aLevel].y, 0.0, 1.0);

    vertexPos.xz += patchScale*morphDir*morphK; //Applying morphing for seamless connectivity

    vec2  uv = vertexPos.xz*uUVXform.xy+uUVXform.zw;
    float h  = textureLod(uHeightmap, uv, 0).r;

    vertexPos.y   = h*uHeightXform.x+uHeightXform.y;

    vec4 viewPos = au_MV*vec4(vertexPos, 1);

    //Varyings

    gl_Position = vec4(viewPos.xy * au_Proj.xy, au_Proj.z*viewPos.z+au_Proj.w, -viewPos.z); //projection
    vHeight     = h;
    vPos        = vertexPos;
    vColor      = uColors[aLevel.x];
    vUV         = (vertex + morphDir*morphK) / 8.0 * 64.0/8.0;
}

