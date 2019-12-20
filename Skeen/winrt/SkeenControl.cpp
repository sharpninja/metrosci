//
// SkeenControl.cpp
// Implementation of the SkeenControl class.
//

#include "pch.h"
using namespace Skeen;
#include <memory>

//using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::System;
using namespace Windows::UI::Input;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Documents;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Media::Imaging;
using namespace Microsoft::WRL;

#include "Scintilla.h"
#include "SciLexer.h"

#include "ScintillaWinRT.h"

// The Templated Control item template is documented at http://go.microsoft.com/fwlink/?LinkId=234235

extern ID2D1Factory1 *pD2DFactory;
extern bool LoadD2D();

SkeenControl::SkeenControl() {
	DefaultStyleKey = "Skeen.SkeenControl";
	LoadD2D();
	pSciRT = Scintilla::NewScintillaWinRT();
}

void SkeenControl::OnApplyTemplate() {
	if (mapSurfaceElement != nullptr) {
		mapSurfaceElement->SizeChanged -= sizeChangedToken;
		mapSurfaceElement = nullptr;
	}
	//Get template child for draw surface
	mapSurfaceElement = (Border^)GetTemplateChild("DrawSurface");
	if (mapSurfaceElement != nullptr) {
		int width = (int)mapSurfaceElement->ActualWidth;
		int height = (int)mapSurfaceElement->ActualHeight;
		imageSource = CreateImageSource(width, height);
		ImageBrush^ brush = ref new ImageBrush();
		brush->ImageSource = imageSource;
		mapSurfaceElement->Background = brush;
		sizeChangedToken = mapSurfaceElement->SizeChanged +=
		        ref new SizeChangedEventHandler(this, &SkeenControl::OnMapSurfaceSizeChanged);
	}
	ticktock = ref new DispatcherTimer();
	ticktock->Tick += ref new Windows::Foundation::EventHandler<Object^>(this, &SkeenControl::OnTicker);
	TimeSpan tspan = {100 * 10 * 1000};
	ticktock->Interval = tspan;
	ticktock->Start();

	scrollView = (ScrollViewer^)GetTemplateChild("ScrollView");
	scrollView->ViewChanged +=
		ref new EventHandler<Windows::UI::Xaml::Controls::ScrollViewerViewChangedEventArgs^>(this, &SkeenControl::OnScrollViewChanged);
	pSciRT->SetElements(this, scrollView, mapSurfaceElement);

	//Focus(Windows::UI::Xaml::FocusState::Programmatic);
}

void SkeenControl::FireStyleNeeded(int position) {
	styleNeededEvent(position);
}

void SkeenControl::FireCharAdded(int ch) {
	charAddedEvent(ch);
}

void SkeenControl::FireSavePointReached() {
	savePointReachedEvent();
}

void SkeenControl::FireSavePointLeft() {
	savePointLeftEvent();
}

void SkeenControl::FireModifyAttemptRO() {
	modifyAttemptROEvent();
}

void SkeenControl::FireDoubleClick(int modifiers, int position, int line) {
	doubleClickEvent(modifiers, position, line);
}

void SkeenControl::FireUpdateUi(int updated) {
	updateUiEvent(updated);
}

void SkeenControl::FireModified(int position, int modificationType,
	size_t text, int length, int linesAdded, int line,
	int foldLevelNow, int foldLevelPrev, int token, int annotationLinesAdded) {
	modifiedEvent(position, modificationType, text, length, linesAdded, line,
		foldLevelNow, foldLevelPrev, token, annotationLinesAdded);
}

void SkeenControl::FireMacroRecord(int message, size_t wParam, ptrdiff_t lParam) {
	macroRecordEvent(message, wParam, lParam);
}

void SkeenControl::FireMarginClick(int position, int modifiers, int margin) {
	marginClickEvent(position, modifiers, margin);
}

void SkeenControl::FireNeedShown(int position, int length) {
	needShownEvent(position, length);
}

void SkeenControl::FirePainted() {
	paintedEvent();
}

void SkeenControl::FireUserListSelection() {
	userListSelectionEvent();
}

void SkeenControl::FireURIDropped() {
	uriDroppedEvent();
}

void SkeenControl::FireDwellStart(int x, int y) {
	dwellStartEvent(x, y);
}

void SkeenControl::FireDwellEnd(int x, int y) {
	dwellEndEvent(x, y);
}

void SkeenControl::FireZoom() {
	zoomEvent();
}

void SkeenControl::FireHotSpotClick(int modifiers, int position) {
	hotSpotClickEvent(modifiers, position);
}

void SkeenControl::FireHotSpotDoubleClick(int modifiers, int position) {
	hotSpotDoubleClickEvent(modifiers, position);
}

void SkeenControl::FireCallTipClick(int position) {
	callTipClickEvent(position);
}

void SkeenControl::FireAutoCSelection(size_t text, int position) {
	autoCSelectionEvent(text, position);
}

void SkeenControl::FireIndicatorClick(int modifiers, int position) {
	indicatorClickEvent(modifiers, position);
}

void SkeenControl::FireIndicatorRelease(int modifiers, int position) {
	indicatorReleaseEvent(modifiers, position);
}

void SkeenControl::FireAutoCCancelled() {
	autoCCancelledEvent();
}

void SkeenControl::FireAutoCCharDeleted() {
	autoCCharDeletedEvent();
}

void SkeenControl::FireHotSpotReleaseClick(int modifiers, int position) {
	hotSpotReleaseClickEvent(modifiers, position);
}

void SkeenControl::FireChange() {
    changeEvent();
}

void SkeenControl::FireFocus(bool focus) {
    focusEvent(focus);
}

ptrdiff_t SkeenControl::Call(unsigned int iMessage, size_t wParam, ptrdiff_t  lParam) {
	return pSciRT->Call(iMessage, wParam, lParam);
}

void SkeenControl::OnKeyDown(Input::KeyRoutedEventArgs^ args) {
	int ret = pSciRT->DoKeyDown(args);
	args->Handled = true;
	Control::OnKeyDown(args);
}

void SkeenControl::OnKeyUp(Input::KeyRoutedEventArgs^ args) {
	int ret = pSciRT->DoKeyUp(args);
}

void SkeenControl::OnTicker(Object^ sender, Object^ args) {
	pSciRT->TickTock();
}

void SkeenControl::OnPointerPressed(Windows::UI::Xaml::Input::PointerRoutedEventArgs^ args) {
	PointerPoint^ ppt = args->GetCurrentPoint(this);
	pSciRT->ButtonDown(ppt, args->KeyModifiers);
	args->Handled = true;
	Control::OnPointerPressed(args);
}

void SkeenControl::OnPointerMoved(Windows::UI::Xaml::Input::PointerRoutedEventArgs^ args) {
	PointerPoint^ ppt = args->GetCurrentPoint(this);
	pSciRT->ButtonMove(ppt, args->KeyModifiers);
}

void SkeenControl::OnPointerReleased(Windows::UI::Xaml::Input::PointerRoutedEventArgs^ args) {
	PointerPoint^ ppt = args->GetCurrentPoint(this);
	pSciRT->ButtonRelease(ppt, args->KeyModifiers);
	Control::OnPointerReleased(args);
}

void SkeenControl::OnGotFocus(Windows::UI::Xaml::RoutedEventArgs^ args) {
	pSciRT->SetFocussed(true);
	Control::OnGotFocus(args);
}

void SkeenControl::OnLostFocus(Windows::UI::Xaml::RoutedEventArgs^ args) {
	pSciRT->SetFocussed(false);
	Control::OnLostFocus(args);
}

VirtualSurfaceImageSource^ SkeenControl::CreateImageSource(int width, int height) {
	//Define the size of the shared surface by passing the height and width to
	//the SurfaceImageSource constructor. You can also indicate whether the surface
	//needs alpha (opacity) support.
	VirtualSurfaceImageSource^  surfaceImageSource = ref new VirtualSurfaceImageSource(width, height, true);
	if (width <= 0 || height <= 0) return surfaceImageSource;

	//Get a pointer to ISurfaceImageSourceNative. Cast the SurfaceImageSource object
	//as IInspectable (or IUnknown), and call QueryInterface on it to get the underlying
	//ISurfaceImageSourceNative implementation. You use the methods defined on this
	//implementation to set the device and run the draw operations.
	IInspectable *sisInspectable = (IInspectable *) reinterpret_cast<IInspectable *>(surfaceImageSource);
	sisInspectable->QueryInterface(__uuidof(ISurfaceImageSourceNative), (void **)&imageSourceNative);

	//Set the DXGI device by first calling D3D11CreateDevice and then passing the device and
	//context to ISurfaceImageSourceNative::SetDevice.

	// This flag adds support for surfaces with a different color channel ordering than the API default.
	// It is recommended usage, and is required for compatibility with Direct2D.
	UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(_DEBUG)
	// If the project is in a debug build, enable debugging via SDK Layers with this flag.
	creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	// This array defines the set of DirectX hardware feature levels this app will support.
	// Note the ordering should be preserved.
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};

	HRESULT hr = D3D11CreateDevice(
	        NULL,
	        D3D_DRIVER_TYPE_HARDWARE,
	        NULL,
	        creationFlags,
	        featureLevels,
	        ARRAYSIZE(featureLevels),
	        D3D11_SDK_VERSION,
	        &d3dDevice,
	        NULL,
	        &d3dContext
	        );
	if (FAILED(hr))
		throw ref new Platform::COMException(hr);
	Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
	// Obtain the underlying DXGI device of the Direct3D11.1 device.
	d3dDevice.As(&dxgiDevice);
	imageSourceNative->SetDevice(dxgiDevice.Get());

	ComPtr<IUnknown> unknown(reinterpret_cast<IUnknown *>(surfaceImageSource));
	unknown.As(&imageSourceNative);

	HRESULT hrCreate = pD2DFactory->CreateDevice(dxgiDevice.Get(), &d2dDevice);
	if (FAILED(hrCreate)) {
		throw Platform::Exception::CreateException(hrCreate);
	}

	HRESULT hrCreateDC = d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &d2dDeviceContext);
	if (FAILED(hrCreateDC)) {
		throw Platform::Exception::CreateException(hrCreateDC);
	}

	pSciRT->SetImageSourceNative(imageSourceNative.Get(), d2dDeviceContext.Get());

	return surfaceImageSource;
}

//surface size changed handler
void SkeenControl::OnMapSurfaceSizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e) {
	int width = (int)mapSurfaceElement->ActualWidth;
	int height = (int)mapSurfaceElement->ActualHeight;
	imageSource = CreateImageSource(width, height);
	ImageBrush^ brush = ref new ImageBrush();
	brush->ImageSource = imageSource;
	mapSurfaceElement->Background = brush;
}

// Scroller changed handler
void SkeenControl::OnScrollViewChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::ScrollViewerViewChangedEventArgs^ e) {
	pSciRT->ScrollViewChanged();
}
