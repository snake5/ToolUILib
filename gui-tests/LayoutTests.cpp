
#include "pch.h"


struct EdgeSliceTest : ui::Buildable
{
	void Build() override
	{
		auto s = ui::Push<ui::Panel>().GetStyle();
		s.SetLayout(ui::layouts::EdgeSlice());
		s.SetBoxSizing(ui::BoxSizing::BorderBox);
		s.SetMargin(0);
		s.SetHeight(ui::Coord::Percent(100));

		ui::MakeWithText<ui::Button>("Top").GetStyle().SetEdge(ui::Edge::Top);
		ui::MakeWithText<ui::Button>("Right").GetStyle().SetEdge(ui::Edge::Right);
		ui::MakeWithText<ui::Button>("Bottom").GetStyle().SetEdge(ui::Edge::Bottom);
		ui::MakeWithText<ui::Button>("Left").GetStyle().SetEdge(ui::Edge::Left);

		ui::Pop();
	}
};
void Test_EdgeSlice()
{
	ui::Make<EdgeSliceTest>();
}


static const char* layoutShortNames[] =
{
	"ST", "SL", "SB", "SR",
	"IB",
	"ET", "EL", "EB", "ER",
};
static const char* layoutLongNames[] =
{
	"Stack top-down",
	"Stack left-to-right",
	"Stack bottom-up",
	"Stack right-to-left",
	"Inline block",
	"Edge top child",
	"Edge left child",
	"Edge bottom child",
	"Edge right child",
};
constexpr int layoutCount = sizeof(layoutShortNames) / sizeof(const char*);

struct LayoutNestComboTest : ui::Buildable
{
	void Build() override
	{
		static int parentLayout = 0;
		static int childLayout = 0;

		LayoutUI(parentLayout);
		LayoutUI(childLayout);

		SetLayout(ui::Push<ui::Panel>().GetStyle(), parentLayout, -1);
		{
			SetLayout(ui::Push<ui::Panel>().GetStyle(), childLayout, parentLayout);
			{
				ui::Text("[c 1A]");
				ui::Text("[c 1B]");
			}
			ui::Pop();

			SetLayout(ui::Push<ui::Panel>().GetStyle(), childLayout, parentLayout);
			{
				ui::Text("[c 2A]");
				ui::Text("[c 2B]");
			}
			ui::Pop();
		}
		ui::Pop();
	}

	void LayoutUI(int& layout)
	{
		ui::Push<ui::Panel>().GetStyle().SetStackingDirection(ui::StackingDirection::LeftToRight);
		for (int i = 0; i < layoutCount; i++)
		{
			BasicRadioButton2(layoutShortNames[i], layout, i) + ui::RebuildOnChange();
		}
		ui::Text(layoutLongNames[layout]);
		ui::Pop();
	}

	void SetLayout(ui::StyleAccessor s, int layout, int parentLayout)
	{
		switch (layout)
		{
		case 0:
			s.SetLayout(ui::layouts::Stack());
			s.SetStackingDirection(ui::StackingDirection::TopDown);
			break;
		case 1:
			s.SetLayout(ui::layouts::Stack());
			s.SetStackingDirection(ui::StackingDirection::LeftToRight);
			break;
		case 2:
			s.SetLayout(ui::layouts::Stack());
			s.SetStackingDirection(ui::StackingDirection::BottomUp);
			break;
		case 3:
			s.SetLayout(ui::layouts::Stack());
			s.SetStackingDirection(ui::StackingDirection::RightToLeft);
			break;
		case 4:
			s.SetLayout(ui::layouts::InlineBlock());
			break;
		case 5:
		case 6:
		case 7:
		case 8:
			s.SetLayout(ui::layouts::EdgeSlice());
			break;
		}

		switch (parentLayout)
		{
		case 5:
			s.SetEdge(ui::Edge::Top);
			break;
		case 6:
			s.SetEdge(ui::Edge::Left);
			break;
		case 7:
			s.SetEdge(ui::Edge::Bottom);
			break;
		case 8:
			s.SetEdge(ui::Edge::Right);
			break;
		}
	}
};
void Test_LayoutNestCombo()
{
	ui::Make<LayoutNestComboTest>();
}


struct StackingLayoutVariationsTest : ui::Buildable
{
	void Build() override
	{
		static int mode = 0;

		static const char* layoutShortNames[] =
		{
			"HSL",
		};
		static const char* layoutLongNames[] =
		{
			"Horizontal stacking layout",
		};
		constexpr int layoutCount = sizeof(layoutShortNames) / sizeof(const char*);

		ui::Push<ui::Panel>().GetStyle().SetStackingDirection(ui::StackingDirection::LeftToRight);
		for (int i = 0; i < layoutCount; i++)
		{
			BasicRadioButton2(layoutShortNames[i], mode, i) + ui::RebuildOnChange();
		}
		ui::Text(layoutLongNames[mode]);
		ui::Pop();

		if (mode == 0)
		{
			{ auto s = ui::Push<ui::Panel>().GetStyle(); s.SetLayout(ui::layouts::StackExpand()); s.SetStackingDirection(ui::StackingDirection::LeftToRight); }
			ui::MakeWithText<ui::Button>("One");
			ui::MakeWithText<ui::Button>("Another one");
			ui::Pop();

			{ auto s = ui::Push<ui::Panel>().GetStyle(); s.SetLayout(ui::layouts::StackExpand()); s.SetStackingDirection(ui::StackingDirection::LeftToRight); }
			ui::MakeWithText<ui::Button>("One").GetStyle().SetWidth(100);
			ui::MakeWithText<ui::Button>("Another one");
			ui::MakeWithText<ui::Button>("The third");
			ui::Pop();

			{ auto s = ui::Push<ui::Panel>().GetStyle(); s.SetLayout(ui::layouts::StackExpand()); s.SetStackingDirection(ui::StackingDirection::LeftToRight); }
			ui::MakeWithText<ui::Button>("One");
			ui::MakeWithText<ui::Button>("Another one").GetStyle().SetWidth(100);
			ui::MakeWithText<ui::Button>("The third");
			ui::Pop();

			{ auto s = ui::Push<ui::Panel>().GetStyle(); s.SetLayout(ui::layouts::StackExpand()); s.SetStackingDirection(ui::StackingDirection::LeftToRight); }
			ui::MakeWithText<ui::Button>("One");
			ui::MakeWithText<ui::Button>("Another one");
			ui::MakeWithText<ui::Button>("The third").GetStyle().SetWidth(100);
			ui::Pop();

			{ auto s = ui::Push<ui::Panel>().GetStyle(); s.SetLayout(ui::layouts::StackExpand()); s.SetStackingDirection(ui::StackingDirection::LeftToRight); }
			ui::MakeWithText<ui::Button>("One").GetStyle().SetMinWidth(50);
			ui::MakeWithText<ui::Button>("Another one").GetStyle().SetMinWidth(100);
			ui::MakeWithText<ui::Button>("The third").GetStyle().SetMinWidth(150);
			ui::Pop();

			{ auto s = ui::Push<ui::Panel>().GetStyle(); s.SetLayout(ui::layouts::StackExpand()); s.SetStackingDirection(ui::StackingDirection::LeftToRight); }
			{ auto s = ui::MakeWithText<ui::Button>("One").GetStyle(); s.SetMinWidth(100); s.SetWidth(ui::Coord::Percent(30)); }
			ui::MakeWithText<ui::Button>("Another one");
			ui::Pop();

			{ auto s = ui::Push<ui::Panel>().GetStyle(); s.SetLayout(ui::layouts::Stack()); s.SetStackingDirection(ui::StackingDirection::LeftToRight); }
			ui::MakeWithText<ui::Button>("One");
			ui::MakeWithText<ui::Button>("Another one");
			ui::Pop();
		}
	}
};
void Test_StackingLayoutVariations()
{
	ui::Make<StackingLayoutVariationsTest>();
}


struct SizeTest : ui::Buildable
{
	struct SizedBox : ui::Placeholder
	{
		void GetSize(ui::Coord& outWidth, ui::Coord& outHeight) override
		{
			outWidth = w;
			outHeight = h;
		}
		int w = 40;
		int h = 40;
	};
	struct Test
	{
		UIObject* obj;
		std::function<std::string()> func;
	};

	void OnPaint() override
	{
		ui::Buildable::OnPaint();
		for (const auto& t : tests)
		{
			auto res = t.func();
			if (res.size())
			{
				ui::DrawTextLine(t.obj->finalRectC.x1, t.obj->finalRectC.y1, res.c_str(), 1, 0, 0);
			}
		}
	}

	void Build() override
	{
		tests.clear();
		ui::Text("Any errors will be drawn next to the element in red");

		TestContentSize(ui::Text("Testing text element size"), ceilf(ui::GetTextWidth("Testing text element size")), ui::GetFontHeight());

		ui::PushBox() + ui::SetLayout(ui::layouts::StackExpand()) + ui::Set(ui::StackingDirection::LeftToRight);
		{
			auto& txt1 = ui::Text("Text size + padding") + ui::SetPadding(5);
			TestContentSize(txt1, ceilf(ui::GetTextWidth("Text size + padding")), ui::GetFontHeight());
			TestFullSize(txt1, ceilf(ui::GetTextWidth("Text size + padding")) + 10, ui::GetFontHeight() + 10);
			TestFullEstSize(txt1, ceilf(ui::GetTextWidth("Text size + padding")) + 10, ui::GetFontHeight() + 10);
		}

		{
			auto& box = ui::PushBox() + ui::SetLayout(ui::layouts::InlineBlock()) + ui::SetPadding(5);
			ui::Text("Testing text in box");
			ui::Pop();
			TestContentSize(box, ceilf(ui::GetTextWidth("Testing text in box")), ui::GetFontHeight());
			TestFullSize(box, ceilf(ui::GetTextWidth("Testing text in box")) + 10, ui::GetFontHeight() + 10);
			TestFullX(box, ceilf(ui::GetTextWidth("Text size + padding")) + 10);
		}
		ui::Pop();

		{
			auto& box = ui::PushBox() + ui::SetLayout(ui::layouts::InlineBlock()) + ui::SetPadding(5) + ui::SetBoxSizing(ui::BoxSizing::BorderBox);
			ui::Text("Testing text in box [border]");
			ui::Pop();
			TestContentSize(box, ceilf(ui::GetTextWidth("Testing text in box [border]")), ui::GetFontHeight());
			TestFullSize(box, ceilf(ui::GetTextWidth("Testing text in box [border]")) + 10, ui::GetFontHeight() + 10);
		}

		{
			auto& box = ui::PushBox() + ui::SetLayout(ui::layouts::InlineBlock()) + ui::SetPadding(5) + ui::SetBoxSizing(ui::BoxSizing::ContentBox);
			ui::Text("Testing text in box [content]");
			ui::Pop();
			TestContentSize(box, ceilf(ui::GetTextWidth("Testing text in box [content]")), ui::GetFontHeight());
			TestFullSize(box, ceilf(ui::GetTextWidth("Testing text in box [content]")) + 10, ui::GetFontHeight() + 10);
		}

		{
			auto& box = ui::PushBox() + ui::SetLayout(ui::layouts::InlineBlock()) + ui::SetPadding(5) + ui::SetWidth(140);
			ui::Text("Testing text in box +W");
			ui::Pop();
			TestContentSize(box, 140 - 10, ui::GetFontHeight());
			TestFullSize(box, 140, ui::GetFontHeight() + 10);
		}

		{
			auto& box = ui::PushBox() + ui::SetLayout(ui::layouts::InlineBlock()) + ui::SetPadding(5) + ui::SetWidth(140) + ui::SetBoxSizing(ui::BoxSizing::BorderBox);
			ui::Text("Testing text in box +W [border]");
			ui::Pop();
			TestContentSize(box, 140 - 10, ui::GetFontHeight());
			TestFullSize(box, 140, ui::GetFontHeight() + 10);
		}

		{
			auto& box = ui::PushBox() + ui::SetLayout(ui::layouts::InlineBlock()) + ui::SetPadding(5) + ui::SetWidth(140) + ui::SetBoxSizing(ui::BoxSizing::ContentBox);
			ui::Text("Testing text in box +W [content]");
			ui::Pop();
			TestContentSize(box, 140, ui::GetFontHeight());
			TestFullSize(box, 140 + 10, ui::GetFontHeight() + 10);
		}

		ui::PushBox() + ui::Set(ui::StackingDirection::LeftToRight);
		{
			auto& box = ui::Make<SizedBox>();
			TestContentSize(box, 40, 40);
			TestFullSize(box, 40, 40);
			TestFullEstSize(box, 40, 40);
		}
		{
			auto& box = ui::Make<SizedBox>() + ui::SetWidth(50);
			TestContentSize(box, 50, 40);
			TestFullSize(box, 50, 40);
			TestFullEstSize(box, 50, 40);
		}
		{
			auto& box = ui::Make<SizedBox>() + ui::SetWidth(50) + ui::SetPadding(2, 7, 8, 3);
			TestContentSize(box, 40, 30);
			TestFullSize(box, 50, 40);
			TestFullEstSize(box, 50, 40);
		}
		{
			auto& box = ui::Make<SizedBox>() + ui::SetBoxSizing(ui::BoxSizing::BorderBox) + ui::SetWidth(50) + ui::SetPadding(2, 7, 8, 3);
			TestContentSize(box, 40, 30);
			TestFullSize(box, 50, 40);
			TestFullEstSize(box, 50, 40);
		}
		{
			auto& box = ui::Make<SizedBox>() + ui::SetBoxSizing(ui::BoxSizing::ContentBox) + ui::SetHeight(30) + ui::SetPadding(2, 7, 8, 3);
			TestContentSize(box, 40, 30);
			TestFullSize(box, 50, 40);
			TestFullEstSize(box, 50, 40);
		}
		ui::Pop();
	}

	static std::string TestSize(ui::UIRect& r, float w, float h)
	{
		if (fabsf(r.GetWidth() - w) > 0.0001f || fabsf(r.GetHeight() - h) > 0.0001f)
		{
			return ui::Format("expected %g;%g - got %g;%g", w, h, r.GetWidth(), r.GetHeight());
		}
		return {};
	}

	void TestContentSize(UIObject& obj, float w, float h)
	{
		auto fn = [obj{ &obj }, w, h]()->std::string
		{
			return TestSize(obj->finalRectC, w, h);
		};
		tests.push_back({ &obj, fn });
	}

	void TestFullSize(UIObject& obj, float w, float h)
	{
		auto fn = [obj{ &obj }, w, h]()->std::string
		{
			return TestSize(obj->finalRectCPB, w, h);
		};
		tests.push_back({ &obj, fn });
	}

	void TestFullX(UIObject& obj, float x)
	{
		auto fn = [obj{ &obj }, x]()->std::string
		{
			auto r = obj->finalRectCPB;
			if (fabsf(r.x0 - x) > 0.0001f)
			{
				return ui::Format("expected x %g - got %g", x, r.x0);
			}
			return {};
		};
		tests.push_back({ &obj, fn });
	}

	void TestFullEstSize(UIObject& obj, float w, float h)
	{
		auto fn = [obj{ &obj }, w, h]()->std::string
		{
			auto ew = obj->GetFullEstimatedWidth({ 500, 500 }, ui::EstSizeType::Exact);
			auto eh = obj->GetFullEstimatedHeight({ 500, 500 }, ui::EstSizeType::Exact);
			if (ew.max < ew.min)
				return "est.width: max < min";
			if (eh.max < eh.min)
				return "est.height: max < min";
			ui::UIRect r{ 0, 0, ew.min, eh.min };
			return TestSize(r, w, h);
		};
		tests.push_back({ &obj, fn });
	}

	std::vector<Test> tests;
};
void Test_Size()
{
	ui::Make<SizeTest>();
}


struct PlacementTest : ui::Buildable
{
	PlacementTest()
	{
		buttonPlacement.bias = { 3, -2, -2, -2 };
		buttonPlacement.applyOnLayout = true;
	}
	void Build() override
	{
		*this + ui::SetPadding(20);

		ui::Text("Expandable menu example:");
		ui::PushBox();

		ui::imm::EditBool(open, nullptr, { ui::MakeOverlay(open, 1.0f) });

		if (open)
		{
			auto& lb = ui::Push<ui::ListBox>();
			lb.RegisterAsOverlay();
			auto* pap = Allocate<ui::PointAnchoredPlacement>();
			pap->SetAnchorAndPivot({ 0, 0 });
			pap->bias = { -5, -5 };
			lb.GetStyle().SetPlacement(pap);

			// room for checkbox
			ui::Make<ui::BoxElement>() + ui::SetWidth(25) + ui::SetHeight(25);

			ui::Text("opened");
			ui::imm::PropEditBool("One", one);
			ui::imm::PropEditInt("Two", two);

			ui::Pop();
		}

		ui::Pop();

		ui::Text("Autocomplete example:");
		static const char* suggestions[] = { "apple", "banana", "car", "duck", "elephant", "file", "grid" };
		ui::PushBox();
		ui::imm::EditString(text.c_str(),
			[this](const char* v) { text = v; }, {
			ui::AddEventHandler(ui::EventType::GotFocus, [this](ui::Event&) { showDropdown = true; curSelection = 0; Rebuild(); }),
			ui::AddEventHandler(ui::EventType::LostFocus, [this](ui::Event&) { showDropdown = false; Rebuild(); }),
			ui::AddEventHandler(ui::EventType::KeyAction, [this](ui::Event& e)
			{
				switch (e.GetKeyAction())
				{
				case ui::KeyAction::Down:
					curSelection++;
					if (curSelection >= curOptionCount)
						curSelection = 0;
					Rebuild();
					break;
				case ui::KeyAction::Up:
					curSelection--;
					if (curSelection < 0)
						curSelection = std::max(curOptionCount - 1, 0);
					Rebuild();
					break;
				case ui::KeyAction::Enter:
					if (text.size())
					{
						for (int num = 0, i = 0; i < 7; i++)
						{
							if (strstr(suggestions[i], text.c_str()))
							{
								if (num == curSelection)
								{
									text = suggestions[i];
									break;
								}
								num++;
							}
						}
						e.context->SetKeyboardFocus(nullptr);
						e.StopPropagation();
						Rebuild();
					}
					break;
				}
			}),
			});
		if (showDropdown)
		{
			auto& lb = ui::Push<ui::ListBox>();
			lb.RegisterAsOverlay();
			auto* pap = Allocate<ui::PointAnchoredPlacement>();
			pap->anchor = { 0, 1 };
			lb.GetStyle().SetPlacement(pap);

			int num = 0;
			for (int i = 0; i < 7; i++)
			{
				if (strstr(suggestions[i], text.c_str()))
				{
					ui::MakeWithText<ui::Selectable>(suggestions[i]).Init(curSelection == num)
						+ ui::AddEventHandler(ui::EventType::Activate, [this, i](ui::Event& e) { text = suggestions[i]; e.context->SetKeyboardFocus(nullptr); });
					num++;
				}
			}
			curOptionCount = num;
			if (curSelection >= curOptionCount)
				curSelection = 0;

			ui::Pop();
		}
		ui::Pop();

		ui::Text("Self-based placement example:");
		ui::MakeWithText<ui::Button>("This should not cover the entire parent").GetStyle().SetPlacement(&buttonPlacement);
		ui::imm::PropEditFloatVec("Anchor", &buttonPlacement.anchor.x0, "LTRB", {}, 0.01f);
		ui::imm::PropEditFloatVec("Bias", &buttonPlacement.bias.x0, "LTRB");
	}

	bool open = false;
	bool one = true;
	int two = 3;

	std::string text;
	bool showDropdown = false;
	int curSelection = 0;
	int curOptionCount = 0;
	ui::RectAnchoredPlacement buttonPlacement;
};
void Test_Placement()
{
	ui::Make<PlacementTest>();
}

