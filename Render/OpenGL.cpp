
#include <stdio.h>
#include <stdlib.h>

#include "OpenGL.h"
#include "../Core/Math.h"

#pragma warning(disable:4996)

#define WIN32_LEAN_AND_MEAN
#define NONLS
#define NOMINMAX
#include <Windows.h>
#include <gl/GL.h>


namespace ui {
namespace rhi {


#define GLCHK(x) do { x; _Check(#x, __FILE__, __LINE__); } while (0)
void _Check(const char* code, const char* file, int line)
{
	auto err = glGetError();
	if (err != GL_NO_ERROR)
	{
		char bfr[2048];
		sprintf(bfr, "Error: %d\nCode: %s\nFile: %s\nLine: %d\nDo you wish to continue?", (int)err, code, file, line);
		if (MessageBoxA(nullptr, bfr, "OpenGL error", MB_ICONERROR | MB_YESNO) == IDNO)
		{
			exit(EXIT_FAILURE);
		}
	}
}

struct RenderContext
{
	void AddToList()
	{
		if (!first)
			first = this;
		prev = last;
		next = nullptr;
		last = this;
	}
	~RenderContext()
	{
		if (prev)
			prev->next = next;
		if (next)
			next->prev = prev;
		if (first == this)
			first = first->next;
		if (last == this)
			last = last->prev;
	}

	HDC dc;
	HGLRC rc;

	static RenderContext* first;
	static RenderContext* last;
	RenderContext* prev = nullptr;
	RenderContext* next = nullptr;
};
RenderContext* RenderContext::first;
RenderContext* RenderContext::last;

RenderContext* CreateRenderContext(void* window)
{
	RenderContext* RC = new RenderContext();
	RC->dc = GetDC((HWND)window);

	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    // Flags
		PFD_TYPE_RGBA,        // The kind of framebuffer. RGBA or palette.
		32,                   // Colordepth of the framebuffer.
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		24,                   // Number of bits for the depthbuffer
		8,                    // Number of bits for the stencilbuffer
		0,                    // Number of Aux buffers in the framebuffer.
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};

	int format = ChoosePixelFormat(RC->dc, &pfd);
	SetPixelFormat(RC->dc, format, &pfd);

	RC->rc = wglCreateContext(RC->dc);
	wglMakeCurrent(RC->dc, RC->rc);
	if (RenderContext::first)
		wglShareLists(RenderContext::first->rc, RC->rc);

	((BOOL(__stdcall *)(int))wglGetProcAddress("wglSwapIntervalEXT"))(0);

	GLCHK(glDisable(GL_CULL_FACE));
	GLCHK(glDisable(GL_DEPTH_TEST));
	GLCHK(glEnable(GL_BLEND));
	GLCHK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
	GLCHK(glEnableClientState(GL_VERTEX_ARRAY));
	GLCHK(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
	GLCHK(glEnableClientState(GL_COLOR_ARRAY));
	GLCHK(glEnable(GL_SCISSOR_TEST));

	RC->AddToList();
	return RC;
}

void FreeRenderContext(RenderContext* RC)
{
	wglMakeCurrent(nullptr, nullptr);
	wglDeleteContext(RC->rc);
	// ReleaseDC(RC->dc); - TODO?
	delete RC;
}

void SetActiveContext(RenderContext* RC)
{
	wglMakeCurrent(RC->dc, RC->rc);
}

static int curRTTHeight;

void SetScissorRect(int x0, int y0, int x1, int y1)
{
	GLCHK(glScissor(x0, curRTTHeight - y1, std::max(x1 - x0, 0), std::max(y1 - y0, 0)));
}

void SetViewport(int x0, int y0, int x1, int y1)
{
	glViewport(x0, y0, x1 - x0, y1 - y0);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	GLCHK(glOrtho(x0, x1, y1, y0, -1, 1));
	curRTTHeight = y1; // TODO fix
}

void Clear(int r, int g, int b, int a)
{
	glClearColor(r / 255.f, g / 255.f, b / 255.f, a / 255.f);
	glClearDepth(1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Present(RenderContext* RC)
{
#if 0
	GLCHK(glDisable(GL_TEXTURE_2D));
	glBegin(GL_TRIANGLES);
	glVertex2f(-10, -10);
	glVertex2f(10, -10);
	glVertex2f(0, 10);
	glVertex2f(0, 10);
	glVertex2f(10, -10);
	glVertex2f(-10, -10);
	glEnd();
	glEnable(GL_TEXTURE_2D);
#endif
#if 0
	GLCHK(glDisable(GL_TEXTURE_2D));
	glBegin(GL_QUADS);
	glVertex2f(10, 10);
	glVertex2f(20, 10);
	glVertex2f(20, 20);
	glVertex2f(10, 20);
	glEnd();
	glEnable(GL_TEXTURE_2D);
#endif

	SwapBuffers(RC->dc);
}

Texture2D* CreateTextureA8(const void* data, unsigned width, unsigned height)
{
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glEnable(GL_TEXTURE_2D);

	return (Texture2D*) tex;
}

Texture2D* CreateTextureRGBA8(const void* data, unsigned width, unsigned height, bool filtering)
{
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtering ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtering ? GL_LINEAR : GL_NEAREST);
	glEnable(GL_TEXTURE_2D);

	return (Texture2D*) tex;
}

void DestroyTexture(Texture2D* tex)
{
	GLuint texid = (GLuint)tex;
	glDeleteTextures(1, &texid);
}

void SetTexture(Texture2D* tex)
{
	glBindTexture(GL_TEXTURE_2D, (GLuint)tex);
	if (tex != 0)
		glEnable(GL_TEXTURE_2D);
	else
		glDisable(GL_TEXTURE_2D);
}

void DrawTriangles(Vertex* verts, size_t num_verts)
{
	glVertexPointer(2, GL_FLOAT, sizeof(Vertex), &verts[0].x);
	glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), &verts[0].u);
	glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(Vertex), &verts[0].col);
	glDrawArrays(GL_TRIANGLES, 0, num_verts);
}

void DrawIndexedTriangles(Vertex* verts, uint16_t* indices, size_t num_indices)
{
	glVertexPointer(2, GL_FLOAT, sizeof(Vertex), &verts[0].x);
	glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), &verts[0].u);
	glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(Vertex), &verts[0].col);
	glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, indices);
}

} // rhi
} // ui
