//
// SkeenControl.h
// Declaration of the SkeenControl class.
//

#pragma once

#include <d3d11_1.h>
#include <dxgi1_2.h>
#include <d2d1_1.h>
#include <wrl.h>
#include <windows.ui.xaml.media.dxinterop.h>

namespace Scintilla {
class ScintillaWinRTBase;
struct SCNotification;
};

namespace Skeen
{

	// For ModifiedHandler and AutoCSelectionHandler, char *s are smuggled
	// through C++/CX type restrictions as size_t. 
	// const Platform::Array<unsigned char>^ could be used instead but that costs
	// an allocation for each event even when empty.
	public delegate void StyleNeededHandler(int position);
	public delegate void CharAddedHandler(int ch);
	public delegate void SavePointReachedHandler();
	public delegate void SavePointLeftHandler();
	public delegate void ModifyAttemptROHandler();
	public delegate void DoubleClickHandler(int modifiers, int position, int line);
	public delegate void UpdateUiHandler(int updated);
	public delegate void ModifiedHandler(int position, int modificationType,
		size_t text, int length, int linesAdded, int line,
		int foldLevelNow, int foldLevelPrev, int token, int annotationLinesAdded);
	public delegate void MacroRecordHandler(int message, size_t wParam, ptrdiff_t lParam);
	public delegate void MarginClickHandler(int position, int modifiers, int margin);
	public delegate void NeedShownHandler(int position, int length);
	public delegate void PaintedHandler();
	public delegate void UserListSelectionHandler();
	public delegate void URIDroppedHandler();
	public delegate void DwellStartHandler(int x, int y);
	public delegate void DwellEndHandler(int x, int y);
	public delegate void ZoomHandler();
	public delegate void HotSpotClickHandler(int modifiers, int position);
	public delegate void HotSpotDoubleClickHandler(int modifiers, int position);
	public delegate void CallTipClickHandler(int position);
	public delegate void AutoCSelectionHandler(size_t text, int position);
	public delegate void IndicatorClickHandler(int modifiers, int position);
	public delegate void IndicatorReleaseHandler(int modifiers, int position);
	public delegate void AutoCCancelledHandler();
	public delegate void AutoCCharDeletedHandler();
	public delegate void HotSpotReleaseClickHandler(int modifiers, int position);
	public delegate void ChangeHandler();
	public delegate void FocusHandler(bool focus);

	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class SkeenControl sealed : public Windows::UI::Xaml::Controls::Control
	{
	public:
		SkeenControl();
		virtual void OnApplyTemplate() override;

		event StyleNeededHandler^ styleNeededEvent;
		event CharAddedHandler^ charAddedEvent;
		event SavePointReachedHandler^ savePointReachedEvent;
		event SavePointLeftHandler^ savePointLeftEvent;
		event ModifyAttemptROHandler^ modifyAttemptROEvent;
		event DoubleClickHandler^ doubleClickEvent;
		event UpdateUiHandler^ updateUiEvent;
		event ModifiedHandler^ modifiedEvent;
		event MacroRecordHandler^ macroRecordEvent;
		event MarginClickHandler^ marginClickEvent;
		event NeedShownHandler^ needShownEvent;
		event PaintedHandler^ paintedEvent;
		event UserListSelectionHandler^ userListSelectionEvent;
		event URIDroppedHandler^ uriDroppedEvent;
		event DwellStartHandler^ dwellStartEvent;
		event DwellEndHandler^ dwellEndEvent;
		event ZoomHandler^ zoomEvent;
		event HotSpotClickHandler^ hotSpotClickEvent;
		event HotSpotDoubleClickHandler^ hotSpotDoubleClickEvent;
		event CallTipClickHandler^ callTipClickEvent;
		event AutoCSelectionHandler^ autoCSelectionEvent;
		event IndicatorClickHandler^ indicatorClickEvent;
		event IndicatorReleaseHandler^ indicatorReleaseEvent;
		event AutoCCancelledHandler^ autoCCancelledEvent;
		event AutoCCharDeletedHandler^ autoCCharDeletedEvent;
		event HotSpotReleaseClickHandler^ hotSpotReleaseClickEvent;
		event ChangeHandler^ changeEvent;
		event FocusHandler^ focusEvent;

		void FireStyleNeeded(int position);
		void FireCharAdded(int ch);
		void FireSavePointReached();
		void FireSavePointLeft();
		void FireModifyAttemptRO();
		void FireDoubleClick(int modifiers, int position, int line);
		void FireUpdateUi(int updated);
		void FireModified(int position, int modificationType,
			size_t text, int length, int linesAdded, int line,
			int foldLevelNow, int foldLevelPrev, int token, int annotationLinesAdded);
		void FireMacroRecord(int message, size_t wParam, ptrdiff_t lParam);
		void FireMarginClick(int position, int modifiers, int margin);
		void FireNeedShown(int position, int length);
		void FirePainted();
		void FireUserListSelection();
		void FireURIDropped();
		void FireDwellStart(int x, int y);
		void FireDwellEnd(int x, int y);
		void FireZoom();
		void FireHotSpotClick(int modifiers, int position);
		void FireHotSpotDoubleClick(int modifiers, int position);
		void FireCallTipClick(int position);
		void FireAutoCSelection(size_t text, int position);
		void FireIndicatorClick(int modifiers, int position);
		void FireIndicatorRelease(int modifiers, int position);
		void FireAutoCCancelled();
		void FireAutoCCharDeleted();
		void FireHotSpotReleaseClick(int modifiers, int position);
		void FireChange();
		void FireFocus(bool focus);

		ptrdiff_t Call(unsigned int iMessage, size_t wParam, ptrdiff_t lParam);

		virtual void OnKeyDown(Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e) override;
		virtual void OnKeyUp(Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e) override;
		void OnTicker(Object^ sender, Object^ args);
		virtual void OnPointerPressed(Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e) override;
		virtual void OnPointerMoved(Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e) override;
		virtual void OnPointerReleased(Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e) override;
		virtual void OnGotFocus(Windows::UI::Xaml::RoutedEventArgs^ e) override;
		virtual void OnLostFocus(Windows::UI::Xaml::RoutedEventArgs^ e) override;

    private:
        Windows::UI::Xaml::Media::Imaging::VirtualSurfaceImageSource^ CreateImageSource(int width, int height);
        void OnMapSurfaceSizeChanged(Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e);
		void OnScrollViewChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::ScrollViewerViewChangedEventArgs^ e);

        // template child that holds the UI Element
        Windows::UI::Xaml::Controls::Border^                mapSurfaceElement;
        // surface image source
        Windows::UI::Xaml::Media::Imaging::VirtualSurfaceImageSource^ imageSource;
        // Native interface of the surface image source
        Microsoft::WRL::ComPtr<IVirtualSurfaceImageSourceNative>   imageSourceNative;
        // D3D device
        Microsoft::WRL::ComPtr<ID3D11Device>                d3dDevice;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext>         d3dContext;
		Microsoft::WRL::ComPtr<ID2D1Device>					d2dDevice;
		Microsoft::WRL::ComPtr<ID2D1DeviceContext>			d2dDeviceContext;

        Windows::Foundation::EventRegistrationToken         sizeChangedToken;

		// template child that holds the scroller
        Windows::UI::Xaml::Controls::ScrollViewer^ scrollView;

		Windows::UI::Xaml::DispatcherTimer ^ticktock;

		Scintilla::ScintillaWinRTBase *pSciRT;
    };
}
