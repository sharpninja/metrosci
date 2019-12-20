// Scintilla source code edit control
/** @file ScintillaWinRT.cxx
 ** WinRT specific subclass of ScintillaBase.
 **/
// Copyright 1998-2012 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include "pch.h"

#include <memory>

#include <d3d11_1.h>
#include <dxgi1_2.h>
#include <d2d1_1.h>
#include <wrl.h>
#include <windows.ui.xaml.media.dxinterop.h>

using namespace Windows::UI::Input;
using namespace Microsoft::WRL;

#include "Platform.h"

#include "ILexer.h"
#include "Scintilla.h"

#include "SciLexer.h"
#include "LexerModule.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "ContractionState.h"
#include "CellBuffer.h"
#include "CallTip.h"
#include "KeyMap.h"
#include "Indicator.h"
#include "XPM.h"
#include "LineMarker.h"
#include "Style.h"
#include "AutoComplete.h"
#include "ViewStyle.h"
#include "CharClassify.h"
#include "Decoration.h"
#include "Document.h"
#include "Selection.h"
#include "PositionCache.h"
#include "Editor.h"
#include "ScintillaBase.h"
#include "UniConversion.h"

#include "ScintillaWinRT.h"
#include "FrameworkElementWrapper.h"
#include "SkeenControl.h"

class ScintillaWinRT : public Scintilla::ScintillaBase,
	public Scintilla::ScintillaWinRTBase,
	public IVirtualSurfaceUpdatesCallbackNative {
	bool capturedMouse;
	bool keyDownShift;
	bool keyDownControl;
	bool keyDownAlt;
	Windows::UI::Xaml::Controls::Control^ control;
	Windows::UI::Xaml::Controls::ScrollViewer^ scrollView;
	IVirtualSurfaceImageSourceNative *imageSourceNative;
	ID2D1DeviceContext *d2dDeviceContext;
	FrameworkElementWrapper svwrap;
	int ref;
public:
	ScintillaWinRT();
	virtual ~ScintillaWinRT();
	virtual void Initialise();
	virtual void Finalise();
	virtual Scintilla::PRectangle GetTextViewRectangle();
	virtual int GetTopLineGraphics() const;
	virtual sptr_t DefWndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam);
	virtual void SetTicking(bool on) {}
	virtual bool HaveMouseCapture() { return capturedMouse; }
	virtual void SetMouseCapture(bool on) {}
	virtual void SetVerticalScrollPos();
	virtual void SetHorizontalScrollPos();
	virtual bool ModifyScrollBars(int nMax, int nPage);
	virtual void NotifyChange();
	virtual void NotifyParent(Scintilla::SCNotification scn);
	virtual void Copy() {}
	virtual void Paste() {}
	virtual void CreateCallTipWindow(Scintilla::PRectangle rc) {}
	virtual void AddToPopUp(const char *label, int cmd = 0, bool enabled = true) {}
	virtual void ClaimSelection() {}
	virtual void CopyToClipboard(const Scintilla::SelectionText &selectedText) {}
	virtual sptr_t WndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam);

	ptrdiff_t Call(unsigned int iMessage, size_t wParam=0, ptrdiff_t lParam=0);
	void SetElements(
		Windows::UI::Xaml::Controls::Control^ control_,
		Windows::UI::Xaml::Controls::ScrollViewer^ scrollView_,
	    Windows::UI::Xaml::Controls::Border^ border_);
	void SetImageSourceNative(IVirtualSurfaceImageSourceNative *imageSourceNative_,
	    ID2D1DeviceContext *d2dDeviceContext_);
	int DoKeyDown(Windows::UI::Xaml::Input::KeyRoutedEventArgs^ args);
	int DoKeyUp(Windows::UI::Xaml::Input::KeyRoutedEventArgs^ args);
	void ButtonDown(PointerPoint^ ppt, Windows::System::VirtualKeyModifiers vkm);
	void ButtonMove(PointerPoint^ ppt, Windows::System::VirtualKeyModifiers vkm);
	void ButtonRelease(PointerPoint^ ppt, Windows::System::VirtualKeyModifiers vkm);
	void SetFocussed(bool focusState);
	void TickTock();
	void ScrollViewChanged();

	void DrawBit(RECT const &drawingBounds);

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(
	    /* [in] */ REFIID riid,
	    /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject);
	virtual ULONG STDMETHODCALLTYPE AddRef(void);
	virtual ULONG STDMETHODCALLTYPE Release(void);
	virtual HRESULT STDMETHODCALLTYPE UpdatesNeeded();
};

ScintillaWinRT::ScintillaWinRT() :
	ref(1),
	capturedMouse(false),
	keyDownShift(false),
	keyDownControl(false),
	keyDownAlt(false),
	scrollView(nullptr) {
}

ScintillaWinRT::~ScintillaWinRT() {
}

void ScintillaWinRT::Initialise() {
}

void ScintillaWinRT::Finalise() {
}

Scintilla::PRectangle ScintillaWinRT::GetTextViewRectangle() {
	Scintilla::PRectangle rc(0,0,800,5000);
	if (svwrap.element && scrollView) {
		rc.top = scrollView->VerticalOffset;
		rc.right = svwrap.element->ActualWidth;
		//rc.bottom = svwrap.element->ActualHeight;
		rc.bottom = rc.top + scrollView->ViewportHeight;
	}
	return rc;
}

int ScintillaWinRT::GetTopLineGraphics() const {
	return 0;
}

sptr_t ScintillaWinRT::DefWndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
	return 0;
}

void ScintillaWinRT::SetVerticalScrollPos() {
	int vOffset = topLine * vs.lineHeight;
	scrollView->ScrollToVerticalOffset(vOffset);
}

void ScintillaWinRT::SetHorizontalScrollPos() {
//	scrollView->ScrollToHorizontalOffset(xOffset);
}

bool ScintillaWinRT::ModifyScrollBars(int nMax, int nPage) { 
	int lines = Call(SCI_GETLINECOUNT, 0, 0);
	int lineHeight = Call(SCI_TEXTHEIGHT, 0, 0);
	int heightNew = lines*lineHeight;
	if (svwrap.element && (heightNew != svwrap.element->Height)) {
		svwrap.element->Height = heightNew;
		control->InvalidateMeasure();
		control->InvalidateArrange();
		return true;
	}
	return false;
}

void ScintillaWinRT::NotifyChange() {
	Skeen::SkeenControl ^sControl = static_cast<Skeen::SkeenControl ^>(control);
	if (sControl)
		sControl->FireChange();
}

void ScintillaWinRT::NotifyParent(Scintilla::SCNotification scn) {
	Skeen::SkeenControl ^sControl = static_cast<Skeen::SkeenControl ^>(control);
	if (!sControl)
		return;
	switch (scn.nmhdr.code) {
	case SCN_STYLENEEDED:
		sControl->FireStyleNeeded(scn.position);
		break;
	case SCN_CHARADDED:
		sControl->FireCharAdded(scn.ch);
		break;
	case SCN_SAVEPOINTREACHED:
		sControl->FireSavePointReached();
		break;
	case SCN_SAVEPOINTLEFT:
		sControl->FireSavePointLeft();
		break;
	case SCN_MODIFYATTEMPTRO:
		sControl->FireModifyAttemptRO();
		break;
	case SCN_DOUBLECLICK:
		sControl->FireDoubleClick(scn.modifiers, scn.position, scn.line);
		break;
	case SCN_UPDATEUI:
		sControl->FireUpdateUi(scn.updated);
		break;
	case SCN_MODIFIED: {
		// A Platform::Array would be more typesafe and work with JavaScript but require an allocation
		//Platform::Array<unsigned char>^ value = ref new Platform::Array<unsigned char>(scn.length);
		//if (scn.text)
		//	memcpy(value->begin(), scn.text, scn.length);
		sControl->FireModified(scn.position, scn.modificationType, reinterpret_cast<size_t>(scn.text), scn.length, scn.linesAdded, scn.line,
			scn.foldLevelNow, scn.foldLevelPrev, scn.token, scn.annotationLinesAdded);
		}
		break;
	case SCN_MACRORECORD:
		sControl->FireMacroRecord(scn.message, scn.wParam, scn.lParam);
		break;
	case SCN_MARGINCLICK:
		sControl->FireMarginClick(scn.position, scn.modifiers, scn.margin);
		break;
	case SCN_NEEDSHOWN:
		sControl->FireNeedShown(scn.position, scn.length);
		break;
	case SCN_PAINTED:
		sControl->FirePainted();
		break;
	case SCN_USERLISTSELECTION:
		sControl->FireUserListSelection();
		break;
	case SCN_URIDROPPED:
		sControl->FireURIDropped();
		break;
	case SCN_DWELLSTART:
		sControl->FireDwellStart(scn.x, scn.y);
		break;
	case SCN_DWELLEND:
		sControl->FireDwellEnd(scn.x, scn.y);
		break;
	case SCN_ZOOM:
		sControl->FireZoom();
		break;
	case SCN_HOTSPOTCLICK:
		sControl->FireHotSpotClick(scn.modifiers, scn.position);
		break;
	case SCN_HOTSPOTDOUBLECLICK:
		sControl->FireHotSpotDoubleClick(scn.modifiers, scn.position);
		break;
	case SCN_CALLTIPCLICK:
		sControl->FireCallTipClick(scn.position);
		break;
	case SCN_AUTOCSELECTION:
		sControl->FireAutoCSelection(reinterpret_cast<size_t>(scn.text), scn.position);
		break;
	case SCN_INDICATORCLICK:
		sControl->FireIndicatorClick(scn.modifiers, scn.position);
		break;
	case SCN_INDICATORRELEASE:
		sControl->FireIndicatorRelease(scn.modifiers, scn.position);
		break;
	case SCN_AUTOCCANCELLED:
		sControl->FireAutoCCancelled();
		break;
	case SCN_AUTOCCHARDELETED:
		sControl->FireAutoCCharDeleted();
		break;
	case SCN_HOTSPOTRELEASECLICK:
		sControl->FireHotSpotReleaseClick(scn.modifiers, scn.position);
		break;
	}
}

sptr_t ScintillaWinRT::WndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
	return ScintillaBase::WndProc(iMessage, wParam, lParam);
}

ptrdiff_t ScintillaWinRT::Call(unsigned int iMessage, size_t wParam, ptrdiff_t lParam) {
	return WndProc(iMessage, wParam, lParam);
}

void ScintillaWinRT::SetElements(
		Windows::UI::Xaml::Controls::Control^ control_,
		Windows::UI::Xaml::Controls::ScrollViewer^ scrollView_,
        Windows::UI::Xaml::Controls::Border^ border_) {
	control = control_;
	scrollView = scrollView_;
	svwrap.element = border_;
	svwrap.scrollView = scrollView_;
	wMain = &svwrap;
	imageSourceNative = nullptr;
}

void ScintillaWinRT::SetImageSourceNative(IVirtualSurfaceImageSourceNative *imageSourceNative_,
        ID2D1DeviceContext *d2dDeviceContext_) {
	imageSourceNative = imageSourceNative_;
	svwrap.source = imageSourceNative;
	d2dDeviceContext = d2dDeviceContext_;

	// Register image source's update callback so update can be made to it.
	imageSourceNative->RegisterForUpdatesNeeded(this);
}

/** Map the key codes to their equivalent SCK_ form. */
static int KeyTranslate(int keyIn) {
//PLATFORM_ASSERT(!keyIn);
	switch (keyIn) {
	case VK_DOWN:
		return SCK_DOWN;
	case VK_UP:
		return SCK_UP;
	case VK_LEFT:
		return SCK_LEFT;
	case VK_RIGHT:
		return SCK_RIGHT;
	case VK_HOME:
		return SCK_HOME;
	case VK_END:
		return SCK_END;
	case VK_PRIOR:
		return SCK_PRIOR;
	case VK_NEXT:
		return SCK_NEXT;
	case VK_DELETE:
		return SCK_DELETE;
	case VK_INSERT:
		return SCK_INSERT;
	case VK_ESCAPE:
		return SCK_ESCAPE;
	case VK_BACK:
		return SCK_BACK;
	case VK_TAB:
		return SCK_TAB;
	case VK_RETURN:
		return SCK_RETURN;
	case VK_ADD:
		return SCK_ADD;
	case VK_SUBTRACT:
		return SCK_SUBTRACT;
	case VK_DIVIDE:
		return SCK_DIVIDE;
	case VK_LWIN:
		return SCK_WIN;
	case VK_RWIN:
		return SCK_RWIN;
	case VK_APPS:
		return SCK_MENU;
	case VK_OEM_2:
		return '/';
	case VK_OEM_3:
		return '`';
	case VK_OEM_4:
		return '[';
	case VK_OEM_5:
		return '\\';
	case VK_OEM_6:
		return ']';
	default:
		return 0;
	}
}

static int CharFromVK(Windows::UI::Xaml::Input::KeyRoutedEventArgs^ args, bool shift) {
	switch (args->Key) {
	case VK_OEM_1:
		return shift ? ':' : ';';	// ';:' for US
	case VK_OEM_PLUS:
		return shift ? '=' : '+';	// '+' any country
	case VK_OEM_COMMA:
		return shift ? '<' : ',';				// ',' any country
	case VK_OEM_MINUS:
		return shift ? '_' :'-';	// '-' any country
	case VK_OEM_PERIOD:
		return shift ? '>' : '.';				// '.' any country
	case VK_OEM_2:
		return shift ? '?' : '/';	// '/?' for US
	case VK_OEM_3:
		return shift ? '~' : '`';	// '`~' for US
	case VK_OEM_4:
		return shift ? '{' : '[';	// '[{' for US
	case VK_OEM_5:
		return shift ? '|' : '\\';	// '\|' for US
	case VK_OEM_6:
		return shift ? '}' : ']';	// ']}' for US
	case VK_OEM_7:
		return shift ? '"' : '\'';	// ''"' for US
	case VK_OEM_102:
		return shift ? '>' : '<';
	}
	return 0;
}

int ScintillaWinRT::DoKeyDown(Windows::UI::Xaml::Input::KeyRoutedEventArgs^ args) {
	// WinRT doesn't appear to have a cooked keyboard event to retrieve the character typed,
	// just bare scan codes and virtual key values.
	switch (args->Key) {
	case Windows::System::VirtualKey::Shift:
		keyDownShift = true;
		return 0;
	case Windows::System::VirtualKey::Control:
		keyDownControl = true;
		return 0;
	case Windows::System::VirtualKey::Menu:
		keyDownAlt = true;
		return 0;
	}
	int k = (int)(args->Key);
	int scancode = args->KeyStatus.ScanCode;
	int sck = KeyTranslate(k);
	int chFromVK = CharFromVK(args, keyDownShift);
	if (((chFromVK) || (sck == 0)) && !keyDownControl && !keyDownAlt) {
		// Treat as text
		wchar_t wcs[2] = {static_cast<wchar_t>(k), 0};
		if (chFromVK) {
			wcs[0] = chFromVK;
		} else if ((k >= 'A' && k <= 'Z') || (k >= '0' && k <= '9') || (k == ' ')) {
			if (!keyDownShift && (k >= 'A' && k <= 'Z'))
				wcs[0] = k - 'A' + 'a';
			if (keyDownShift && (k >= '0' && k <= '9'))
				wcs[0] = ")!@#$%^&*("[k - '0'];
		} else {
			wcs[0] = scancode;
		}
		char utfval[4];
		unsigned int len = UTF8Length(wcs, 1);
		UTF8FromUTF16(wcs, 1, utfval, len);
		AddCharUTF(utfval, len);
		return 0;
	} else {
		// Treat as command
		bool lastKeyDownConsumed = false;
		return KeyDown(sck ? sck : k, keyDownShift, keyDownControl, keyDownAlt, &lastKeyDownConsumed);
	}
}

int ScintillaWinRT::DoKeyUp(Windows::UI::Xaml::Input::KeyRoutedEventArgs^ args) {
	switch (args->Key) {
	case Windows::System::VirtualKey::Shift:
		keyDownShift = false;
		return 0;
	case Windows::System::VirtualKey::Control:
		keyDownControl = false;
		return 0;
	case Windows::System::VirtualKey::Menu:
		keyDownAlt = false;
		return 0;
	}
	return 0;
}

void ScintillaWinRT::ButtonDown(PointerPoint^ ppt, Windows::System::VirtualKeyModifiers vkm) {
	capturedMouse = true;
	Scintilla::Point spt(ppt->Position.X + scrollView->HorizontalOffset, ppt->Position.Y + scrollView->VerticalOffset);
	ScintillaBase::ButtonDown(spt, 0,
	        (vkm & Windows::System::VirtualKeyModifiers::Shift) == Windows::System::VirtualKeyModifiers::Shift,
	        (vkm & Windows::System::VirtualKeyModifiers::Control) == Windows::System::VirtualKeyModifiers::Control,
	        (vkm & Windows::System::VirtualKeyModifiers::Menu) == Windows::System::VirtualKeyModifiers::Menu);
}

void ScintillaWinRT::ButtonMove(PointerPoint^ ppt, Windows::System::VirtualKeyModifiers vkm) {
	Scintilla::Point spt(ppt->Position.X + scrollView->HorizontalOffset, ppt->Position.Y + scrollView->VerticalOffset);
	ScintillaBase::ButtonMove(spt);
}

void ScintillaWinRT::ButtonRelease(PointerPoint^ ppt, Windows::System::VirtualKeyModifiers vkm) {
	capturedMouse = false;
	Scintilla::Point spt(ppt->Position.X + scrollView->HorizontalOffset, ppt->Position.Y + scrollView->VerticalOffset);
	ScintillaBase::ButtonUp(spt, 0,
	        (vkm & Windows::System::VirtualKeyModifiers::Control) == Windows::System::VirtualKeyModifiers::Control);
}

void ScintillaWinRT::SetFocussed(bool focusState) {
	SetFocusState(focusState);
}

void ScintillaWinRT::TickTock() {
	Tick();
}

void ScintillaWinRT::ScrollViewChanged() {
	int topLineNew = 0;
	if (scrollView && vs.lineHeight)
		topLineNew = scrollView->VerticalOffset / vs.lineHeight;
	SetTopLine(topLineNew);
}

void ScintillaWinRT::DrawBit(RECT const &drawingBounds) {
	ComPtr<IDXGISurface> surface;
	POINT surfaceOffset = {0};

	//Provide a pointer to IDXGISurface object to ISurfaceImageSourceNative::BeginDraw, and
	//draw into that surface using DirectX. Only the area specified for update in the
	//updateRect parameter is drawn.
	//
	//This method returns the point (x,y) offset of the updated target rectangle in the offset
	//parameter. You use this offset to determine where to draw into inside the IDXGISurface.
	HRESULT beginDrawHR = imageSourceNative->BeginDraw(drawingBounds, &surface, &surfaceOffset);
	if (beginDrawHR == DXGI_ERROR_DEVICE_REMOVED || beginDrawHR == DXGI_ERROR_DEVICE_RESET) {
		// device changed
	} else {
		ComPtr<ID2D1Bitmap1> bitmap;
		HRESULT hrBitMap = d2dDeviceContext->CreateBitmapFromDxgiSurface(
		        surface.Get(), nullptr, &bitmap);
		if (FAILED(hrBitMap)) {
			throw Platform::Exception::CreateException(hrBitMap);
		}
		d2dDeviceContext->BeginDraw();
		d2dDeviceContext->SetTarget(bitmap.Get());
		d2dDeviceContext->SetTransform(D2D1::IdentityMatrix());

		// Translate the drawing to the designated place on the surface.
		D2D1::Matrix3x2F transform =
		    D2D1::Matrix3x2F::Scale(1.0f, 1.0f) *
		    D2D1::Matrix3x2F::Translation(
		        static_cast<float>(surfaceOffset.x - drawingBounds.left),
		        static_cast<float>(surfaceOffset.y - drawingBounds.top)
		    );

		// Constrain the drawing only to the designated portion of the surface
		d2dDeviceContext->PushAxisAlignedClip(
		    D2D1::RectF(
		        static_cast<float>(surfaceOffset.x),
		        static_cast<float>(surfaceOffset.y),
		        static_cast<float>(surfaceOffset.x + (drawingBounds.right - drawingBounds.left)),
		        static_cast<float>(surfaceOffset.y + (drawingBounds.bottom - drawingBounds.top))
		    ),
		    D2D1_ANTIALIAS_MODE_ALIASED
		);

		d2dDeviceContext->SetTransform(transform);

		Scintilla::Surface *surf = Scintilla::Surface::Allocate(0);
		surf->Init(d2dDeviceContext, NULL);
		surf->SetUnicodeMode(true);
		surf->SetDBCSMode(0);
		rcPaint = Scintilla::PRectangle(drawingBounds.left, drawingBounds.top, drawingBounds.right, drawingBounds.bottom);
		paintingAllText = true;
		Paint(surf, rcPaint);

		surf->Release();

		d2dDeviceContext->EndDraw();
	}

	imageSourceNative->EndDraw();
}

STDMETHODIMP ScintillaWinRT::QueryInterface(REFIID riid, PVOID *ppv) {
	*ppv = NULL;
	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG)ScintillaWinRT::AddRef() {
	return ++ref;
}

STDMETHODIMP_(ULONG)ScintillaWinRT::Release() {
	ref--;
	if (ref > 0)
		return ref;
	delete this;
	return 0;
}

STDMETHODIMP ScintillaWinRT::UpdatesNeeded() {
	ULONG drawingBoundsCount = 0;
	HRESULT hr = imageSourceNative->GetUpdateRectCount(&drawingBoundsCount);
	if (FAILED(hr))
		return hr;

	std::unique_ptr<RECT[]> drawingBounds(new RECT[drawingBoundsCount]);
	hr = imageSourceNative->GetUpdateRects(drawingBounds.get(), drawingBoundsCount);
	if (FAILED(hr))
		return hr;

	// This code doesn't try to coalesce multiple drawing bounds into one. Although that
	// extra process will reduce the number of draw calls, it requires the virtual surface
	// image source to manage non-uniform tile size, which requires it to make extra copy
	// operations to the compositor. By using the drawing bounds it directly returns, which are
	// of non-overlapping uniform tile size, the compositor is able to use these tiles directly,
	// which can greatly reduce the amount of memory needed by the virtual surface image source.
	// It will result in more draw calls though, but Direct2D will be able to accommodate that
	// without significant impact on presentation frame rate.
	for (ULONG i = 0; i < drawingBoundsCount; ++i) {
		DrawBit(drawingBounds[i]);
	}

	return hr;
}

Scintilla::ScintillaWinRTBase *Scintilla::NewScintillaWinRT() {
	return new ScintillaWinRT;
}
