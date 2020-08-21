
#include "../Core/HashTable.h"
#include "Objects.h"
#include "Native.h"
#include "System.h"
#include "Theme.h"


namespace ui {
extern uint32_t g_curLayoutFrame;
extern FrameContents* g_curSystem;

struct EventHandlerEntry
{
	EventHandlerEntry* next;
	UIObject* target;
	UIEventType type;
	bool isLocal;
	EventFunc func;
};
} // ui

UIObject::UIObject()
{
	SetStyle(ui::Theme::current->object);
}

UIObject::~UIObject()
{
	ClearEventHandlers();
	UnregisterAsOverlay();
}

void UIObject::_SerializePersistent(IDataSerializer& s)
{
	s << system;
	s << parent;
	s << prev;
	s << next;
	s << firstChild;
	s << lastChild;
	s << flags;
	OnSerialize(s);
}

void UIObject::_Reset()
{
	OnReset();
}

void UIObject::_DoEvent(UIEvent& e)
{
	e.current = this;
	for (auto* n = _firstEH; n && !e.IsPropagationStopped(); n = n->next)
	{
		if (n->type != UIEventType::Any && n->type != e.type)
			continue;
		if (n->target && !e.target->IsChildOrSame(n->target))
			continue;
		n->func(e);
	}

	if (!e.IsPropagationStopped())
		OnEvent(e);

	if (!e.IsPropagationStopped())
	{
		// default behaviors
		_PerformDefaultBehaviors(e);
	}
}

void UIObject::_PerformDefaultBehaviors(UIEvent& e)
{
	if (!IsInputDisabled())
	{
		if (HasFlags(UIObject_DB_IMEdit) && e.target == this && e.type == UIEventType::Activate)
		{
			flags |= UIObject_IsEdited;
			RerenderNode();
		}

		if (HasFlags(UIObject_DB_Button))
		{
			if (e.type == UIEventType::ButtonUp && e.GetButton() == UIMouseButton::Left && e.context->GetMouseCapture() == this)
			{
				e.context->OnActivate(this);
			}
			if (e.type == UIEventType::MouseMove)
			{
				if (e.context->GetMouseCapture() == this) // TODO separate flag for this!
					SetFlag(UIObject_IsPressedMouse, finalRectCPB.Contains(e.x, e.y));
				//e.StopPropagation();
			}
			if (e.type == UIEventType::KeyAction && e.GetKeyAction() == UIKeyAction::ActivateDown)
			{
				e.context->CaptureMouse(this);
				flags |= UIObject_IsPressedOther;
				e.StopPropagation();
			}
			if (e.type == UIEventType::KeyAction && e.GetKeyAction() == UIKeyAction::ActivateUp)
			{
				if (flags & UIObject_IsPressedOther)
				{
					e.context->OnActivate(this);
					flags &= ~UIObject_IsPressedOther;
				}
				if (!(flags & UIObject_IsPressedMouse))
					e.context->ReleaseMouse();
				e.StopPropagation();
			}
		}

		if (HasFlags(UIObject_DB_Draggable))
		{
			if (e.context->DragCheck(e, UIMouseButton::Left))
			{
				UIEvent e(&system->eventSystem, this, UIEventType::DragStart);
				e.context->BubblingEvent(e);
			}
		}

		if (HasFlags(UIObject_DB_Selectable))
		{
			if (e.type == UIEventType::ButtonDown && e.GetButton() == UIMouseButton::Left)
			{
				e.context->OnActivate(this);
				e.StopPropagation();
			}
		}

		if (HasFlags(UIObject_DB_FocusOnLeftClick))
		{
			if (e.type == UIEventType::ButtonDown && e.GetButton() == UIMouseButton::Left)
			{
				e.context->SetKeyboardFocus(this);
				e.StopPropagation();
			}
		}

		if (HasFlags(UIObject_DB_CaptureMouseOnLeftClick))
		{
			if (e.type == UIEventType::ButtonDown && e.GetButton() == UIMouseButton::Left)
			{
				e.context->CaptureMouse(e.current);
				e.current->flags |= UIObject_IsPressedMouse;
				e.StopPropagation();
			}
			if (e.type == UIEventType::ButtonUp && e.GetButton() == UIMouseButton::Left)
			{
				if (e.context->GetMouseCapture() == this)
					e.context->ReleaseMouse();
				e.StopPropagation();
			}
			if (e.type == UIEventType::MouseCaptureChanged)
			{
				flags &= ~UIObject_IsPressedMouse;
				e.StopPropagation();
			}
		}
	}
}

void UIObject::SendUserEvent(int id, uintptr_t arg0, uintptr_t arg1)
{
	system->eventSystem.OnUserEvent(this, id, arg0, arg1);
}

ui::EventFunc& UIObject::HandleEvent(UIObject* target, UIEventType type)
{
	auto eh = new ui::EventHandlerEntry;
	if (_lastEH)
		_lastEH->next = eh;
	else
		_firstEH = _lastEH = eh;
	eh->next = nullptr;
	eh->target = target;
	eh->type = type;
	eh->isLocal = system->container._curNode == this;
	_lastEH = eh;
	return eh->func;
}

void UIObject::ClearLocalEventHandlers()
{
	auto** eh = &_firstEH;
	ui::EventHandlerEntry* lastEH = nullptr;
	while (*eh)
	{
		auto* n = (*eh)->next;
		if ((*eh)->isLocal)
		{
			delete *eh;
			*eh = n;
		}
		else
		{
			lastEH = *eh;
			eh = &(*eh)->next;
		}
	}
	_lastEH = lastEH;
}

void UIObject::ClearEventHandlers()
{
	while (_firstEH)
	{
		auto* n = _firstEH->next;
		delete _firstEH;
		_firstEH = n;
	}
	_lastEH = nullptr;
}

void UIObject::RegisterAsOverlay(float depth)
{
	system->overlays.Register(this, depth);
	flags |= UIObject_IsOverlay;
}

void UIObject::UnregisterAsOverlay()
{
	if (flags & UIObject_IsOverlay)
	{
		system->overlays.Unregister(this);
		flags &= ~UIObject_IsOverlay;
	}
}

void UIObject::OnPaint()
{
	styleProps->paint_func(this);
	PaintChildren();
}

void UIObject::Paint()
{
	if (_CanPaint())
		OnPaint();
}

void UIObject::PaintChildren()
{
	if (!firstChild)
		return;
	ui::draw::PushScissorRect(finalRectC.x0, finalRectC.y0, finalRectC.x1, finalRectC.y1);
	for (auto* ch = firstChild; ch; ch = ch->next)
		ch->Paint();
	ui::draw::PopScissorRect();
}

float UIObject::CalcEstimatedWidth(const Size<float>& containerSize, style::EstSizeType type)
{
	auto layout = GetStyle().GetLayout();
	if (layout == nullptr)
		layout = style::layouts::Stack();
	return layout->CalcEstimatedWidth(this, containerSize, type);
}

float UIObject::CalcEstimatedHeight(const Size<float>& containerSize, style::EstSizeType type)
{
	float size = 0;
	auto layout = GetStyle().GetLayout();
	if (layout == nullptr)
		layout = style::layouts::Stack();
	return layout->CalcEstimatedHeight(this, containerSize, type);
}

Range<float> UIObject::GetEstimatedWidth(const Size<float>& containerSize, style::EstSizeType type)
{
	auto style = GetStyle();
	float size = 0;

	auto width = style.GetWidth();
	if (!width.IsDefined())
	{
		auto height = style::Coord::Undefined();
		GetSize(width, height);
	}

	float maxsize = FLT_MAX;
	if (width.IsDefined())
	{
		size = maxsize = ResolveUnits(width, containerSize.x);
	}
	else
	{
		size = CalcEstimatedWidth(containerSize, type);
	}

	auto min_width = style.GetMinWidth();
	if (min_width.IsDefined())
	{
		float w = ResolveUnits(min_width, containerSize.x);
		size = std::max(size, w);
		maxsize = std::max(maxsize, w);
	}

	auto max_width = style.GetMaxWidth();
	if (max_width.IsDefined())
	{
		maxsize = ResolveUnits(max_width, containerSize.x);
		size = std::min(size, maxsize);
	}

	return { size, maxsize };
}

Range<float> UIObject::GetEstimatedHeight(const Size<float>& containerSize, style::EstSizeType type)
{
	auto style = GetStyle();
	float size = 0;

	auto height = style.GetHeight();
	if (!height.IsDefined())
	{
		auto width = style::Coord::Undefined();
		GetSize(width, height);
	}

	float maxsize = FLT_MAX;
	if (height.IsDefined())
	{
		size = maxsize = ResolveUnits(height, containerSize.y);
	}
	else
	{
		size = CalcEstimatedHeight(containerSize, type);
	}

	auto min_height = style.GetMinHeight();
	if (min_height.IsDefined())
	{
		float h = ResolveUnits(min_height, containerSize.y);
		size = std::max(size, h);
		maxsize = std::max(maxsize, h);
	}

	auto max_height = style.GetMaxHeight();
	if (max_height.IsDefined())
	{
		maxsize = ResolveUnits(max_height, containerSize.y);
		size = std::min(size, maxsize);
	}

	return { size, maxsize };
}

Range<float> UIObject::GetFullEstimatedWidth(const Size<float>& containerSize, style::EstSizeType type, bool forParentLayout)
{
	if (!(forParentLayout ? _IsPartOfParentLayout() : _NeedsLayout()))
		return { 0, FLT_MAX };
	if (ui::g_curLayoutFrame == _cacheFrameWidth)
		return _cacheValueWidth;
	auto style = GetStyle();
	auto s = GetEstimatedWidth(containerSize, type);
	auto box_sizing = style.GetBoxSizing();

	float w_add = ResolveUnits(style.GetMarginLeft(), containerSize.x) + ResolveUnits(style.GetMarginRight(), containerSize.x);
	if (box_sizing == style::BoxSizing::ContentBox || !style.GetWidth().IsDefined())
	{
		w_add += ResolveUnits(style.GetPaddingLeft(), containerSize.x) + ResolveUnits(style.GetPaddingRight(), containerSize.x);
	}
	s.min += w_add;
	if (s.max < FLT_MAX)
		s.max += w_add;

	_cacheFrameWidth = ui::g_curLayoutFrame;
	_cacheValueWidth = s;
	return s;
}

Range<float> UIObject::GetFullEstimatedHeight(const Size<float>& containerSize, style::EstSizeType type, bool forParentLayout)
{
	if (!(forParentLayout ? _IsPartOfParentLayout() : _NeedsLayout()))
		return { 0, FLT_MAX };
	if (ui::g_curLayoutFrame == _cacheFrameHeight)
		return _cacheValueHeight;
	auto style = GetStyle();
	auto s = GetEstimatedHeight(containerSize, type);
	auto box_sizing = style.GetBoxSizing();

	float h_add = ResolveUnits(style.GetMarginTop(), containerSize.y) + ResolveUnits(style.GetMarginBottom(), containerSize.y);
	if (box_sizing == style::BoxSizing::ContentBox || !style.GetHeight().IsDefined())
	{
		h_add += ResolveUnits(style.GetPaddingTop(), containerSize.y) + ResolveUnits(style.GetPaddingBottom(), containerSize.y);
	}
	s.min += h_add;
	if (s.max < FLT_MAX)
		s.max += h_add;

	_cacheFrameHeight = ui::g_curLayoutFrame;
	_cacheValueHeight = s;
	return s;
}

void UIObject::PerformLayout(const UIRect& rect, const Size<float>& containerSize)
{
	if (_IsPartOfParentLayout())
	{
		OnLayout(rect, containerSize);
		OnLayoutChanged();
	}
}

void UIObject::_PerformPlacement(const UIRect& rect, const Size<float>& containerSize)
{
	if (_NeedsLayout() && GetStyle().GetPlacement() && !GetStyle().GetPlacement()->applyOnLayout)
	{
		OnLayout(rect, containerSize);
		OnLayoutChanged();
	}
}

void UIObject::OnLayout(const UIRect& inRect, const Size<float>& containerSize)
{
	using namespace style;

	lastLayoutInputRect = inRect;
	lastLayoutInputCSize = containerSize;

	auto style = GetStyle();

	auto swidth = style::Coord::Undefined();
	auto sheight = style::Coord::Undefined();
	GetSize(swidth, sheight);

	auto placement = style.GetPlacement();
	UIRect rect = inRect;
	if (placement)
	{
		if (!placement->applyOnLayout)
		{
			if (parent)
				rect = parent->GetContentRect();
			else
				rect = { 0, 0, system->eventSystem.width, system->eventSystem.height };
		}
		placement->OnApplyPlacement(this, rect);
	}

	auto width = style.GetWidth();
	if (width.unit == style::CoordTypeUnit::Fraction)
		width = rect.GetWidth();
	if (!width.IsDefined())
		width = swidth;

	auto height = style.GetHeight();
	if (height.unit == style::CoordTypeUnit::Fraction)
		height = rect.GetHeight();
	if (!height.IsDefined())
		height = sheight;

	auto min_width = style.GetMinWidth();
	auto min_height = style.GetMinHeight();
	auto max_width = style.GetMaxWidth();
	auto max_height = style.GetMaxHeight();
	auto box_sizing = style.GetBoxSizing();

	UIRect Mrect = GetMarginRect(style.block, rect.GetWidth());
	UIRect Prect = GetPaddingRect(style.block, rect.GetWidth());
	UIRect Arect =
	{
		Mrect.x0 + Prect.x0,
		Mrect.y0 + Prect.y0,
		Mrect.x1 + Prect.x1,
		Mrect.y1 + Prect.y1,
	};
	UIRect inrect =
	{
		rect.x0 + Arect.x0,
		rect.y0 + Arect.y0,
		rect.x1 - Arect.x1,
		rect.y1 - Arect.y1,
	};

	auto layout = style.GetLayout();
	if (layout == nullptr)
		layout = style::layouts::Stack();
	LayoutState state = { inrect, { inrect.x0, inrect.y0 } };
	layout->OnLayout(this, inrect, state);

	if (placement)
		state.finalContentRect = inrect;

	if (width.IsDefined() || min_width.IsDefined() || max_width.IsDefined())
	{
		float orig = state.finalContentRect.GetWidth();
		float tgt = width.IsDefined() ? ResolveUnits(width, containerSize.x) : orig;
		if (min_width.IsDefined())
			tgt = std::max(tgt, ResolveUnits(min_width, containerSize.x));
		if (max_width.IsDefined())
			tgt = std::min(tgt, ResolveUnits(max_width, containerSize.x));
		if (box_sizing != BoxSizing::ContentBox)
			tgt -= Prect.x0 + Prect.x1;
		if (tgt != orig)
		{
			if (orig)
			{
				float scale = tgt / orig;
				state.finalContentRect.x0 = state.scaleOrigin.x + (state.finalContentRect.x0 - state.scaleOrigin.x) * scale;
				state.finalContentRect.x1 = state.scaleOrigin.x + (state.finalContentRect.x1 - state.scaleOrigin.x) * scale;
			}
			else
				state.finalContentRect.x1 += tgt;
		}
	}
	if (height.IsDefined() || min_height.IsDefined() || max_height.IsDefined())
	{
		float orig = state.finalContentRect.GetHeight();
		float tgt = height.IsDefined() ? ResolveUnits(height, containerSize.y) : orig;
		if (min_height.IsDefined())
			tgt = std::max(tgt, ResolveUnits(min_height, containerSize.y));
		if (max_height.IsDefined())
			tgt = std::min(tgt, ResolveUnits(max_height, containerSize.y));
		if (box_sizing != BoxSizing::ContentBox)
			tgt -= Prect.y0 + Prect.y1;
		if (tgt != orig)
		{
			if (orig)
			{
				float scale = tgt / orig;
				state.finalContentRect.y0 = state.scaleOrigin.y + (state.finalContentRect.y0 - state.scaleOrigin.y) * scale;
				state.finalContentRect.y1 = state.scaleOrigin.y + (state.finalContentRect.y1 - state.scaleOrigin.y) * scale;
			}
			else
				state.finalContentRect.y1 += tgt;
		}
	}

	finalRectC = state.finalContentRect;
	finalRectCP = state.finalContentRect.ExtendBy(Prect);
	finalRectCPB = finalRectCP; // no border yet

	for (UIObject* ch = firstChild; ch; ch = ch->next)
		ch->_PerformPlacement(finalRectC, finalRectC.GetSize());
}

void UIObject::SetFlag(UIObjectFlags flag, bool set)
{
	if (set)
		flags |= flag;
	else
		flags &= ~flag;
}

bool UIObject::IsChildOf(UIObject* obj) const
{
	for (auto* p = parent; p; p = p->parent)
		if (p == obj)
			return true;
	return false;
}

bool UIObject::IsChildOrSame(UIObject* obj) const
{
	return obj == this || IsChildOf(obj);
}

int UIObject::CountChildrenImmediate() const
{
	int o = 0;
	for (auto* ch = firstChild; ch; ch = ch->next)
		o += 1;
	return o;
}

int UIObject::CountChildrenRecursive() const
{
	int o = 0;
	for (auto* ch = firstChild; ch; ch = ch->next)
		o += 1 + ch->CountChildrenRecursive();
	return o;
}

UIObject* UIObject::GetPrevInOrder()
{
	if (UIObject* ret = prev)
	{
		while (ret->lastChild)
			ret = ret->lastChild;
		return ret;
	}
	return parent;
}

UIObject* UIObject::GetNextInOrder()
{
	if (firstChild)
		return firstChild;
	for (UIObject* it = this; it; it = it->parent)
	{
		if (it->next)
			return it->next;
	}
	return nullptr;
}

UIObject* UIObject::GetFirstInOrder()
{
	return this;
}

UIObject* UIObject::GetLastInOrder()
{
	UIObject* ret = this;
	while (ret->lastChild)
		ret = ret->lastChild;
	return ret;
}

void UIObject::RerenderNode()
{
	if (auto* n = FindParentOfType<ui::Node>())
		n->Rerender();
}

bool UIObject::IsHovered() const
{
	return !!(flags & UIObject_IsHovered);
}

bool UIObject::IsPressed() const
{
	return !!(flags & UIObject_IsPressedAny);
}

bool UIObject::IsClicked(int button) const
{
	assert(button >= 0 && button < 5);
	return !!(flags & (_UIObject_IsClicked_First << button));
}

bool UIObject::IsFocused() const
{
	return this == system->eventSystem.focusObj;
}

bool UIObject::IsVisible() const
{
	return !(flags & UIObject_IsHidden);
}

void UIObject::SetVisible(bool v)
{
	if (v)
		flags &= ~UIObject_IsHidden;
	else
		flags |= UIObject_IsHidden;
}

bool UIObject::IsInputDisabled() const
{
	return !!(flags & UIObject_IsDisabled);
}

void UIObject::SetInputDisabled(bool v)
{
	if (v)
		flags |= UIObject_IsDisabled;
	else
		flags &= ~UIObject_IsDisabled;
}

style::Accessor UIObject::GetStyle()
{
	return style::Accessor(styleProps, this);
}

void UIObject::SetStyle(style::Block* style)
{
	styleProps = style;
	_OnChangeStyle();
}

void UIObject::_OnChangeStyle()
{
	ui::g_curSystem->container.layoutStack.Add(parent ? parent : this);
}

float UIObject::ResolveUnits(style::Coord coord, float ref)
{
	using namespace style;

	switch (coord.unit)
	{
	case CoordTypeUnit::Pixels:
		return coord.value;
	case CoordTypeUnit::Percent:
		return coord.value * ref * 0.01f;
	case CoordTypeUnit::Fraction:
		// special unit that takes some amount of the remaining space relative to other items in the same container
		return coord.value * ref;
	default:
		return 0;
	}
}

UIRect UIObject::GetMarginRect(style::Block* style, float ref)
{
	return
	{
		ResolveUnits(style->margin_left, ref),
		ResolveUnits(style->margin_top, ref),
		ResolveUnits(style->margin_right, ref),
		ResolveUnits(style->margin_bottom, ref),
	};
}

UIRect UIObject::GetPaddingRect(style::Block* style, float ref)
{
	return
	{
		ResolveUnits(style->padding_left, ref),
		ResolveUnits(style->padding_top, ref),
		ResolveUnits(style->padding_right, ref),
		ResolveUnits(style->padding_bottom, ref),
	};
}

float UIObject::GetFontSize(style::Block* styleOverride)
{
	style::Coord c;
	if (styleOverride)
		c = styleOverride->font_size;
	for (auto* p = this; p && (c.unit == style::CoordTypeUnit::Undefined || c.unit == style::CoordTypeUnit::Inherit); p = p->parent)
	{
		c = p->GetStyle().GetFontSize();
	}
	if (!c.IsDefined() || c.unit == style::CoordTypeUnit::Inherit)
		c = 12;
	return ResolveUnits(c, GetContentRect().GetWidth());
}

int UIObject::GetFontWeight(style::Block* styleOverride)
{
	auto w = style::FontWeight::Undefined;
	if (styleOverride)
		w = styleOverride->font_weight;
	for (auto* p = this; p && (w == style::FontWeight::Undefined || w == style::FontWeight::Inherit); p = p->parent)
	{
		w = p->GetStyle().GetFontWeight();
	}
	if (w == style::FontWeight::Undefined || w == style::FontWeight::Inherit)
		w = style::FontWeight::Normal;
	return int(w);
}

bool UIObject::GetFontIsItalic(style::Block* styleOverride)
{
	auto s = style::FontStyle::Undefined;
	if (styleOverride)
		s = styleOverride->font_style;
	for (auto* p = this; p && (s == style::FontStyle::Undefined || s == style::FontStyle::Inherit); p = p->parent)
	{
		s = p->GetStyle().GetFontStyle();
	}
	if (s == style::FontStyle::Undefined || s == style::FontStyle::Inherit)
		s = style::FontStyle::Normal;
	return s == style::FontStyle::Italic;
}

ui::Color4b UIObject::GetTextColor(style::Block* styleOverride)
{
	style::Color c;
	if (styleOverride)
		c = styleOverride->text_color;
	for (auto* p = this; p && c.inherit; p = p->parent)
	{
		c = p->GetStyle().GetTextColor();
	}
	if (c.inherit)
		c = ui::Color4b::White();
	return c.color;
}

ui::NativeWindowBase* UIObject::GetNativeWindow() const
{
	return system->nativeWindow;
}


namespace ui {

TextElement::TextElement()
{
	styleProps = Theme::current->text;
}

void TextElement::GetSize(style::Coord& outWidth, style::Coord& outHeight)
{
	int size = int(GetFontSize());
	int weight = GetFontWeight();
	bool italic = GetFontIsItalic();

	auto font = GetFontByFamily(FONT_FAMILY_SANS_SERIF, weight, italic);

	outWidth = ceilf(GetTextWidth(font, size, text));
	outHeight = size;
}

void TextElement::OnPaint()
{
	int size = int(GetFontSize());
	int weight = GetFontWeight();
	bool italic = GetFontIsItalic();
	Color4b color = GetTextColor();

	auto font = GetFontByFamily(FONT_FAMILY_SANS_SERIF, weight, italic);

	styleProps->paint_func(this);
	auto r = GetContentRect();
	float w = r.x1 - r.x0;
	ui::draw::TextLine(font, size, r.x0, r.y1 - (r.y1 - r.y0 - GetFontHeight()) / 2, text, color);
	PaintChildren();
}


struct SubscrTableKey
{
	bool operator == (const SubscrTableKey& o) const
	{
		return tag == o.tag && at == o.at;
	}

	DataCategoryTag* tag;
	uintptr_t at;
};

} // ui

namespace std {

template <>
struct hash<ui::SubscrTableKey>
{
	size_t operator () (const ui::SubscrTableKey& k) const
	{
		return (uintptr_t(k.tag) * 151) ^ k.at;
	}
};

} // std

namespace ui {

struct SubscrTableValue
{
	Subscription* _firstSub = nullptr;
	Subscription* _lastSub = nullptr;
};

using SubscrTable = HashMap<SubscrTableKey, SubscrTableValue*>;
static SubscrTable* g_subscrTable;

void SubscriptionTable_Init()
{
	g_subscrTable = new SubscrTable;
}

void SubscriptionTable_Free()
{
	//assert(g_subscrTable->empty());
	for (auto& p : *g_subscrTable)
		delete p.value;
	delete g_subscrTable;
	g_subscrTable = nullptr;
}

struct Subscription
{
	void Link()
	{
		// append to front because removal starts from that side as well
		prevInNode = nullptr;
		nextInNode = node->_firstSub;
		if (nextInNode)
			nextInNode->prevInNode = this;
		else
			node->_lastSub = this;
		node->_firstSub = this;

		prevInTable = nullptr;
		nextInTable = tableEntry->_firstSub;
		if (nextInTable)
			nextInTable->prevInTable = this;
		else
			tableEntry->_lastSub = this;
		tableEntry->_firstSub = this;
	}
	void Unlink()
	{
		// node
		if (prevInNode)
			prevInNode->nextInNode = nextInNode;
		if (nextInNode)
			nextInNode->prevInNode = prevInNode;
		if (node->_firstSub == this)
			node->_firstSub = nextInNode;
		if (node->_lastSub == this)
			node->_lastSub = prevInNode;

		// table
		if (prevInTable)
			prevInTable->nextInTable = nextInTable;
		if (nextInTable)
			nextInTable->prevInTable = prevInTable;
		if (tableEntry->_firstSub == this)
			tableEntry->_firstSub = nextInTable;
		if (tableEntry->_lastSub == this)
			tableEntry->_lastSub = prevInTable;
	}

	Node* node;
	SubscrTableValue* tableEntry;
	DataCategoryTag* tag;
	uintptr_t at;
	Subscription* prevInNode;
	Subscription* nextInNode;
	Subscription* prevInTable;
	Subscription* nextInTable;
};

static void _Notify(DataCategoryTag* tag, uintptr_t at)
{
	auto it = g_subscrTable->find({ tag, at });
	if (it.is_valid())
	{
		for (auto* s = it->value->_firstSub; s; s = s->nextInTable)
			s->node->OnNotify(tag, at);
	}
}

void Notify(DataCategoryTag* tag, uintptr_t at)
{
	if (at != ANY_ITEM)
		_Notify(tag, at);
	_Notify(tag, ANY_ITEM);
}

Node::~Node()
{
	while (_firstSub)
	{
		auto* s = _firstSub;
		s->Unlink();
		delete s;
	}
	_PerformDestructions();
}

void Node::Rerender()
{
	system->container.AddToRenderStack(this);
	GetNativeWindow()->InvalidateAll();
}

void Node::OnNotify(DataCategoryTag* /*tag*/, uintptr_t /*at*/)
{
	Rerender();
}

bool Node::Subscribe(DataCategoryTag* tag, uintptr_t at)
{
	SubscrTableValue* lst;
	auto it = g_subscrTable->find({ tag, at });
	if (it.is_valid())
	{
		lst = it->value;
		// TODO compare list sizes to decide which is the shorter one to iterate
		for (auto* s = _firstSub; s; s = s->nextInNode)
			if (s->tag == tag && s->at == at)
				return false;
		//for (auto* s = lst->_firstSub; s; s = s->nextInTable)
		//	if (s->node == this)
		//		return false;
	}
	else
		g_subscrTable->insert(SubscrTableKey{ tag, at }, lst = new SubscrTableValue);

	auto* s = new Subscription;
	s->node = this;
	s->tableEntry = lst;
	s->tag = tag;
	s->at = at;
	s->Link();
	return true;
}

bool Node::Unsubscribe(DataCategoryTag* tag, uintptr_t at)
{
	auto it = g_subscrTable->find({ tag, at });
	if (it.is_valid())
		return false;

	// TODO compare list sizes to decide which is the shorter one to iterate
	for (auto* s = _firstSub; s; s = s->nextInNode)
	{
		if (s->tag == tag && s->at == at)
		{
			s->Unlink();
			delete s;
		}
	}
	return true;
}


AddTooltip::AddTooltip(const std::string& s)
{
	_evfn = [s](UIContainer* ctx)
	{
		ctx->Text(s);
	};
}

void AddTooltip::Apply(UIObject* obj) const
{
	auto fn = _evfn;
	obj->HandleEvent(UIEventType::Tooltip) = [fn{ std::move(fn) }](UIEvent& e)
	{
		ui::Tooltip::Set(fn);
		e.StopPropagation();
	};
}

} // ui
