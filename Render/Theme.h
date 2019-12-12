
#pragma once

#include "OpenGL.h"


struct Sprite
{
	int ox0, oy0, ox1, oy1;
	int bx0, by0, bx1, by1;
};

enum EThemeElement
{
	TE_ButtonNormal,
	TE_ButtonHover,
	TE_ButtonPressed,
	TE_ButtonDisabled,

	TE_CheckBgrNormal,
	TE_CheckBgrHover,
	TE_CheckBgrPressed,
	TE_CheckBgrDisabled,
	TE_CheckMark,
	TE_CheckMarkDisabled,

	TE_RadioBgrNormal,
	TE_RadioBgrHover,
	TE_RadioBgrPressed,
	TE_RadioBgrDisabled,
	TE_RadioMark,
	TE_RadioMarkDisabled,

	TE_TreeTickClosedNormal,
	TE_TreeTickClosedHover,
	TE_TreeTickOpenNormal,
	TE_TreeTickOpenHover,

	TE_Panel,
	TE_Textbox,

	TE__COUNT,
};


void InitFont();
float GetTextWidth(const char* text, size_t num = SIZE_MAX);
float GetFontHeight();
void DrawTextLine(float x, float y, const char* text, float r, float g, float b, float a = 1);

extern GL::TexID g_themeTexture;
void InitTheme();
void DrawThemeElement(EThemeElement e, float x0, float y0, float x1, float y1);
