// Copied into Schott experimental project from SciTE to bootstrap features then removed unwanted code.
// SciTE - Scintilla based Text Editor
/** @file GUIWin.cxx
 ** Interface to platform GUI facilities.
 ** Split off from Scintilla's Platform.h to avoid SciTE depending on implementation of Scintilla.
 **/
// Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include "pch.h"

#include <time.h>

#include <string>
#include <vector>

#ifdef __MINGW_H
#define _WIN32_IE	0x0400
#endif

#undef _WIN32_WINNT
#define _WIN32_WINNT  0x0500
#ifdef _MSC_VER
// windows.h, et al, use a lot of nameless struct/unions - can't fix it, so allow it
#pragma warning(disable: 4201)
#endif
#include <windows.h>
#ifdef _MSC_VER
// okay, that's done, don't allow it in our code
#pragma warning(default: 4201)
#pragma warning(disable: 4244)
#endif

#include "Scintilla.h"
#include "GUI.h"

namespace GUI {

enum { SURROGATE_LEAD_FIRST = 0xD800 };
enum { SURROGATE_TRAIL_FIRST = 0xDC00 };
enum { SURROGATE_TRAIL_LAST = 0xDFFF };

static unsigned int UTF8Length(const wchar_t *uptr, size_t tlen) {
	unsigned int len = 0;
	for (size_t i = 0; i < tlen && uptr[i];) {
		unsigned int uch = uptr[i];
		if (uch < 0x80) {
			len++;
		} else if (uch < 0x800) {
			len += 2;
		} else if ((uch >= SURROGATE_LEAD_FIRST) &&
			(uch <= SURROGATE_TRAIL_LAST)) {
			len += 4;
			i++;
		} else {
			len += 3;
		}
		i++;
	}
	return len;
}

static void UTF8FromUTF16(const wchar_t *uptr, size_t tlen, char *putf, size_t len) {
	int k = 0;
	for (size_t i = 0; i < tlen && uptr[i];) {
		unsigned int uch = uptr[i];
		if (uch < 0x80) {
			putf[k++] = static_cast<char>(uch);
		} else if (uch < 0x800) {
			putf[k++] = static_cast<char>(0xC0 | (uch >> 6));
			putf[k++] = static_cast<char>(0x80 | (uch & 0x3f));
		} else if ((uch >= SURROGATE_LEAD_FIRST) &&
			(uch <= SURROGATE_TRAIL_LAST)) {
			// Half a surrogate pair
			i++;
			unsigned int xch = 0x10000 + ((uch & 0x3ff) << 10) + (uptr[i] & 0x3ff);
			putf[k++] = static_cast<char>(0xF0 | (xch >> 18));
			putf[k++] = static_cast<char>(0x80 | ((xch >> 12) & 0x3f));
			putf[k++] = static_cast<char>(0x80 | ((xch >> 6) & 0x3f));
			putf[k++] = static_cast<char>(0x80 | (xch & 0x3f));
		} else {
			putf[k++] = static_cast<char>(0xE0 | (uch >> 12));
			putf[k++] = static_cast<char>(0x80 | ((uch >> 6) & 0x3f));
			putf[k++] = static_cast<char>(0x80 | (uch & 0x3f));
		}
		i++;
	}
	putf[len] = '\0';
}

static size_t UTF16Length(const char *s, unsigned int len) {
	size_t ulen = 0;
	size_t charLen;
	for (size_t i=0; i<len;) {
		unsigned char ch = static_cast<unsigned char>(s[i]);
		if (ch < 0x80) {
			charLen = 1;
		} else if (ch < 0x80 + 0x40 + 0x20) {
			charLen = 2;
		} else if (ch < 0x80 + 0x40 + 0x20 + 0x10) {
			charLen = 3;
		} else {
			charLen = 4;
			ulen++;
		}
		i += charLen;
		ulen++;
	}
	return ulen;
}

static size_t UTF16FromUTF8(const char *s, size_t len, gui_char *tbuf, size_t tlen) {
	size_t ui=0;
	const unsigned char *us = reinterpret_cast<const unsigned char *>(s);
	size_t i=0;
	while ((i<len) && (ui<tlen)) {
		unsigned char ch = us[i++];
		if (ch < 0x80) {
			tbuf[ui] = ch;
		} else if (ch < 0x80 + 0x40 + 0x20) {
			tbuf[ui] = static_cast<wchar_t>((ch & 0x1F) << 6);
			ch = us[i++];
			tbuf[ui] = static_cast<wchar_t>(tbuf[ui] + (ch & 0x7F));
		} else if (ch < 0x80 + 0x40 + 0x20 + 0x10) {
			tbuf[ui] = static_cast<wchar_t>((ch & 0xF) << 12);
			ch = us[i++];
			tbuf[ui] = static_cast<wchar_t>(tbuf[ui] + ((ch & 0x7F) << 6));
			ch = us[i++];
			tbuf[ui] = static_cast<wchar_t>(tbuf[ui] + (ch & 0x7F));
		} else {
			// Outside the BMP so need two surrogates
			int val = (ch & 0x7) << 18;
			ch = us[i++];
			val += (ch & 0x3F) << 12;
			ch = us[i++];
			val += (ch & 0x3F) << 6;
			ch = us[i++];
			val += (ch & 0x3F);
			tbuf[ui] = static_cast<wchar_t>(((val - 0x10000) >> 10) + SURROGATE_LEAD_FIRST);
			ui++;
			tbuf[ui] = static_cast<wchar_t>((val & 0x3ff) + SURROGATE_TRAIL_FIRST);
		}
		ui++;
	}
	return ui;
}

gui_string StringFromUTF8(const char *s) {
	size_t sLen = s ? strlen(s) : 0;
	size_t wideLen = UTF16Length(s, static_cast<int>(sLen));
	std::vector<gui_char> vgc(wideLen + 1);
	size_t outLen = UTF16FromUTF8(s, sLen, &vgc[0], wideLen);
	vgc[outLen] = 0;
	return gui_string(&vgc[0], outLen);
}

std::string UTF8FromString(const gui_string &s) {
	size_t sLen = s.size();
	size_t narrowLen = UTF8Length(s.c_str(), sLen);
	std::vector<char> vc(narrowLen + 1);
	UTF8FromUTF16(s.c_str(), sLen, &vc[0], narrowLen);
	return std::string(&vc[0], narrowLen);
}

gui_string StringFromInteger(int i) {
	gui_char number[32];
#if defined(_MSC_VER) && (_MSC_VER > 1310)
	swprintf(number, 30, L"%0d", i);
#else
	swprintf(number, L"%0d", i);
#endif
	return gui_string(number);
}

}
