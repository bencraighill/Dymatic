#include "Themes.h"

namespace Dymatic {

	std::string GetStringFromTheme(ColorSchemeType theme)
	{
		if (theme == Text) { return "Text"; }
		else if (theme == TextDisabled) { return "Text Disabled"; }
		else if (theme == TextSelectedBg) { return "Text Selected Background"; }
		else if (theme == WindowBg) { return "Window Background"; }
		else if (theme == MenuBarBg) { return "Menu Bar Background"; }
		else if (theme == MenuBarGrip) { return "Menu Bar Grip"; }
		else if (theme == MenuBarGripBorder) { return "Menu Bar Grip Border"; }
		else if (theme == Header) { return "Header"; }
		else if (theme == HeaderHovered) { return "Header Hovered"; }
		else if (theme == HeaderActive) { return "Header Active"; }
		else if (theme == Tab) { return "Tab"; }
		else if (theme == TabHovered) { return "Tab Hovered"; }
		else if (theme == TabActive) { return "Tab Active"; }
		else if (theme == TabUnfocused) { return "Tab Unfocused"; }
		else if (theme == TabUnfocusedActive) { return "Tab Unfocused Active"; }
		else if (theme == TitleBg) { return "Title Background"; }
		else if (theme == TitleBgActive) { return "Title Background Active"; }
		else if (theme == TitleBgCollapsed) { return "Title Background Collapsed"; }
		else if (theme == Button) { return "Button"; }
		else if (theme == ButtonHovered) { return "Button Hovered"; }
		else if (theme == ButtonActive) { return "Button Active"; }
		else if (theme == ButtonToggled) { return "Button Toggled"; }
		else if (theme == ButtonToggledHovered) { return "Button Toggled & Hovered"; }
		else if (theme == PopupBg) { return "Popup Background"; }
		else if (theme == ModalWindowDimBg) { return "Modal Window Dim Background"; }
		else if (theme == Border) { return "Border"; }
		else if (theme == BorderShadow) { return "Border Shadow"; }
		else if (theme == FrameBg) { return "Frame Background"; }
		else if (theme == FrameBgHovered) { return "Frame Background Hovered"; }
		else if (theme == FrameBgActive) { return "Frame Background Active"; }
		else if (theme == ScrollbarBg) { return "Scroll Bar Background"; }
		else if (theme == ScrollbarGrab) { return "Scroll Bar Grab"; }
		else if (theme == ScrollbarGrabHovered) { return "Scroll Bar Grab Hovered"; }
		else if (theme == ScrollbarGrabActive) { return "Scroll Bar Grab Active"; }
		else if (theme == ScrollbarDots) { return "Scroll Bar Dots"; }
		else if (theme == ProgressBarBg) { return "Progress Bar Background"; }
		else if (theme == ProgressBarBorder) { return "Progress Bar Border"; }
		else if (theme == ProgressBarFill) { return "Progress Bar Fill"; }
		else if (theme == SliderGrab) { return "Slider Grab"; }
		else if (theme == SliderGrabActive) { return "Slider Grab Active"; }
		else if (theme == Separator) { return "Separator"; }
		else if (theme == SeparatorHovered) { return "Separator Hovered"; }
		else if (theme == SeparatorActive) { return "Separator Active"; }
		else if (theme == ResizeGrip) { return "Resize Grip"; }
		else if (theme == ResizeGripHovered) { return "Resize Grip Hovered"; }
		else if (theme == ResizeGripActive) { return "Resize Grip Active"; }
		else if (theme == FileBackground) { return "File Background"; }
		else if (theme == FileIcon) { return "File Icon"; }
		else if (theme == FileHovered) { return "File Hovered"; }
		else if (theme == FileSelected) { return "File Selected"; }
		else if (theme == TextEditorDefault) { return "Default"; }
		else if (theme == TextEditorKeyword) { return "Keyword"; }
		else if (theme == TextEditorNumber) { return "Number"; }
		else if (theme == TextEditorString) { return "String"; }
		else if (theme == TextEditorCharLiteral) { return "Character Literal"; }
		else if (theme == TextEditorPunctuation) { return "Punctuation"; }
		else if (theme == TextEditorPreprocessor) { return "Preprocessor"; }
		else if (theme == TextEditorIdentifier) { return "Identifier"; }
		else if (theme == TextEditorComment) { return "Comment"; }
		else if (theme == TextEditorMultiLineComment) { return "Multiline Comment"; }
		else if (theme == TextEditorLineNumber) { return "Line Number"; }
		else if (theme == TextEditorCurrentLineFill) { return "Current Line Fill"; }
		else if (theme == TextEditorCurrentLineFillInactive) { return "Current Line Fill Inactive"; }
		else if (theme == TextEditorCurrentLineEdge) { return "Current Line Edge"; }
		else if (theme == CheckMark) { return "Check Mark"; }
		else if (theme == Checkbox) { return "Checkbox"; }
		else if (theme == CheckboxHovered) { return "Checkbox Hovered"; }
		else if (theme == CheckboxActive) { return "Checkbox Active"; }
		else if (theme == CheckboxTicked) { return "Checkbox Ticked"; }
		else if (theme == CheckboxHoveredTicked) { return "Check Box Hovered Ticked"; }
		else if (theme == DockingPreview) { return "Docking Preview"; }
		else if (theme == DockingEmptyBg) { return "Docking Empty Background"; }
		else if (theme == PlotLines) { return "Plot Lines"; }
		else if (theme == PlotLinesHovered) { return "Plot Lines Hovered"; }
		else if (theme == PlotHistogram) { return "Plot Histogram"; }
		else if (theme == PlotHistogramHovered) { return "Plot Histogram Hovered"; }
		else if (theme == DragDropTarget) { return "Drag Drop Target"; }
		else if (theme == NavHighlight) { return "Nav Highlight"; }
		else if (theme == NavWindowingHighlight) { return "Nav Windowing Highlight"; }
		else if (theme == NavWindowingDimBg) { return "Nav Windowing Dim Background"; }

		DY_CORE_ASSERT(false, "Theme color variable, does not exist");
		return "";
	}

	ColorSchemeType GetThemeFromString(std::string string)
	{
		if (string == "Text") { return Text; }
		else if (string == "Text Disabled") { return TextDisabled; }
		else if (string == "Text Selected Background") { return TextSelectedBg; }
		else if (string == "Window Background") { return WindowBg; }
		else if (string == "Menu Bar Background") { return MenuBarBg; }
		else if (string == "Menu Bar Grip") { return MenuBarGrip; }
		else if (string == "Menu Bar Grip Border") { return MenuBarGripBorder; }
		else if (string == "Header") { return Header; }
		else if (string == "Header Hovered") { return HeaderHovered; }
		else if (string == "Header Active") { return HeaderActive; }
		else if (string == "Tab") { return Tab; }
		else if (string == "Tab Hovered") { return TabHovered; }
		else if (string == "Tab Active") { return TabActive; }
		else if (string == "Tab Unfocused") { return TabUnfocused; }
		else if (string == "Tab Unfocused Active") { return TabUnfocusedActive; }
		else if (string == "Title Background") { return TitleBg; }
		else if (string == "Title Background Active") { return TitleBgActive; }
		else if (string == "Title Background Collapsed") { return TitleBgCollapsed; }
		else if (string == "Button") { return Button; }
		else if (string == "Button Hovered") { return ButtonHovered; }
		else if (string == "Button Active") { return ButtonActive; }
		else if (string == "Button Toggled") { return ButtonToggled; }
		else if (string == "Button Toggled & Hovered") { return ButtonToggledHovered; }
		else if (string == "Popup Background") { return PopupBg; }
		else if (string == "Modal Window Dim Background") { return ModalWindowDimBg; }
		else if (string == "Border") { return Border; }
		else if (string == "Border Shadow") { return BorderShadow; }
		else if (string == "Frame Background") { return FrameBg; }
		else if (string == "Frame Background Hovered") { return FrameBgHovered; }
		else if (string == "Frame Background Active") { return FrameBgActive; }
		else if (string == "Scroll Bar Background") { return ScrollbarBg; }
		else if (string == "Scroll Bar Grab") { return ScrollbarGrab; }
		else if (string == "Scroll Bar Grab Hovered") { return ScrollbarGrabHovered; }
		else if (string == "Scroll Bar Grab Active") { return ScrollbarGrabActive; }
		else if (string == "Scroll Bar Dots") { return ScrollbarDots; }
		else if (string == "Progress Bar Background") { return ProgressBarBg; }
		else if (string == "Progress Bar Border") { return ProgressBarBorder; }
		else if (string == "Progress Bar Fill") { return ProgressBarFill; }
		else if (string == "Slider Grab") { return SliderGrab; }
		else if (string == "Slider Grab Active") { return SliderGrabActive; }
		else if (string == "Separator") { return Separator; }
		else if (string == "Separator Hovered") { return SeparatorHovered; }
		else if (string == "Separator Active") { return SeparatorActive; }
		else if (string == "Resize Grip") { return ResizeGrip; }
		else if (string == "Resize Grip Hovered") { return ResizeGripHovered; }
		else if (string == "Resize Grip Active") { return ResizeGripActive; }
		else if (string == "File Background") { return FileBackground; }
		else if (string == "File Icon") { return FileIcon; }
		else if (string == "File Hovered") { return FileHovered; }
		else if (string == "File Selected") { return FileSelected; }
		else if (string == "Default") { return TextEditorDefault; }
		else if (string == "Keyword") { return TextEditorKeyword; }
		else if (string == "Number") { return TextEditorNumber; }
		else if (string == "String") { return TextEditorString; }
		else if (string == "Character Literal") { return TextEditorCharLiteral; }
		else if (string == "Punctuation") { return TextEditorPunctuation; }
		else if (string == "Preprocessor") { return TextEditorPreprocessor; }
		else if (string == "Identifier") { return TextEditorIdentifier; }
		else if (string == "Comment") { return TextEditorComment; }
		else if (string == "Multiline Comment") { return TextEditorMultiLineComment; }
		else if (string == "Line Number") { return TextEditorLineNumber; }
		else if (string == "Current Line Fill") { return TextEditorCurrentLineFill; }
		else if (string == "Current Line Fill Inactive") { return TextEditorCurrentLineFillInactive; }
		else if (string == "Current Line Edge") { return TextEditorCurrentLineEdge; }
		else if (string == "Check Mark") { return CheckMark; }
		else if (string == "Checkbox") { return Checkbox; }
		else if (string == "Checkbox Hovered") { return CheckboxHovered; }
		else if (string == "Checkbox Active") { return CheckboxActive; }
		else if (string == "Checkbox Ticked") { return CheckboxTicked; }
		else if (string == "Check Box Hovered Ticked") { return CheckboxHoveredTicked; }
		else if (string == "Docking Preview") { return DockingPreview; }
		else if (string == "Docking Empty Background") { return DockingEmptyBg; }
		else if (string == "Plot Lines") { return PlotLines; }
		else if (string == "Plot Lines Hovered") { return PlotLinesHovered; }
		else if (string == "Plot Histogram") { return PlotHistogram; }
		else if (string == "Plot Histogram Hovered") { return PlotHistogramHovered; }
		else if (string == "Drag Drop Target") { return DragDropTarget; }
		else if (string == "Nav Highlight") { return NavHighlight; }
		else if (string == "Nav Windowing Highlight") { return NavWindowingHighlight; }
		else if (string == "Nav Windowing Dim Background") { return NavWindowingDimBg; }

		DY_CORE_ASSERT(false, "Theme color variable, does not exist");
		return {};
	}
}
