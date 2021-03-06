
#pragma once
#include "Objects.h"
#include "Native.h"
#include "../Core/Image.h"
#include "../Core/3DMath.h"
#include "../Render/Render.h"


namespace ui {

enum class ScaleMode
{
	None, // do not scale the image
	Stretch, // the whole rectangle is covered by the whole image, aspect may not be preserved
	Fit, // the whole image fits in the rectangle, aspect is preserved
	Fill, // the whole rectangle is covered by some part of the image, aspect is preserved
};

struct ColorBlock : UIElement
{
	void OnInit() override;
	void OnPaint() override;

	Color4b GetColor() const { return _color; }
	ColorBlock& SetColor(Color4b col) { _color = col; return *this; }

	Color4b _color = Color4b::Black();
	draw::ImageHandle _bgImage;
};

struct ColorInspectBlock : UIElement
{
	void OnInit() override;
	void OnPaint() override;

	Color4b GetColor() const { return _color; }
	ColorInspectBlock& SetColor(Color4b col) { _color = col; return *this; }

	Color4b _color = Color4b::Black();
	draw::ImageHandle _bgImage;

	Coord alphaBarHeight = 2;
};

void DrawImage(UIRect rect, draw::IImage* img, ScaleMode sm = ScaleMode::Fit, float ax = 0.5f, float ay = 0.5f);

struct ImageElement : UIElement
{
	void OnInit() override;
	void OnPaint() override;
	void GetSize(Coord& outWidth, Coord& outHeight) override;

	ImageElement& SetImage(draw::IImage* img);
	ImageElement& SetPath(StringView path);
	ImageElement& SetDelayLoadPath(StringView path);
	// range: 0-1 (0.5 = middle)
	ImageElement& SetScaleMode(ScaleMode sm, float ax = 0.5f, float ay = 0.5f);
	ImageElement& SetAlphaBackgroundEnabled(bool enabled);

	draw::ImageHandle _image;
	ScaleMode _scaleMode = ScaleMode::Fit;
	float _anchorX = 0.5f;
	float _anchorY = 0.5f;

	bool _tryDelayLoad = false;
	std::string _delayLoadPath;

	draw::ImageHandle _bgImage;
};

struct HueSatPicker : UIElement
{
	static constexpr bool Persistent = true;

	void OnInit() override;
	void OnEvent(Event& e) override;
	void OnPaint() override;

	HueSatPicker& Init(float& hue, float& sat)
	{
		_hue = hue;
		_sat = sat;
		HandleEvent(EventType::Change) = [&hue, &sat, this](Event&) { hue = _hue; sat = _sat; };
		return *this;
	}

	void _RegenerateBackground(int w);

	StyleBlockRef selectorStyle;
	float _hue = 0;
	float _sat = 0;
	draw::ImageHandle _bgImage;
};

enum ColorMode
{
	CM_RGB = 0,
	CM_HSV = 1,
};

enum ColorComponent
{
	CC_0 = 0,
	CC_1 = 1,
	CC_2 = 2,
	CC_Red = 0,
	CC_Green = 1,
	CC_Blue = 2,
	CC_Hue = 0,
	CC_Sat = 1,
	CC_Val = 2,
};

struct ColorCompPicker2DSettings
{
	ColorCompPicker2DSettings() : _mode(CM_HSV), _ccx(CC_Hue), _ccy(CC_Sat), _invx(false), _invy(true), _baseColor(1, 0, 0) {}
	ColorCompPicker2DSettings(ColorMode cm, ColorComponent ccx, ColorComponent ccy, Color4f baseColor = Color4f(1, 0, 0)) :
		ColorCompPicker2DSettings(cm, ccx, false, ccy, true, baseColor) {}
	ColorCompPicker2DSettings(ColorMode cm, ColorComponent ccx, bool invertX, ColorComponent ccy, bool invertY, Color4f baseColor = Color4f(1, 0, 0)) :
		_mode(cm), _ccx(ccx), _ccy(ccy), _invx(invertX), _invy(invertY), _baseColor(baseColor)
	{
	}
	bool operator == (const ColorCompPicker2DSettings& o) const
	{
		return _mode == o._mode && _ccx == o._ccx && _ccy == o._ccy && _invx == o._invx && _invy == o._invy && _baseColor == o._baseColor;
	}
	bool operator != (const ColorCompPicker2DSettings& o) const { return !(*this == o); }

	ColorMode _mode;
	ColorComponent _ccx;
	ColorComponent _ccy;
	bool _invx;
	bool _invy;
	Color4f _baseColor;
};

struct ColorDragDropData : DragDropData
{
	static constexpr const char* NAME = "color";
	ColorDragDropData(const Color4f& c) : DragDropData(NAME), color(c) {}
	void Build() override;
	Color4f color;
};

struct ColorCompPicker2D : UIElement
{
	ColorCompPicker2D();
	void OnEvent(Event& e) override;
	void OnPaint() override;

	ColorCompPicker2DSettings GetSettings() const { return _settings; }
	ColorCompPicker2D& SetSettings(const ColorCompPicker2DSettings& s) { _settings = s; return *this; }

	float GetX() const { return _x; }
	ColorCompPicker2D& SetX(float v) { _x = v; return *this; }

	float GetY() const { return _y; }
	ColorCompPicker2D& SetY(float v) { _y = v; return *this; }

	ColorCompPicker2D& Init(float& x, float& y, ColorCompPicker2DSettings s = {})
	{
		SetSettings(s);
		SetX(x);
		SetY(y);
		HandleEvent(EventType::Change) = [&x, &y, this](Event&) { x = _x; y = _y; Rebuild(); };
		return *this;
	}

	void _RegenerateBackground(int w, int h);

	StyleBlockRef selectorStyle;
	ColorCompPicker2DSettings _settings, _curImgSettings;
	float _x = 0;
	float _y = 0;
	draw::ImageHandle _bgImage;
};

struct MultiFormatColor
{
	MultiFormatColor() {}
	MultiFormatColor(const Color4f& c);
	MultiFormatColor(Color4b c);
	MultiFormatColor(float h, float s, float v, float a = 1);

	float GetAlpha() const;
	Color4f GetRGBA() const;
	Color4b GetRGBA8() const;
	void GetHSV(float& h, float& s, float& v) const;

	void SetAlpha(float a);
	void SetRGBA(const Color4f& c);
	void SetRGBA8(Color4b c);
	void SetHSV(float h, float s, float v);
	void SetHSVA(float h, float s, float v, float a = 1);

	void _UpdateRGB();
	void _RGB2HSV();
	void _UpdateHSV();
	void _UpdateHex();

	Color4f _rgba = Color4f::White();
	float _hue = 0;
	float _sat = 0;
	float _val = 1;
	char hex[7] = "FFFFFF";
};

struct ColorPicker : Buildable
{
	void Build() override;

	const MultiFormatColor& GetColor() const { return _color; }
	ColorPicker& SetColor(const MultiFormatColor& c)
	{
		_color = c;
		return *this;
	}

	struct SavedColors
	{
		static constexpr int COUNT = 16;

		SavedColors();

		Color4f colors[COUNT];
	};
	static SavedColors& GetSavedColors();

	MultiFormatColor _color;
};

struct ColorPickerWindow : NativeDialogWindow
{
	ColorPickerWindow();
	void OnBuild() override;

	const MultiFormatColor& GetColor() const { return _color; }
	void SetColor(const MultiFormatColor& c) { _color = c; }

	MultiFormatColor _color;
};

struct IColorEdit : Buildable
{
	virtual const MultiFormatColor& GetColor() const = 0;
	virtual void SetColorAny(const MultiFormatColor& c) = 0;
};

struct ColorEdit : IColorEdit
{
	MultiFormatColor _color;

	void Build() override;

	const MultiFormatColor& GetColor() const override { return _color; }
	void SetColorAny(const MultiFormatColor& c) override { SetColor(c); }

	ColorEdit& SetColor(const MultiFormatColor& c);
};

struct ColorEditRT : IColorEdit
{
	MultiFormatColor _color;
	struct ColorPickerWindowRT* _rtWindow = nullptr;

	void Build() override;
	void OnDestroy() override;

	const MultiFormatColor& GetColor() const override { return _color; }
	void SetColorAny(const MultiFormatColor& c) override { SetColor(c); }

	ColorEditRT& SetColor(const MultiFormatColor& c);
};

struct View2D : UIElement
{
	void OnPaint() override;

	std::function<void(UIRect)> onPaint;
};

struct View3D : UIElement
{
	void OnPaint() override;

	std::function<void(UIRect)> onRender;
	std::function<void(UIRect)> onPaintOverlay;
};

struct CameraBase
{
	Mat4f _mtxView = Mat4f::Identity();
	Mat4f _mtxInvView = Mat4f::Identity();
	Mat4f _mtxProj = Mat4f::Identity();
	Mat4f _mtxInvProj = Mat4f::Identity();
	Mat4f _mtxViewProj = Mat4f::Identity();
	Mat4f _mtxInvViewProj = Mat4f::Identity();
	UIRect _windowRect = { -1, -1, 0, 0 };

	void _UpdateViewProjMatrix();

	UI_FORCEINLINE const Mat4f& GetViewMatrix() const { return _mtxView; }
	UI_FORCEINLINE const Mat4f& GetInverseViewMatrix() const { return _mtxInvView; }
	void SetViewMatrix(const Mat4f& m);

	UI_FORCEINLINE const Mat4f& GetProjectionMatrix() const { return _mtxProj; }
	UI_FORCEINLINE const Mat4f& GetInverseProjectionMatrix() const { return _mtxInvProj; }
	void SetProjectionMatrix(const Mat4f& m);

	UI_FORCEINLINE const Mat4f& GetViewProjectionMatrix() const { return _mtxViewProj; }
	UI_FORCEINLINE const Mat4f& GetInverseViewProjectionMatrix() const { return _mtxInvViewProj; }

	UI_FORCEINLINE const UIRect& GetWindowRect() const { return _windowRect; }
	void SetWindowRect(const UIRect& rect);

	Point2f WindowToNormalizedPoint(Point2f p) const;
	Point2f NormalizedToWindowPoint(Point2f p) const;
	Point2f WorldToNormalizedPoint(const Vec3f& p) const;
	Point2f WorldToWindowPoint(const Vec3f& p) const;

	float WorldToWindowSize(float size, const Vec3f& refp) const;
	float WindowToWorldSize(float size, const Vec3f& refp) const;

	Ray3f GetRayNP(Point2f p) const;
	Ray3f GetLocalRayNP(Point2f p, const Mat4f& world2local) const;
	Ray3f GetRayWP(Point2f p) const;
	Ray3f GetLocalRayWP(Point2f p, const Mat4f& world2local) const;
	Ray3f GetRayEP(const Event& e) const;
	Ray3f GetLocalRayEP(const Event& e, const Mat4f& world2local) const;
};

struct OrbitCamera : CameraBase
{
	OrbitCamera(bool rh = false);
	bool OnEvent(Event& e);
	void SerializeState(IObjectIterator& oi, const FieldInfo& FI);

	void Rotate(float dx, float dy);
	void Pan(float dx, float dy);
	void Zoom(float delta);

	void ResetState();

	void _UpdateViewMatrix();

	// state/settings
	Vec3f pivot = {};
	float yaw = 45;
	float pitch = 45;
	float distance = 1;

	// settings
	bool rightHanded = false;
	float minPitch = -85;
	float maxPitch = 85;
	float rotationSpeed = 0.5f; // degrees per pixel
	float distanceScale = 1.2f; // scale per scroll
	MouseButton rotateButton = MouseButton::Left;
	MouseButton panButton = MouseButton::Middle;

	// state
	bool rotating = false;
	bool panning = false;
};

} // ui
