
#pragma once

#include "Platform.h"


namespace ui {

template <class T> UI_FORCEINLINE T min(T a, T b) { return a < b ? a : b; }
template <class T> UI_FORCEINLINE T max(T a, T b) { return a > b ? a : b; }
template <class T> UI_FORCEINLINE T clamp(T x, T vmin, T vmax) { return x < vmin ? vmin : x > vmax ? vmax : x; }

template <class T>
struct TmpEdit
{
	TmpEdit(T& dst, T src) : dest(dst), backup(dst)
	{
		dst = src;
	}
	~TmpEdit()
	{
		dest = backup;
	}
	T& dest;
	T backup;
};

template <class F>
struct DeferImpl
{
	F& func;
	UI_FORCEINLINE ~DeferImpl()
	{
		func();
	}
};
#define UI_DEFER(fn) auto __deferimpl_##__LINE__ = [&](){ fn; }; \
	::ui::DeferImpl<decltype(__deferimpl_##__LINE__)> __defer_##__LINE__ = { __deferimpl_##__LINE__ };

} // ui