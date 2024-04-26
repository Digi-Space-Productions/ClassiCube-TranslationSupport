#include "Core.h"
#ifdef CC_BUILD_NDS
#include "_GraphicsBase.h"
#include "Errors.h"
#include "Logger.h"
#include "Window.h"
#include <nds.h>

/*########################################################################################################################*
*---------------------------------------------------------General---------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_Create(void) {
	Gfx_RestoreState();

	Gfx.MinTexWidth  =   8;
	Gfx.MinTexHeight =   8;
	Gfx.MaxTexWidth  = 256;
	Gfx.MaxTexHeight = 256;
    //Gfx.MaxTexSize   = 256 * 256;
	Gfx.Created      = true;
    glInit();
    
    glClearColor(0, 15, 10, 31);
    glClearPolyID(63);
    glAlphaFunc(7);

    glClearDepth(0x7FFF);
    glViewport(0, 0, 255, 191);
    
    vramSetBankA(VRAM_A_TEXTURE);
    vramSetBankB(VRAM_B_TEXTURE);
    vramSetBankC(VRAM_C_TEXTURE);
    vramSetBankD(VRAM_D_TEXTURE);
    
    glPolyFmt(POLY_ALPHA(31) | POLY_CULL_NONE);
}

cc_bool Gfx_TryRestoreContext(void) {
	return true;
}

void Gfx_Free(void) {
	Gfx_FreeState();
	vramSetBankA(VRAM_A_LCD);
    vramSetBankB(VRAM_B_LCD);
    vramSetBankC(VRAM_C_LCD);
    vramSetBankD(VRAM_D_LCD);
}


/*########################################################################################################################*
*-----------------------------------------------------------Misc----------------------------------------------------------*
*#########################################################################################################################*/
cc_result Gfx_TakeScreenshot(struct Stream* output) {
	return ERR_NOT_SUPPORTED;
}

void Gfx_GetApiInfo(cc_string* info) {
	String_AppendConst(info, "-- Using Nintendo DS --\n");
	PrintMaxTextureInfo(info);
}

void Gfx_SetFpsLimit(cc_bool vsync, float minFrameMs) {
	gfx_minFrameMs = minFrameMs;
	gfx_vsync      = vsync;
}

void Gfx_OnWindowResize(void) { 
}

void Gfx_SetViewport(int x, int y, int w, int h) { }

void Gfx_BeginFrame(void) {
}

void Gfx_ClearBuffers(GfxBuffers buffers) {
	// TODO
} 

void Gfx_ClearColor(PackedCol color) {
	int R = PackedCol_R(color) >> 3;
	int G = PackedCol_G(color) >> 3;
	int B = PackedCol_B(color) >> 3;
	glClearColor(R, G, B, 31);
}

void Gfx_EndFrame(void) {
	glFlush(0);
	// TODO not needed?
	swiWaitForVBlank();
 
	if (gfx_minFrameMs) LimitFPS();
}


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
// B8 G8 R8 A8 > R5 G5 B5 A1
#define BGRA8_to_DS(src) \
	((src[2] & 0xF8) >> 3) | ((src[1] & 0xF8) << 2) | ((src[0] & 0xF8) << 7) | ((src[3] & 0x80) << 8);	

static void ConvertTexture(cc_uint16* dst, struct Bitmap* bmp, int rowWidth) {
	for (int y = 0; y < bmp->height; y++)
	{
		cc_uint8* src = (cc_uint8*)(bmp->scan0 + y * rowWidth);
		
		for (int x = 0; x < bmp->width; x++, src += 4)
		{
			*dst++ = BGRA8_to_DS(src);
		}
	}
}

static GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, int rowWidth, cc_uint8 flags, cc_bool mipmaps) {
    vramSetBankA(VRAM_A_TEXTURE);

    cc_uint16* tmp = Mem_TryAlloc(bmp->width * bmp->height, 2);
    if (!tmp) return 0;
    ConvertTexture(tmp, bmp, rowWidth);

    int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(0, textureID);
    glTexImage2D(0, 0, GL_RGBA, bmp->width, bmp->height, 0, TEXGEN_TEXCOORD, tmp);
    glTexParameter(0, GL_TEXTURE_WRAP_S | GL_TEXTURE_WRAP_T);

    cc_uint16* vram_ptr = glGetTexturePointer(textureID);
    if (!vram_ptr) Platform_Log2("No VRAM for %i x %i texture", &bmp->width, &bmp->height);

    Mem_Free(tmp);
	return (void*)textureID;
}

void Gfx_BindTexture(GfxResourceID texId) {
    glBindTexture(0, (int)texId);	
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
    int texture = (int)texId;
    glBindTexture(0, texture);
    
    int width = 0;
    glGetInt(GL_GET_TEXTURE_WIDTH,  &width);
    cc_uint16* vram_ptr = glGetTexturePointer(texture);
    return;
    // TODO doesn't work without VRAM bank changing to LCD and back maybe??
    // (see what glTeximage2D does ??)

    for (int yy = 0; yy < part->height; yy++)
	{
		cc_uint16* dst = vram_ptr + width * (y + yy) + x;
		cc_uint8* src  = (cc_uint8*)(part->scan0 + rowWidth * yy);
		
		for (int xx = 0; xx < part->width; xx++, src += 4, dst++)
		{
			*dst = BGRA8_to_DS(src);
		}
	}
}

void Gfx_DeleteTexture(GfxResourceID* texId) {
    int texture = (int)(*texId);
    if (texture) glDeleteTextures(1, &texture);
    *texId = 0;
}

void Gfx_EnableMipmaps(void) { }
void Gfx_DisableMipmaps(void) { }


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
void Gfx_SetFaceCulling(cc_bool enabled)   { }
void Gfx_SetAlphaBlending(cc_bool enabled) { }
void Gfx_SetAlphaArgBlend(cc_bool enabled) { }

static void SetColorWrite(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
	// TODO
}

void Gfx_SetDepthWrite(cc_bool enabled) { }
void Gfx_SetDepthTest(cc_bool enabled)  { }

static void Gfx_FreeState(void) { FreeDefaultResources(); }
static void Gfx_RestoreState(void) {
	InitDefaultResources();
}

cc_bool Gfx_WarnIfNecessary(void) { return true; }


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_CalcOrthoMatrix(struct Matrix* matrix, float width, float height, float zNear, float zFar) {
	/* Transposed, source https://learn.microsoft.com/en-us/windows/win32/opengl/glortho */
	/*   The simplified calculation below uses: L = 0, R = width, T = 0, B = height */
	*matrix = Matrix_Identity;
	width  /= 64.0f; 
	height /= 64.0f; 

	matrix->row1.x =  2.0f / width;
	matrix->row2.y = -2.0f / height;
	matrix->row3.z = -2.0f / (zFar - zNear);

	matrix->row4.x = -1.0f;
	matrix->row4.y =  1.0f;
	matrix->row4.z = -(zFar + zNear) / (zFar - zNear);
}

static double Cotangent(double x) { return Math_Cos(x) / Math_Sin(x); }
void Gfx_CalcPerspectiveMatrix(struct Matrix* matrix, float fov, float aspect, float zFar) {
	float zNear = 0.1f;
	float c = (float)Cotangent(0.5f * fov);

	/* Transposed, source https://learn.microsoft.com/en-us/windows/win32/opengl/glfrustum */
	/* For a FOV based perspective matrix, left/right/top/bottom are calculated as: */
	/*   left = -c * aspect, right = c * aspect, bottom = -c, top = c */
	/* Calculations are simplified because of left/right and top/bottom symmetry */
	*matrix = Matrix_Identity;

	matrix->row1.x =  c / aspect;
	matrix->row2.y =  c;
	matrix->row3.z = -(zFar + zNear) / (zFar - zNear);
	matrix->row3.w = -1.0f;
	matrix->row4.z = -(2.0f * zFar * zNear) / (zFar - zNear);
	matrix->row4.w =  0.0f;
}


/*########################################################################################################################*
*----------------------------------------------------------Buffers--------------------------------------------------------*
*#########################################################################################################################*/
// Preprocess vertex buffers into optimised layout for DS
static VertexFormat buf_fmt;
static int buf_count;

static void* gfx_vertices;

struct DSTexturedVertex {
    vu32 xy; v16 z;
    vu32 rgb;
    int u, v;
};
struct DSColouredVertex {
    vu32 xy; v16 z;
    vu32 rgb;
};

// Precalculate all the expensive vertex data conversion,
//  so that actual drawing of them is as fast as possible
static void PreprocessTexturedVertices(void) {
    struct   VertexTextured* src = gfx_vertices;
    struct DSTexturedVertex* dst = gfx_vertices;

    for (int i = 0; i < buf_count; i++, src++, dst++)
    {
        struct VertexTextured v = *src;
        v16 x = floattov16(v.x / 64.0f);
        v16 y = floattov16(v.y / 64.0f);
        v16 z = floattov16(v.z / 64.0f);
        dst->xy = (y << 16) | (x & 0xFFFF);
        dst->z  = z;
    
        dst->u = floattof32(v.U);
        dst->v = floattof32(v.V);

        int r = PackedCol_R(v.Col);
        int g = PackedCol_G(v.Col);
        int b = PackedCol_B(v.Col);
        dst->rgb = RGB15(r >> 3, g >> 3, b >> 3);
    }
}

static void PreprocessColouredVertices(void) {
    struct   VertexColoured* src = gfx_vertices;
    struct DSColouredVertex* dst = gfx_vertices;

    for (int i = 0; i < buf_count; i++, src++, dst++)
    {
        struct VertexColoured v = *src;
        v16 x = floattov16(v.x / 64.0f);
        v16 y = floattov16(v.y / 64.0f);
        v16 z = floattov16(v.z / 64.0f);
        dst->xy = (y << 16) | (x & 0xFFFF);
        dst->z  = z;

        int r = PackedCol_R(v.Col);
        int g = PackedCol_G(v.Col);
        int b = PackedCol_B(v.Col);
        dst->rgb = RGB15(r >> 3, g >> 3, b >> 3);
    }
}

GfxResourceID Gfx_CreateIb2(int count, Gfx_FillIBFunc fillFunc, void* obj) {
	return (void*)1;
}

void Gfx_BindIb(GfxResourceID ib)    { }
void Gfx_DeleteIb(GfxResourceID* ib) { }


static GfxResourceID Gfx_AllocStaticVb(VertexFormat fmt, int count) {
	return Mem_TryAlloc(count, strideSizes[fmt]);
}

void Gfx_BindVb(GfxResourceID vb) { gfx_vertices = vb; }

void Gfx_DeleteVb(GfxResourceID* vb) {
	GfxResourceID data = *vb;
	if (data) Mem_Free(data);
	*vb = 0;
}

void* Gfx_LockVb(GfxResourceID vb, VertexFormat fmt, int count) {
    buf_fmt   = fmt;
    buf_count = count;
	return vb;
}

void Gfx_UnlockVb(GfxResourceID vb) { 
    gfx_vertices = vb;

    if (buf_fmt == VERTEX_FORMAT_TEXTURED) {
        PreprocessTexturedVertices();
    } else {
        PreprocessColouredVertices();
    }
}


static GfxResourceID Gfx_AllocDynamicVb(VertexFormat fmt, int maxVertices) {
	return Mem_TryAlloc(maxVertices, strideSizes[fmt]);
}

void Gfx_BindDynamicVb(GfxResourceID vb) { Gfx_BindVb(vb); }

void* Gfx_LockDynamicVb(GfxResourceID vb, VertexFormat fmt, int count) {
	return Gfx_LockVb(vb, fmt, count);
}

void Gfx_UnlockDynamicVb(GfxResourceID vb) { Gfx_UnlockVb(vb); }

void Gfx_DeleteDynamicVb(GfxResourceID* vb) { Gfx_DeleteVb(vb); }


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
static cc_bool skipRendering;

void Gfx_SetFog(cc_bool enabled) {
}

void Gfx_SetFogCol(PackedCol color) {
}

void Gfx_SetFogDensity(float value) {
}

void Gfx_SetFogEnd(float value) {
}

void Gfx_SetFogMode(FogFunc func) {
}

void Gfx_SetAlphaTest(cc_bool enabled) {
    if (enabled) {
        //glEnable(GL_ALPHA_TEST);
    } else {
        //glDisable(GL_ALPHA_TEST);
    }
}

void Gfx_DepthOnlyRendering(cc_bool depthOnly) {
	skipRendering = depthOnly;
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
static int matrix_modes[] = { GL_PROJECTION, GL_MODELVIEW, GL_TEXTURE };
static int lastMatrix;

void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	if (type != lastMatrix) { lastMatrix = type; glMatrixMode(matrix_modes[type]); }
	
	m4x4 m;
	const float* src = (const float*)matrix;
	
	for (int i = 0; i < 4 * 4; i++)
	{
		m.m[i] = floattof32(src[i]);
	}
	glLoadMatrix4x4(&m);

    // Vertex commands are signed 16 bit values, with 12 bits fractional
    //  aka only from -8.0 to 8.0
    // That's way too small to be useful, so counteract that by scaling down
    //  vertices and then scaling up the matrix multiplication
    if (type == MATRIX_VIEW)
        glScalef32(floattof32(64.0f), floattof32(64.0f), floattof32(64.0f));
}

void Gfx_LoadIdentityMatrix(MatrixType type) {
	if (type != lastMatrix) { lastMatrix = type; glMatrixMode(matrix_modes[type]); }
	glLoadIdentity();
}

static struct Matrix texMatrix;
void Gfx_EnableTextureOffset(float x, float y) {
	texMatrix.row1.x = x; texMatrix.row2.y = y;
	Gfx_LoadMatrix(2, &texMatrix);
    //glTexParameter(0, TEXGEN_NORMAL | GL_TEXTURE_WRAP_S | GL_TEXTURE_WRAP_T);

}

void Gfx_DisableTextureOffset(void) {
	texMatrix.row1.x = 0; texMatrix.row1.y = 0;
	Gfx_LoadMatrix(2, &texMatrix);
    //glTexParameter(0, TEXGEN_TEXCOORD | GL_TEXTURE_WRAP_S | GL_TEXTURE_WRAP_T);
}


/*########################################################################################################################*
*--------------------------------------------------------Rendering--------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_SetVertexFormat(VertexFormat fmt) {
	gfx_format = fmt;
	gfx_stride = strideSizes[fmt];
    
    if (fmt == VERTEX_FORMAT_TEXTURED) {
        glEnable(GL_TEXTURE_2D);
    } else {
        glDisable(GL_TEXTURE_2D);
    }
}

void Gfx_DrawVb_Lines(int verticesCount) {
}


static void Draw_ColouredTriangles(int verticesCount, int startVertex) {
	glBegin(GL_QUADS);
	for (int i = 0; i < verticesCount; i++) 
	{
		struct DSColouredVertex* v = (struct DSColouredVertex*)gfx_vertices + startVertex + i;
		
		GFX_COLOR    = v->rgb;
		GFX_VERTEX16 = v->xy;
        GFX_VERTEX16 = v->z;
	}
	glEnd();
}

static void Draw_TexturedTriangles(int verticesCount, int startVertex) {
	glBegin(GL_QUADS);
    int width = 0, height = 0;
    glGetInt(GL_GET_TEXTURE_WIDTH,  &width);  width  = inttof32(width);
    glGetInt(GL_GET_TEXTURE_HEIGHT, &height); height = inttof32(height);
    

	for (int i = 0; i < verticesCount; i++)
	{
		struct DSTexturedVertex* v = (struct DSTexturedVertex*)gfx_vertices + startVertex + i;
		
		GFX_COLOR    = v->rgb;
        glTexCoord2t16(f32tot16(mulf32(v->u, width)), f32tot16(mulf32(v->v, height)));
		GFX_VERTEX16 = v->xy;
        GFX_VERTEX16 = v->z;
	}
	glEnd();
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex) {
	if (gfx_format == VERTEX_FORMAT_TEXTURED) {
		Draw_TexturedTriangles(verticesCount, startVertex);
	} else {
		Draw_ColouredTriangles(verticesCount, startVertex);
	}
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	if (gfx_format == VERTEX_FORMAT_TEXTURED) {
		Draw_TexturedTriangles(verticesCount, 0);
	} else {
		Draw_ColouredTriangles(verticesCount, 0);
	}
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {
	if (skipRendering) return;
	Draw_TexturedTriangles(verticesCount, startVertex);
}
#endif
