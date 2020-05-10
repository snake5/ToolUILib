
#pragma once
#include <assert.h>
#include <string.h>

#include "Math.h"


static inline bool IsSpace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
static inline bool IsAlpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
static inline bool IsDigit(char c) { return c >= '0' && c <= '9'; }
static inline bool IsAlphaNum(char c) { return IsAlpha(c) || IsDigit(c); }


struct StringView
{
	StringView() : _data(nullptr), _size(0) {}
	StringView(const char* s) : _data(s), _size(strlen(s)) {}
	StringView(const char* ptr, size_t sz) : _data(ptr), _size(sz) {}
	template <class T>
	StringView(const T& o) : _data(o.data()), _size(o.size()) {}

	const char* data() const { return _data; }
	size_t size() const { return _size; }
	bool empty() const { return _size == 0; }
	const char& first() const { return _data[0]; }
	const char& last() const { return _data[_size - 1]; }
	const char* begin() const { return _data; }
	const char* end() const { return _data + _size; }
	bool operator == (const StringView& o) const { return _size == o._size && memcmp(_data, o._data, _size) == 0; }
	bool operator != (const StringView& o) const { return !(*this == o); }

	StringView substr(size_t at, size_t size = SIZE_MAX) const
	{
		assert(at <= _size);
		return StringView(_data + at, std::min(_size - at, size));
	}
	StringView ltrim() const
	{
		StringView out = *this;
		while (out._size && IsSpace(*out._data))
		{
			out._data++;
			out._size--;
		}
		return out;
	}
	StringView rtrim() const
	{
		StringView out = *this;
		while (out._size && IsSpace(out._data[out._size - 1]))
		{
			out._size--;
		}
		return out;
	}
	StringView trim() const
	{
		return ltrim().rtrim();
	}

	bool starts_with(StringView sub) const
	{
		return _size >= sub._size && memcmp(_data, sub._data, sub._size) == 0;
	}

	int count(StringView sub, size_t maxpos = SIZE_MAX) const
	{
		int out = 0;
		auto at = find_first_at(sub);
		while (at < maxpos)
		{
			out++;
			at = find_first_at(sub, at + 1);
		}
		return out;
	}
	size_t find_first_at(StringView sub, size_t from = 0, size_t def = SIZE_MAX) const
	{
		for (size_t i = from; i <= _size - sub._size; i++)
			if (memcmp(&_data[i], sub._data, sub._size) == 0)
				return i;
		return def;
	}
	size_t find_last_at(StringView sub, size_t from = SIZE_MAX, size_t def = SIZE_MAX) const
	{
		for (size_t i = std::min(_size - sub._size - 1, from); i < _size; i--)
			if (memcmp(&_data[i], sub._data, sub._size) == 0)
				return i;
		return def;
	}

	StringView until_first(StringView sub) const
	{
		size_t at = find_first_at(sub);
		return at < SIZE_MAX ? substr(0, at) : StringView();
	}
	StringView after_first(StringView sub) const
	{
		size_t at = find_first_at(sub);
		return at < SIZE_MAX ? substr(at + sub._size) : StringView();
	}

	void skip_c_whitespace(bool single_line_comments = true, bool multiline_comments = true)
	{
		bool found = true;
		while (found)
		{
			found = false;
			*this = ltrim();
			if (single_line_comments && starts_with("//"))
			{
				found = true;
				*this = after_first("\n");
			}
			if (multiline_comments && starts_with("/*"))
			{
				found = true;
				*this = after_first("*/");
			}
		}
	}
	template <class F>
	bool first_char_is(F f)
	{
		return _size != 0 && !!f(_data[0]);
	}
	char take_char()
	{
		assert(_size);
		_size--;
		return *_data++;
	}
	bool take_if_equal(StringView s)
	{
		if (starts_with(s))
		{
			_data += s._size;
			_size -= s._size;
			return true;
		}
		return false;
	}
	template <class F>
	StringView take_while(F f)
	{
		const char* start = _data;
		while (_size && f(_data[0]))
		{
			_data++;
			_size--;
		}
		return StringView(start, _data - start);
	}

	const char* _data;
	size_t _size;
};

namespace std {
template <>
struct hash<StringView>
{
	size_t operator () (const StringView& v) const
	{
		uint64_t hash = 0xcbf29ce484222325;
		for (char c : v)
			hash = (hash ^ c) * 1099511628211;
		return size_t(hash);
	}
};
}
