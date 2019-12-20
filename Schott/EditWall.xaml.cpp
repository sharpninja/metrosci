//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "EditWall.xaml.h"

#include "Scintilla.h"
#include "SciLexer.h"

#include "GUI.h"

#include "SString.h"
#include "FilePath.h"
#include "PropSetFile.h"

using namespace Schott;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::Storage;
using namespace Windows::Storage::Pickers;
using namespace Windows::Storage::Streams;
using namespace Skeen;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

extern const char *props1;
extern const char *props2;
extern const char *props3;
extern const char *props4;
extern const char *props5;
extern const char *props6;

MainPage::MainPage()
{
	std::string props;
	props.reserve(400000);
	props += props1;
	props += props2;
	props += props3;
	props += props4;
	props += props5;
	props += props6;

	ImportFilter filter;
	filter.SetFilter("", "");
	propsEmbed = new PropSetFile();
	propsEmbed->Set("PLAT_WIN", "1");
	propsEmbed->ReadFromMemory(
		props.c_str(), props.size(), FilePath(), filter);

	InitializeComponent();
	SkeenMain->changeEvent += ref new ChangeHandler(this, &MainPage::OnSkeenChange);
	SkeenMain->modifiedEvent += ref new ModifiedHandler(this, &MainPage::OnSkeenModified);
	SetUpControl();
}

void MainPage::SetUpControl()
{
	const char *txtInit =
	    "int main(int argc, char **argv) {\n"
	    "    // Start up the gnome\n"
	    "    /// A documentation comment\n"
	    "    gnome_init(\"stest\", \"1.0\", argc, argv);\n}\n";

	const char *keywords =
	    "and and_eq asm auto bitand bitor bool break "
	    "case catch char class compl const const_cast continue "
	    "default delete do double dynamic_cast else enum explicit export extern false float for "
	    "friend goto if inline int long mutable namespace new not not_eq "
	    "operator or or_eq private protected public "
	    "register reinterpret_cast return short signed sizeof static static_cast struct switch "
	    "template this throw true try typedef typeid typename union unsigned using "
	    "virtual void volatile wchar_t while xor xor_eq";

	Call(SCI_SETTECHNOLOGY, SC_TECHNOLOGY_DIRECTWRITE);
	Call(SCI_SETBUFFEREDDRAW, 0);
	Call(SCI_SETFONTQUALITY, SC_EFF_QUALITY_LCD_OPTIMIZED);
	//Call(SCI_SETCARETPERIOD, 0);
	Call(SCI_STYLECLEARALL);
	Call(SCI_SETMARGINWIDTHN, 0, 35);
	Call(SCI_SETSCROLLWIDTH, 200, 0);
	Call(SCI_SETSCROLLWIDTHTRACKING, 1, 0);
	Call(SCI_SETLEXER, SCLEX_CPP, 0);
	Call(SCI_SETSTYLEBITS, 7);
	Call(SCI_SETKEYWORDS, 0, (sptr_t)keywords);

	Call(SCI_STYLESETFONT, STYLE_DEFAULT, reinterpret_cast<ptrdiff_t>("Segoe UI"));
	Call(SCI_STYLESETSIZEFRACTIONAL, STYLE_DEFAULT, 1150);
	Call(SCI_STYLESETWEIGHT, STYLE_DEFAULT, 500);
	Call(SCI_STYLECLEARALL);

	for (int style=64; style<87; style++) {
		// Preprocessor disabled styles
		Call(SCI_STYLESETBACK, style, 0xd0f5ff);
		Call(SCI_STYLESETEOLFILLED, style, 1);
	}

	for (int validity=0; validity<2; validity++) {
		int v = validity * 64;
		Call(SCI_STYLESETFORE, v+SCE_C_COMMENT, 0x008000);
		Call(SCI_STYLESETFORE, v+SCE_C_COMMENTLINE, 0x008000);
		Call(SCI_STYLESETFORE, v+SCE_C_COMMENTLINEDOC, 0x008040);
		Call(SCI_STYLESETFORE, v+SCE_C_COMMENTDOC, 0x008040);
		Call(SCI_STYLESETITALIC, v+SCE_C_COMMENTDOC, 1);
		Call(SCI_STYLESETFORE, v+SCE_C_NUMBER, 0x808000);
		Call(SCI_STYLESETFORE, v+SCE_C_WORD, 0x800000);
		Call(SCI_STYLESETBOLD, v+SCE_C_WORD, 1);
		Call(SCI_STYLESETFORE, v+SCE_C_STRING, 0x800080);
		Call(SCI_STYLESETFORE, v+SCE_C_STRINGEOL, 0x0000FF);
		Call(SCI_STYLESETFORE, v+SCE_C_PREPROCESSOR, 0x008080);
		Call(SCI_STYLESETBOLD, v+SCE_C_OPERATOR, 1);
	}

	Call(SCI_INSERTTEXT, 0, (sptr_t)(void *)txtInit);
	Call(SCI_SETSEL, 8, 4);
	Call(SCI_SETMULTIPLESELECTION, 1);
	//Call(SCI_SETVIRTUALSPACEOPTIONS,
	//     SCVS_RECTANGULARSELECTION | SCVS_USERACCESSIBLE);
	Call(SCI_SETADDITIONALSELECTIONTYPING, 1);

	Call(SCI_STYLESETFORE, STYLE_INDENTGUIDE, 0x808080);
	Call(SCI_SETINDENTATIONGUIDES, SC_IV_LOOKBOTH);
	//Call(SCI_SETVIEWWS, SCWS_VISIBLEALWAYS);

	Call(SCI_SETSELBACK, 1, 0x008080);
	Call(SCI_SETSELALPHA, 64);

	Call(SCI_SETFOLDMARGINCOLOUR, 1, 0xD0D0D0);
	Call(SCI_SETFOLDMARGINHICOLOUR, 1, 0xC0C0C0);
	Call(SCI_SETMARGINTYPEN, 2, SC_MARGIN_SYMBOL);
	//Call(SCI_SETMARGINWIDTHN, 2, 18);
	Call(SCI_SETMARGINMASKN, 2, SC_MASK_FOLDERS);
	Call(SCI_SETMARGINSENSITIVEN, 2, 1);

	Call(SCI_SETPROPERTY, (uptr_t)("fold"), (sptr_t)("1"));
	DefineMarker(SC_MARKNUM_FOLDEROPEN, SC_MARK_BOXMINUS, 0xffffff, 0x808080);
	DefineMarker(SC_MARKNUM_FOLDER, SC_MARK_BOXPLUS, 0xffffff, 0x808080);
	DefineMarker(SC_MARKNUM_FOLDERSUB, SC_MARK_VLINE, 0xffffff, 0x808080);
	DefineMarker(SC_MARKNUM_FOLDERTAIL, SC_MARK_LCORNER, 0xffffff, 0x808080);
	DefineMarker(SC_MARKNUM_FOLDEREND, SC_MARK_BOXPLUSCONNECTED, 0xffffff, 0x808080);
	DefineMarker(SC_MARKNUM_FOLDEROPENMID, SC_MARK_BOXMINUSCONNECTED, 0xffffff, 0x808080);
	DefineMarker(SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_TCORNER, 0xffffff, 0x808080);

	Call(SCI_COLOURISE, 0, -1);
}

void MainPage::DefineMarker(int marker, int markerType, int fore, int back)
{
	Call(SCI_MARKERDEFINE, marker, markerType);
	Call(SCI_MARKERSETFORE, marker, fore);
	Call(SCI_MARKERSETBACK, marker, back);
}

static SString SStringFromString(String^ s)
{
	int lenUTF8 = WideCharToMultiByte(CP_UTF8,0,s->Data(),-1,nullptr,0,nullptr,nullptr) - 1;
	if (lenUTF8) {
		SBuffer sb(lenUTF8);
		WideCharToMultiByte(CP_UTF8,0,s->Data(),-1,sb.ptr(),lenUTF8+1,nullptr,nullptr);
		return SString(sb);
	}
	return SString();
}

void Schott::MainPage::SearchAndHighlight(Platform::String^ sSearch, bool fromEndOfSelection)
{
	int startPos = fromEndOfSelection ? Call(SCI_GETSELECTIONEND) : Call(SCI_GETSELECTIONSTART);
	int length = Call(SCI_GETLENGTH);
	Call(SCI_SETTARGETSTART, startPos);
	Call(SCI_SETTARGETEND, length);
	SString ssSearch = SStringFromString(sSearch);
	if (ssSearch.length()) {
		int found = Call(SCI_SEARCHINTARGET, ssSearch.length(), reinterpret_cast<ptrdiff_t>(ssSearch.c_str()));
		if (found < 0) {
			Call(SCI_SETTARGETSTART, 0);
			Call(SCI_SETTARGETEND, startPos);
			found = Call(SCI_SEARCHINTARGET, ssSearch.length(), reinterpret_cast<ptrdiff_t>(ssSearch.c_str()));
		}
		if (found >= 0) {
			Call(SCI_SETSELECTIONSTART, found);
			Call(SCI_SETSELECTIONEND, found+ssSearch.length());
			int line = Call(SCI_LINEFROMPOSITION, found);
			Call(SCI_ENSUREVISIBLEENFORCEPOLICY, line);
			/*
			Call(SCI_SETINDICATORCURRENT, INDIC_CONTAINER);
			Call(SCI_INDICATORCLEARRANGE, 0, length);
			Call(SCI_INDICSETSTYLE, INDIC_CONTAINER, INDIC_ROUNDBOX);
			Call(SCI_INDICSETFORE, INDIC_CONTAINER, 0x008000);
			Call(SCI_INDICATORFILLRANGE, found, lenUTF8);
			*/
		}
	} else {
		// Empty search so no selection
		Call(SCI_SETSELECTIONEND, startPos);
	}
}

class StyleDefinition {
public:
	SString font;
	float sizeFractional;
	int size;
	SString fore;
	SString back;
	int weight;
	bool italics;
	bool eolfilled;
	bool underlined;
	int caseForce;
	bool visible;
	bool changeable;
	enum flags { sdNone = 0, sdFont = 0x1, sdSize = 0x2, sdFore = 0x4, sdBack = 0x8,
	        sdWeight = 0x10, sdItalics = 0x20, sdEOLFilled = 0x40, sdUnderlined = 0x80,
	        sdCaseForce = 0x100, sdVisible = 0x200, sdChangeable = 0x400} specified;
	StyleDefinition(const char *definition);
	bool ParseStyleDefinition(const char *definition);
	long ForeAsLong() const;
	long BackAsLong() const;
	int FractionalSize() const;
	bool IsBold() const;
};

void Schott::MainPage::SetOneStyle(int style, const StyleDefinition &sd) {
	if (sd.specified & StyleDefinition::sdItalics)
		Call(SCI_STYLESETITALIC, style, sd.italics ? 1 : 0);
	if (sd.specified & StyleDefinition::sdWeight)
		Call(SCI_STYLESETWEIGHT, style, sd.weight);
	if (sd.specified & StyleDefinition::sdFont)
		CallString(SCI_STYLESETFONT, style,
			const_cast<char *>(sd.font.c_str()));
	if (sd.specified & StyleDefinition::sdFore)
		Call(SCI_STYLESETFORE, style, sd.ForeAsLong());
	if (sd.specified & StyleDefinition::sdBack)
		Call(SCI_STYLESETBACK, style, sd.BackAsLong());
	if (sd.specified & StyleDefinition::sdSize)
		Call(SCI_STYLESETSIZEFRACTIONAL, style, sd.FractionalSize());
	if (sd.specified & StyleDefinition::sdEOLFilled)
		Call(SCI_STYLESETEOLFILLED, style, sd.eolfilled ? 1 : 0);
	if (sd.specified & StyleDefinition::sdUnderlined)
		Call(SCI_STYLESETUNDERLINE, style, sd.underlined ? 1 : 0);
	if (sd.specified & StyleDefinition::sdCaseForce)
		Call(SCI_STYLESETCASE, style, sd.caseForce);
	if (sd.specified & StyleDefinition::sdVisible)
		Call(SCI_STYLESETVISIBLE, style, sd.visible ? 1 : 0);
	if (sd.specified & StyleDefinition::sdChangeable)
		Call(SCI_STYLESETCHANGEABLE, style, sd.changeable ? 1 : 0);
	//Call(SCI_STYLESETCHARACTERSET, style, characterSet);
}

int IntFromHexDigit(int ch) {
	if ((ch >= '0') && (ch <= '9')) {
		return ch - '0';
	} else if (ch >= 'A' && ch <= 'F') {
		return ch - 'A' + 10;
	} else if (ch >= 'a' && ch <= 'f') {
		return ch - 'a' + 10;
	} else {
		return 0;
	}
}

int IntFromHexByte(const char *hexByte) {
	return IntFromHexDigit(hexByte[0]) * 16 + IntFromHexDigit(hexByte[1]);
}

typedef long Colour;
inline Colour ColourRGB(unsigned int red, unsigned int green, unsigned int blue) {
	return red | (green << 8) | (blue << 16);
}

static Colour ColourFromString(const SString &s) {
	if (s.length()) {
		int r = IntFromHexByte(s.c_str() + 1);
		int g = IntFromHexByte(s.c_str() + 3);
		int b = IntFromHexByte(s.c_str() + 5);
		return ColourRGB(r, g, b);
	} else {
		return 0;
	}
}

StyleDefinition::StyleDefinition(const char *definition) :
		sizeFractional(11.0), size(10), fore("#000000"), back("#FFFFFF"),
		weight(SC_WEIGHT_NORMAL), italics(false), eolfilled(false), underlined(false),
		caseForce(SC_CASE_MIXED),
		visible(true), changeable(true),
		specified(sdNone) {
	ParseStyleDefinition(definition);
}

bool StyleDefinition::ParseStyleDefinition(const char *definition) {
	if (definition == NULL || *definition == '\0') {
		return false;
	}
	char *val = StringDup(definition);
	char *opt = val;
	while (opt) {
		// Find attribute separator
		char *cpComma = strchr(opt, ',');
		if (cpComma) {
			// If found, we terminate the current attribute (opt) string
			*cpComma = '\0';
		}
		// Find attribute name/value separator
		char *colon = strchr(opt, ':');
		if (colon) {
			// If found, we terminate the current attribute name and point on the value
			*colon++ = '\0';
		}
		if (0 == strcmp(opt, "italics")) {
			specified = static_cast<flags>(specified | sdItalics);
			italics = true;
		}
		if (0 == strcmp(opt, "notitalics")) {
			specified = static_cast<flags>(specified | sdItalics);
			italics = false;
		}
		if (0 == strcmp(opt, "bold")) {
			specified = static_cast<flags>(specified | sdWeight);
			weight = SC_WEIGHT_BOLD;
		}
		if (0 == strcmp(opt, "notbold")) {
			specified = static_cast<flags>(specified | sdWeight);
			weight = SC_WEIGHT_NORMAL;
		}
		if ((0 == strcmp(opt, "weight")) && colon) {
			specified = static_cast<flags>(specified | sdWeight);
			weight = atoi(colon);
		}
		if (0 == strcmp(opt, "font")) {
			specified = static_cast<flags>(specified | sdFont);
			font = colon;
			font.substitute('|', ',');
		}
		if (0 == strcmp(opt, "fore")) {
			specified = static_cast<flags>(specified | sdFore);
			fore = colon;
		}
		if (0 == strcmp(opt, "back")) {
			specified = static_cast<flags>(specified | sdBack);
			back = colon;
		}
		if ((0 == strcmp(opt, "size")) && colon) {
			specified = static_cast<flags>(specified | sdSize);
			sizeFractional = static_cast<float>(atof(colon));
			size = static_cast<int>(sizeFractional);
		}
		if (0 == strcmp(opt, "eolfilled")) {
			specified = static_cast<flags>(specified | sdEOLFilled);
			eolfilled = true;
		}
		if (0 == strcmp(opt, "noteolfilled")) {
			specified = static_cast<flags>(specified | sdEOLFilled);
			eolfilled = false;
		}
		if (0 == strcmp(opt, "underlined")) {
			specified = static_cast<flags>(specified | sdUnderlined);
			underlined = true;
		}
		if (0 == strcmp(opt, "notunderlined")) {
			specified = static_cast<flags>(specified | sdUnderlined);
			underlined = false;
		}
		if (0 == strcmp(opt, "case")) {
			specified = static_cast<flags>(specified | sdCaseForce);
			caseForce = SC_CASE_MIXED;
			if (colon) {
				if (*colon == 'u')
					caseForce = SC_CASE_UPPER;
				else if (*colon == 'l')
					caseForce = SC_CASE_LOWER;
			}
		}
		if (0 == strcmp(opt, "visible")) {
			specified = static_cast<flags>(specified | sdVisible);
			visible = true;
		}
		if (0 == strcmp(opt, "notvisible")) {
			specified = static_cast<flags>(specified | sdVisible);
			visible = false;
		}
		if (0 == strcmp(opt, "changeable")) {
			specified = static_cast<flags>(specified | sdChangeable);
			changeable = true;
		}
		if (0 == strcmp(opt, "notchangeable")) {
			specified = static_cast<flags>(specified | sdChangeable);
			changeable = false;
		}
		if (cpComma)
			opt = cpComma + 1;
		else
			opt = 0;
	}
	delete []val;
	return true;
}

long StyleDefinition::ForeAsLong() const {
	return ColourFromString(fore);
}

long StyleDefinition::BackAsLong() const {
	return ColourFromString(back);
}

int StyleDefinition::FractionalSize() const {
	return static_cast<int>(sizeFractional * 1.2 * SC_FONT_SIZE_MULTIPLIER);
}

bool StyleDefinition::IsBold() const {
	return weight > SC_WEIGHT_NORMAL;
}

void Schott::MainPage::SetStyleBlock(const char *lang, int start, int last) {
	for (int style = start; style <= last; style++) {
		if (style != STYLE_DEFAULT) {
			char key[200];
			sprintf(key, "style.%s.%0d", lang, style-start);
			SString sval = propsEmbed->GetExpanded(key);
			if (sval.length()) {
				SetOneStyle(style, sval.c_str());
			}
		}
	}
}

void Schott::MainPage::SetStyleFor(const char *lang) {
	int maxStyle = (1 << Call(SCI_GETSTYLEBITS)) - 1;
	if (maxStyle < STYLE_LASTPREDEFINED)
		maxStyle = STYLE_LASTPREDEFINED;
	SetStyleBlock(lang, 0, maxStyle);
}

void Schott::MainPage::LoadText(const Platform::Array<unsigned char>^ value)
{
	Call(SCI_CLEARALL, 0, 0);
	Call(SCI_ADDTEXT, value->Length, reinterpret_cast<ptrdiff_t>(value->begin()));
	
	PropSetFile &props=*propsEmbed;

	SString fileNameForExtension = SStringFromString(sFilePath);
	SString language = props.GetNewExpand("lexer.", fileNameForExtension.c_str());
	if (language.length()) {
		Call(SCI_SETLEXERLANGUAGE, 0, reinterpret_cast<ptrdiff_t>(language.c_str()));
	} else {
		Call(SCI_SETLEXER, SCLEX_NULL);
	}

	SString kw0 = props.GetNewExpand("keywords.", fileNameForExtension.c_str());
	CallString(SCI_SETKEYWORDS, 0, kw0.c_str());

	for (int wl = 1; wl <= KEYWORDSET_MAX; wl++) {
		SString kwk(wl+1);
		kwk += '.';
		kwk.insert(0, "keywords");
		SString kw = props.GetNewExpand(kwk.c_str(), fileNameForExtension.c_str());
		CallString(SCI_SETKEYWORDS, wl, kw.c_str());
	}

	Call(SCI_STYLERESETDEFAULT, 0, 0);

	char key[200];
	SString sval;
	sprintf(key, "style.%s.%0d", "*", STYLE_DEFAULT);
	sval = props.GetNewExpand(key);
	SetOneStyle(STYLE_DEFAULT, sval.c_str());

	sprintf(key, "style.%s.%0d", language.c_str(), STYLE_DEFAULT);
	sval = props.GetNewExpand(key);
	SetOneStyle(STYLE_DEFAULT, sval.c_str());

	Call(SCI_STYLECLEARALL, 0, 0);

	SetStyleFor("*");
	SetStyleFor(language.c_str());
}

void Schott::MainPage::LoadFile(Windows::Storage::StorageFile^ fs)
{
	SkeenControl^ content = SkeenMain;
	Windows::UI::Core::CoreWindow ^window = Windows::UI::Core::CoreWindow::GetForCurrentThread();
	Windows::UI::Core::CoreDispatcher ^dispatcher = window->Dispatcher;
	fileStorage = fs;
	if (fileStorage) {
		sFilePath = fileStorage->Path;
		auto openOp = fileStorage->OpenSequentialReadAsync();
		concurrency::create_task(openOp).then([dispatcher, content, this] (IInputStream^ stream) {
			auto reader = ref new DataReader(stream);
			// Some mentions of file->Size but doesn't work now so just ask for 10 Megabytes
			auto loadOp = reader->LoadAsync(10000000);
			auto loadTask = concurrency::create_task(loadOp);
			loadTask.then([dispatcher, this, reader] (unsigned int countBytes) {
				Platform::Array<unsigned char>^ value = ref new Platform::Array<unsigned char>(countBytes);
				reader->ReadBytes(value);
				dispatcher->RunAsync(CoreDispatcherPriority::Normal,
					ref new DispatchedHandler([this, value](){
						LoadText(value);
				}));
			});
		});
	}
}

Platform::Array<unsigned char>^ Schott::MainPage::GetRange(int start, int end)
{
	Platform::Array<unsigned char> ^text = ref new Platform::Array<unsigned char>(end-start);
	Sci_TextRange tr;
	tr.chrg.cpMin = start;
	tr.chrg.cpMax = end;
	tr.lpstrText = reinterpret_cast<char *>(text->begin());
	Call(SCI_GETTEXTRANGE, 0, reinterpret_cast<ptrdiff_t>(&tr));
	return text;
}

ptrdiff_t MainPage::Call(unsigned int iMessage, size_t wParam, ptrdiff_t lParam)
{
	return SkeenMain->Call(iMessage, wParam, lParam);
}

ptrdiff_t MainPage::CallString(unsigned int iMessage, size_t wParam, const char *s)
{
	return SkeenMain->Call(iMessage, wParam, reinterpret_cast<ptrdiff_t>(s));
}

void MainPage::OnSkeenChange()
{
	// Demonstrate reacting to change event by changing colour of a style
	/*
	static int iiio=0;
	iiio = (iiio + 1) % 5;
	int colours[] = {0x008000, 0x800000, 0x000080, 0x808080, 0x800080};
	Call(SCI_STYLESETFORE, 2, colours[iiio]);
	*/
}

void MainPage::OnSkeenModified(int position, int modificationType,
	size_t text, int length, int linesAdded, int line,
	int foldLevelNow, int foldLevelPrev, int token, int annotationLinesAdded) {
}

/// <summary>
/// Invoked when this page is about to be displayed in a Frame.
/// </summary>
/// <param name="e">Event data that describes how this page was reached.  The Parameter
/// property is typically used to configure the page.</param>
void MainPage::OnNavigatedTo(NavigationEventArgs^ e)
{
	(void) e;	// Unused parameter
}

void Schott::MainPage::TxtSearch_KeyDown(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e)
{
}

void Schott::MainPage::TxtSearch_KeyUp(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e)
{
	Platform::String^ sSearch = TxtSearch->Text;
	SkeenControl^ content = SkeenMain;
	SearchAndHighlight(sSearch, false);
}


void Schott::MainPage::BtnFind_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	Platform::String^ sSearch = TxtSearch->Text;
	SkeenControl^ content = SkeenMain;
	SearchAndHighlight(sSearch, true);
}

// For asynchronous WinRT techniques in C++ see
// http://msdn.microsoft.com/en-us/library/windows/apps/hh780559.aspx

void Schott::MainPage::BtnLoad_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	auto openPicker = ref new FileOpenPicker();
	openPicker->ViewMode = PickerViewMode::List;
	openPicker->SuggestedStartLocation = PickerLocationId::DocumentsLibrary;
	SString sourceFiles = propsEmbed->Get("source.files");
	sourceFiles += ';';
	sourceFiles.substitute(';','\0');
	const char *ext = sourceFiles.c_str();
	while (*ext) {
		if (*ext == '*') {
			GUI::gui_string uExt = GUI::StringFromUTF8(ext+1);
			String^ sExt = ref new String(uExt.c_str());
			openPicker->FileTypeFilter->Append(sExt);
		}
		ext += strlen(ext) + 1;
	}
	/*
	openPicker->FileTypeFilter->Append(".cxx");
	openPicker->FileTypeFilter->Append(".cpp");
	openPicker->FileTypeFilter->Append(".c");
	openPicker->FileTypeFilter->Append(".cc");
	openPicker->FileTypeFilter->Append(".m");
	openPicker->FileTypeFilter->Append(".mm");
	openPicker->FileTypeFilter->Append(".h");
	*/

	auto pickOp = openPicker->PickSingleFileAsync();
	concurrency::create_task(pickOp).then([this] (StorageFile ^fs) {
		LoadFile(fs);
	});
}

void Schott::MainPage::BtnSave_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	int lengthText = Call(SCI_GETLENGTH);
	Platform::Array<unsigned char>^ text = GetRange(0, lengthText);

	auto writeOp = FileIO::WriteBytesAsync(fileStorage, text);
	auto writeTask = concurrency::create_task(writeOp);
	// Seems to block if no "then"
	writeTask.then([] () {});
}
