
#include "System.h"


void UIObjectDirtyStack::Add(UIObject* n)
{
	if (n->flags & flag)
		return;
	stack[size++] = n;
	assert(size < 128);
	n->flags |= flag;
}

UIObject* UIObjectDirtyStack::Pop()
{
	assert(size > 0);
	auto* n = stack[--size];
	n->flags &= ~flag;
	return n;
}

void UIObjectDirtyStack::RemoveChildren()
{
	if (size <= 1)
		return;

	for (int i = 0; i < size; i++)
	{
		auto* p = stack[i]->parent;
		while (p)
		{
			if (p->flags & flag)
				break;
			p = p->parent;
		}
		if (p)
		{
			stack[i]->flags &= ~flag;
			if (i + 1 < size)
				stack[i] = stack[size - 1];
			size--;
			i--;
		}
	}
}


void UIContainer::Free()
{
	if (rootNode)
	{
		objectStack[0] = rootNode;
		objectStackSize = 1;
		ProcessObjectDeleteStack();
		rootNode = nullptr;
	}
}

void UIContainer::ProcessObjectDeleteStack(int first)
{
	while (objectStackSize > first)
	{
		auto* cur = objectStack[--objectStackSize];

		for (auto* n = cur->firstChild; n != nullptr; n = n->next)
			objectStack[objectStackSize++] = n;

		printf("    deleting %p\n", cur);
		delete cur;
	}
}

void UIContainer::DeleteObjectsStartingFrom(UIObject* obj)
{
	_Destroy(obj);

	obj->parent->lastChild = obj->prev;
	if (obj->prev)
		obj->prev->next = nullptr;
	else
		obj->parent->firstChild = nullptr;

	int first = objectStackSize;
	for (auto* o = obj; o; o = o->next)
		objectStack[objectStackSize++] = o;
	ProcessObjectDeleteStack(first);
}

void UIContainer::ProcessNodeRenderStack()
{
	if (nodeRenderStack.ContainsAny())
		puts(" ---- processing node RENDER stack ----");

	nodeRenderStack.RemoveChildren();

	while (nodeRenderStack.ContainsAny())
	{
		UINode* currentNode = static_cast<UINode*>(nodeRenderStack.Pop());

		objectStackSize = 0;
		_Push(currentNode);

		printf("rendering %s\n", typeid(*currentNode).name());
		currentNode->Render(this);

		if (objectStackSize > 1)
		{
			printf("WARNING: elements not popped: %d\n", objectStackSize);
			while (objectStackSize > 1)
			{
				Pop();
			}
		}

		_Pop(); // root
	}
	//currentNode = nullptr;
}

void UIContainer::_BuildUsing(UINode* n)
{
	rootNode = n;
	assert(!nodeRenderStack.ContainsAny());
	nodeRenderStack.ClearWithoutFlags(); // TODO: is this needed?
	AddToRenderStack(rootNode);
	ProcessNodeRenderStack();
}

void UIContainer::_Push(UIObject* obj)
{
	objectStack[objectStackSize] = obj;
	objChildStack[objectStackSize] = obj->firstChild;
	objectStackSize++;
}

void UIContainer::_Destroy(UIObject* obj)
{
	obj->OnDestroy();
	for (auto* ch = obj->firstChild; ch; ch = ch->next)
		_Destroy(ch);
}

void UIContainer::_Pop()
{
	objectStack[objectStackSize - 1]->OnCompleteStructure();
	if (objChildStack[objectStackSize - 1])
	{
		// remove leftover children
		DeleteObjectsStartingFrom(objChildStack[objectStackSize - 1]);
	}
	objectStackSize--;
}
