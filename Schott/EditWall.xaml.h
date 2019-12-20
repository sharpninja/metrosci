//
// EditWall.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "EditWall.g.h"

class PropSetFile;
class StyleDefinition;

namespace Schott
{
	/// <summary>
	/// The main page of the Skeen demonstration application.
	/// </summary>
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class MainPage sealed
	{
		Platform::String^ sFilePath;
		Windows::Storage::StorageFile^ fileStorage;

	public:
		MainPage();
		void LoadFile(Windows::Storage::StorageFile^ fs);

	protected:
		virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;
	private:
		void SetUpControl();
		void DefineMarker(int marker, int markerType, int fore, int back);
		void SearchAndHighlight(Platform::String^ sSearch, bool fromEndOfSelection);

		void SetOneStyle(int style, const StyleDefinition &sd);
		void SetStyleBlock(const char *lang, int start, int last);
		void SetStyleFor(const char *lang);
		void LoadText(const Platform::Array<unsigned char>^ value);
		Platform::Array<unsigned char>^ GetRange(int start, int end);
		ptrdiff_t Call(unsigned int iMessage, size_t wParam=0, ptrdiff_t lParam=0);
		ptrdiff_t CallString(unsigned int iMessage, size_t wParam, const char *s);

		void OnSkeenChange();
		void OnSkeenModified(int position, int modificationType,
			size_t text, int length, int linesAdded, int line,
			int foldLevelNow, int foldLevelPrev, int token, int annotationLinesAdded);

		void TxtSearch_KeyDown(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e);
		void TxtSearch_KeyUp(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e);
		void BtnFind_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void BtnLoad_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void BtnSave_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);

		PropSetFile *propsEmbed;
	};
}
