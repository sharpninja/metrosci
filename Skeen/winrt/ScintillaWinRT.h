// Scintilla source code edit control
/** @file ScintillaWinRT.h
 ** WinRT specific subclass of ScintillaBase.
 **/
// Copyright 1998-2012 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

namespace Scintilla {

class ScintillaWinRTBase {
public:
	virtual ptrdiff_t Call(unsigned int iMessage, size_t wParam=0, ptrdiff_t lParam=0) = 0;
	virtual void SetElements(
		Windows::UI::Xaml::Controls::Control^ control_,
		Windows::UI::Xaml::Controls::ScrollViewer^ scrollView_,
	    Windows::UI::Xaml::Controls::Border^ border_) = 0;
	virtual void SetImageSourceNative(
		IVirtualSurfaceImageSourceNative *imageSourceNative_,
	    ID2D1DeviceContext *d2dDeviceContext_) = 0;
	virtual int DoKeyDown(Windows::UI::Xaml::Input::KeyRoutedEventArgs^ args) = 0;
	virtual int DoKeyUp(Windows::UI::Xaml::Input::KeyRoutedEventArgs^ args) = 0;
	virtual void ButtonDown(PointerPoint^ ppt, Windows::System::VirtualKeyModifiers vkm) = 0;
	virtual void ButtonMove(PointerPoint^ ppt, Windows::System::VirtualKeyModifiers vkm) = 0;
	virtual void ButtonRelease(PointerPoint^ ppt, Windows::System::VirtualKeyModifiers vkm) = 0;
	virtual void SetFocussed(bool focusState) = 0;
	virtual void TickTock() = 0;
	virtual void ScrollViewChanged() = 0;
};

ScintillaWinRTBase *NewScintillaWinRT();

};
