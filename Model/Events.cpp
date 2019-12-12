
#include "Events.h"
#include "Objects.h"
#include "System.h"


UINode* UIEvent::GetTargetNode() const
{
	for (auto* t = target; t; t = t->parent)
		if (auto* n = dynamic_cast<UINode*>(t))
			return n;
	return nullptr;
}

UIMouseButton UIEvent::GetButton() const
{
	return static_cast<UIMouseButton>(shortCode);
}

UIKeyAction UIEvent::GetKeyAction() const
{
	return static_cast<UIKeyAction>(shortCode);
}

uint32_t UIEvent::GetUTF32Char() const
{
	return longCode;
}

bool UIEvent::GetUTF8Text(char out[5]) const
{
	auto utf32char = GetUTF32Char();
	if (utf32char <= 0x7f)
	{
		out[0] = utf32char;
		out[1] = 0;
	}
	else if (utf32char <= 0x7ff)
	{
		out[0] = 0xc0 | (utf32char & 0x1f);
		out[1] = 0x80 | ((utf32char >> 5) & 0x3f);
		out[2] = 0;
	}
	else if (utf32char <= 0xffff)
	{
		out[0] = 0xe0 | (utf32char & 0xf);
		out[1] = 0x80 | ((utf32char >> 4) & 0x3f);
		out[2] = 0x80 | ((utf32char >> 10) & 0x3f);
		out[3] = 0;
	}
	else if (utf32char <= 0xffff)
	{
		out[0] = 0xf0 | (utf32char & 0xe);
		out[1] = 0x80 | ((utf32char >> 3) & 0x3f);
		out[2] = 0x80 | ((utf32char >> 9) & 0x3f);
		out[3] = 0x80 | ((utf32char >> 15) & 0x3f);
		out[4] = 0;
	}
	else
		return false;
	return true;
}


void UIEventSystem::BubblingEvent(UIEvent& e, UIObject* tgt)
{
	UIObject* obj = e.target;
	while (obj != tgt && !e.handled)
	{
		e.current = obj;
		obj->OnEvent(e);
		obj = obj->parent;
	}
}

void UIEventSystem::RecomputeLayout()
{
	if (container->rootNode)
		container->rootNode->OnLayout({ 0, 0, width, height });
}

void UIEventSystem::ProcessTimers(float dt)
{
	size_t endOfInitialTimers = pendingTimers.size();
	for (size_t i = 0; i < endOfInitialTimers; i++)
	{
		auto& T = pendingTimers[i];
		T.timeLeft -= dt;
		if (T.timeLeft <= 0)
		{
			UIEvent ev(this, T.target, UIEventType::Timer);
			size_t sizeBefore = pendingTimers.size();
			T.target->OnEvent(ev);
			bool added = pendingTimers.size() > sizeBefore;

			if (i + 1 < pendingTimers.size())
				std::swap(T, pendingTimers.back());
			pendingTimers.pop_back();
			if (!added)
			{
				i--;
				endOfInitialTimers--;
			}
		}
	}
}

void UIEventSystem::Repaint(UIElement* e)
{
	// TODO
}

void UIEventSystem::OnDestroy(UIObject* o)
{
	if (hoverObj == o)
		hoverObj = nullptr;
	for (size_t i = 0; i < sizeof(clickObj) / sizeof(clickObj[0]); i++)
		if (clickObj[i] == o)
			clickObj[i] = nullptr;
	if (focusObj == o)
		focusObj = nullptr;
	for (size_t i = 0; i < pendingTimers.size(); i++)
	{
		if (pendingTimers[i].target == o)
		{
			if (i + 1 < pendingTimers.size())
				std::swap(pendingTimers[i], pendingTimers.back());
			pendingTimers.pop_back();
			i--;
		}
	}
}

void UIEventSystem::OnCommit(UIElement* e)
{
	UIEvent ev(this, e, UIEventType::Commit);
	BubblingEvent(ev);
}

void UIEventSystem::OnChange(UIElement* e)
{
	UIEvent ev(this, e, UIEventType::Change);
	BubblingEvent(ev);
}

void UIEventSystem::OnChange(UINode* n)
{
	n->Rerender();
	UIEvent ev(this, n, UIEventType::Change);
	BubblingEvent(ev);
}

void UIEventSystem::SetKeyboardFocus(UIObject* o)
{
	if (focusObj == o)
		return;

	if (focusObj)
	{
		UIEvent ev(this, focusObj, UIEventType::LostFocus);
		focusObj->OnEvent(ev);
	}

	focusObj = o;

	if (focusObj)
	{
		UIEvent ev(this, focusObj, UIEventType::GotFocus);
		focusObj->OnEvent(ev);
	}
}

void UIEventSystem::SetTimer(UIElement* tgt, float t, int id)
{
	pendingTimers.push_back({ tgt, t, id });
}

UIObject* UIEventSystem::FindObjectAtPosition(float x, float y)
{
	UIObject* o = container->rootNode;
	if (o && !o->Contains(x, y))
		return nullptr;

	bool found = true;
	while (found)
	{
		found = false;
		for (auto* ch = o->lastChild; ch; ch = ch->prev)
		{
			if (ch->Contains(x, y))
			{
				o = ch;
				found = true;
				break;
			}
		}
	}
	return o;
}

void UIEventSystem::OnMouseMove(UIMouseCoord x, UIMouseCoord y)
{
	UIEvent ev(this, hoverObj, UIEventType::MouseLeave);
	ev.x = x;
	ev.y = y;

	auto* o = hoverObj;
	while (o && !o->Contains(x, y))
	{
		o->flags &= ~UIObject_IsHovered;
		ev.current = o;
		o->OnEvent(ev);
		o = o->parent;
	}

	ev.type = UIEventType::MouseMove;
	for (auto* p = o; p; p = p->parent)
	{
		ev.current = o;
		o->OnEvent(ev);
	}

	ev.type = UIEventType::MouseEnter;
	if (!o && container->rootNode && container->rootNode->Contains(x, y))
	{
		o = container->rootNode;
		o->flags |= UIObject_IsHovered;
		ev.current = o;
		ev.target = o;
		o->OnEvent(ev);
	}

	if (o)
	{
		bool found = true;
		while (found)
		{
			found = false;
			for (auto* ch = o->lastChild; ch; ch = ch->prev)
			{
				if (ch->Contains(x, y))
				{
					o = ch;
					o->flags |= UIObject_IsHovered;
					ev.current = o;
					ev.target = o;
					o->OnEvent(ev);
					found = true;
					break;
				}
			}
		}
	}

	hoverObj = o;
	prevMouseX = x;
	prevMouseY = y;
}

void UIEventSystem::OnMouseButton(bool down, UIMouseButton which, UIMouseCoord x, UIMouseCoord y)
{
	int id = int(which);
	UIEvent ev(this, !down ? clickObj[id] : hoverObj, down ? UIEventType::ButtonDown : UIEventType::ButtonUp);
	bool clicked = !down && clickObj[id] == hoverObj;
	clickObj[id] = down ? hoverObj : nullptr;
	ev.shortCode = id;
	ev.x = x;
	ev.y = y;
	if (down)
	{
		for (auto* p = ev.target; p; p = p->parent)
			p->flags |= _UIObject_IsClicked_First << id;
	}
	else
	{
		for (auto* p = ev.target; p; p = p->parent)
			p->flags &= ~(_UIObject_IsClicked_First << id);
	}
	BubblingEvent(ev);
	if (clicked)
	{
		ev.type = UIEventType::Click;
		BubblingEvent(ev);
		if (which == UIMouseButton::Left)
		{
			ev.type = UIEventType::Activate;
			BubblingEvent(ev);
		}
	}
}

void UIEventSystem::OnKeyInput(bool down, uint32_t vk, uint8_t pk, uint16_t numRepeats)
{
	if (focusObj)
	{
		UIEvent ev(this, focusObj, down ? UIEventType::KeyDown : UIEventType::KeyUp);
		ev.longCode = vk;
		ev.shortCode = pk;
		ev.numRepeats = numRepeats;
	}
}

void UIEventSystem::OnKeyAction(UIKeyAction act, uint16_t numRepeats)
{
	if (focusObj)
	{
		UIEvent ev(this, focusObj, UIEventType::KeyAction);
		ev.shortCode = uint8_t(act);
		ev.numRepeats = numRepeats;
		BubblingEvent(ev);
	}
}

void UIEventSystem::OnTextInput(uint32_t ch, uint16_t numRepeats)
{
	if (focusObj)
	{
		UIEvent ev(this, focusObj, UIEventType::TextInput);
		ev.longCode = ch;
		ev.numRepeats = numRepeats;
		BubblingEvent(ev);
	}
}
