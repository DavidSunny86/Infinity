#version 150

#define MAX_LOD_COUNT 8

uniform vec3		uOffset;
uniform vec3		uScale;

uniform vec2		uPatchBase;
uniform int			uLevel;

uniform vec3		uViewPos;

uniform sampler2D	uHeightmap;
uniform vec4		uHMDim;

// Morph parameter are evaluated as follows:
// uMorphParam[...].x = 1.0/(morphEnd-morphStart)
// uMorphParam[...].y = -morphStart/(morphEnd-morphStart)
uniform vec2		uMorphParams[MAX_LOD_COUNT];

float fetchHeight(vec2 gridPos)
{
//Add support for proper mipmapping
	vec2 tc = (gridPos+vec2(0.5))*uHMDim.zw;
	return textureLod(uHeightmap, tc, 0).r;
}

vec3 getWorldPos(vec2 gridPos)
{
	//clamping to match terrain dimentions
	gridPos = clamp(gridPos, vec2(0), uHMDim.xy-vec2(1.0));
	return uOffset+uScale*vec3(gridPos.x, fetchHeight(gridPos), gridPos.y);
}

void main()
{
	vec2	patchPos, gridPos;
	vec3	worldPos;
	vec2	patchScale = vec2(1<<uLevel);
	
	patchPos = gl_Vertex.xy;
	gridPos = uPatchBase+patchScale*patchPos;
	worldPos = getWorldPos(gridPos);
	
	//Applying morphing for seamless connectivity
	vec2 morphDir = fract(patchPos*vec2(0.5))*vec2(2.0);
	vec2 gridMorphDest = gridPos + patchScale*morphDir;
	vec3 worldMorphDest = getWorldPos(gridMorphDest);
	
	float	distance, morphK;

	distance = length(worldPos-uViewPos);
	morphK = clamp(distance*uMorphParams[uLevel].x+uMorphParams[uLevel].y, 0.0, 1.0);
	worldPos = mix(worldPos, worldMorphDest, morphK);
	
	gl_FrontColor = gl_BackColor = gl_Color;
	gl_Position = gl_ModelViewProjectionMatrix*vec4(worldPos, 1);
}

