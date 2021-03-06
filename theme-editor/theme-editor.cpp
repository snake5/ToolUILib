
#include "TECore.h"
#include "TENodes.h"
#include "TETheme.h"


enum TE_PreviewMode
{
	TEPM_Original,
	TEPM_Sliced,
};
static TE_PreviewMode g_previewMode = TEPM_Original;
static ScaleMode g_previewScaleMode = ScaleMode::None;
static unsigned g_previewZoomPercent = 100;
static unsigned g_previewSlicedWidth = 128;
static unsigned g_previewSlicedHeight = 32;

struct TE_SlicedImageElement : UIElement
{
	void OnPaint() override
	{
		UIRect r = GetContentRect();
		float iw = _image->GetWidth();
		float ih = _image->GetHeight();
		float x0 = roundf((r.x0 + r.x1 - _width) * 0.5f);
		float y0 = roundf((r.y0 + r.y1 - _height) * 0.5f);
		UIRect outer = { x0, y0, x0 + _width, y0 + _height };
		UIRect inner = outer.ShrinkBy({ float(_left), float(_top), float(_right), float(_bottom) });
		UIRect texinner = { invlerp(0, iw, _left), invlerp(0, ih, _top), invlerp(iw, 0, _right), invlerp(ih, 0, _bottom) };
		draw::RectColTex9Slice(outer, inner, Color4b::White(), _image, { 0, 0, 1, 1 }, texinner);
	}
	TE_SlicedImageElement& SetImage(draw::IImage* img)
	{
		_image = img;
		return *this;
	}
	TE_SlicedImageElement& SetTargetSize(int w, int h)
	{
		_width = w;
		_height = h;
		return *this;
	}
	TE_SlicedImageElement& SetBorderSizes(int l, int t, int r, int b)
	{
		_left = l;
		_top = t;
		_right = r;
		_bottom = b;
		return *this;
	}

	draw::ImageHandle _image;
	int _width = 0;
	int _height = 0;
	int _left = 0;
	int _top = 0;
	int _right = 0;
	int _bottom = 0;
};

struct TE_MainPreviewNode : Buildable
{
	TE_Template* tmpl;

	void Build() override
	{
		Subscribe(DCT_NodePreviewInvalidated);
		Subscribe(DCT_EditProcGraph);
		Subscribe(DCT_EditProcGraphNode);

		PushBox() + Set(StackingDirection::LeftToRight);
		{
			Text("Preview") + SetPadding(5);
			if (tmpl->curPreviewImage && imm::Button("Reset to template"))
			{
				tmpl->SetCurPreviewImage(nullptr);
			}
		}
		Pop();

		if (tmpl->renderSettings.layer)
		{
			PushBox() + SetLayout(layouts::EdgeSlice());
			{
				PushBox() + Set(StackingDirection::LeftToRight);
				{
					imm::RadioButton(g_previewMode, TEPM_Original, "Original", {}, imm::ButtonStateToggleSkin());
					imm::RadioButton(g_previewMode, TEPM_Sliced, "Sliced", {}, imm::ButtonStateToggleSkin());
				}
				Pop();
				if (g_previewMode == TEPM_Original)
				{
					PushBox() + Set(StackingDirection::LeftToRight);
					{
						imm::RadioButton(g_previewScaleMode, ScaleMode::None, "No scaling", {}, imm::ButtonStateToggleSkin());
						imm::RadioButton(g_previewScaleMode, ScaleMode::Fit, "Fit", {}, imm::ButtonStateToggleSkin());
					}
					Pop();
				}
				if (g_previewMode == TEPM_Sliced)
				{
					Push<LabeledProperty>();
					{
						imm::PropEditInt("\bWidth", g_previewSlicedWidth, { SetMinWidth(20) });
						imm::PropEditInt("\bHeight", g_previewSlicedHeight, { SetMinWidth(20) });
					}
					Pop();
				}

				Push<ListBox>() + SetHeight(Coord::Percent(95)); // TODO

				Canvas canvas;
#if 0
				auto rc = imgNode->MakeRenderContext();
				if (auto* prevImg = theme->curPreviewImage)
					rc.overrides = prevImg->overrides[theme->curVariation].get();
#endif
				auto& rs = tmpl->renderSettings;
				rs.layer->Render(canvas, tmpl->GetRenderContext());
				auto img = draw::ImageCreateFromCanvas(canvas, draw::TexFlags::NoFilter);
				if (g_previewMode == TEPM_Original)
				{
					Make<ImageElement>()
						.SetImage(img)
						.SetScaleMode(g_previewScaleMode)
						+ SetWidth(Coord::Percent(100))
						+ SetHeight(Coord::Percent(100));
				}
				if (g_previewMode == TEPM_Sliced)
				{
					Make<TE_SlicedImageElement>()
						.SetImage(img)
						.SetTargetSize(g_previewSlicedWidth, g_previewSlicedHeight)
						.SetBorderSizes(rs.l, rs.t, rs.r, rs.b)
						+ SetWidth(Coord::Percent(100))
						+ SetHeight(Coord::Percent(100));
				}

				Pop();
			}
			Pop();
		}
	}
};

struct TE_ImageEditorNode : Buildable
{
	TE_Theme* theme;
	TE_Template* tmpl;

	void Build() override
	{
		Subscribe(DCT_ChangeActiveImage);

		PushBox() + Set(StackingDirection::LeftToRight);
		{
			Text("Images") + SetPadding(5);
			if (imm::Button("Add"))
			{
				auto img = std::make_shared<TE_Image>();
				img->name = "<unnamed>";
				tmpl->images.insert(tmpl->images.begin(), img);
				Rebuild();
			}
		}
		Pop();

		auto& imged = Make<SequenceEditor>();
		imged + SetHeight(Coord::Percent(100));
		imged.showDeleteButton = false;
		imged.SetSequence(Allocate<StdSequence<decltype(tmpl->images)>>(tmpl->images));
		imged.itemUICallback = [this](SequenceEditor* se, size_t idx, void* ptr)
		{
			EditImage(static_cast<std::shared_ptr<TE_Image>*>(ptr)->get(), se, idx, ptr);
		};
	}

	void EditImage(TE_Image* img, SequenceEditor* se, size_t idx, void* ptr)
	{
		PushBox();
		{
			PushBox() + SetLayout(layouts::StackExpand()) + Set(StackingDirection::LeftToRight);
			{
				imm::EditBool(img->expanded, nullptr, { SetWidth(Coord::Fraction(0)) }, imm::TreeStateToggleSkin());
				if (imm::RadioButtonRaw(tmpl->curPreviewImage == img, "P", { SetWidth(Coord::Fraction(0)) }, imm::ButtonStateToggleSkin()))
				{
					tmpl->SetCurPreviewImage(img);
				}
				imm::EditString(img->name.c_str(), [&img](const char* v) { img->name = v; });
				se->OnBuildDeleteButton(idx);
			}
			Pop();

			if (img->expanded)
			{
				Push<Panel>().HandleEvent() = [this](Event& e)
				{
					if (e.type == EventType::Change || e.type == EventType::Commit || e.type == EventType::IMChange)
						tmpl->InvalidateAllNodes();
				};
				{
					auto& ovr = img->overrides[theme->curVariation];
					if (!ovr)
						ovr = std::make_shared<TE_Overrides>();

					auto& coed = Make<SequenceEditor>();
					// TODO making Allocate automatically work in the current node would avoid this bug (Allocate instead of GetCurrentNode()->)
					coed.SetSequence(ui::BuildAlloc<StdSequence<decltype(ovr->colorOverrides)>>(ovr->colorOverrides));
					coed.itemUICallback = [](SequenceEditor* se, size_t idx, void* ptr)
					{
						auto& co = *static_cast<TE_ColorOverride*>(ptr);
						if (auto ncr = co.ncref.lock())
							MakeWithText<BoxElement>(ncr->name) + SetPadding(5);
						else
							EditNCRef(co.ncref);
						imm::EditColor(co.color, false, { SetWidth(40) });
					};

					if (imm::Button("Add override"))
					{
						std::vector<MenuItem> items;
						for (auto& col : theme->curTemplate->colors)
						{
							bool added = false;
							for (auto& acol : ovr->colorOverrides)
							{
								if (col == acol.ncref.lock())
								{
									added = true;
									break;
								}
							}
							if (!added)
							{
								items.push_back(MenuItem(col->name).Func([ovr, col]() { ovr->colorOverrides.push_back({ col, col->color }); }));
							}
						}
						if (items.empty())
							items.push_back(MenuItem("- no overrides left to add -", {}, true));
						Menu(items).Show(this);
					}
				}
				Pop();
			}
		}
		Pop();
	}
};

static bool showRenderSettings = true;
static bool showColors = true;
struct TE_TemplateEditorNode : Buildable
{
	TE_Theme* theme;
	TE_Template* tmpl;

	void Build() override
	{
		auto& hsp = Push<SplitPane>();
		{
			Push<ListBox>();
			{
				auto& pge = Make<ProcGraphEditor>();
				pge + SetHeight(Coord::Percent(100));
				pge.Init(tmpl);
			}
			Pop();

			auto& ien = Make<TE_ImageEditorNode>();
			ien.theme = theme;
			ien.tmpl = tmpl;

			PushBox();
			{
				auto& vsp = Push<SplitPane>();
				{
					auto& preview = Make<TE_MainPreviewNode>();
					preview.tmpl = tmpl;

					Push<PropertyList>()
						+ AddEventHandler(EventType::IMChange, [this](Event&) { tmpl->InvalidateAllNodes(); });
					{
						imm::EditBool(showRenderSettings, "Render settings", {}, imm::TreeStateToggleSkin());
						if (showRenderSettings)
						{
							tmpl->renderSettings.UI();
						}

						imm::EditBool(showColors, "Colors", {}, imm::TreeStateToggleSkin());
						if (showColors)
						{
							auto& ced = Make<SequenceEditor>();
							ced + SetHeight(Coord::Percent(100));
							ced.SetSequence(Allocate<StdSequence<decltype(tmpl->colors)>>(tmpl->colors));
							ced.itemUICallback = [this](SequenceEditor* se, size_t idx, void* ptr)
							{
								auto& NC = *static_cast<std::shared_ptr<TE_NamedColor>*>(ptr);
								imm::EditColor(NC->color);
								imm::EditString(NC->name.c_str(), [&NC](const char* v) { NC->name = v; });
							};
						}
					}
					Pop();
				}
				Pop();
				vsp.SetDirection(true);
				vsp.SetSplits({ 0.5f });
			}
			Pop();
		}
		Pop();
		hsp.SetSplits({ 0.6f, 0.8f });
	}
};

struct TE_ThemeEditorNode : Buildable
{
	void Build() override
	{
		Push<MenuBarElement>();
		{
			Push<MenuItemElement>().SetText("File");
			{
				Make<MenuItemElement>().SetText("New").Func([this]() { theme->Clear(); Rebuild(); });
				Make<MenuItemElement>().SetText("Open").Func([this]() { theme->LoadFromFile("sample.ths"); Rebuild(); });
				Make<MenuItemElement>().SetText("Save").Func([this]() { theme->SaveToFile("sample.ths"); Rebuild(); });
			}
			Pop();
		}
		Pop();

		Push<TabGroup>() + SetHeight(Coord::Percent(100)) + SetLayout(layouts::EdgeSlice());
		{
			Push<TabButtonList>();
			{
				for (TE_Template* tmpl : theme->templates)
				{
					Push<TabButtonT<TE_Template*>>().Init(theme->curTemplate, tmpl);
					if (editNameTemplate != tmpl)
					{
						Text(tmpl->name).HandleEvent(EventType::Click) = [this, tmpl](Event& e)
						{
							if (e.numRepeats == 2)
							{
								editNameTemplate = tmpl;
								Rebuild();
							}
						};
					}
					else
					{
						auto efn = [this](Event& e)
						{
							if (e.type == EventType::LostFocus)
							{
								editNameTemplate = nullptr;
								Rebuild();
							}
						};
						imm::EditString(
							tmpl->name.c_str(),
							[tmpl](const char* v) { tmpl->name = v; },
							{ SetWidth(100), AddEventHandler(efn) });
					}
					Pop();
				}
			}
			if (imm::Button("+"))
			{
				auto* p = new TE_Template(theme);
				p->name = "<name>";
				theme->templates.push_back(p);
			}
			Pop();

			if (theme->curTemplate)
			{
				Push<TabPanel>() + SetHeight(Coord::Percent(100));
				{
					auto& pen = Make<TE_TemplateEditorNode>();
					pen + SetHeight(Coord::Percent(100));
					pen.theme = theme;
					pen.tmpl = theme->curTemplate;
				}
				Pop();
			}
		}
		Pop();
	}

	TE_Theme* theme;
	TE_Template* editNameTemplate = nullptr;
};

struct ThemeEditorMainWindow : NativeMainWindow
{
	ThemeEditorMainWindow()
	{
		SetTitle("Theme Editor");
		SetSize(1280, 720);
	}
	void OnBuild() override
	{
		auto& ten = Make<TE_ThemeEditorNode>();
		ten + SetHeight(Coord::Percent(100));
		ten.theme = &theme;

		Make<DefaultOverlayBuilder>();
	}

	TE_Theme theme;
};

int uimain(int argc, char* argv[])
{
	Application app(argc, argv);
	ThemeEditorMainWindow mw;
	mw.SetVisible(true);
	return app.Run();
}
