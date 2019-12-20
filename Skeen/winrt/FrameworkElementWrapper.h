// Scintilla source code edit control
/** @file FrameworkElementWrapper.h
 ** Implementation of platform facilities on WinRT.
 **/
// Copyright 2012 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

struct FrameworkElementWrapper {
	Windows::UI::Xaml::FrameworkElement ^element;
	IVirtualSurfaceImageSourceNative *source;
	Windows::UI::Xaml::Controls::ScrollViewer ^scrollView;
	FrameworkElementWrapper(Windows::UI::Xaml::FrameworkElement^ element_=nullptr,
		IVirtualSurfaceImageSourceNative *source_=nullptr) : element(element_), source(source_), scrollView(nullptr) {
	}
};
