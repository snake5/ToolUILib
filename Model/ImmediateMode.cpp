
#include "ImmediateMode.h"
#include "Controls.h"
#include "Graphics.h"
#include "System.h"


namespace ui {
namespace imm {

void CheckboxStateToggleSkin::BuildContents(UIContainer* ctx, StateToggleBase& parent, StringView text, uint8_t state) const
{
	ctx->Make<CheckboxIcon>();
	if (!text.empty())
		ctx->Text(text) + SetPadding(4);
}

void RadioButtonStateToggleSkin::BuildContents(UIContainer* ctx, StateToggleBase& parent, StringView text, uint8_t state) const
{
	ctx->Make<RadioButtonIcon>();
	if (!text.empty())
		ctx->Text(text) + SetPadding(4);
}

void ButtonStateToggleSkin::BuildContents(UIContainer* ctx, StateToggleBase& parent, StringView text, uint8_t state) const
{
	ctx->MakeWithText<StateButtonSkin>(text);
}

void TreeStateToggleSkin::BuildContents(UIContainer* ctx, StateToggleBase& parent, StringView text, uint8_t state) const
{
	ctx->Make<TreeExpandIcon>();
	if (!text.empty())
		ctx->Text(text) + SetPadding(4);
}

bool Button(UIContainer* ctx, const char* text, ModInitList mods)
{
	auto& btn = ctx->MakeWithText<ui::Button>(text);
	btn.flags |= UIObject_DB_IMEdit;
	for (auto& mod : mods)
		mod->Apply(&btn);
	bool clicked = false;
	if (btn.flags & UIObject_IsEdited)
	{
		clicked = true;
		btn.flags &= ~UIObject_IsEdited;
		btn._OnIMChange();
	}
	return clicked;
}

bool CheckboxRaw(UIContainer* ctx, bool val, const char* text, ModInitList mods, const IStateToggleSkin& skin)
{
	auto& cb = ctx->Push<StateToggle>();
	skin.BuildContents(ctx, cb, text ? text : StringView(), val);
	ctx->Pop();

	cb.flags |= UIObject_DB_IMEdit;
	for (auto& mod : mods)
		mod->Apply(&cb);
	bool edited = false;
	if (cb.flags & UIObject_IsEdited)
	{
		cb.flags &= ~UIObject_IsEdited;
		edited = true;
		cb._OnIMChange();
	}
	cb.InitReadOnly(val);
	return edited;
}

bool EditBool(UIContainer* ctx, bool& val, const char* text, ModInitList mods, const IStateToggleSkin& skin)
{
	if (CheckboxRaw(ctx, val, text, mods, skin))
	{
		val = !val;
		return true;
	}
	return false;
}

bool RadioButtonRaw(UIContainer* ctx, bool val, const char* text, ModInitList mods, const IStateToggleSkin& skin)
{
	auto& rb = ctx->Push<StateToggle>();
	skin.BuildContents(ctx, rb, text ? text : StringView(), val);
	ctx->Pop();

	rb.flags |= UIObject_DB_IMEdit;
	for (auto& mod : mods)
		mod->Apply(&rb);
	bool edited = false;
	if (rb.flags & UIObject_IsEdited)
	{
		rb.flags &= ~UIObject_IsEdited;
		edited = true;
		rb._OnIMChange();
	}
	rb.InitReadOnly(val);
	return edited;
}

struct NumFmtBox
{
	char fmt[8];

	NumFmtBox(const char* f)
	{
		fmt[0] = ' ';
		strncpy(fmt + 1, f, 6);
		fmt[7] = '\0';
	}
};

const char* RemoveNegZero(const char* str)
{
	return strncmp(str, "-0", 3) == 0 ? "0" : str;
}

template <class T> struct MakeSigned {};
template <> struct MakeSigned<int> { using type = int; };
template <> struct MakeSigned<unsigned> { using type = int; };
template <> struct MakeSigned<int64_t> { using type = int64_t; };
template <> struct MakeSigned<uint64_t> { using type = int64_t; };
template <> struct MakeSigned<float> { using type = float; };

template <class TNum> bool EditNumber(UIContainer* ctx, UIObject* dragObj, TNum& val, ModInitList mods, float speed, TNum vmin, TNum vmax, const char* fmt)
{
	auto& tb = ctx->Make<Textbox>();
	for (auto& mod : mods)
		mod->Apply(&tb);

	NumFmtBox fb(fmt);

	bool edited = false;
	if (tb.flags & UIObject_IsEdited)
	{
		decltype(val + 0) tmp = 0;
		sscanf(tb.GetText().c_str(), fb.fmt, &tmp);
		if (tmp == 0)
			tmp = 0;
		if (tmp > vmax)
			tmp = vmax;
		if (tmp < vmin)
			tmp = vmin;
		val = tmp;
		tb.flags &= ~UIObject_IsEdited;
		edited = true;
		tb._OnIMChange();
	}

	char buf[1024];
	snprintf(buf, 1024, fb.fmt + 1, val);
	tb.SetText(RemoveNegZero(buf));

	if (dragObj)
	{
		dragObj->SetFlag(UIObject_DB_CaptureMouseOnLeftClick, true);
		dragObj->HandleEvent() = [val, speed, vmin, vmax, &tb, fb](Event& e)
		{
			if (tb.IsInputDisabled())
				return;
			if (e.type == EventType::MouseMove && e.target->IsPressed() && e.delta.x != 0)
			{
				if (tb.IsFocused())
					e.context->SetKeyboardFocus(nullptr);

				float diff = e.delta.x * speed * UNITS_PER_PX;
				tb.accumulator += diff;
				TNum nv = val;
				if (fabsf(tb.accumulator) >= speed)
				{
					nv += trunc(tb.accumulator / speed) * speed;
					tb.accumulator = fmodf(tb.accumulator, speed);
				}

				if (nv > vmax || (diff > 0 && nv < val))
					nv = vmax;
				if (nv < vmin || (diff < 0 && nv > val))
					nv = vmin;

				char buf[1024];
				snprintf(buf, 1024, fb.fmt + 1, nv);
				tb.SetText(RemoveNegZero(buf));
				tb.flags |= UIObject_IsEdited;

				e.context->OnCommit(e.target);
				tb.RebuildContainer();
			}
			if (e.type == EventType::SetCursor)
			{
				e.context->SetDefaultCursor(DefaultCursor::ResizeHorizontal);
				e.StopPropagation();
			}
		};
	}
	tb.HandleEvent(EventType::Commit) = [&tb](Event& e)
	{
		tb.flags |= UIObject_IsEdited;
		tb.RebuildContainer();
	};

	return edited;
}

bool EditInt(UIContainer* ctx, UIObject* dragObj, int& val, ModInitList mods, float speed, int vmin, int vmax, const char* fmt)
{
	return EditNumber(ctx, dragObj, val, mods, speed, vmin, vmax, fmt);
}

bool EditInt(UIContainer* ctx, UIObject* dragObj, unsigned& val, ModInitList mods, float speed, unsigned vmin, unsigned vmax, const char* fmt)
{
	return EditNumber(ctx, dragObj, val, mods, speed, vmin, vmax, fmt);
}

bool EditInt(UIContainer* ctx, UIObject* dragObj, int64_t& val, ModInitList mods, float speed, int64_t vmin, int64_t vmax, const char* fmt)
{
	return EditNumber(ctx, dragObj, val, mods, speed, vmin, vmax, fmt);
}

bool EditInt(UIContainer* ctx, UIObject* dragObj, uint64_t& val, ModInitList mods, float speed, uint64_t vmin, uint64_t vmax, const char* fmt)
{
	return EditNumber(ctx, dragObj, val, mods, speed, vmin, vmax, fmt);
}

bool EditFloat(UIContainer* ctx, UIObject* dragObj, float& val, ModInitList mods, float speed, float vmin, float vmax, const char* fmt)
{
	return EditNumber(ctx, dragObj, val, mods, speed, vmin, vmax, fmt);
}

bool EditString(UIContainer* ctx, const char* text, const std::function<void(const char*)>& retfn, ModInitList mods)
{
	auto& tb = ctx->Make<Textbox>();
	for (auto& mod : mods)
		mod->Apply(&tb);
	bool changed = false;
	if (tb.flags & UIObject_IsEdited)
	{
		retfn(tb.GetText().c_str());
		tb.flags &= ~UIObject_IsEdited;
		tb._OnIMChange();
		changed = true;
	}
	else // text can be invalidated if retfn is called
		tb.SetText(text);
	tb.HandleEvent(EventType::Change) = [&tb](Event&)
	{
		tb.flags |= UIObject_IsEdited;
		tb.RebuildContainer();
	};
	return changed;
}

bool EditColor(UIContainer* ctx, Color4f& val, ModInitList mods)
{
	auto& ced = ctx->Make<ColorEdit>();
	for (auto& mod : mods)
		mod->Apply(&ced);
	bool changed = false;
	if (ced.flags & UIObject_IsEdited)
	{
		val = ced.GetColor().GetRGBA();
		ced.flags &= ~UIObject_IsEdited;
		ced._OnIMChange();
		changed = true;
	}
	else
		ced.SetColor(val);
	ced.HandleEvent(EventType::Change) = [&ced](Event&)
	{
		ced.flags |= UIObject_IsEdited;
		ced.RebuildContainer();
	};
	return changed;
}

bool EditColor(UIContainer* ctx, Color4b& val, ModInitList mods)
{
	Color4f tmp = val;
	if (EditColor(ctx, tmp, mods))
	{
		val = tmp;
		return true;
	}
	return false;
}

bool EditFloatVec(UIContainer* ctx, float* val, const char* axes, ModInitList mods, float speed, float vmin, float vmax, const char* fmt)
{
	bool any = false;
	char axisLabel[3] = "\b\0";
	while (*axes)
	{
		axisLabel[1] = *axes++;
		any |= PropEditFloat(ctx, axisLabel, *val++, mods, speed, vmin, vmax, fmt);
	}
	return any;
}


void PropText(UIContainer* ctx, const char* label, const char* text, ModInitList mods)
{
	LabeledProperty::Scope ps(ctx, label);
	auto& ctrl = ctx->Text(text) + SetPadding(5);
	for (auto& mod : mods)
		mod->Apply(&ctrl);
}

bool PropButton(UIContainer* ctx, const char* label, const char* text, ModInitList mods)
{
	LabeledProperty::Scope ps(ctx, label);
	return Button(ctx, text, mods);
}

bool PropEditBool(UIContainer* ctx, const char* label, bool& val, ModInitList mods)
{
	LabeledProperty::Scope ps(ctx, label);
	return EditBool(ctx, val, nullptr, mods);
}

bool PropEditInt(UIContainer* ctx, const char* label, int& val, ModInitList mods, float speed, int vmin, int vmax, const char* fmt)
{
	LabeledProperty::Scope ps(ctx, label);
	return EditInt(ctx, ps.label, val, mods, speed, vmin, vmax, fmt);
}

bool PropEditInt(UIContainer* ctx, const char* label, unsigned& val, ModInitList mods, float speed, unsigned vmin, unsigned vmax, const char* fmt)
{
	LabeledProperty::Scope ps(ctx, label);
	return EditInt(ctx, ps.label, val, mods, speed, vmin, vmax, fmt);
}

bool PropEditInt(UIContainer* ctx, const char* label, int64_t& val, ModInitList mods, float speed, int64_t vmin, int64_t vmax, const char* fmt)
{
	LabeledProperty::Scope ps(ctx, label);
	return EditInt(ctx, ps.label, val, mods, speed, vmin, vmax, fmt);
}

bool PropEditInt(UIContainer* ctx, const char* label, uint64_t& val, ModInitList mods, float speed, uint64_t vmin, uint64_t vmax, const char* fmt)
{
	LabeledProperty::Scope ps(ctx, label);
	return EditInt(ctx, ps.label, val, mods, speed, vmin, vmax, fmt);
}

bool PropEditFloat(UIContainer* ctx, const char* label, float& val, ModInitList mods, float speed, float vmin, float vmax, const char* fmt)
{
	LabeledProperty::Scope ps(ctx, label);
	return EditFloat(ctx, ps.label, val, mods, speed, vmin, vmax, fmt);
}

bool PropEditString(UIContainer* ctx, const char* label, const char* text, const std::function<void(const char*)>& retfn, ModInitList mods)
{
	LabeledProperty::Scope ps(ctx, label);
	return EditString(ctx, text, retfn, mods);
}

bool PropEditColor(UIContainer* ctx, const char* label, Color4f& val, ModInitList mods)
{
	LabeledProperty::Scope ps(ctx, label);
	return EditColor(ctx, val, mods);
}

bool PropEditColor(UIContainer* ctx, const char* label, Color4b& val, ModInitList mods)
{
	LabeledProperty::Scope ps(ctx, label);
	return EditColor(ctx, val, mods);
}

bool PropEditFloatVec(UIContainer* ctx, const char* label, float* val, const char* axes, ModInitList mods, float speed, float vmin, float vmax, const char* fmt)
{
	LabeledProperty::Scope ps(ctx, label);
	return EditFloatVec(ctx, val, axes, mods, speed, vmin, vmax, fmt);
}

} // imm
} // ui
