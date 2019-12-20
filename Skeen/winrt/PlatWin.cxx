// Scintilla source code edit control
/** @file PlatWin.cxx
 ** Implementation of XPlatform facilities on Windows.
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include "pch.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <limits.h>
#include <math.h>

#include <vector>
#include <map>

#undef _WIN32_WINNT
#define _WIN32_WINNT  0x0500
#include <windows.h>
//#include <commctrl.h>
//#include <richedit.h>
//#include <windowsx.h>

#define USE_D2D 1

#if defined(USE_D2D)
#include <d2d1.h>
#include <dwrite.h>
#endif

#include "Platform.h"
#include "UniConversion.h"
//#include "XPM.h"
#include "FontQuality.h"

#include "FrameworkElementWrapper.h"

//static CRITICAL_SECTION crPlatformLock;

#ifdef SCI_NAMESPACE
//using namespace Scintilla;
using Scintilla::XYPOSITION;
using Scintilla::Point;
using Scintilla::PRectangle;
using Scintilla::ColourDesired;
using Scintilla::Surface;
using Scintilla::Window;
using Scintilla::ListBox;
using Scintilla::Menu;
using Scintilla::ElapsedTime;
using Scintilla::DynamicLibrary;
using Scintilla::FontParameters;
using Scintilla::Font;
using Scintilla::FontID;
using Scintilla::SurfaceID;
using Scintilla::WindowID;
#endif

Point Point::FromLong(long lpoint) {
	return Point(static_cast<short>(LOWORD(lpoint)), static_cast<short>(HIWORD(lpoint)));
}

static RECT RectFromPRectangle(PRectangle prc) {
	RECT rc = {static_cast<LONG>(prc.left), static_cast<LONG>(prc.top),
		static_cast<LONG>(prc.right), static_cast<LONG>(prc.bottom)};
	return rc;
}

IDWriteFactory *pIDWriteFactory = 0;
ID2D1Factory1 *pD2DFactory = 0;
#if defined(USE_D2D)

bool LoadD2D() {
	static bool triedLoadingD2D = false;
	static HMODULE hDLLD2D = 0;
	static HMODULE hDLLDWrite = 0;
	if (!triedLoadingD2D) {

		DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(IDWriteFactory),
			reinterpret_cast<IUnknown**>(&pIDWriteFactory));

	    D2D1_FACTORY_OPTIONS options;
		ZeroMemory(&options, sizeof(D2D1_FACTORY_OPTIONS));
		D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
			__uuidof(ID2D1Factory1),
			&options,
			reinterpret_cast<void**>(&pD2DFactory));

		/*
		typedef HRESULT (WINAPI *D2D1CFSig)(D2D1_FACTORY_TYPE factoryType, REFIID riid,
			CONST D2D1_FACTORY_OPTIONS *pFactoryOptions, IUnknown **factory);
		typedef HRESULT (WINAPI *DWriteCFSig)(DWRITE_FACTORY_TYPE factoryType, REFIID iid,
			IUnknown **factory);

		hDLLD2D = ::LoadLibrary(TEXT("D2D1.DLL"));
		if (hDLLD2D) {
			D2D1CFSig fnD2DCF = (D2D1CFSig)::GetProcAddress(hDLLD2D, "D2D1CreateFactory");
			if (fnD2DCF) {
				// A single threaded factory as Scintilla always draw on the GUI thread
				fnD2DCF(D2D1_FACTORY_TYPE_SINGLE_THREADED,
					__uuidof(ID2D1Factory),
					0,
					reinterpret_cast<IUnknown**>(&pD2DFactory));
			}
		}
		hDLLDWrite = ::LoadLibrary(TEXT("DWRITE.DLL"));
		if (hDLLDWrite) {
			DWriteCFSig fnDWCF = (DWriteCFSig)::GetProcAddress(hDLLDWrite, "DWriteCreateFactory");
			if (fnDWCF) {
				fnDWCF(DWRITE_FACTORY_TYPE_SHARED,
					__uuidof(IDWriteFactory),
					reinterpret_cast<IUnknown**>(&pIDWriteFactory));
			}
		}
		*/
	}
	triedLoadingD2D = true;
	return pIDWriteFactory && pD2DFactory;
}
#endif

struct FormatAndMetrics {
	int technology;
#if defined(USE_D2D)
	IDWriteTextFormat *pTextFormat;
#endif
	int extraFontFlag;
	FLOAT yAscent;
	FLOAT yDescent;
	FLOAT yInternalLeading;
	FormatAndMetrics(HFONT hfont_, int extraFontFlag_) : 
		technology(SCWIN_TECH_DIRECTWRITE),
#if defined(USE_D2D)
		pTextFormat(0),
#endif
		extraFontFlag(extraFontFlag_), yAscent(2), yDescent(1), yInternalLeading(0) {
	}
#if defined(USE_D2D)
	FormatAndMetrics(IDWriteTextFormat *pTextFormat_, int extraFontFlag_, FLOAT yAscent_, FLOAT yDescent_, FLOAT yInternalLeading_) : 
		technology(SCWIN_TECH_DIRECTWRITE), pTextFormat(pTextFormat_), extraFontFlag(extraFontFlag_), yAscent(yAscent_), yDescent(yDescent_), yInternalLeading(yInternalLeading_) {
	}
#endif
	~FormatAndMetrics() {
#if defined(USE_D2D)
		if (pTextFormat)
			pTextFormat->Release();
		pTextFormat = 0;
#endif
		extraFontFlag = 0;
		yAscent = 2;
		yDescent = 1;
		yInternalLeading = 0;
	}
};

#ifndef CLEARTYPE_QUALITY
#define CLEARTYPE_QUALITY 5
#endif

static BYTE Win32MapFontQuality(int extraFontFlag) {
	switch (extraFontFlag & SC_EFF_QUALITY_MASK) {

		case SC_EFF_QUALITY_NON_ANTIALIASED:
			return NONANTIALIASED_QUALITY;

		case SC_EFF_QUALITY_ANTIALIASED:
			return ANTIALIASED_QUALITY;

		case SC_EFF_QUALITY_LCD_OPTIMIZED:
			return CLEARTYPE_QUALITY;

		default:
			return SC_EFF_QUALITY_DEFAULT;
	}
}

#if defined(USE_D2D)
static D2D1_TEXT_ANTIALIAS_MODE DWriteMapFontQuality(int extraFontFlag) {
	switch (extraFontFlag & SC_EFF_QUALITY_MASK) {

		case SC_EFF_QUALITY_NON_ANTIALIASED:
			return D2D1_TEXT_ANTIALIAS_MODE_ALIASED;

		case SC_EFF_QUALITY_ANTIALIASED:
			return D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE;

		case SC_EFF_QUALITY_LCD_OPTIMIZED:
			return D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE;

		default:
			return D2D1_TEXT_ANTIALIAS_MODE_DEFAULT;
	}
}
#endif

static void SetLogFont(LOGFONTA &lf, const char *faceName, int characterSet, float size, int weight, bool italic, int extraFontFlag) {
	memset(&lf, 0, sizeof(lf));
	// The negative is to allow for leading
	lf.lfHeight = -(abs(static_cast<int>(size + 0.5)));
	lf.lfWeight = weight;
	lf.lfItalic = static_cast<BYTE>(italic ? 1 : 0);
	lf.lfCharSet = static_cast<BYTE>(characterSet);
	lf.lfQuality = Win32MapFontQuality(extraFontFlag);
	memcpy(lf.lfFaceName, faceName, strlen(faceName) + 1);
}

/**
 * Create a hash from the parameters for a font to allow easy checking for identity.
 * If one font is the same as another, its hash will be the same, but if the hash is the
 * same then they may still be different.
 */
static int HashFont(const FontParameters &fp) {
	return
		static_cast<int>(fp.size) ^
		(fp.characterSet << 10) ^
		((fp.extraFontFlag & SC_EFF_QUALITY_MASK) << 9) ^
		((fp.weight/100) << 12) ^
		(fp.italic ? 0x20000000 : 0) ^
		(fp.technology << 15) ^
		fp.faceName[0];
}

class FontCached : Font {
	FontCached *next;
	int usage;
	float size;
	LOGFONTA lf;
	int technology;
	int hash;
	FontCached(const FontParameters &fp);
	~FontCached() {}
	bool SameAs(const FontParameters &fp);
	virtual void Release();

	static FontCached *first;
public:
	static FontID FindOrCreate(const FontParameters &fp);
	static void ReleaseId(FontID fid_);
};

FontCached *FontCached::first = 0;

FontCached::FontCached(const FontParameters &fp) :
	next(0), usage(0), size(1.0), hash(0) {
	SetLogFont(lf, fp.faceName, fp.characterSet, fp.size, fp.weight, fp.italic, fp.extraFontFlag);
	technology = fp.technology;
	hash = HashFont(fp);
	fid = 0;
	IDWriteTextFormat *pTextFormat;
	const int faceSize = 200;
	WCHAR wszFace[faceSize];
	UTF16FromUTF8(fp.faceName, static_cast<unsigned int>(strlen(fp.faceName))+1, wszFace, faceSize);
	FLOAT fHeight = fp.size;
	DWRITE_FONT_STYLE style = fp.italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;
	HRESULT hr = pIDWriteFactory->CreateTextFormat(wszFace, NULL,
		static_cast<DWRITE_FONT_WEIGHT>(fp.weight),
		style,
		DWRITE_FONT_STRETCH_NORMAL, fHeight, L"en-us", &pTextFormat);
	if (SUCCEEDED(hr)) {
		pTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

		const int maxLines = 2;
		DWRITE_LINE_METRICS lineMetrics[maxLines];
		UINT32 lineCount = 0;
		FLOAT yAscent = 1.0f;
		FLOAT yDescent = 1.0f;
		FLOAT yInternalLeading = 0.0f;
		IDWriteTextLayout *pTextLayout = 0;
		hr = pIDWriteFactory->CreateTextLayout(L"X", 1, pTextFormat,
				100.0f, 100.0f, &pTextLayout);
		if (SUCCEEDED(hr)) {
			hr = pTextLayout->GetLineMetrics(lineMetrics, maxLines, &lineCount);
			if (SUCCEEDED(hr)) {
				yAscent = lineMetrics[0].baseline;
				yDescent = lineMetrics[0].height - lineMetrics[0].baseline;

				FLOAT emHeight;
				hr = pTextLayout->GetFontSize(0, &emHeight);
				if (SUCCEEDED(hr)) {
					yInternalLeading = lineMetrics[0].height - emHeight;
				}
			}
			pTextLayout->Release();
		}
		fid = reinterpret_cast<void *>(new FormatAndMetrics(pTextFormat, fp.extraFontFlag, yAscent, yDescent, yInternalLeading));
	}
	usage = 1;
}

bool FontCached::SameAs(const FontParameters &fp) {
	return
		(size == fp.size) &&
		(lf.lfWeight == fp.weight) &&
		(lf.lfItalic == static_cast<BYTE>(fp.italic ? 1 : 0)) &&
		(lf.lfCharSet == fp.characterSet) &&
		(lf.lfQuality == Win32MapFontQuality(fp.extraFontFlag)) &&
		(technology == fp.technology) &&
		0 == strcmp(lf.lfFaceName,fp.faceName);
}

void FontCached::Release() {
	delete reinterpret_cast<FormatAndMetrics *>(fid);
	fid = 0;
}

FontID FontCached::FindOrCreate(const FontParameters &fp) {
	FontID ret = 0;
	//::EnterCriticalSection(&crPlatformLock);
	int hashFind = HashFont(fp);
	for (FontCached *cur=first; cur; cur=cur->next) {
		if ((cur->hash == hashFind) &&
			cur->SameAs(fp)) {
			cur->usage++;
			ret = cur->fid;
		}
	}
	if (ret == 0) {
		FontCached *fc = new FontCached(fp);
		if (fc) {
			fc->next = first;
			first = fc;
			ret = fc->fid;
		}
	}
	//::LeaveCriticalSection(&crPlatformLock);
	return ret;
}

void FontCached::ReleaseId(FontID fid_) {
	//::EnterCriticalSection(&crPlatformLock);
	FontCached **pcur=&first;
	for (FontCached *cur=first; cur; cur=cur->next) {
		if (cur->fid == fid_) {
			cur->usage--;
			if (cur->usage == 0) {
				*pcur = cur->next;
				cur->Release();
				cur->next = 0;
				delete cur;
			}
			break;
		}
		pcur=&cur->next;
	}
	//::LeaveCriticalSection(&crPlatformLock);
}

Font::Font() {
	fid = 0;
}

Font::~Font() {
}

#define FONTS_CACHED

void Font::Create(const FontParameters &fp) {
	Release();
	if (fp.faceName)
		fid = FontCached::FindOrCreate(fp);
}

void Font::Release() {
	if (fid)
		FontCached::ReleaseId(fid);
	fid = 0;
}

// Buffer to hold strings and string position arrays without always allocating on heap.
// May sometimes have string too long to allocate on stack. So use a fixed stack-allocated buffer
// when less than safe size otherwise allocate on heap and free automatically.
template<typename T, int lengthStandard>
class VarBuffer {
	T bufferStandard[lengthStandard];
public:
	T *buffer;
	VarBuffer(size_t length) : buffer(0) {
		if (length > lengthStandard) {
			buffer = new T[length];
		} else {
			buffer = bufferStandard;
		}
	}
	~VarBuffer() {
		if (buffer != bufferStandard) {
			delete []buffer;
			buffer = 0;
		}
	}
};

const int stackBufferLength = 10000;
class TextWide : public VarBuffer<wchar_t, stackBufferLength> {
public:
	int tlen;
	TextWide(const char *s, int len, bool unicodeMode, int codePage=0) :
		VarBuffer<wchar_t, stackBufferLength>(len) {
		if (unicodeMode) {
			tlen = UTF16FromUTF8(s, len, buffer, len);
		} else {
			// Support Asian string display in 9x English
			tlen = ::MultiByteToWideChar(codePage, 0, s, len, buffer, len);
		}
	}
};
typedef VarBuffer<XYPOSITION, stackBufferLength> TextPositions;

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

class SurfaceD2D : public Surface {
	bool unicodeMode;
	int x, y;

	int codePage;

	ID2D1RenderTarget *pRenderTarget;
	bool ownRenderTarget;
	int clipsActive;

	IDWriteTextFormat *pTextFormat;
	FLOAT yAscent;
	FLOAT yDescent;
	FLOAT yInternalLeading;

	ID2D1SolidColorBrush *pBrush;

	int logPixelsY;
	float dpiScaleX;
	float dpiScaleY;

	void SetFont(Font &font_);

	// Private so SurfaceD2D objects can not be copied
	SurfaceD2D(const SurfaceD2D &);
	SurfaceD2D &operator=(const SurfaceD2D &);
public:
	SurfaceD2D();
	virtual ~SurfaceD2D();

	void SetScale();
	void Init(WindowID wid);
	void Init(SurfaceID sid, WindowID wid);
	void InitPixMap(int width, int height, Surface *surface_, WindowID wid);

	void Release();
	bool Initialised();

	HRESULT FlushDrawing();

	void PenColour(ColourDesired fore);
	void D2DPenColour(ColourDesired fore, int alpha=255);
	int LogPixelsY();
	int DeviceHeightFont(int points);
	void MoveTo(int x_, int y_);
	void LineTo(int x_, int y_);
	void Polygon(Point *pts, int npts, ColourDesired fore, ColourDesired back);
	void RectangleDraw(PRectangle rc, ColourDesired fore, ColourDesired back);
	void FillRectangle(PRectangle rc, ColourDesired back);
	void FillRectangle(PRectangle rc, Surface &surfacePattern);
	void RoundedRectangle(PRectangle rc, ColourDesired fore, ColourDesired back);
	void AlphaRectangle(PRectangle rc, int cornerSize, ColourDesired fill, int alphaFill,
		ColourDesired outline, int alphaOutline, int flags);
	void DrawRGBAImage(PRectangle rc, int width, int height, const unsigned char *pixelsImage);
	void Ellipse(PRectangle rc, ColourDesired fore, ColourDesired back);
	void Copy(PRectangle rc, Point from, Surface &surfaceSource);

	void DrawTextCommon(PRectangle rc, Font &font_, XYPOSITION ybase, const char *s, int len, UINT fuOptions);
	void DrawTextNoClip(PRectangle rc, Font &font_, XYPOSITION ybase, const char *s, int len, ColourDesired fore, ColourDesired back);
	void DrawTextClipped(PRectangle rc, Font &font_, XYPOSITION ybase, const char *s, int len, ColourDesired fore, ColourDesired back);
	void DrawTextTransparent(PRectangle rc, Font &font_, XYPOSITION ybase, const char *s, int len, ColourDesired fore);
	void MeasureWidths(Font &font_, const char *s, int len, XYPOSITION *positions);
	XYPOSITION WidthText(Font &font_, const char *s, int len);
	XYPOSITION WidthChar(Font &font_, char ch);
	XYPOSITION Ascent(Font &font_);
	XYPOSITION Descent(Font &font_);
	XYPOSITION InternalLeading(Font &font_);
	XYPOSITION ExternalLeading(Font &font_);
	XYPOSITION Height(Font &font_);
	XYPOSITION AverageCharWidth(Font &font_);

	void SetClip(PRectangle rc);
	void FlushCachedState();

	void SetUnicodeMode(bool unicodeMode_);
	void SetDBCSMode(int codePage_);
};

#ifdef SCI_NAMESPACE
} //namespace Scintilla
#endif

using Scintilla::SurfaceD2D;

SurfaceD2D::SurfaceD2D() :
	unicodeMode(false),
	x(0), y(0) {

	codePage = 0;

	pRenderTarget = NULL;
	ownRenderTarget = false;
	clipsActive = 0;

	// From selected font
	pTextFormat = NULL;
	yAscent = 2;
	yDescent = 1;
	yInternalLeading = 0;

	pBrush = NULL;

	logPixelsY = 72;
	dpiScaleX = 1.0;
	dpiScaleY = 1.0;
}

SurfaceD2D::~SurfaceD2D() {
	Release();
}

void SurfaceD2D::Release() {
	if (pBrush) {
		pBrush->Release();
		pBrush = 0;
	}
	if (pRenderTarget) {
		while (clipsActive) {
			pRenderTarget->PopAxisAlignedClip();
			clipsActive--;
		}
		if (ownRenderTarget) {
			pRenderTarget->Release();
		}
		pRenderTarget = 0;
	}
}

void SurfaceD2D::SetScale() {
	dpiScaleX = 1.0;
	dpiScaleY = 1.0;
}

bool SurfaceD2D::Initialised() {
	return pRenderTarget != 0;
}

HRESULT SurfaceD2D::FlushDrawing() {
	return pRenderTarget->Flush();
}

void SurfaceD2D::Init(WindowID /* wid */) {
	Release();
	SetScale();
}

void SurfaceD2D::Init(SurfaceID sid, WindowID) {
	Release();
	SetScale();
	pRenderTarget = reinterpret_cast<ID2D1RenderTarget *>(sid);
}

void SurfaceD2D::InitPixMap(int width, int height, Surface *surface_, WindowID) {
	Release();
	SetScale();
	SurfaceD2D *psurfOther = static_cast<SurfaceD2D *>(surface_);
	ID2D1BitmapRenderTarget *pCompatibleRenderTarget = NULL;
	D2D1_SIZE_F desiredSize = D2D1::SizeF(width, height);
	D2D1_PIXEL_FORMAT desiredFormat = psurfOther->pRenderTarget->GetPixelFormat();
	desiredFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
	HRESULT hr = psurfOther->pRenderTarget->CreateCompatibleRenderTarget(
		&desiredSize, NULL, &desiredFormat, D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE, &pCompatibleRenderTarget);
	if (SUCCEEDED(hr)) {
		pRenderTarget = pCompatibleRenderTarget;
		pRenderTarget->BeginDraw();
		ownRenderTarget = true;
	}
}

void SurfaceD2D::PenColour(ColourDesired fore) {
	D2DPenColour(fore);
}

void SurfaceD2D::D2DPenColour(ColourDesired fore, int alpha) {
	if (pRenderTarget) {
		D2D_COLOR_F col;
		col.r = (fore.AsLong() & 0xff) / 255.0;
		col.g = ((fore.AsLong() & 0xff00) >> 8) / 255.0;
		col.b = (fore.AsLong() >> 16) / 255.0;
		col.a = alpha / 255.0;
		if (pBrush) {
			pBrush->SetColor(col);
		} else {
			HRESULT hr = pRenderTarget->CreateSolidColorBrush(col, &pBrush);
			if (!SUCCEEDED(hr) && pBrush) {						
				pBrush->Release();
				pBrush = 0;
			}
		}
	}
}

void SurfaceD2D::SetFont(Font &font_) {
	FormatAndMetrics *pfm = reinterpret_cast<FormatAndMetrics *>(font_.GetID());
	PLATFORM_ASSERT(pfm->technology == SCWIN_TECH_DIRECTWRITE);
	pTextFormat = pfm->pTextFormat;
	yAscent = pfm->yAscent;
	yDescent = pfm->yDescent;
	yInternalLeading = pfm->yInternalLeading;
	if (pRenderTarget) {
		pRenderTarget->SetTextAntialiasMode(DWriteMapFontQuality(pfm->extraFontFlag));
	}
}

int SurfaceD2D::LogPixelsY() {
	return logPixelsY;
}

int SurfaceD2D::DeviceHeightFont(int points) {
	return ::MulDiv(points, LogPixelsY(), 72);
}

void SurfaceD2D::MoveTo(int x_, int y_) {
	x = x_;
	y = y_;
}

static int Delta(int difference) {
	if (difference < 0)
		return -1;
	else if (difference > 0)
		return 1;
	else
		return 0;
}

static int RoundFloat(float f) {
	return int(f+0.5);
}

void SurfaceD2D::LineTo(int x_, int y_) {
	if (pRenderTarget) {
		int xDiff = x_ - x;
		int xDelta = Delta(xDiff);
		int yDiff = y_ - y;
		int yDelta = Delta(yDiff);
		if ((xDiff == 0) || (yDiff == 0)) {
			// Horizontal or vertical lines can be more precisely drawn as a filled rectangle
			int xEnd = x_ - xDelta;
			int left = Platform::Minimum(x, xEnd);
			int width = abs(x - xEnd) + 1;
			int yEnd = y_ - yDelta;
			int top = Platform::Minimum(y, yEnd);
			int height = abs(y - yEnd) + 1;
			D2D1_RECT_F rectangle1 = D2D1::RectF(left, top, left+width, top+height);
			pRenderTarget->FillRectangle(&rectangle1, pBrush);
		} else if ((abs(xDiff) == abs(yDiff))) {
			// 45 degree slope
			pRenderTarget->DrawLine(D2D1::Point2F(x + 0.5, y + 0.5), 
				D2D1::Point2F(x_ + 0.5 - xDelta, y_ + 0.5 - yDelta), pBrush);
		} else {
			// Line has a different slope so difficult to avoid last pixel
			pRenderTarget->DrawLine(D2D1::Point2F(x + 0.5, y + 0.5), 
				D2D1::Point2F(x_ + 0.5, y_ + 0.5), pBrush);
		}
		x = x_;
		y = y_;
	}
}

void SurfaceD2D::Polygon(Point *pts, int npts, ColourDesired fore, ColourDesired back) {
	if (pRenderTarget) {
		ID2D1Factory *pFactory = 0;
		pRenderTarget->GetFactory(&pFactory);
		ID2D1PathGeometry *geometry=0;
		HRESULT hr = pFactory->CreatePathGeometry(&geometry);
		if (SUCCEEDED(hr)) {
			ID2D1GeometrySink *sink = 0;
			hr = geometry->Open(&sink);
			if (SUCCEEDED(hr)) {
				sink->BeginFigure(D2D1::Point2F(pts[0].x + 0.5f, pts[0].y + 0.5f), D2D1_FIGURE_BEGIN_FILLED);
				for (size_t i=1; i<static_cast<size_t>(npts); i++) {
					sink->AddLine(D2D1::Point2F(pts[i].x + 0.5f, pts[i].y + 0.5f));
				}
				sink->EndFigure(D2D1_FIGURE_END_CLOSED);
				sink->Close();
				sink->Release();

				D2DPenColour(back);
				pRenderTarget->FillGeometry(geometry,pBrush);
				D2DPenColour(fore);
				pRenderTarget->DrawGeometry(geometry,pBrush);
			}

			geometry->Release();
		}
	}
}

void SurfaceD2D::RectangleDraw(PRectangle rc, ColourDesired fore, ColourDesired back) {
	if (pRenderTarget) {
		D2D1_RECT_F rectangle1 = D2D1::RectF(RoundFloat(rc.left) + 0.5, rc.top+0.5, RoundFloat(rc.right) - 0.5, rc.bottom-0.5);
		D2DPenColour(back);
		pRenderTarget->FillRectangle(&rectangle1, pBrush);
		D2DPenColour(fore);
		pRenderTarget->DrawRectangle(&rectangle1, pBrush);
	}
}

void SurfaceD2D::FillRectangle(PRectangle rc, ColourDesired back) {
	if (pRenderTarget) {
		D2DPenColour(back);
        D2D1_RECT_F rectangle1 = D2D1::RectF(RoundFloat(rc.left), rc.top, RoundFloat(rc.right), rc.bottom);
        pRenderTarget->FillRectangle(&rectangle1, pBrush);
	}
}

void SurfaceD2D::FillRectangle(PRectangle rc, Surface &surfacePattern) {
	SurfaceD2D &surfOther = static_cast<SurfaceD2D &>(surfacePattern);
	surfOther.FlushDrawing();
	ID2D1Bitmap *pBitmap = NULL;
	ID2D1BitmapRenderTarget *pCompatibleRenderTarget = reinterpret_cast<ID2D1BitmapRenderTarget *>(
		surfOther.pRenderTarget);
	HRESULT hr = pCompatibleRenderTarget->GetBitmap(&pBitmap);
	if (SUCCEEDED(hr)) {
		ID2D1BitmapBrush *pBitmapBrush = NULL;
		D2D1_BITMAP_BRUSH_PROPERTIES brushProperties =
	        D2D1::BitmapBrushProperties(D2D1_EXTEND_MODE_WRAP, D2D1_EXTEND_MODE_WRAP,
			D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
		// Create the bitmap brush.
		hr = pRenderTarget->CreateBitmapBrush(pBitmap, brushProperties, &pBitmapBrush);
		pBitmap->Release();
		if (SUCCEEDED(hr)) {
			pRenderTarget->FillRectangle(
				D2D1::RectF(rc.left, rc.top, rc.right, rc.bottom),
				pBitmapBrush);
			pBitmapBrush->Release();
		}
	}
}

void SurfaceD2D::RoundedRectangle(PRectangle rc, ColourDesired fore, ColourDesired back) {
	if (pRenderTarget) {
		D2D1_ROUNDED_RECT roundedRectFill = D2D1::RoundedRect(
			D2D1::RectF(rc.left+1.0, rc.top+1.0, rc.right-1.0, rc.bottom-1.0),
			8, 8);
		D2DPenColour(back);
		pRenderTarget->FillRoundedRectangle(roundedRectFill, pBrush);

		D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(
			D2D1::RectF(rc.left + 0.5, rc.top+0.5, rc.right - 0.5, rc.bottom-0.5),
			8, 8);
		D2DPenColour(fore);
		pRenderTarget->DrawRoundedRectangle(roundedRect, pBrush);
	}
}

void SurfaceD2D::AlphaRectangle(PRectangle rc, int cornerSize, ColourDesired fill, int alphaFill,
		ColourDesired outline, int alphaOutline, int /* flags*/ ) {
	if (pRenderTarget) {
		if (cornerSize == 0) {
			// When corner size is zero, draw square rectangle to prevent blurry pixels at corners
			D2D1_RECT_F rectFill = D2D1::RectF(RoundFloat(rc.left) + 1.0, rc.top + 1.0, RoundFloat(rc.right) - 1.0, rc.bottom - 1.0);
			D2DPenColour(fill, alphaFill);
			pRenderTarget->FillRectangle(rectFill, pBrush);

			D2D1_RECT_F rectOutline = D2D1::RectF(RoundFloat(rc.left) + 0.5, rc.top + 0.5, RoundFloat(rc.right) - 0.5, rc.bottom - 0.5);
			D2DPenColour(outline, alphaOutline);
			pRenderTarget->DrawRectangle(rectOutline, pBrush);
		} else {
			D2D1_ROUNDED_RECT roundedRectFill = D2D1::RoundedRect(
				D2D1::RectF(RoundFloat(rc.left) + 1.0, rc.top + 1.0, RoundFloat(rc.right) - 1.0, rc.bottom - 1.0),
				cornerSize, cornerSize);
			D2DPenColour(fill, alphaFill);
			pRenderTarget->FillRoundedRectangle(roundedRectFill, pBrush);

			D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(
				D2D1::RectF(RoundFloat(rc.left) + 0.5, rc.top + 0.5, RoundFloat(rc.right) - 0.5, rc.bottom - 0.5),
				cornerSize, cornerSize);
			D2DPenColour(outline, alphaOutline);
			pRenderTarget->DrawRoundedRectangle(roundedRect, pBrush);
		}
	}
}

void SurfaceD2D::DrawRGBAImage(PRectangle rc, int width, int height, const unsigned char *pixelsImage) {
	if (pRenderTarget) {
		if (rc.Width() > width)
			rc.left += static_cast<int>((rc.Width() - width) / 2);
		rc.right = rc.left + width;
		if (rc.Height() > height)
			rc.top += static_cast<int>((rc.Height() - height) / 2);
		rc.bottom = rc.top + height;

		std::vector<unsigned char> image(height * width * 4);
		for (int y=0; y<height; y++) {
			for (int x=0; x<width; x++) {
				unsigned char *pixel = &image[0] + (y*width+x) * 4;
				unsigned char alpha = pixelsImage[3];
				// Input is RGBA, output is BGRA with premultiplied alpha
				pixel[2] = (*pixelsImage++) * alpha / 255;
				pixel[1] = (*pixelsImage++) * alpha / 255;
				pixel[0] = (*pixelsImage++) * alpha / 255;
				pixel[3] = *pixelsImage++;
			}
		}

		ID2D1Bitmap *bitmap = 0;
		D2D1_SIZE_U size = D2D1::SizeU(width, height);
		D2D1_BITMAP_PROPERTIES props = {{DXGI_FORMAT_B8G8R8A8_UNORM,	
		    D2D1_ALPHA_MODE_PREMULTIPLIED}, 72.0, 72.0};
		HRESULT hr = pRenderTarget->CreateBitmap(size, &image[0],
                  width * 4, &props, &bitmap);
		if (SUCCEEDED(hr)) {
			D2D1_RECT_F rcDestination = {rc.left, rc.top, rc.right, rc.bottom};
			pRenderTarget->DrawBitmap(bitmap, rcDestination);
		}
		bitmap->Release();
	}
}

void SurfaceD2D::Ellipse(PRectangle rc, ColourDesired fore, ColourDesired back) {
	if (pRenderTarget) {
		FLOAT radius = rc.Width() / 2.0f - 1.0f;
		D2D1_ELLIPSE ellipse = D2D1::Ellipse(
			D2D1::Point2F((rc.left + rc.right) / 2.0f, (rc.top + rc.bottom) / 2.0f),
			radius,radius);

		PenColour(back);
		pRenderTarget->FillEllipse(ellipse, pBrush);
		PenColour(fore);
		pRenderTarget->DrawEllipse(ellipse, pBrush);
	}
}

void SurfaceD2D::Copy(PRectangle rc, Point from, Surface &surfaceSource) {
	SurfaceD2D &surfOther = static_cast<SurfaceD2D &>(surfaceSource);
	surfOther.FlushDrawing();
	ID2D1BitmapRenderTarget *pCompatibleRenderTarget = reinterpret_cast<ID2D1BitmapRenderTarget *>(
		surfOther.pRenderTarget);
	ID2D1Bitmap *pBitmap = NULL;
	HRESULT hr = pCompatibleRenderTarget->GetBitmap(&pBitmap);
	if (SUCCEEDED(hr)) {
		D2D1_RECT_F rcDestination = {rc.left, rc.top, rc.right, rc.bottom};
		D2D1_RECT_F rcSource = {from.x, from.y, from.x + rc.Width(), from.y + rc.Height()};
		pRenderTarget->DrawBitmap(pBitmap, rcDestination, 1.0f, 
			D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, rcSource);
		pRenderTarget->Flush();
		pBitmap->Release();
	}
}

void SurfaceD2D::DrawTextCommon(PRectangle rc, Font &font_, XYPOSITION ybase, const char *s, int len, UINT) {
	SetFont(font_);

	// Use Unicode calls
	const TextWide tbuf(s, len, unicodeMode, codePage);
	if (pRenderTarget && pTextFormat && pBrush) {
		
		// Explicitly creating a text layout appears a little faster 
		IDWriteTextLayout *pTextLayout;
		HRESULT hr = pIDWriteFactory->CreateTextLayout(tbuf.buffer, tbuf.tlen, pTextFormat,
				rc.Width(), rc.Height(), &pTextLayout);
		if (SUCCEEDED(hr)) {
			D2D1_POINT_2F origin = {rc.left, ybase-yAscent};
			pRenderTarget->DrawTextLayout(origin, pTextLayout, pBrush, D2D1_DRAW_TEXT_OPTIONS_NONE);
			pTextLayout->Release();
		}
	}
}

void SurfaceD2D::DrawTextNoClip(PRectangle rc, Font &font_, XYPOSITION ybase, const char *s, int len,
	ColourDesired fore, ColourDesired back) {
	if (pRenderTarget) {
		FillRectangle(rc, back);
		D2DPenColour(fore);
		DrawTextCommon(rc, font_, ybase, s, len, ETO_OPAQUE);
	}
}

void SurfaceD2D::DrawTextClipped(PRectangle rc, Font &font_, XYPOSITION ybase, const char *s, int len,
	ColourDesired fore, ColourDesired back) {
	if (pRenderTarget) {
		FillRectangle(rc, back);
		D2DPenColour(fore);
		DrawTextCommon(rc, font_, ybase, s, len, ETO_OPAQUE | ETO_CLIPPED);
	}
}

void SurfaceD2D::DrawTextTransparent(PRectangle rc, Font &font_, XYPOSITION ybase, const char *s, int len,
	ColourDesired fore) {
	// Avoid drawing spaces in transparent mode
	for (int i=0;i<len;i++) {
		if (s[i] != ' ') {
			if (pRenderTarget) {
				D2DPenColour(fore);
				DrawTextCommon(rc, font_, ybase, s, len, 0);
			}
			return;
		}
	}
}

XYPOSITION SurfaceD2D::WidthText(Font &font_, const char *s, int len) {
	FLOAT width = 1.0;
	SetFont(font_);
	const TextWide tbuf(s, len, unicodeMode, codePage);
	if (pIDWriteFactory && pTextFormat) {
		// Create a layout
		IDWriteTextLayout *pTextLayout = 0;
		HRESULT hr = pIDWriteFactory->CreateTextLayout(tbuf.buffer, tbuf.tlen, pTextFormat, 1000.0, 1000.0, &pTextLayout);
		if (SUCCEEDED(hr)) {
			DWRITE_TEXT_METRICS textMetrics;
			pTextLayout->GetMetrics(&textMetrics);
			width = textMetrics.widthIncludingTrailingWhitespace;
			pTextLayout->Release();
		}
	}
	return width;
}

void SurfaceD2D::MeasureWidths(Font &font_, const char *s, int len, XYPOSITION *positions) {
	SetFont(font_);
	int fit = 0;
	const TextWide tbuf(s, len, unicodeMode, codePage);
	TextPositions poses(tbuf.tlen);
	fit = tbuf.tlen;
	const int clusters = 1000;
	DWRITE_CLUSTER_METRICS clusterMetrics[clusters];
	UINT32 count = 0;
	if (pIDWriteFactory && pTextFormat) {
		SetFont(font_);
		// Create a layout
		IDWriteTextLayout *pTextLayout = 0;
		HRESULT hr = pIDWriteFactory->CreateTextLayout(tbuf.buffer, tbuf.tlen, pTextFormat, 10000.0, 1000.0, &pTextLayout);
		if (!SUCCEEDED(hr))
			return;
		// For now, assuming WCHAR == cluster
		pTextLayout->GetClusterMetrics(clusterMetrics, clusters, &count);
		FLOAT position = 0.0f;
		size_t ti=0;
		for (size_t ci=0;ci<count;ci++) {
			position += clusterMetrics[ci].width;
			for (size_t inCluster=0; inCluster<clusterMetrics[ci].length; inCluster++) {
				//poses.buffer[ti++] = int(position + 0.5);
				poses.buffer[ti++] = position;
			}
		}
		PLATFORM_ASSERT(ti == static_cast<size_t>(tbuf.tlen));
		pTextLayout->Release();
	}
	if (unicodeMode) {
		// Map the widths given for UTF-16 characters back onto the UTF-8 input string
		int ui=0;
		const unsigned char *us = reinterpret_cast<const unsigned char *>(s);
		int i=0;
		while (ui<fit) {
			unsigned char uch = us[i];
			unsigned int lenChar = 1;
			if (uch >= (0x80 + 0x40 + 0x20 + 0x10)) {
				lenChar = 4;
				ui++;
			} else if (uch >= (0x80 + 0x40 + 0x20)) {
				lenChar = 3;
			} else if (uch >= (0x80)) {
				lenChar = 2;
			}
			for (unsigned int bytePos=0; (bytePos<lenChar) && (i<len); bytePos++) {
				positions[i++] = poses.buffer[ui];
			}
			ui++;
		}
		int lastPos = 0;
		if (i > 0)
			lastPos = positions[i-1];
		while (i<len) {
			positions[i++] = lastPos;
		}
	} else if (codePage == 0) {

		// One character per position
		PLATFORM_ASSERT(len == tbuf.tlen);
		for (size_t kk=0;kk<static_cast<size_t>(len);kk++) {
			positions[kk] = poses.buffer[kk];
		}

	} else {
#ifdef WINDOWS7
		// May be more than one byte per position
		int ui = 0;
		for (int i=0;i<len;) {
			if (::IsDBCSLeadByteEx(codePage, s[i])) {
				positions[i] = poses.buffer[ui];
				positions[i+1] = poses.buffer[ui];
				i += 2;
			} else {
				positions[i] = poses.buffer[ui];
				i++;
			}

			ui++;
		}
#endif
	}
}

XYPOSITION SurfaceD2D::WidthChar(Font &font_, char ch) {
	FLOAT width = 1.0;
	SetFont(font_);
	if (pIDWriteFactory && pTextFormat) {
		// Create a layout
		IDWriteTextLayout *pTextLayout = 0;
		const WCHAR wch = ch;
		HRESULT hr = pIDWriteFactory->CreateTextLayout(&wch, 1, pTextFormat, 1000.0, 1000.0, &pTextLayout);
		if (SUCCEEDED(hr)) {
			DWRITE_TEXT_METRICS textMetrics;
			pTextLayout->GetMetrics(&textMetrics);
			width = textMetrics.widthIncludingTrailingWhitespace;
			pTextLayout->Release();
		}
	}
	return width;
}

XYPOSITION SurfaceD2D::Ascent(Font &font_) {
	SetFont(font_);
	return ceil(yAscent);
}

XYPOSITION SurfaceD2D::Descent(Font &font_) {
	SetFont(font_);
	return ceil(yDescent);
}

XYPOSITION SurfaceD2D::InternalLeading(Font &font_) {
	SetFont(font_);
	return floor(yInternalLeading);
}

XYPOSITION SurfaceD2D::ExternalLeading(Font &) {
	// Not implemented, always return one
	return 1;
}

XYPOSITION SurfaceD2D::Height(Font &font_) {
	return Ascent(font_) + Descent(font_);
}

XYPOSITION SurfaceD2D::AverageCharWidth(Font &font_) {
	FLOAT width = 1.0;
	SetFont(font_);
	if (pIDWriteFactory && pTextFormat) {
		// Create a layout
		IDWriteTextLayout *pTextLayout = 0;
		const WCHAR wszAllAlpha[] = L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
		HRESULT hr = pIDWriteFactory->CreateTextLayout(wszAllAlpha, static_cast<UINT32>(wcslen(wszAllAlpha)), 
			pTextFormat, 1000.0, 1000.0, &pTextLayout);
		if (SUCCEEDED(hr)) {
			DWRITE_TEXT_METRICS textMetrics;
			pTextLayout->GetMetrics(&textMetrics);
			width = textMetrics.width / wcslen(wszAllAlpha);
			pTextLayout->Release();
		}
	}
	return width;
}

void SurfaceD2D::SetClip(PRectangle rc) {
	if (pRenderTarget) {
		D2D1_RECT_F rcClip = {rc.left, rc.top, rc.right, rc.bottom};
		pRenderTarget->PushAxisAlignedClip(rcClip, D2D1_ANTIALIAS_MODE_ALIASED);
		clipsActive++;
	}
}

void SurfaceD2D::FlushCachedState() {
}

void SurfaceD2D::SetUnicodeMode(bool unicodeMode_) {
	unicodeMode=unicodeMode_;
}

void SurfaceD2D::SetDBCSMode(int codePage_) {
	// No action on window as automatically handled by system.
	codePage = codePage_;
}

Surface *Surface::Allocate(int technology) {
	return new SurfaceD2D;
}

Window::~Window() {
}

void Window::Destroy() {
	wid = 0;
}

bool Window::HasFocus() {
	return true;
}

PRectangle Window::GetPosition() {
	RECT rc = {0,0,0,0};
	return PRectangle(rc.left, rc.top, rc.right, rc.bottom);
}

void Window::SetPosition(PRectangle) {
}

void Window::SetPositionRelative(PRectangle, Window) {
}

PRectangle Window::GetClientPosition() {
	RECT rc={0,0,800,5000};
	FrameworkElementWrapper *few = reinterpret_cast<FrameworkElementWrapper *>(GetID());
	if (few && few->element) {
		rc.top = few->scrollView->VerticalOffset;
		rc.right = few->element->ActualWidth;
		rc.bottom = few->element->ActualHeight;
		//rc.bottom = rc.top + few->scrollView->ViewportHeight;
	}
	return  PRectangle(rc.left, rc.top, rc.right, rc.bottom);
}

void Window::Show(bool) {
}

void Window::InvalidateAll() {
	FrameworkElementWrapper *few = reinterpret_cast<FrameworkElementWrapper *>(GetID());
	if (few && few->element && few->source) {
		RECT bounds = {0,0,0,0};
		bounds.right = few->element->ActualWidth;
		bounds.bottom = few->element->ActualHeight;
		few->source->Invalidate(bounds);
	}
}

void Window::InvalidateRectangle(PRectangle rc) {
	RECT rcw = RectFromPRectangle(rc);
	FrameworkElementWrapper *few = reinterpret_cast<FrameworkElementWrapper *>(GetID());
	if (few && few->source) {
		few->source->Invalidate(rcw);
	}
}

void Window::SetFont(Font &) {
}

void Window::SetCursor(Cursor) {
}

void Window::SetTitle(const char *) {
}

PRectangle Window::GetMonitorRect(Point) {
	return PRectangle(0,0,400,400);
}

ListBox::ListBox() {
}

ListBox::~ListBox() {
}

// A null placeholder implementation
class ListBoxRT : public ListBox {
public:
	ListBoxRT() {
	}
	virtual ~ListBoxRT() {
	}
	virtual void SetFont(Font &font);
	virtual void Create(Window &parent, int ctrlID, Point location_, int lineHeight_, bool unicodeMode_, int technology_);
	virtual void SetAverageCharWidth(int width);
	virtual void SetVisibleRows(int rows);
	virtual int GetVisibleRows() const;
	virtual PRectangle GetDesiredRect();
	virtual int CaretFromEdge();
	virtual void Clear();
	virtual void Append(char *s, int type = -1);
	virtual int Length();
	virtual void Select(int n);
	virtual int GetSelection();
	virtual int Find(const char *prefix);
	virtual void GetValue(int n, char *value, int len);
	virtual void RegisterImage(int type, const char *xpm_data);
	virtual void RegisterRGBAImage(int type, int width, int height, const unsigned char *pixelsImage);
	virtual void ClearRegisteredImages();
	virtual void SetDoubleClickAction(Scintilla::CallBackAction, void *);
	virtual void SetList(const char *list, char separator, char typesep);
};
void ListBoxRT::SetFont(Font &) {
}
void ListBoxRT::Create(Window &, int, Point, int, bool, int) {
}
void ListBoxRT::SetAverageCharWidth(int) {
}
void ListBoxRT::SetVisibleRows(int) {
}
int ListBoxRT::GetVisibleRows() const {
	return 0;
}
PRectangle ListBoxRT::GetDesiredRect() {
	return PRectangle(0,0,10,10);
}
int ListBoxRT::CaretFromEdge() {
	return 0;
}
void ListBoxRT::Clear() {
}
void ListBoxRT::Append(char *, int) {
}
int ListBoxRT::Length() {
	return 0;
}
void ListBoxRT::Select(int n) {
}
int ListBoxRT::GetSelection() {
	return 0;
}
int ListBoxRT::Find(const char *prefix) {
	return -1;
}
void ListBoxRT::GetValue(int, char *value, int) {
	*value = 0;
}
void ListBoxRT::RegisterImage(int, const char *) {
}
void ListBoxRT::RegisterRGBAImage(int, int, int, const unsigned char *) {
}

void ListBoxRT::ClearRegisteredImages() {
}
void ListBoxRT::SetDoubleClickAction(Scintilla::CallBackAction, void *) {
}
void ListBoxRT::SetList(const char *list, char separator, char typesep) {
}

ListBox *ListBox::Allocate() {
	return new ListBoxRT;
}

Menu::Menu() : mid(0) {
}

void Menu::CreatePopUp() {
}

void Menu::Destroy() {
	mid = 0;
}

void Menu::Show(Point, Window &) {
}

static bool initialisedET = false;
static bool usePerformanceCounter = false;
static LARGE_INTEGER frequency;

ElapsedTime::ElapsedTime() {
	if (!initialisedET) {
		usePerformanceCounter = ::QueryPerformanceFrequency(&frequency) != 0;
		initialisedET = true;
	}
	if (usePerformanceCounter) {
		LARGE_INTEGER timeVal;
		::QueryPerformanceCounter(&timeVal);
		bigBit = timeVal.HighPart;
		littleBit = timeVal.LowPart;
	} else {
		bigBit = clock();
	}
}

double ElapsedTime::Duration(bool reset) {
	double result;
	long endBigBit;
	long endLittleBit;

	if (usePerformanceCounter) {
		LARGE_INTEGER lEnd;
		::QueryPerformanceCounter(&lEnd);
		endBigBit = lEnd.HighPart;
		endLittleBit = lEnd.LowPart;
		LARGE_INTEGER lBegin;
		lBegin.HighPart = bigBit;
		lBegin.LowPart = littleBit;
		double elapsed = lEnd.QuadPart - lBegin.QuadPart;
		result = elapsed / static_cast<double>(frequency.QuadPart);
	} else {
		endBigBit = clock();
		endLittleBit = 0;
		double elapsed = endBigBit - bigBit;
		result = elapsed / CLOCKS_PER_SEC;
	}
	if (reset) {
		bigBit = endBigBit;
		littleBit = endLittleBit;
	}
	return result;
}

/*

class DynamicLibraryImpl : public DynamicLibrary {
protected:
	HMODULE h;
public:
	DynamicLibraryImpl(const char *modulePath) {
		h = ::LoadLibraryA(modulePath);
	}

	virtual ~DynamicLibraryImpl() {
		if (h != NULL)
			::FreeLibrary(h);
	}

	// Use GetProcAddress to get a pointer to the relevant function.
	virtual Function FindFunction(const char *name) {
		if (h != NULL) {
			// C++ standard doesn't like casts betwen function pointers and void pointers so use a union
			union {
				FARPROC fp;
				Function f;
			} fnConv;
			fnConv.fp = ::GetProcAddress(h, name);
			return fnConv.f;
		} else
			return NULL;
	}

	virtual bool IsValid() {
		return h != NULL;
	}
};

*/

DynamicLibrary *DynamicLibrary::Load(const char * /* modulePath */) {
	//return static_cast<DynamicLibrary *>(new DynamicLibraryImpl(modulePath));
	return 0;
}

static bool assertionPopUps = true;

#if !defined(__cplusplus_winrt)

ColourDesired Scintilla::Platform::Chrome() {
	//return ::GetSysColor(COLOR_3DFACE);
	return ColourDesired(0xC0C0C0);
}

ColourDesired Scintilla::Platform::ChromeHighlight() {
	//return ::GetSysColor(COLOR_3DHIGHLIGHT);
	return ColourDesired(0xDFDFDF);
}

const char *Scintilla::Platform::DefaultFont() {
	return "Verdana";
}

int Scintilla::Platform::DefaultFontSize() {
	return 8;
}

unsigned int Scintilla::Platform::DoubleClickTime() {
	//return ::GetDoubleClickTime();
	return 200;
}

bool Scintilla::Platform::MouseButtonBounce() {
	return false;
}

void Scintilla::Platform::DebugDisplay(const char *s) {
	::OutputDebugStringA(s);
}

bool Scintilla::Platform::IsKeyDown(int key) {
	//return (::GetKeyState(key) & 0x80000000) != 0;
	return false;
}

long Scintilla::Platform::SendScintilla(WindowID w, unsigned int msg, unsigned long wParam, long lParam) {
	//return ::SendMessage(reinterpret_cast<HWND>(w), msg, wParam, lParam);
	return 0;
}

long Scintilla::Platform::SendScintillaPointer(WindowID w, unsigned int msg, unsigned long wParam, void *lParam) {
	//return ::SendMessage(reinterpret_cast<HWND>(w), msg, wParam,
	//	reinterpret_cast<LPARAM>(lParam));
	return 0;
}

bool Scintilla::Platform::IsDBCSLeadByte(int codePage, char ch) {
	return false;
}

int Scintilla::Platform::DBCSCharLength(int codePage, const char *s) {
	return 1;
}

int Scintilla::Platform::DBCSCharMaxLength() {
	return 2;
}

// These are utility functions not really tied to a XPlatform

int Scintilla::Platform::Minimum(int a, int b) {
	if (a < b)
		return a;
	else
		return b;
}

int Scintilla::Platform::Maximum(int a, int b) {
	if (a > b)
		return a;
	else
		return b;
}

//#define TRACE

#ifdef TRACE
void XPlatform::DebugPrintf(const char *format, ...) {
	char buffer[2000];
	va_list pArguments;
	va_start(pArguments, format);
	vsprintf(buffer,format,pArguments);
	va_end(pArguments);
	XPlatform::DebugDisplay(buffer);
}
#else
void Scintilla::Platform::DebugPrintf(const char *, ...) {
}
#endif

bool Scintilla::Platform::ShowAssertionPopUps(bool assertionPopUps_) {
	bool ret = assertionPopUps;
	assertionPopUps = assertionPopUps_;
	return ret;
}

void Scintilla::Platform::Assert(const char *c, const char *file, int line) {
	char buffer[2000];
	sprintf_s(buffer, "Assertion [%s] failed at %s %d", c, file, line);
	if (assertionPopUps) {
		/*
		int idButton = ::MessageBoxA(0, buffer, "Assertion failure",
			MB_ABORTRETRYIGNORE|MB_ICONHAND|MB_SETFOREGROUND|MB_TASKMODAL);
		if (idButton == IDRETRY) {
			//::DebugBreak();
		} else if (idButton == IDIGNORE) {
			// all OK
		} else {
			abort();
		}
		*/
	} else {
		strcat_s(buffer, "\r\n");
		Scintilla::Platform::DebugDisplay(buffer);
		//::DebugBreak();
		abort();
	}
}

int Scintilla::Platform::Clamp(int val, int minVal, int maxVal) {
	if (val > maxVal)
		val = maxVal;
	if (val < minVal)
		val = minVal;
	return val;
}

void Platform_Initialise(void *hInstance) {
}

void Platform_Finalise() {
}

#endif

#if defined(__cplusplus_winrt)

namespace Platform {

ColourDesired Chrome() {
	return ColourDesired(0xC0C0C0);
}

ColourDesired ChromeHighlight() {
	return ColourDesired(0xDFDFDF);
}

const char *DefaultFont() {
	return "Verdana";
}

int DefaultFontSize() {
	return 8;
}

unsigned int DoubleClickTime() {
	return 200;
}

bool MouseButtonBounce() {
	return false;
}

void DebugDisplay(const char *s) {
	::OutputDebugStringA(s);
}

bool IsKeyDown(int key) {
	return false;
}

long SendScintilla(WindowID w, unsigned int msg, unsigned long wParam, long lParam) {
	return 0;
}

long SendScintillaPointer(WindowID w, unsigned int msg, unsigned long wParam, void *lParam) {
	return 0;
}

int Minimum(int a, int b) {
	if (a < b)
		return a;
	else
		return b;
}

int Maximum(int a, int b) {
	if (a > b)
		return a;
	else
		return b;
}

void DebugPrintf(const char *, ...) {
}

bool ShowAssertionPopUps(bool assertionPopUps_) {
	bool ret = assertionPopUps;
	assertionPopUps = assertionPopUps_;
	return ret;
}

void Assert(const char *c, const char *file, int line) {
	char buffer[2000];
	sprintf_s(buffer, "Assertion [%s] failed at %s %d", c, file, line);
	if (assertionPopUps) {
	} else {
		strcat_s(buffer, "\r\n");
		Platform::DebugDisplay(buffer);
		//::DebugBreak();
		abort();
	}
}

int Clamp(int val, int minVal, int maxVal) {
	if (val > maxVal)
		val = maxVal;
	if (val < minVal)
		val = minVal;
	return val;
}

}

#endif
