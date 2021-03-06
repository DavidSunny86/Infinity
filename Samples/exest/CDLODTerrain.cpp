#include "CDLODTerrain.h"

struct PatchData
{
    float baseX, baseZ;
    int   level;
    float padding;
};

struct TerrainData
{
    v128 uAABB;
    v128 uUVXform;
    v128 uHeightXform;
    v128 uPatchScales[CDLODTerrain::MAX_LOD_COUNT];
    v128 uMorphParams[CDLODTerrain::MAX_LOD_COUNT];
    v128 uColors[CDLODTerrain::MAX_LOD_COUNT];
};

struct ViewData
{
    ml::mat4x4  uMV;
    v128        uProj;
    v128        uLODViewK;
};

#define ATTR_PATCH_BASE 0
#define ATTR_LEVEL      1

#define UNI_TERRAIN_BINDING 0
#define UNI_VIEW_BINDING    1
#define UNI_PATCH_BINDING   2

void CDLODTerrain::initialize()
{
    // Create instanced terrain program
    prgTerrain  = res::createProgramFromFiles("Terrain.CDLOD.Instanced.vert", "Terrain.SHLighting.frag");

#ifdef _DEBUG
    {
        GLint structSize;
        GLint uniTerrain, uniView;

        uniTerrain  = glGetUniformBlockIndex(prgTerrain, "uniTerrain");
        uniView     = glGetUniformBlockIndex(prgTerrain, "uniView");

        glGetActiveUniformBlockiv(prgTerrain, uniTerrain, GL_UNIFORM_BLOCK_DATA_SIZE, &structSize);
        assert(structSize==sizeof(TerrainData));
        glGetActiveUniformBlockiv(prgTerrain, uniView, GL_UNIFORM_BLOCK_DATA_SIZE, &structSize);
        assert(structSize==sizeof(ViewData));
    }
#endif

    glCreateTextures(GL_TEXTURE_2D, 1, &mipTexture);
    glTextureStorage2D(mipTexture, 6, GL_RGBA8, 32, 32);
    glTextureParameteri(mipTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(mipTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(mipTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(mipTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    uint32_t levelColor[6] = 
    {
        0xCCFF0000,
        0x66FF8000,
        0x00FFFFFF,
        0x3300B3FF,
        0x66004DFF,
        0xCC0000FF
    };

    for (GLint i=0; i<6; ++i)
    {
        glClearTexImage(mipTexture, i, GL_RGBA, GL_UNSIGNED_BYTE, &levelColor[i]);
    }

    glCreateBuffers(1, &ibo);

    gfx::vertex_element_t ve[2] = {
        {0, 0, ATTR_PATCH_BASE, GL_INT,   2, GL_TRUE,  GL_FALSE},
        {0, 8, ATTR_LEVEL,      GL_FLOAT, 1, GL_FALSE, GL_FALSE}
    };

    GLuint div = 1;

    vao = gfx::createVAO(2, ve, 1, &div);

    glBindVertexArray(vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBindVertexArray(0);

    glCreateBuffers(1, &ubo);
    glNamedBufferStorage(ubo, sizeof(TerrainData), 0, GL_MAP_WRITE_BIT);

    ubufView = gfx::createUBODesc(prgTerrain, "uniView");

    gfx::gpu_timer_init(&gpuTimer);
}

void CDLODTerrain::reset()
{
}

void CDLODTerrain::cleanup()
{
    gfx::gpu_timer_fini(&gpuTimer);

    glDeleteBuffers(1, &ubo);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &ibo);
    glDeleteProgram(prgTerrain);
    glDeleteTextures(1, &mHeightmapTex);
    glDeleteTextures(1, &mipTexture);

    gfx::destroyUBODesc(ubufView);
}

void CDLODTerrain::setSelectMatrix(v128 m[4])
{
    // Attention! Column order is changed!
    selectionMVP.r0 = m[0];
    selectionMVP.r1 = m[2];
    selectionMVP.r2 = m[1];
    selectionMVP.r3 = m[3];
}

inline bool patchIntersectsCircle(v128 vmin, v128 vmax, v128 vcenter, float radius)
{
    v128  vdist;
    float d2;

    vdist = ml::distanceToAABB(vmin, vmax, vcenter);
    d2 = vi_dot2(vdist, vdist);

    return d2<=radius*radius;
}

void CDLODTerrain::selectQuadsForDrawing(size_t level, float bx, float bz, float patchSize, bool skipFrustumTest)
{
    if (bx>=maxX || bz>=maxZ) return;

    if (patchCount>=MAX_PATCH_COUNT) return;

    ml::aabb AABB;

    v128 vp = vi_set(viewPoint.x, viewPoint.z, 0.0f, 0.0f);

    float sizeX = core::min(patchSize, maxX-bx);
    float sizeZ = core::min(patchSize, maxZ-bz);

    float minX = bx;
    float minZ = bz;
    float maxX = minX+sizeX;
    float maxZ = minZ+sizeZ;

    AABB.min = vi_set(minX, minZ, minY, 1.0f);
    AABB.max = vi_set(maxX, maxZ, maxY, 1.0f);

    if (!skipFrustumTest)
    {
        //Check whether AABB intersects frustum
        int result = ml::intersectionTest(&AABB, &selectionMVP);

        if (result==ml::IT_OUTSIDE) return;
        skipFrustumTest = result==ml::IT_INSIDE;
    }

    if (level==maxLevel || !patchIntersectsCircle(AABB.min, AABB.max, vp, LODRange[level]))
    {
        instData->baseX = bx;
        instData->baseZ = bz;
        instData->level = level;

        ++instData;
        ++patchCount;
    }
    else
    {
        //TODO: add front to back sorting to minimize overdraw(optimization)
        size_t levelNext = level-1;
        float  offset    = patchSize/2;
        selectQuadsForDrawing(levelNext, bx,        bz,        offset, skipFrustumTest);
        selectQuadsForDrawing(levelNext, bx+offset, bz,        offset, skipFrustumTest);
        selectQuadsForDrawing(levelNext, bx,        bz+offset, offset, skipFrustumTest);
        selectQuadsForDrawing(levelNext, bx+offset, bz+offset, offset, skipFrustumTest);
    }
}

ml::vec4 colors[] = 
{
    {0.5f, 0.0f, 1.0f, 1.0f},
    {1.0f, 1.0f, 0.0f, 1.0f},
    {1.0f, 0.6f, 0.6f, 1.0f},
    {1.0f, 0.0f, 1.0f, 1.0f},
    {0.0f, 0.0f, 1.0f, 1.0f},
    {1.0f, 0.2f, 0.2f, 1.0f},
    {0.0f, 1.0f, 0.0f, 1.0f},
    {0.0f, 1.0f, 1.0f, 1.0f}
};

void CDLODTerrain::generateGeometry(size_t vertexCount)
{
#define INDEX_FOR_LOCATION(x, y) ((y)*vertexCount+(x))

    size_t quadsInRow = (vertexCount-1);

    idxCount = quadsInRow*quadsInRow*6;
    glNamedBufferData(ibo, sizeof(uint16_t)*idxCount, 0, GL_STATIC_DRAW);
    uint16_t* ptr2 = (uint16_t*)glMapNamedBufferRange(ibo, 0, sizeof(uint16_t)*idxCount, GL_MAP_WRITE_BIT);
    uint16_t vtx0 = 0;
    uint16_t vtx1 = 1;
    uint16_t vtx2 = vertexCount;
    uint16_t vtx3 = vertexCount + 1;
    for (size_t y=0; y<quadsInRow; ++y)
    {
        for (size_t x=0; x<quadsInRow; ++x)
        {
            if ((x + y) % 2 == 0)
            {
                *ptr2++ = vtx0; *ptr2++ = vtx2; *ptr2++ = vtx1;
                *ptr2++ = vtx1; *ptr2++ = vtx2; *ptr2++ = vtx3;
            }
            else
            {
                *ptr2++ = vtx2; *ptr2++ = vtx3; *ptr2++ = vtx0;
                *ptr2++ = vtx0; *ptr2++ = vtx3; *ptr2++ = vtx1;
            }
            ++vtx0;
            ++vtx1;
            ++vtx2;
            ++vtx3;
        }
        ++vtx0;
        ++vtx1;
        ++vtx2;
        ++vtx3;
    }

#undef INDEX_FOR_LOCATION

    glUnmapNamedBuffer(ibo);
}

void CDLODTerrain::drawTerrain()
{
    PROFILER_CPU_TIMESLICE("CDLODTerrain");
    cpu_timer_start(&cpuTimer);

    GLuint baseInstance;

    instData = gfx::frameAllocVertices<PatchData>(MAX_PATCH_COUNT, &baseInstance);

    vertDistToTerrain = core::max(viewPoint.y-maxY, minY-viewPoint.y);
    vertDistToTerrain = core::max(vertDistToTerrain, 0.0f);

    maxLevel = 1;
    while (maxLevel<LODCount && vertDistToTerrain>LODRange[maxLevel])
        maxLevel++;
    --maxLevel;

    {
        PROFILER_CPU_TIMESLICE("Select");
        cpu_timer_start(&cpuSelectTimer);
        patchCount = 0;
        size_t level = LODCount-1;
        for (float bz=minZ; bz<maxZ; bz+=chunkSize)
            for (float bx=minX; bx<maxX; bx+=chunkSize)
                selectQuadsForDrawing(level, bx, bz, chunkSize);
        cpu_timer_stop(&cpuSelectTimer);
    }

    cpu_timer_start(&cpuRenderTimer);
    gfx::gpu_timer_start(&gpuTimer);

    patchCount = core::min(patchCount, maxPatchCount);
    if (patchCount)
    {
        PROFILER_CPU_TIMESLICE("Render");

        {
            PROFILER_CPU_TIMESLICE("BindProgram");
            glUseProgram(prgTerrain);
        }

        {
            PROFILER_CPU_TIMESLICE("SetTextures");
            GLuint textureSet[2] = {mHeightmapTex, mipTexture};
            glBindTextures(0, 2, textureSet);
        }

        {
            PROFILER_CPU_TIMESLICE("Uniforms");
            GLuint    offset;
            ViewData* ptrViewData;
            
            ptrViewData = (ViewData*) gfx::dynbufAllocMem(sizeof(ViewData), gfx::caps.uboAlignment, &offset);
            //memcpy(&ptrViewData->uMVP, &viewMVP, sizeof(viewMVP));
            gfx::updateUBO(ubufView, ptrViewData, sizeof(ViewData));
            ptrViewData->uLODViewK = vi_set(-viewPoint.x, vertDistToTerrain, -viewPoint.z, 0.0f);

            glBindBufferRange(GL_UNIFORM_BUFFER, UNI_TERRAIN_BINDING, ubo,                 0, sizeof(TerrainData));
            glBindBufferRange(GL_UNIFORM_BUFFER, UNI_VIEW_BINDING,    gfx::dynBuffer, offset, sizeof(ViewData));
        }

        {
            PROFILER_CPU_TIMESLICE("BindGeom");
            glBindVertexArray(vao);
            glBindVertexBuffer(0, gfx::dynBuffer, 0, sizeof(PatchData));
        }

        {
            PROFILER_CPU_TIMESLICE("DIP");
            glDrawElementsInstancedBaseInstance(GL_TRIANGLES, idxCount, GL_UNSIGNED_SHORT, 0, patchCount, baseInstance);
        }

        {
            PROFILER_CPU_TIMESLICE("ClearBindings");
            glBindVertexArray(0);
       }
    }

    gfx::gpu_timer_stop(&gpuTimer);
    cpu_timer_stop(&cpuRenderTimer);
    cpu_timer_stop(&cpuTimer);
}

void CDLODTerrain::setHeightmap(uint16_t* data, size_t width, size_t height)
{
    //TODO: LODCount should clamped based on pixelsPerChunk
    LODCount  = 5;
    patchDim  = 8;
    chunkSize = 100;//patchSize[LODCount-1]*cellSize;

    float  pixelsPerChunk = 129;
    float  heightScale;
    float  cellSize = chunkSize/(pixelsPerChunk-1);
    //TODO: morphZoneRatio should should be less then 1-patchDiag/LODDistance
    float  morphZoneRatio = 0.30f;

    maxPatchCount = MAX_PATCH_COUNT;

    minX = -0.5f*cellSize*(width-1);
    minZ = -0.5f*cellSize*(height-1);
    maxX = minX+(width-1)*cellSize;
    maxZ = minZ+(height-1)*cellSize;

    heightScale = 65535.0f/732.0f;
    minY        = -39.938129f;//0.0f;
    maxY        =  133.064011f;//heightScale;
    heightScale = maxY-minY;

    float du = 1.0f/width;
    float dv = 1.0f/height;

    float pixelsPerMeter = (pixelsPerChunk-1)/chunkSize;

    generateGeometry(patchDim+1);

    TerrainData& terrainData = *(TerrainData*) glMapNamedBufferRange(ubo, 0, sizeof(TerrainData), GL_MAP_WRITE_BIT);

    assert(LODCount<=MAX_LOD_COUNT);

    float size2=cellSize*cellSize*patchDim*patchDim,
          range=100.0f, curRange=0.0f;

    for (size_t i=0; i<LODCount; ++i)
    {
        //To avoid cracks when diagonal is greater then minRange,
        float minRange = 2.0f*ml::sqrt(size2+size2);
        range = core::max(range, minRange);

        LODRange[i]  = curRange;
        curRange    += range;

        float rangeEnd   = curRange;
        float morphRange = range*morphZoneRatio;
        float morphStart = rangeEnd-morphRange;

        terrainData.uMorphParams[i] = vi_set(1.0f/morphRange, -morphStart/morphRange, 0.0f, 0.0f);
        terrainData.uPatchScales[i] = vi_set_all(cellSize);

        range    *= 1.5f;
        size2    *= 4.0f;
        cellSize *= 2.0f;
    }

    //Fix unnecessary morph in last LOD level
    terrainData.uMorphParams[LODCount-1] = vi_set_zero();

    terrainData.uAABB        = vi_set(minX, minZ, maxX, maxZ);
    terrainData.uUVXform     = vi_set(pixelsPerMeter*du, pixelsPerMeter*dv, (-minX*pixelsPerMeter+0.5f)*du, (-minZ*pixelsPerMeter+0.5f)*dv);
    terrainData.uHeightXform = vi_set(heightScale, minY, 0.0f, 0.0f);
    mem_copy(terrainData.uColors, colors, sizeof(terrainData.uColors));

    glUnmapNamedBuffer(ubo);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glCreateTextures(GL_TEXTURE_2D, 1, &mHeightmapTex);
    glTextureStorage2D(mHeightmapTex, 1, GL_R16, width, height);
    glTextureSubImage2D(mHeightmapTex, 0, 0, 0, width, height, GL_RED, GL_UNSIGNED_SHORT, data);
    glTextureParameteri(mHeightmapTex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(mHeightmapTex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(mHeightmapTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(mHeightmapTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(mHeightmapTex, GL_TEXTURE_MAX_LEVEL, 0);
}
