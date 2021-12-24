#include "TextEditor.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <algorithm>
#include <chrono>
#include <string>
#include <regex>
#include <cmath>

#include "Dymatic.h"
#include "Dymatic/Core/Base.h"

#include "../TextSymbols.h"

//Opening Files
#include "Dymatic/Utils/PlatformUtils.h"
#include "Dymatic/Core/Input.h"

// TODO
// - multiline comments vs single-line: latter is blocking start of a ML

namespace TextEditorInternal {

template<class InputIt1, class InputIt2, class BinaryPredicate>
bool equals(InputIt1 first1, InputIt1 last1,
	InputIt2 first2, InputIt2 last2, BinaryPredicate p)
{
	for (; first1 != last1 && first2 != last2; ++first1, ++first2)
	{
		if (!p(*first1, *first2))
			return false;
	}
	return first1 == last1 && first2 == last2;
}

TextEditor::TextEditor()
	: mLineSpacing(1.0f)
	, mUndoIndex(0)
	, mTabSize(4)
	, mOverwrite(false)
	, mReadOnly(false)
	, mWithinRender(false)
	, mScrollToCursor(false)
	, mScrollToTop(false)
	, mTextChanged(false)
	, mColorizerEnabled(true)
	, mTextStart(20.0f)
	, mLeftMargin(10)
	, mCursorPositionChanged(false)
	, mColorRangeMin(0)
	, mColorRangeMax(0)
	, mSelectionMode(SelectionMode::Normal)
	, mCheckComments(true)
	, mLastClick(-1.0f)
	, mHandleKeyboardInputs(true)
	, mHandleMouseInputs(true)
	, mIgnoreImGuiChild(false)
	, mShowWhitespaces(true)
	, mStartTime(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())
{
	SetLanguageDefinition(LanguageDefinition::None());
	mLines.push_back(Line());
}

TextEditor::~TextEditor()
{
}

void TextEditor::SetLanguageDefinition(const LanguageDefinition& aLanguageDef)
{
	mLanguageDefinition = aLanguageDef;
	mRegexList.clear();

	for (auto& r : mLanguageDefinition.mTokenRegexStrings)
		mRegexList.push_back(std::make_pair(std::regex(r.first, std::regex_constants::optimize), r.second));

	Colorize();
}

std::string TextEditor::GetText(const Coordinates& aStart, const Coordinates& aEnd) const
{
	std::string result;

	auto lstart = aStart.mLine;
	auto lend = aEnd.mLine;
	auto istart = GetCharacterIndex(aStart);
	auto iend = GetCharacterIndex(aEnd);
	size_t s = 0;

	for (size_t i = lstart; i < lend; i++)
		s += mLines[i].size();

	result.reserve(s + s / 8);

	while (istart < iend || lstart < lend)
	{
		if (lstart >= (int)mLines.size())
			break;

		auto& line = mLines[lstart];
		if (istart < (int)line.size())
		{
			result += line[istart].mChar;
			istart++;
		}
		else
		{
			istart = 0;
			++lstart;
			result += '\n';
		}
	}

	return result;
}

TextEditor::Coordinates TextEditor::GetActualCursorCoordinates() const
{
	return SanitizeCoordinates(mState.mCursorPosition);
}

TextEditor::Coordinates TextEditor::SanitizeCoordinates(const Coordinates& aValue) const
{
	auto line = aValue.mLine;
	auto column = aValue.mColumn;
	if (line >= (int)mLines.size())
	{
		if (mLines.empty())
		{
			line = 0;
			column = 0;
		}
		else
		{
			line = (int)mLines.size() - 1;
			column = GetLineMaxColumn(line);
		}
		return Coordinates(line, column);
	}
	else
	{
		column = mLines.empty() ? 0 : std::min(column, GetLineMaxColumn(line));
		return Coordinates(line, column);
	}
}

// https://en.wikipedia.org/wiki/UTF-8
// We assume that the char is a standalone character (<128) or a leading byte of an UTF-8 code sequence (non-10xxxxxx code)
static int UTF8CharLength(TextEditor::Char c)
{
	if ((c & 0xFE) == 0xFC)
		return 6;
	if ((c & 0xFC) == 0xF8)
		return 5;
	if ((c & 0xF8) == 0xF0)
		return 4;
	else if ((c & 0xF0) == 0xE0)
		return 3;
	else if ((c & 0xE0) == 0xC0)
		return 2;
	return 1;
}

// "Borrowed" from ImGui source
static inline int ImTextCharToUtf8(char* buf, int buf_size, unsigned int c)
{
	if (c < 0x80)
	{
		buf[0] = (char)c;
		return 1;
	}
	if (c < 0x800)
	{
		if (buf_size < 2) return 0;
		buf[0] = (char)(0xc0 + (c >> 6));
		buf[1] = (char)(0x80 + (c & 0x3f));
		return 2;
	}
	if (c >= 0xdc00 && c < 0xe000)
	{
		return 0;
	}
	if (c >= 0xd800 && c < 0xdc00)
	{
		if (buf_size < 4) return 0;
		buf[0] = (char)(0xf0 + (c >> 18));
		buf[1] = (char)(0x80 + ((c >> 12) & 0x3f));
		buf[2] = (char)(0x80 + ((c >> 6) & 0x3f));
		buf[3] = (char)(0x80 + ((c) & 0x3f));
		return 4;
	}
	//else if (c < 0x10000)
	{
		if (buf_size < 3) return 0;
		buf[0] = (char)(0xe0 + (c >> 12));
		buf[1] = (char)(0x80 + ((c >> 6) & 0x3f));
		buf[2] = (char)(0x80 + ((c) & 0x3f));
		return 3;
	}
}

void TextEditor::Advance(Coordinates& aCoordinates) const
{
	if (aCoordinates.mLine < (int)mLines.size())
	{
		auto& line = mLines[aCoordinates.mLine];
		auto cindex = GetCharacterIndex(aCoordinates);

		if (cindex + 1 < (int)line.size())
		{
			auto delta = UTF8CharLength(line[cindex].mChar);
			cindex = std::min(cindex + delta, (int)line.size() - 1);
		}
		else
		{
			++aCoordinates.mLine;
			cindex = 0;
		}
		aCoordinates.mColumn = GetCharacterColumn(aCoordinates.mLine, cindex);
	}
}

void TextEditor::DeleteRange(const Coordinates& aStart, const Coordinates& aEnd)
{
	assert(aEnd >= aStart);
	assert(!mReadOnly);

	//printf("D(%d.%d)-(%d.%d)\n", aStart.mLine, aStart.mColumn, aEnd.mLine, aEnd.mColumn);

	if (aEnd == aStart)
		return;

	auto start = GetCharacterIndex(aStart);
	auto end = GetCharacterIndex(aEnd);

	if (aStart.mLine == aEnd.mLine)
	{
		auto& line = mLines[aStart.mLine];
		auto n = GetLineMaxColumn(aStart.mLine);
		if (aEnd.mColumn >= n)
			line.erase(line.begin() + start, line.end());
		else
			line.erase(line.begin() + start, line.begin() + end);
	}
	else
	{
		auto& firstLine = mLines[aStart.mLine];
		auto& lastLine = mLines[aEnd.mLine];

		firstLine.erase(firstLine.begin() + start, firstLine.end());
		lastLine.erase(lastLine.begin(), lastLine.begin() + end);

		if (aStart.mLine < aEnd.mLine)
			firstLine.insert(firstLine.end(), lastLine.begin(), lastLine.end());

		if (aStart.mLine < aEnd.mLine)
			RemoveLine(aStart.mLine + 1, aEnd.mLine + 1);
	}

	mTextChanged = true;
}

int TextEditor::InsertTextAt(Coordinates& /* inout */ aWhere, const char* aValue)
{
	assert(!mReadOnly);

	int cindex = GetCharacterIndex(aWhere);
	int totalLines = 0;
	while (*aValue != '\0')
	{
		assert(!mLines.empty());

		if (*aValue == '\r')
		{
			// skip
			++aValue;
		}
		else if (*aValue == '\n')
		{
			if (cindex < (int)mLines[aWhere.mLine].size())
			{
				auto& newLine = InsertLine(aWhere.mLine + 1);
				auto& line = mLines[aWhere.mLine];
				newLine.insert(newLine.begin(), line.begin() + cindex, line.end());
				line.erase(line.begin() + cindex, line.end());
			}
			else
			{
				InsertLine(aWhere.mLine + 1);
			}
			++aWhere.mLine;
			aWhere.mColumn = 0;
			cindex = 0;
			++totalLines;
			++aValue;
		}
		else
		{
			auto& line = mLines[aWhere.mLine];
			auto d = UTF8CharLength(*aValue);
			while (d-- > 0 && *aValue != '\0')
				line.insert(line.begin() + cindex++, Glyph(*aValue++, ImGuiCol_TextEditorDefault));
			++aWhere.mColumn;
		}

		mTextChanged = true;
	}

	return totalLines;
}

void TextEditor::AddUndo(UndoRecord& aValue)
{
	assert(!mReadOnly);
	//printf("AddUndo: (@%d.%d) +\'%s' [%d.%d .. %d.%d], -\'%s', [%d.%d .. %d.%d] (@%d.%d)\n",
	//	aValue.mBefore.mCursorPosition.mLine, aValue.mBefore.mCursorPosition.mColumn,
	//	aValue.mAdded.c_str(), aValue.mAddedStart.mLine, aValue.mAddedStart.mColumn, aValue.mAddedEnd.mLine, aValue.mAddedEnd.mColumn,
	//	aValue.mRemoved.c_str(), aValue.mRemovedStart.mLine, aValue.mRemovedStart.mColumn, aValue.mRemovedEnd.mLine, aValue.mRemovedEnd.mColumn,
	//	aValue.mAfter.mCursorPosition.mLine, aValue.mAfter.mCursorPosition.mColumn
	//	);

	mUndoBuffer.resize((size_t)(mUndoIndex + 1));
	mUndoBuffer.back() = aValue;
	++mUndoIndex;
}

TextEditor::Coordinates TextEditor::ScreenPosToCoordinates(const ImVec2& aPosition) const
{
	ImVec2 origin = ImGui::GetCursorScreenPos();
	ImVec2 local(aPosition.x - origin.x, aPosition.y - origin.y);

	int lineNo = std::max(0, (int)floor(local.y / mCharAdvance.y));

	if (lineNo >= mLines.size())
	{
		return SanitizeCoordinates(Coordinates(lineNo, 0));
	}

	int columnCoord = 0;

	if (lineNo >= 0 && lineNo < (int)mLines.size())
	{
		auto& line = mLines.at(lineNo);

		int columnIndex = 0;
		float columnX = 0.0f;

		while ((size_t)columnIndex < line.size())
		{
			float columnWidth = 0.0f;

			if (line[columnIndex].mChar == '\t')
			{
				float spaceSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, " ").x;
				float oldX = columnX;
				float newColumnX = (1.0f + std::floor((1.0f + columnX) / (float(mTabSize) * spaceSize))) * (float(mTabSize) * spaceSize);
				columnWidth = newColumnX - oldX;
				if (mTextStart + columnX + columnWidth * 0.5f > local.x)
					break;
				columnX = newColumnX;
				columnCoord = (columnCoord / mTabSize) * mTabSize + mTabSize;
				columnIndex++;
			}
			else
			{
				char buf[7];
				auto d = UTF8CharLength(line[columnIndex].mChar);
				int i = 0;
				while (i < 6 && d-- > 0)
					buf[i++] = line[columnIndex++].mChar;
				buf[i] = '\0';
				columnWidth = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, buf).x;
				if (mTextStart + columnX + columnWidth * 0.5f > local.x)
					break;
				columnX += columnWidth;
				columnCoord++;
			}
		}
	}

	return SanitizeCoordinates(Coordinates(lineNo, columnCoord));
}

TextEditor::Coordinates TextEditor::FindWordStart(const Coordinates& aFrom) const
{
	Coordinates at = aFrom;
	if (at.mLine >= (int)mLines.size())
		return at;

	auto& line = mLines[at.mLine];
	auto cindex = GetCharacterIndex(at);

	if (cindex >= (int)line.size())
		return at;

	while (cindex > 0 && isspace(line[cindex].mChar))
		--cindex;

	auto cstart = (ImGuiCol_)line[cindex].mColorIndex;
	while (cindex > 0)
	{
		auto c = line[cindex].mChar;
		if ((c & 0xC0) != 0x80)	// not UTF code sequence 10xxxxxx
		{
			if (c <= 32 && isspace(c))
			{
				cindex++;
				break;
			}
			if (cstart != (ImGuiCol_)line[size_t(cindex - 1)].mColorIndex)
				break;
		}
		--cindex;
	}
	return Coordinates(at.mLine, GetCharacterColumn(at.mLine, cindex));
}

TextEditor::Coordinates TextEditor::FindWordEnd(const Coordinates& aFrom) const
{
	Coordinates at = aFrom;
	if (at.mLine >= (int)mLines.size())
		return at;

	auto& line = mLines[at.mLine];
	auto cindex = GetCharacterIndex(at);

	if (cindex >= (int)line.size())
		return at;

	bool prevspace = (bool)isspace(line[cindex].mChar);
	auto cstart = (ImGuiCol_)line[cindex].mColorIndex;
	while (cindex < (int)line.size())
	{
		auto c = line[cindex].mChar;
		auto d = UTF8CharLength(c);
		if (cstart != (ImGuiCol_)line[cindex].mColorIndex)
			break;

		if (prevspace != !!isspace(c))
		{
			if (isspace(c))
				while (cindex < (int)line.size() && isspace(line[cindex].mChar))
					++cindex;
			break;
		}
		cindex += d;
	}
	return Coordinates(aFrom.mLine, GetCharacterColumn(aFrom.mLine, cindex));
}

TextEditor::Coordinates TextEditor::FindNextWord(const Coordinates& aFrom) const
{
	Coordinates at = aFrom;
	if (at.mLine >= (int)mLines.size())
		return at;

	// skip to the next non-word character
	auto cindex = GetCharacterIndex(aFrom);
	bool isword = false;
	bool skip = false;
	if (cindex < (int)mLines[at.mLine].size())
	{
		auto& line = mLines[at.mLine];
		isword = isalnum(line[cindex].mChar);
		skip = isword;
	}

	while (!isword || skip)
	{
		if (at.mLine >= mLines.size())
		{
			auto l = std::max(0, (int)mLines.size() - 1);
			return Coordinates(l, GetLineMaxColumn(l));
		}

		auto& line = mLines[at.mLine];
		if (cindex < (int)line.size())
		{
			isword = isalnum(line[cindex].mChar);

			if (isword && !skip)
				return Coordinates(at.mLine, GetCharacterColumn(at.mLine, cindex));

			if (!isword)
				skip = false;

			cindex++;
		}
		else
		{
			cindex = 0;
			++at.mLine;
			skip = false;
			isword = false;
		}
	}

	return at;
}

int TextEditor::GetCharacterIndex(const Coordinates& aCoordinates) const
{
	if (aCoordinates.mLine >= mLines.size())
		return -1;
	auto& line = mLines[aCoordinates.mLine];
	int c = 0;
	int i = 0;
	for (; i < line.size() && c < aCoordinates.mColumn;)
	{
		if (line[i].mChar == '\t')
			c = (c / mTabSize) * mTabSize + mTabSize;
		else
			++c;
		i += UTF8CharLength(line[i].mChar);
	}
	return i;
}

int TextEditor::GetCharacterColumn(int aLine, int aIndex) const
{
	if (aLine >= mLines.size())
		return 0;
	auto& line = mLines[aLine];
	int col = 0;
	int i = 0;
	while (i < aIndex && i < (int)line.size())
	{
		auto c = line[i].mChar;
		i += UTF8CharLength(c);
		if (c == '\t')
			col = (col / mTabSize) * mTabSize + mTabSize;
		else
			col++;
	}
	return col;
}

int TextEditor::GetLineCharacterCount(int aLine) const
{
	if (aLine >= mLines.size())
		return 0;
	auto& line = mLines[aLine];
	int c = 0;
	for (unsigned i = 0; i < line.size(); c++)
		i += UTF8CharLength(line[i].mChar);
	return c;
}

int TextEditor::GetLineMaxColumn(int aLine) const
{
	if (aLine >= mLines.size())
		return 0;
	auto& line = mLines[aLine];
	int col = 0;
	for (unsigned i = 0; i < line.size(); )
	{
		auto c = line[i].mChar;
		if (c == '\t')
			col = (col / mTabSize) * mTabSize + mTabSize;
		else
			col++;
		i += UTF8CharLength(c);
	}
	return col;
}

bool TextEditor::IsOnWordBoundary(const Coordinates& aAt) const
{
	if (aAt.mLine >= (int)mLines.size() || aAt.mColumn == 0)
		return true;

	auto& line = mLines[aAt.mLine];
	auto cindex = GetCharacterIndex(aAt);
	if (cindex >= (int)line.size())
		return true;

	if (mColorizerEnabled)
		return line[cindex].mColorIndex != line[size_t(cindex - 1)].mColorIndex;

	return isspace(line[cindex].mChar) != isspace(line[cindex - 1].mChar);
}

void TextEditor::RemoveLine(int aStart, int aEnd)
{
	assert(!mReadOnly);
	assert(aEnd >= aStart);
	assert(mLines.size() > (size_t)(aEnd - aStart));

	ErrorMarkers etmp;
	for (auto& i : mErrorMarkers)
	{
		if (i.first >= aStart && i.first < aEnd)
			continue;

		int num = aEnd - aStart;

		int ypos = i.first;

		if (ypos >= aEnd)
		{
			ypos -= num;
		}

		ErrorMarkers::value_type e(ypos, i.second);
		etmp.insert(e);
	}
	mErrorMarkers = std::move(etmp);

	Breakpoints btmp;
	for (auto i : mBreakpoints)
	{
		if (i >= aStart && i < aEnd)
			continue;

		int num = aEnd - aStart;

		int ypos = i;

		if (ypos >= aEnd)
		{
			ypos -= num;
		}

		btmp.insert(ypos);
	}
	mBreakpoints = std::move(btmp);

	mLines.erase(mLines.begin() + aStart, mLines.begin() + aEnd);

	assert(!mLines.empty());

	mTextChanged = true;
}

void TextEditor::RemoveLine(int aIndex)
{
	assert(!mReadOnly);
	assert(mLines.size() > 1);

	auto coord = GetActualCursorCoordinates();

	ErrorMarkers etmp;
	for (auto& i : mErrorMarkers)
	{
		///if we delete from the 0th character of this line
		///or the 0th character of the *next* line, preserve
		if (i.first == coord.mLine)
		{
			if (aIndex == coord.mLine)
			{
				if (coord.mColumn == 0)
				{
					ErrorMarkers::value_type e(i.first, i.second);
					etmp.insert(e);
				}

				///deleting anywhere else removes the breakpoint
			}

			continue;
		}
		ErrorMarkers::value_type e(i.first > aIndex ? i.first - 1 : i.first, i.second);
		etmp.insert(e);
	}
	mErrorMarkers = std::move(etmp);

	Breakpoints btmp;
	for (auto i : mBreakpoints)
	{
		if (i == coord.mLine)
		{
			if (aIndex == coord.mLine)
			{
				if (coord.mColumn == 0)
				{
					btmp.insert(i);
				}

				///deleting anywhere else removes the breakpoint
			}

			continue;
		}

		btmp.insert(i > aIndex ? i - 1 : i);
	}
	mBreakpoints = std::move(btmp);

	mLines.erase(mLines.begin() + aIndex);
	assert(!mLines.empty());

	mTextChanged = true;
}

TextEditor::Line& TextEditor::InsertLine(int aIndex)
{
	assert(!mReadOnly);

	auto coord = GetActualCursorCoordinates();

	auto& result = *mLines.insert(mLines.begin() + aIndex, Line());

	ErrorMarkers etmp;
	for (auto& i : mErrorMarkers)
	{
		if (i.first == coord.mLine + 1)
		{
			if (aIndex == coord.mLine + 1)
			{
				auto& line = mLines[coord.mLine];

				if (coord.mColumn == ((int)line.size()))
				{
					ErrorMarkers::value_type next(i.first, i.second);
					etmp.insert(next);
					continue;
				}

				else if (coord.mColumn == 0)
				{
					ErrorMarkers::value_type next(i.first + 1, i.second);
					etmp.insert(next);
				}
			}
			
			continue;
		}

		ErrorMarkers::value_type e(i.first >= aIndex ? i.first + 1 : i.first, i.second);

		etmp.insert(e);
	}
	mErrorMarkers = std::move(etmp);

	Breakpoints btmp;
	for (auto& i : mBreakpoints)
	{
		if (i == coord.mLine + 1)
		{
			if (aIndex == coord.mLine + 1)
			{
				auto& line = mLines[coord.mLine];

				if (coord.mColumn == ((int)line.size()))
				{
					btmp.insert(i);
				}

				else if (coord.mColumn == 0)
				{
					btmp.insert(i + 1);
				}
			}

			continue;
		}


		btmp.insert(i >= aIndex ? i + 1 : i);
	}

	mBreakpoints = std::move(btmp);

	return result;
}

std::string TextEditor::GetWordUnderCursor() const
{
	auto c = GetCursorPosition();
	return GetWordAt(c);
}

std::string TextEditor::GetWordAt(const Coordinates& aCoords) const
{
	auto start = FindWordStart(aCoords);
	auto end = FindWordEnd(aCoords);

	std::string r;

	auto istart = GetCharacterIndex(start);
	auto iend = GetCharacterIndex(end);

	for (auto it = istart; it < iend; ++it)
		r.push_back(mLines[aCoords.mLine][it].mChar);

	return r;
}

ImU32 TextEditor::GetGlyphColor(const Glyph& aGlyph) const
{
	if (!mColorizerEnabled)
		return ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_TextEditorDefault));
	if (aGlyph.mComment)
		return ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_TextEditorComment));
	if (aGlyph.mMultiLineComment)
		return ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_TextEditorMultiLineComment));
	auto const color = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(aGlyph.mColorIndex));
	if (aGlyph.mPreprocessor)
	{
		if (aGlyph.mColorIndex == ImGuiCol_TextEditorString)
		{
			return ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_TextEditorString));
			//const auto ppcolor = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_TextEditorPreprocessor));
			//const int c0 = ((ppcolor & 0xff) + (color & 0xff)) / 2;
			//const int c1 = (((ppcolor >> 8) & 0xff) + ((color >> 8) & 0xff)) / 2;
			//const int c2 = (((ppcolor >> 16) & 0xff) + ((color >> 16) & 0xff)) / 2;
			//const int c3 = (((ppcolor >> 24) & 0xff) + ((color >> 24) & 0xff)) / 2;
			//return ImU32(c0 | (c1 << 8) | (c2 << 16) | (c3 << 24));
		}
		else
		{
			return ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_TextEditorPreprocessor));
		}
	}
	return color;
}

void TextEditor::HandleKeyboardInputs()
{
	ImGuiIO& io = ImGui::GetIO();
	auto shift = io.KeyShift;
	auto ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
	auto alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;

	if (ImGui::IsWindowFocused())
	{
		if (ImGui::IsWindowHovered())
			ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);
		//ImGui::CaptureKeyboardFromApp(true);

		io.WantCaptureKeyboard = true;
		io.WantTextInput = true;

		if (!IsReadOnly() && ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Z)))
			Undo();
		else if (!IsReadOnly() && !ctrl && !shift && alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Backspace)))
			Undo();
		else if (!IsReadOnly() && ctrl && shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Z)))
			Redo();
		else if (!ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow)))
			MoveUp(1, shift);
		else if (!ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow)))
			MoveDown(1, shift);
		else if (!alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_LeftArrow)))
			MoveLeft(1, shift, ctrl);
		else if (!alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_RightArrow)))
			MoveRight(1, shift, ctrl);
		else if (!alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_PageUp)))
			MoveUp(GetPageSize() - 4, shift);
		else if (!alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_PageDown)))
			MoveDown(GetPageSize() - 4, shift);
		else if (!alt && ctrl && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Home)))
			MoveTop(shift);
		else if (ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_End)))
			MoveBottom(shift);
		else if (!ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Home)))
			MoveHome(shift);
		else if (!ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_End)))
			MoveEnd(shift);
		else if (!IsReadOnly() && !ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Delete)))
			Delete();
		else if (!IsReadOnly() && !ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Backspace)))
			Backspace();
		else if (!IsReadOnly() && ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Backspace))) { MoveLeft(1, true, true); Backspace(); MoveLeft(0, false, false); }
		else if (!ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Insert)))
			mOverwrite ^= true;
		else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Insert)))
			Copy();
		else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_C)))
			Copy();
		else if (!IsReadOnly() && !ctrl && shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Insert)))
			Paste();
		else if (!IsReadOnly() && ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_V)))
			Paste();
		else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_X)))
			Cut();
		else if (!ctrl && shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Delete)))
			Cut();
		else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_A)))
			SelectAll();
		else if (!IsReadOnly() && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter)))
			EnterCharacter('\n', false);
		else if (!IsReadOnly() && !ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Tab)))
			EnterCharacter('\t', shift);
		//else if (!IsReadOnly() && ctrl && !alt && Dymatic::Input::IsKeyPressed(Dymatic::Key::D))
		//	Duplicate();
		//else if (!IsReadOnly() && !ctrl && alt && Dymatic::Input::IsKeyPressed(Dymatic::Key::Up))
		//	SwapLineUp();
		//else if (!IsReadOnly() && !ctrl && alt && Dymatic::Input::IsKeyPressed(Dymatic::Key::Down))
		//	SwapLineDown();

		if (!IsReadOnly() && !io.InputQueueCharacters.empty())
		{
			for (int i = 0; i < io.InputQueueCharacters.Size; i++)
			{
				auto c = io.InputQueueCharacters[i];
				if (c != 0 && (c == '\n' || c >= 32))
					EnterCharacter(c, shift);
			}
			io.InputQueueCharacters.resize(0);
		}
	}
}

void TextEditor::HandleMouseInputs()
{
	ImGuiIO& io = ImGui::GetIO();
	auto shift = io.KeyShift;
	auto ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
	auto alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;

	if (ImGui::IsWindowHovered())
	{
		if (!alt)
		{
			auto click = ImGui::IsMouseClicked(0);
			auto doubleClick = ImGui::IsMouseDoubleClicked(0);
			auto t = ImGui::GetTime();
			auto tripleClick = click && !doubleClick && (mLastClick != -1.0f && (t - mLastClick) < io.MouseDoubleClickTime);

			/*
			Left mouse button triple click
			*/

			if (!shift && tripleClick)
			{
				if (!ctrl)
				{
					mState.mCursorPosition = mInteractiveStart = mInteractiveEnd = ScreenPosToCoordinates(ImGui::GetMousePos());
					mSelectionMode = SelectionMode::Line;
					SetSelection(mInteractiveStart, mInteractiveEnd, mSelectionMode);
				}

				mLastClick = -1.0f;
			}

			/*
			Left mouse button double click
			*/

			else if (!shift && doubleClick)
			{
				if (!ctrl)
				{
					mState.mCursorPosition = mInteractiveStart = mInteractiveEnd = ScreenPosToCoordinates(ImGui::GetMousePos());
					if (mSelectionMode == SelectionMode::Line)
						mSelectionMode = SelectionMode::Normal;
					else
						mSelectionMode = SelectionMode::Word;
					SetSelection(mInteractiveStart, mInteractiveEnd, mSelectionMode);
				}

				mLastClick = (float)ImGui::GetTime();
			}

			/*
			Left mouse button click
			*/
			else if (click)
			{
				if (shift)
				{
					mState.mCursorPosition = mInteractiveEnd = ScreenPosToCoordinates(ImGui::GetMousePos());
					SetSelection(mInteractiveStart, mInteractiveEnd, mSelectionMode);
				}
				else
				{
					mState.mCursorPosition = mInteractiveStart = mInteractiveEnd = ScreenPosToCoordinates(ImGui::GetMousePos());
					if (ctrl)
						mSelectionMode = SelectionMode::Word;
					else
						mSelectionMode = SelectionMode::Normal;
					SetSelection(mInteractiveStart, mInteractiveEnd, mSelectionMode);
				}

				mLastClick = (float)ImGui::GetTime();
			}
			// Mouse left button dragging (=> update selection)
			else if (!shift && ImGui::IsMouseDragging(0) && ImGui::IsMouseDown(0))
			{
				io.WantCaptureMouse = true;
				mState.mCursorPosition = mInteractiveEnd = ScreenPosToCoordinates(ImGui::GetMousePos());
				SetSelection(mInteractiveStart, mInteractiveEnd, mSelectionMode);
			}
		}
	}
}

static ImFont* ReleasedFont = nullptr;
static void ReleaseCurrentFont() { if (ReleasedFont == nullptr) { ReleasedFont = ImGui::GetFont(); ImGui::PopFont(); } }
static void ReturnCurrentFont() { if (ReleasedFont != nullptr) { ImGui::PushFont(ReleasedFont); ReleasedFont = nullptr; } }

void TextEditor::Render()
{
	/* Compute mCharAdvance regarding to scaled font size (Ctrl + mouse wheel)*/
	const float fontSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, "#", nullptr, nullptr).x;
	mCharAdvance = ImVec2(fontSize, ImGui::GetTextLineHeightWithSpacing() * mLineSpacing);

	/* Update palette with the current alpha from style */
	//for (int i = 0; i < (int)ImGuiCol_COUNT; ++i)
	//{
	//	auto color = ImGui::ColorConvertU32ToFloat4(mPaletteBase[i]);
	//	color.w *= ImGui::GetStyle().Alpha;
	//	mPalette[i] = ImGui::ColorConvertFloat4ToU32(color);
	//}

	assert(mLineBuffer.empty());

	auto contentSize = ImGui::GetWindowContentRegionMax();
	auto drawList = ImGui::GetWindowDrawList();
	float longest(mTextStart);

	if (mScrollToTop)
	{
		mScrollToTop = false;
		ImGui::SetScrollY(0.f);
	}

	ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();
	auto scrollX = ImGui::GetScrollX();
	auto scrollY = ImGui::GetScrollY();

	auto lineNo = (int)floor(scrollY / mCharAdvance.y);
	auto globalLineMax = (int)mLines.size();
	auto lineMax = std::max(0, std::min((int)mLines.size() - 1, lineNo + (int)floor((scrollY + contentSize.y) / mCharAdvance.y)));

	float breakpoint_width = /*10*/ 40.0f * *mZoom;
	float breakpoint_pad = /*2*/ 8.0f * *mZoom;

	// Deduce mTextStart by evaluating mLines size (global lineMax) plus two spaces as text width
	char buf[16];
	snprintf(buf, 16, " %d ", globalLineMax);
	mTextStart = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, buf, nullptr, nullptr).x + mLeftMargin + breakpoint_width;

	if (!mLines.empty())
	{
		float spaceSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, " ", nullptr, nullptr).x;

		while (lineNo <= lineMax)
		{
			ImVec2 lineStartScreenPos = ImVec2(cursorScreenPos.x, cursorScreenPos.y + lineNo * mCharAdvance.y);
			ImVec2 textScreenPos = ImVec2(lineStartScreenPos.x + mTextStart, lineStartScreenPos.y);

			auto& line = mLines[lineNo];
			longest = std::max(mTextStart + TextDistanceToLineStart(Coordinates(lineNo, GetLineMaxColumn(lineNo))), longest);
			auto columnNo = 0;
			Coordinates lineStartCoord(lineNo, 0);
			Coordinates lineEndCoord(lineNo, GetLineMaxColumn(lineNo));

			// Draw selection for the current line
			float sstart = -1.0f;
			float ssend = -1.0f;

			assert(mState.mSelectionStart <= mState.mSelectionEnd);
			if (mState.mSelectionStart <= lineEndCoord)
				sstart = mState.mSelectionStart > lineStartCoord ? TextDistanceToLineStart(mState.mSelectionStart) : 0.0f;
			if (mState.mSelectionEnd > lineStartCoord)
				ssend = TextDistanceToLineStart(mState.mSelectionEnd < lineEndCoord ? mState.mSelectionEnd : lineEndCoord);

			if (mState.mSelectionEnd.mLine > lineNo)
				ssend += mCharAdvance.x;

			if (sstart != -1 && ssend != -1 && sstart < ssend)
			{
				ImVec2 vstart(lineStartScreenPos.x + mTextStart + sstart, lineStartScreenPos.y);
				ImVec2 vend(lineStartScreenPos.x + mTextStart + ssend, lineStartScreenPos.y + mCharAdvance.y);
				drawList->AddRectFilled(vstart, vend, ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_TextSelectedBg)));
			}

			// Draw breakpoints
			auto start = ImVec2(lineStartScreenPos.x + scrollX, lineStartScreenPos.y);

			/*if (mBreakpoints.count(lineNo + 1) != 0)
			{
				auto end = ImVec2(lineStartScreenPos.x + contentSize.x + 2.0f * scrollX, lineStartScreenPos.y + mCharAdvance.y);
				drawList->AddRectFilled(start, end, mPalette[(int)PaletteIndex::Breakpoint]);
			}*/

			if (mHighlightLines.count(lineNo + 1) != 0)
			{
				auto end = ImVec2(lineStartScreenPos.x + contentSize.x + 2.0f * scrollX, lineStartScreenPos.y + mCharAdvance.y);
				drawList->AddRectFilled(start, end, ImGui::ColorConvertFloat4ToU32(mHighlightLines[lineNo + 1]));
			}

			// Draw error markers
			auto errorIt = mErrorMarkers.find(lineNo + 1);
			if (errorIt != mErrorMarkers.end())
			{
				auto end = ImVec2(lineStartScreenPos.x + contentSize.x + 2.0f * scrollX, lineStartScreenPos.y + mCharAdvance.y);
				drawList->AddRectFilled(start, end, ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.0f, 0.0f, 1.0f)));

				if (ImGui::IsMouseHoveringRect(lineStartScreenPos, end))
				{
					ReleaseCurrentFont();
					ImGui::BeginTooltip();
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
					ImGui::Text("Error at line %d:", errorIt->first);
					ImGui::PopStyleColor();
					ImGui::Separator();
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.2f, 1.0f));
					ImGui::Text("%s", errorIt->second.c_str());
					ImGui::PopStyleColor();
					ImGui::EndTooltip();
					ReturnCurrentFont();
				}
			}

			// Draw line number (right aligned)
			snprintf(buf, 16, "%d", lineNo + 1);

			auto lineNoWidth = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, buf, nullptr, nullptr).x;
			//drawList->AddText(ImVec2(lineStartScreenPos.x + mTextStart - lineNoWidth - breakpoint_width - breakpoint_pad, lineStartScreenPos.y), ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_TextEditorLineNumber)), buf);
			drawList->AddText(ImVec2(lineStartScreenPos.x + mTextStart - lineNoWidth - breakpoint_width / 2.0f, lineStartScreenPos.y), ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_TextEditorLineNumber)), buf);

			if (mBreakpoints.find(lineNo + 1) != mBreakpoints.end())
			{
				// {ORIGINAL} drawList->AddText(ImVec2(lineStartScreenPos.x + mTextStart - breakpoint_width / 2 - breakpoint_pad - 4, lineStartScreenPos.y), IM_COL32(150, 40, 40, 255), "B");
				/* Alterations need to be applied below */ drawList->AddCircleFilled(ImVec2(ImGui::GetWindowPos().x + breakpoint_width / 2 + breakpoint_pad, lineStartScreenPos.y + breakpoint_width / 2 + ImGui::GetTextLineHeightWithSpacing() / 5), breakpoint_width / 2, IM_COL32(255, 68, 68, 255));
			}

			if (ImGui::IsWindowHovered()&&
				// {ORIGINAL} ImGui::IsMouseHoveringRect(ImVec2(lineStartScreenPos.x + mTextStart - breakpoint_width - breakpoint_pad - 4, lineStartScreenPos.y), ImVec2(lineStartScreenPos.x + mTextStart, lineStartScreenPos.y + ImGui::CalcTextSize(" ").y)))
				ImGui::IsMouseHoveringRect(ImVec2(ImGui::GetWindowPos().x + breakpoint_pad, lineStartScreenPos.y), ImVec2(ImGui::GetWindowPos().x + breakpoint_width + breakpoint_pad, lineStartScreenPos.y + breakpoint_width + ImGui::GetTextLineHeightWithSpacing() / 5)))
			{
				ReleaseCurrentFont();
				ImGui::BeginTooltip();
				ImGui::TextUnformatted("Toggle Breakpoint");
				ImGui::EndTooltip();
				ReturnCurrentFont();

				if (ImGui::IsMouseClicked(0))
				{
					if (mBreakpoints.find(lineNo + 1) == mBreakpoints.end())
					{
						mBreakpoints.insert(lineNo + 1);
					}
					else
					{
						mBreakpoints.erase(lineNo + 1);
					}
				}
			}

			if (mState.mCursorPosition.mLine == lineNo)
			{
				auto focused = ImGui::IsWindowFocused();

				// Highlight the current line (where the cursor is)
				if (!HasSelection())
				{
					auto end = ImVec2(start.x + contentSize.x + scrollX, start.y + mCharAdvance.y);
					drawList->AddRectFilled(start, end, ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4((focused ? ImGuiCol_TextEditorCurrentLineFill : ImGuiCol_TextEditorCurrentLineFillInactive))));
					drawList->AddRect(start, end, ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_TextEditorCurrentLineEdge)), 1.0f);
				}

				// Render the cursor
				if (focused)
				{
					auto timeEnd = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
					auto elapsed = timeEnd - mStartTime;
					if (elapsed > 400)
					{
						float width = 1.0f;
						auto cindex = GetCharacterIndex(mState.mCursorPosition);
						float cx = TextDistanceToLineStart(mState.mCursorPosition);

						if (mOverwrite && cindex < (int)line.size())
						{
							auto c = line[cindex].mChar;
							if (c == '\t')
							{
								auto x = (1.0f + std::floor((1.0f + cx) / (float(mTabSize) * spaceSize))) * (float(mTabSize) * spaceSize);
								width = x - cx;
							}
							else
							{
								char buf2[2];
								buf2[0] = line[cindex].mChar;
								buf2[1] = '\0';
								width = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, buf2).x;
							}
						}
						ImVec2 cstart(textScreenPos.x + cx, lineStartScreenPos.y);
						ImVec2 cend(textScreenPos.x + cx + width, lineStartScreenPos.y + mCharAdvance.y);
						drawList->AddRectFilled(cstart, cend, ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_Text)));
						if (elapsed > 800)
							mStartTime = timeEnd;
					}
				}
			}

			// Render colorized text
			auto prevColor = line.empty() ? ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_TextEditorDefault)) : GetGlyphColor(line[0]);
			ImVec2 bufferOffset;

			for (int i = 0; i < line.size();)
			{
				auto& glyph = line[i];
				auto color = GetGlyphColor(glyph);

				if ((color != prevColor || glyph.mChar == '\t' || glyph.mChar == ' ') && !mLineBuffer.empty())
				{
					const ImVec2 newOffset(textScreenPos.x + bufferOffset.x, textScreenPos.y + bufferOffset.y);
					drawList->AddText(newOffset, prevColor, mLineBuffer.c_str());
					auto textSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, mLineBuffer.c_str(), nullptr, nullptr);
					bufferOffset.x += textSize.x;
					mLineBuffer.clear();
				}
				prevColor = color;

				if (glyph.mChar == '\t')
				{
					auto oldX = bufferOffset.x;
					bufferOffset.x = (1.0f + std::floor((1.0f + bufferOffset.x) / (float(mTabSize) * spaceSize))) * (float(mTabSize) * spaceSize);
					++i;

					if (mShowWhitespaces)
					{
						const auto s = ImGui::GetFontSize();
						const auto x1 = textScreenPos.x + oldX + 1.0f;
						const auto x2 = textScreenPos.x + bufferOffset.x - 1.0f;
						const auto y = textScreenPos.y + bufferOffset.y + s * 0.5f;
						const ImVec2 p1(x1, y);
						const ImVec2 p2(x2, y);
						const ImVec2 p3(x2 - s * 0.2f, y - s * 0.2f);
						const ImVec2 p4(x2 - s * 0.2f, y + s * 0.2f);
						drawList->AddLine(p1, p2, 0x90909090);
						drawList->AddLine(p2, p3, 0x90909090);
						drawList->AddLine(p2, p4, 0x90909090);
					}
				}
				else if (glyph.mChar == ' ')
				{
					if (mShowWhitespaces)
					{
						const auto s = ImGui::GetFontSize();
						const auto x = textScreenPos.x + bufferOffset.x + spaceSize * 0.5f;
						const auto y = textScreenPos.y + bufferOffset.y + s * 0.5f;
						drawList->AddCircleFilled(ImVec2(x, y), 1.5f, 0x80808080, 4);
					}
					bufferOffset.x += spaceSize;
					i++;
				}
				else
				{
					auto l = UTF8CharLength(glyph.mChar);
					while (l-- > 0)
						mLineBuffer.push_back(line[i++].mChar);
				}
				++columnNo;
			}

			if (!mLineBuffer.empty())
			{
				const ImVec2 newOffset(textScreenPos.x + bufferOffset.x, textScreenPos.y + bufferOffset.y);
				drawList->AddText(newOffset, prevColor, mLineBuffer.c_str());
				mLineBuffer.clear();
			}

			++lineNo;
		}

		// Draw a tooltip on known identifiers/preprocessor symbols
		if (ImGui::IsMousePosValid())
		{
			float x_origin = ImGui::GetCursorScreenPos().x;
			auto id = GetWordAt(ScreenPosToCoordinates(ImGui::GetMousePos()));

			static auto prevId = id;
			static float hovertime = 0.0f;
			if (id == prevId) hovertime += 1.0f / ImGui::GetIO().Framerate;
			else hovertime = 0.0f;
			prevId = id;

			if (!id.empty() && ImGui::IsWindowHovered() && ImGui::GetMousePos().x >= x_origin + mTextStart && hovertime > 0.5f)
			{
				ReleaseCurrentFont();
				auto it = mLanguageDefinition.mIdentifiers.find(id);
				if (it != mLanguageDefinition.mIdentifiers.end())
				{
					ImGui::BeginTooltip();
					ImGui::PushTextWrapPos(400);
					ImGui::TextUnformatted(it->second.mDeclaration.c_str());
					ImGui::PopTextWrapPos();
					ImGui::EndTooltip();
				}
				else
				{
					// Custom Code for Keywords
					auto it = mLanguageDefinition.mKeywords.find(id);
					if (it != mLanguageDefinition.mKeywords.end())
					{
						ImGui::BeginTooltip();
						ImGui::PushTextWrapPos(400);
						ImGui::TextUnformatted(it->second.mDeclaration.c_str());
						ImGui::PopTextWrapPos();
						ImGui::EndTooltip();
					}
					//End Custom Code
					else
					{
						auto it = mLanguageDefinition.mSpecialKeywords.find(id);
						if (it != mLanguageDefinition.mSpecialKeywords.end())
						{
							ImGui::BeginTooltip();
							ImGui::PushTextWrapPos(400);
							ImGui::TextUnformatted(it->second.mDeclaration.c_str());
							ImGui::PopTextWrapPos();
							ImGui::EndTooltip();
						}
						else
						{
							auto pi = mLanguageDefinition.mPreprocIdentifiers.find(id);
							if (pi != mLanguageDefinition.mPreprocIdentifiers.end())
							{
								ImGui::BeginTooltip();
								ImGui::PushTextWrapPos(400);
								ImGui::TextUnformatted(pi->second.mDeclaration.c_str());
								ImGui::PopTextWrapPos();
								ImGui::EndTooltip();
							}
						}
					}
				}
				ReturnCurrentFont();
			}
		}
	}

	ErrorMarkers etmp;
	for (auto& i : mErrorMarkers)
	{
		if (mLines[i.first - 1].size() == 0)
			continue;

		etmp.insert(ErrorMarkers::value_type(i.first, i.second));
	}

	mErrorMarkers = std::move(etmp);

	Breakpoints btmp;

	for (auto& i : mBreakpoints)
	{
		if (mLines[i - 1].size() == 0)
			continue;

		btmp.insert(i);
	}

	mBreakpoints = std::move(btmp);

	ImGui::Dummy(ImVec2((longest + 2), mLines.size() * mCharAdvance.y));

	if (mScrollToCursor)
	{
		EnsureCursorVisible();
		ImGui::SetWindowFocus();
		mScrollToCursor = false;
	}
}

void TextEditor::Render(const char* aTitle, float aFontSize, const ImVec2& aSize, bool aBorder)
{
	mWithinRender = true;
	mTextChanged = false;
	mCursorPositionChanged = false;

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
	if (!mIgnoreImGuiChild)
		ImGui::BeginChild(aTitle, aSize, aBorder, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav);

	ImGui::SetWindowFontScale(aFontSize);

	if (mHandleKeyboardInputs)
	{
		HandleKeyboardInputs();
		ImGui::PushAllowKeyboardFocus(true);
	}

	if (mHandleMouseInputs)
		HandleMouseInputs();

	ColorizeInternal();
	Render();

	if (mHandleKeyboardInputs)
		ImGui::PopAllowKeyboardFocus();

	if (!mIgnoreImGuiChild)
		ImGui::EndChild();

	ImGui::PopStyleVar();

	mWithinRender = false;
}

void TextEditor::SetText(const std::string& aText)
{
	mLines.clear();
	mLines.emplace_back(Line());
	for (auto chr : aText)
	{
		if (chr == '\r')
		{
			// ignore the carriage return character
		}
		else if (chr == '\n')
			mLines.emplace_back(Line());
		else
		{
			mLines.back().emplace_back(Glyph(chr, ImGuiCol_TextEditorDefault));
		}
	}

	mTextChanged = true;
	mScrollToTop = true;

	mUndoBuffer.clear();
	mUndoIndex = 0;

	Colorize();
}

void TextEditor::SetTextLines(const std::vector<std::string>& aLines)
{
	mLines.clear();

	if (aLines.empty())
	{
		mLines.emplace_back(Line());
	}
	else
	{
		mLines.resize(aLines.size());

		for (size_t i = 0; i < aLines.size(); ++i)
		{
			const std::string& aLine = aLines[i];

			mLines[i].reserve(aLine.size());
			for (size_t j = 0; j < aLine.size(); ++j)
				mLines[i].emplace_back(Glyph(aLine[j], ImGuiCol_TextEditorDefault));
		}
	}

	mTextChanged = true;
	mScrollToTop = true;

	mUndoBuffer.clear();
	mUndoIndex = 0;

	Colorize();
}

void TextEditor::EnterCharacter(ImWchar aChar, bool aShift)
{
	assert(!mReadOnly);

	UndoRecord u;

	u.mBefore = mState;

	if (HasSelection())
	{
		if (aChar == '\t' && mState.mSelectionStart.mLine != mState.mSelectionEnd.mLine)
		{

			auto start = mState.mSelectionStart;
			auto end = mState.mSelectionEnd;
			auto originalEnd = end;

			if (start > end)
				std::swap(start, end);
			start.mColumn = 0;
			//			end.mColumn = end.mLine < mLines.size() ? mLines[end.mLine].size() : 0;
			if (end.mColumn == 0 && end.mLine > 0)
				--end.mLine;
			if (end.mLine >= (int)mLines.size())
				end.mLine = mLines.empty() ? 0 : (int)mLines.size() - 1;
			end.mColumn = GetLineMaxColumn(end.mLine);

			//if (end.mColumn >= GetLineMaxColumn(end.mLine))
			//	end.mColumn = GetLineMaxColumn(end.mLine) - 1;

			u.mRemovedStart = start;
			u.mRemovedEnd = end;
			u.mRemoved = GetText(start, end);

			bool modified = false;

			for (int i = start.mLine; i <= end.mLine; i++)
			{
				auto& line = mLines[i];
				if (aShift)
				{
					if (!line.empty())
					{
						if (line.front().mChar == '\t')
						{
							line.erase(line.begin());
							modified = true;
						}
						else
						{
							for (int j = 0; j < mTabSize && !line.empty() && line.front().mChar == ' '; j++)
							{
								line.erase(line.begin());
								modified = true;
							}
						}
					}
				}
				else
				{
					line.insert(line.begin(), Glyph('\t', ImGuiCol_WindowBg));
					modified = true;
				}
			}

			if (modified)
			{
				start = Coordinates(start.mLine, GetCharacterColumn(start.mLine, 0));
				Coordinates rangeEnd;
				if (originalEnd.mColumn != 0)
				{
					end = Coordinates(end.mLine, GetLineMaxColumn(end.mLine));
					rangeEnd = end;
					u.mAdded = GetText(start, end);
				}
				else
				{
					end = Coordinates(originalEnd.mLine, 0);
					rangeEnd = Coordinates(end.mLine - 1, GetLineMaxColumn(end.mLine - 1));
					u.mAdded = GetText(start, rangeEnd);
				}

				u.mAddedStart = start;
				u.mAddedEnd = rangeEnd;
				u.mAfter = mState;

				mState.mSelectionStart = start;
				mState.mSelectionEnd = end;
				AddUndo(u);

				mTextChanged = true;

				EnsureCursorVisible();
			}

			return;
		} // c == '\t'
		else
		{
			u.mRemoved = GetSelectedText();
			u.mRemovedStart = mState.mSelectionStart;
			u.mRemovedEnd = mState.mSelectionEnd;
			DeleteSelection();
		}
	} // HasSelection

	auto coord = GetActualCursorCoordinates();
	u.mAddedStart = coord;

	assert(!mLines.empty());

	if (aChar == '\n')
	{
		InsertLine(coord.mLine + 1);
		auto& line = mLines[coord.mLine];
		auto& newLine = mLines[coord.mLine + 1];

		if (mLanguageDefinition.mAutoIndentation)
			for (size_t it = 0; it < line.size() && isascii(line[it].mChar) && isblank(line[it].mChar); ++it)
				newLine.push_back(line[it]);

		const size_t whitespaceSize = newLine.size();
		auto cindex = GetCharacterIndex(coord);
		newLine.insert(newLine.end(), line.begin() + cindex, line.end());
		line.erase(line.begin() + cindex, line.begin() + line.size());
		SetCursorPosition(Coordinates(coord.mLine + 1, GetCharacterColumn(coord.mLine + 1, (int)whitespaceSize)));
		u.mAdded = (char)aChar;

		if (mLanguageDefinition.mAutoIndentation && !line.empty())
			if (line[line.size() - 1].mChar == '{')
				EnterCharacter('\t', false);
	}
	else
	{
		char buf[7];
		int e = ImTextCharToUtf8(buf, 7, aChar);
		if (e > 0)
		{
			buf[e] = '\0';
			auto& line = mLines[coord.mLine];
			auto cindex = GetCharacterIndex(coord);

			if (mOverwrite && cindex < (int)line.size())
			{
				auto d = UTF8CharLength(line[cindex].mChar);

				u.mRemovedStart = mState.mCursorPosition;
				u.mRemovedEnd = Coordinates(coord.mLine, GetCharacterColumn(coord.mLine, cindex + d));

				while (d-- > 0 && cindex < (int)line.size())
				{
					u.mRemoved += line[cindex].mChar;
					line.erase(line.begin() + cindex);
				}
			}

			for (auto p = buf; *p != '\0'; p++, ++cindex)
				line.insert(line.begin() + cindex, Glyph(*p, ImGuiCol_TextEditorDefault));
			u.mAdded = buf;

			SetCursorPosition(Coordinates(coord.mLine, GetCharacterColumn(coord.mLine, cindex)));
		}
		else
			return;
	}

	mTextChanged = true;

	u.mAddedEnd = GetActualCursorCoordinates();
	u.mAfter = mState;

	AddUndo(u);

	Colorize(coord.mLine - 1, 3);
	EnsureCursorVisible();
}

void TextEditor::SetReadOnly(bool aValue)
{
	mReadOnly = aValue;
}

void TextEditor::SetColorizerEnable(bool aValue)
{
	mColorizerEnabled = aValue;
}

void TextEditor::SetCursorPosition(const Coordinates& aPosition)
{
	if (mState.mCursorPosition != aPosition)
	{
		mState.mCursorPosition = aPosition;
		mCursorPositionChanged = true;
		EnsureCursorVisible();
	}
}

void TextEditor::SetSelectionStart(const Coordinates& aPosition)
{
	mState.mSelectionStart = SanitizeCoordinates(aPosition);
	if (mState.mSelectionStart > mState.mSelectionEnd)
		std::swap(mState.mSelectionStart, mState.mSelectionEnd);
}

void TextEditor::SetSelectionEnd(const Coordinates& aPosition)
{
	mState.mSelectionEnd = SanitizeCoordinates(aPosition);
	if (mState.mSelectionStart > mState.mSelectionEnd)
		std::swap(mState.mSelectionStart, mState.mSelectionEnd);
}

void TextEditor::SetSelection(const Coordinates& aStart, const Coordinates& aEnd, SelectionMode aMode)
{
	auto oldSelStart = mState.mSelectionStart;
	auto oldSelEnd = mState.mSelectionEnd;

	mState.mSelectionStart = SanitizeCoordinates(aStart);
	mState.mSelectionEnd = SanitizeCoordinates(aEnd);
	if (mState.mSelectionStart > mState.mSelectionEnd)
		std::swap(mState.mSelectionStart, mState.mSelectionEnd);

	switch (aMode)
	{
	case TextEditor::SelectionMode::Normal:
		break;
	case TextEditor::SelectionMode::Word:
	{
		mState.mSelectionStart = FindWordStart(mState.mSelectionStart);
		//if (!IsOnWordBoundary(mState.mSelectionEnd))
			mState.mSelectionEnd = FindWordEnd(FindWordStart(mState.mSelectionEnd));
		break;
	}
	case TextEditor::SelectionMode::Line:
	{
		const auto lineNo = mState.mSelectionEnd.mLine;
		const auto lineSize = (size_t)lineNo < mLines.size() ? mLines[lineNo].size() : 0;
		mState.mSelectionStart = Coordinates(mState.mSelectionStart.mLine, 0);
		mState.mSelectionEnd = Coordinates(lineNo, GetLineMaxColumn(lineNo));
		break;
	}
	default:
		break;
	}

	if (mState.mSelectionStart != oldSelStart ||
		mState.mSelectionEnd != oldSelEnd)
		mCursorPositionChanged = true;
}

void TextEditor::SetTabSize(int aValue)
{
	mTabSize = std::max(0, std::min(32, aValue));
}

void TextEditor::InsertText(const std::string& aValue)
{
	InsertText(aValue.c_str());
}

void TextEditor::InsertText(const char* aValue)
{
	if (aValue == nullptr)
		return;

	auto pos = GetActualCursorCoordinates();
	auto start = std::min(pos, mState.mSelectionStart);
	int totalLines = pos.mLine - start.mLine;

	totalLines += InsertTextAt(pos, aValue);

	SetSelection(pos, pos);
	SetCursorPosition(pos);
	Colorize(start.mLine - 1, totalLines + 2);
}

void TextEditor::DeleteSelection()
{
	assert(mState.mSelectionEnd >= mState.mSelectionStart);

	if (mState.mSelectionEnd == mState.mSelectionStart)
		return;

	DeleteRange(mState.mSelectionStart, mState.mSelectionEnd);

	SetSelection(mState.mSelectionStart, mState.mSelectionStart);
	SetCursorPosition(mState.mSelectionStart);
	Colorize(mState.mSelectionStart.mLine, 1);
}

void TextEditor::MoveUp(int aAmount, bool aSelect)
{
	auto oldPos = mState.mCursorPosition;
	mState.mCursorPosition.mLine = std::max(0, mState.mCursorPosition.mLine - aAmount);
	if (oldPos != mState.mCursorPosition)
	{
		if (aSelect)
		{
			if (oldPos == mInteractiveStart)
				mInteractiveStart = mState.mCursorPosition;
			else if (oldPos == mInteractiveEnd)
				mInteractiveEnd = mState.mCursorPosition;
			else
			{
				mInteractiveStart = mState.mCursorPosition;
				mInteractiveEnd = oldPos;
			}
		}
		else
			mInteractiveStart = mInteractiveEnd = mState.mCursorPosition;
		SetSelection(mInteractiveStart, mInteractiveEnd);

		EnsureCursorVisible();
	}
}

void TextEditor::MoveDown(int aAmount, bool aSelect)
{
	assert(mState.mCursorPosition.mColumn >= 0);
	auto oldPos = mState.mCursorPosition;
	mState.mCursorPosition.mLine = std::max(0, std::min((int)mLines.size() - 1, mState.mCursorPosition.mLine + aAmount));

	if (mState.mCursorPosition != oldPos)
	{
		if (aSelect)
		{
			if (oldPos == mInteractiveEnd)
				mInteractiveEnd = mState.mCursorPosition;
			else if (oldPos == mInteractiveStart)
				mInteractiveStart = mState.mCursorPosition;
			else
			{
				mInteractiveStart = oldPos;
				mInteractiveEnd = mState.mCursorPosition;
			}
		}
		else
			mInteractiveStart = mInteractiveEnd = mState.mCursorPosition;
		SetSelection(mInteractiveStart, mInteractiveEnd);

		EnsureCursorVisible();
	}
}

static bool IsUTFSequence(char c)
{
	return (c & 0xC0) == 0x80;
}

void TextEditor::MoveLeft(int aAmount, bool aSelect, bool aWordMode)
{
	if (mLines.empty())
		return;

	auto oldPos = mState.mCursorPosition;
	mState.mCursorPosition = GetActualCursorCoordinates();
	auto line = mState.mCursorPosition.mLine;
	auto cindex = GetCharacterIndex(mState.mCursorPosition);

	while (aAmount-- > 0)
	{
		if (cindex == 0)
		{
			if (line > 0)
			{
				--line;
				if ((int)mLines.size() > line)
					cindex = (int)mLines[line].size();
				else
					cindex = 0;
			}
		}
		else
		{
			--cindex;
			if (cindex > 0)
			{
				if ((int)mLines.size() > line)
				{
					while (cindex > 0 && IsUTFSequence(mLines[line][cindex].mChar))
						--cindex;
				}
			}
		}

		mState.mCursorPosition = Coordinates(line, GetCharacterColumn(line, cindex));
		if (aWordMode)
		{
			mState.mCursorPosition = FindWordStart(mState.mCursorPosition);
			cindex = GetCharacterIndex(mState.mCursorPosition);
		}
	}

	mState.mCursorPosition = Coordinates(line, GetCharacterColumn(line, cindex));

	assert(mState.mCursorPosition.mColumn >= 0);
	if (aSelect)
	{
		if (oldPos == mInteractiveStart)
			mInteractiveStart = mState.mCursorPosition;
		else if (oldPos == mInteractiveEnd)
			mInteractiveEnd = mState.mCursorPosition;
		else
		{
			mInteractiveStart = mState.mCursorPosition;
			mInteractiveEnd = oldPos;
		}
	}
	else
		mInteractiveStart = mInteractiveEnd = mState.mCursorPosition;
	SetSelection(mInteractiveStart, mInteractiveEnd, aSelect && aWordMode ? SelectionMode::Word : SelectionMode::Normal);

	EnsureCursorVisible();
}

void TextEditor::MoveRight(int aAmount, bool aSelect, bool aWordMode)
{
	auto oldPos = mState.mCursorPosition;

	if (mLines.empty() || oldPos.mLine >= mLines.size())
		return;

	auto cindex = GetCharacterIndex(mState.mCursorPosition);
	while (aAmount-- > 0)
	{
		auto lindex = mState.mCursorPosition.mLine;
		auto& line = mLines[lindex];

		if (cindex >= line.size())
		{
			if (mState.mCursorPosition.mLine < mLines.size() - 1)
			{
				mState.mCursorPosition.mLine = std::max(0, std::min((int)mLines.size() - 1, mState.mCursorPosition.mLine + 1));
				mState.mCursorPosition.mColumn = 0;
			}
			else
				return;
		}
		else
		{
			cindex += UTF8CharLength(line[cindex].mChar);
			mState.mCursorPosition = Coordinates(lindex, GetCharacterColumn(lindex, cindex));
			if (aWordMode)
				mState.mCursorPosition = FindNextWord(mState.mCursorPosition);
		}
	}

	if (aSelect)
	{
		if (oldPos == mInteractiveEnd)
			mInteractiveEnd = SanitizeCoordinates(mState.mCursorPosition);
		else if (oldPos == mInteractiveStart)
			mInteractiveStart = mState.mCursorPosition;
		else
		{
			mInteractiveStart = oldPos;
			mInteractiveEnd = mState.mCursorPosition;
		}
	}
	else
		mInteractiveStart = mInteractiveEnd = mState.mCursorPosition;
	SetSelection(mInteractiveStart, mInteractiveEnd, aSelect && aWordMode ? SelectionMode::Word : SelectionMode::Normal);

	EnsureCursorVisible();
}

void TextEditor::MoveTop(bool aSelect)
{
	auto oldPos = mState.mCursorPosition;
	SetCursorPosition(Coordinates(0, 0));

	if (mState.mCursorPosition != oldPos)
	{
		if (aSelect)
		{
			mInteractiveEnd = oldPos;
			mInteractiveStart = mState.mCursorPosition;
		}
		else
			mInteractiveStart = mInteractiveEnd = mState.mCursorPosition;
		SetSelection(mInteractiveStart, mInteractiveEnd);
	}
}

void TextEditor::TextEditor::MoveBottom(bool aSelect)
{
	auto oldPos = GetCursorPosition();
	auto newPos = Coordinates((int)mLines.size() - 1, 0);
	SetCursorPosition(newPos);
	if (aSelect)
	{
		mInteractiveStart = oldPos;
		mInteractiveEnd = newPos;
	}
	else
		mInteractiveStart = mInteractiveEnd = newPos;
	SetSelection(mInteractiveStart, mInteractiveEnd);
}

void TextEditor::MoveHome(bool aSelect)
{
	auto oldPos = mState.mCursorPosition;
	SetCursorPosition(Coordinates(mState.mCursorPosition.mLine, 0));

	if (mState.mCursorPosition != oldPos)
	{
		if (aSelect)
		{
			if (oldPos == mInteractiveStart)
				mInteractiveStart = mState.mCursorPosition;
			else if (oldPos == mInteractiveEnd)
				mInteractiveEnd = mState.mCursorPosition;
			else
			{
				mInteractiveStart = mState.mCursorPosition;
				mInteractiveEnd = oldPos;
			}
		}
		else
			mInteractiveStart = mInteractiveEnd = mState.mCursorPosition;
		SetSelection(mInteractiveStart, mInteractiveEnd);
	}
}

void TextEditor::MoveEnd(bool aSelect)
{
	auto oldPos = mState.mCursorPosition;
	SetCursorPosition(Coordinates(mState.mCursorPosition.mLine, GetLineMaxColumn(oldPos.mLine)));

	if (mState.mCursorPosition != oldPos)
	{
		if (aSelect)
		{
			if (oldPos == mInteractiveEnd)
				mInteractiveEnd = mState.mCursorPosition;
			else if (oldPos == mInteractiveStart)
				mInteractiveStart = mState.mCursorPosition;
			else
			{
				mInteractiveStart = oldPos;
				mInteractiveEnd = mState.mCursorPosition;
			}
		}
		else
			mInteractiveStart = mInteractiveEnd = mState.mCursorPosition;
		SetSelection(mInteractiveStart, mInteractiveEnd);
	}
}

void TextEditor::Delete()
{
	assert(!mReadOnly);

	if (mLines.empty())
		return;

	UndoRecord u;
	u.mBefore = mState;

	if (HasSelection())
	{
		u.mRemoved = GetSelectedText();
		u.mRemovedStart = mState.mSelectionStart;
		u.mRemovedEnd = mState.mSelectionEnd;

		DeleteSelection();
	}
	else
	{
		auto pos = GetActualCursorCoordinates();
		SetCursorPosition(pos);
		auto& line = mLines[pos.mLine];

		if (pos.mColumn == GetLineMaxColumn(pos.mLine))
		{
			if (pos.mLine == (int)mLines.size() - 1)
				return;

			u.mRemoved = '\n';
			u.mRemovedStart = u.mRemovedEnd = GetActualCursorCoordinates();
			Advance(u.mRemovedEnd);

			auto& nextLine = mLines[pos.mLine + 1];
			line.insert(line.end(), nextLine.begin(), nextLine.end());
			RemoveLine(pos.mLine + 1);
		}
		else
		{
			auto cindex = GetCharacterIndex(pos);
			u.mRemovedStart = u.mRemovedEnd = GetActualCursorCoordinates();
			u.mRemovedEnd.mColumn++;
			u.mRemoved = GetText(u.mRemovedStart, u.mRemovedEnd);

			auto d = UTF8CharLength(line[cindex].mChar);
			while (d-- > 0 && cindex < (int)line.size())
				line.erase(line.begin() + cindex);
		}

		mTextChanged = true;

		Colorize(pos.mLine, 1);
	}

	u.mAfter = mState;
	AddUndo(u);
}

void TextEditor::Duplicate()
{
	UndoRecord u;
	u.mBefore = mState;

	auto beforePos = GetCursorPosition();

	auto text = GetCurrentLineText();
	InsertLine(GetCursorPosition().mLine);
	InsertTextAt(Coordinates(GetCursorPosition().mLine, 0), text.c_str());
	MoveDown();

	u.mAdded = ("\n" + text);
	u.mAddedStart = Coordinates(beforePos.mLine, GetLineMaxColumn(beforePos.mLine));
	u.mAddedEnd = Coordinates(GetCursorPosition().mLine, GetLineMaxColumn(GetCursorPosition().mLine));

	u.mAfter = mState;

	Colorize(beforePos.mLine, GetCursorPosition().mLine);

	AddUndo(u);
}

void TextEditor::SwapLineUp()
{
	if (GetCursorPosition().mLine > 0)
	{
		UndoRecord u;
		u.mBefore = mState;

		auto currentPos = GetCursorPosition();
		std::string currentString = "";
		for (auto character : mLines[currentPos.mLine]) { currentString += character.mChar; }

		std::string otherString = "";
		for (auto character : mLines[currentPos.mLine - 1]) { otherString += character.mChar; }

		u.mRemoved = otherString + "\n" + currentString;
		u.mRemovedStart = Coordinates(currentPos.mLine - 1, 0);
		u.mRemovedEnd = Coordinates(currentPos.mLine, GetLineMaxColumn(currentPos.mLine));

		u.mAdded = currentString + "\n" + otherString;
		u.mAddedStart = Coordinates(currentPos.mLine - 1, 0);
		u.mAddedEnd = Coordinates(currentPos.mLine, GetLineMaxColumn(currentPos.mLine - 1));

		u.mAfter = mState;

		std::iter_swap(mLines.begin() + GetCursorPosition().mLine, mLines.begin() + GetCursorPosition().mLine - 1);
		MoveUp();

		Colorize(currentPos.mLine);
		Colorize(currentPos.mLine - 1);

		AddUndo(u);
	}
}

void TextEditor::SwapLineDown()
{
	if (GetCursorPosition().mLine < mLines.size() - 1)
	{
		UndoRecord u;
		u.mBefore = mState;

		auto currentPos = GetCursorPosition();
		std::string currentString = "";
		for (auto character : mLines[currentPos.mLine]) { currentString += character.mChar; }

		std::string otherString = "";
		for (auto character : mLines[currentPos.mLine + 1]) { otherString += character.mChar; }

		u.mRemoved = currentString + "\n" + otherString;
		u.mRemovedStart = Coordinates(currentPos.mLine, 0);
		u.mRemovedEnd = Coordinates(currentPos.mLine + 1, GetLineMaxColumn(currentPos.mLine + 1));

		u.mAdded = otherString + "\n" + currentString;
		u.mAddedStart = Coordinates(currentPos.mLine, 0);
		u.mAddedEnd = Coordinates(currentPos.mLine + 1, GetLineMaxColumn(currentPos.mLine));

		u.mAfter = mState;

		std::iter_swap(mLines.begin() + GetCursorPosition().mLine, mLines.begin() + GetCursorPosition().mLine + 1);
		MoveDown();

		Colorize(currentPos.mLine);
		Colorize(currentPos.mLine + 1);

		AddUndo(u);
	}
}

void TextEditor::Backspace()
{
	assert(!mReadOnly);

	if (mLines.empty())
		return;

	UndoRecord u;
	u.mBefore = mState;

	if (HasSelection())
	{
		u.mRemoved = GetSelectedText();
		u.mRemovedStart = mState.mSelectionStart;
		u.mRemovedEnd = mState.mSelectionEnd;

		DeleteSelection();
	}
	else
	{
		auto pos = GetActualCursorCoordinates();
		SetCursorPosition(pos);

		if (mState.mCursorPosition.mColumn == 0)
		{
			if (mState.mCursorPosition.mLine == 0)
				return;

			u.mRemoved = '\n';
			u.mRemovedStart = u.mRemovedEnd = Coordinates(pos.mLine - 1, GetLineMaxColumn(pos.mLine - 1));
			Advance(u.mRemovedEnd);

			auto& line = mLines[mState.mCursorPosition.mLine];
			auto& prevLine = mLines[mState.mCursorPosition.mLine - 1];
			auto prevSize = GetLineMaxColumn(mState.mCursorPosition.mLine - 1);
			prevLine.insert(prevLine.end(), line.begin(), line.end());

			ErrorMarkers etmp;
			for (auto& i : mErrorMarkers)
				etmp.insert(ErrorMarkers::value_type(i.first - 1 == mState.mCursorPosition.mLine ? i.first - 1 : i.first, i.second));
			mErrorMarkers = std::move(etmp);

			Breakpoints btmp;
			for (auto& i : mBreakpoints)
				btmp.insert(i - 1 == mState.mCursorPosition.mLine ? i - 1 : i);
			mBreakpoints = std::move(btmp);

			RemoveLine(mState.mCursorPosition.mLine);
			--mState.mCursorPosition.mLine;
			mState.mCursorPosition.mColumn = prevSize;
		}
		else
		{
			auto& line = mLines[mState.mCursorPosition.mLine];
			auto cindex = GetCharacterIndex(pos) - 1;
			auto cend = cindex + 1;
			while (cindex > 0 && IsUTFSequence(line[cindex].mChar))
				--cindex;

			//if (cindex > 0 && UTF8CharLength(line[cindex].mChar) > 1)
			//	--cindex;

			u.mRemovedStart = u.mRemovedEnd = GetActualCursorCoordinates();
			--u.mRemovedStart.mColumn;
			--mState.mCursorPosition.mColumn;

			while (cindex < line.size() && cend-- > cindex)
			{
				u.mRemoved += line[cindex].mChar;
				line.erase(line.begin() + cindex);
			}

			ErrorMarkers etmp;
			for (auto& i : mErrorMarkers)
			{
				if ((i.first - 1) == mState.mCursorPosition.mLine)
					continue;

				etmp.insert(ErrorMarkers::value_type(i.first, i.second));
			}

			mErrorMarkers = std::move(etmp);

			Breakpoints btmp;
			for (auto& i : mBreakpoints)
			{
				if ((i - 1) == mState.mCursorPosition.mLine)
					continue;

				btmp.insert(i);
			}
			mBreakpoints = std::move(btmp);
		}

		mTextChanged = true;

		EnsureCursorVisible();
		Colorize(mState.mCursorPosition.mLine, 1);
	}

	u.mAfter = mState;
	AddUndo(u);
}

void TextEditor::SelectWordUnderCursor()
{
	auto c = GetCursorPosition();
	SetSelection(FindWordStart(c), FindWordEnd(c));
}

void TextEditor::SelectAll()
{
	SetSelection(Coordinates(0, 0), Coordinates((int)mLines.size(), 0));
}

bool TextEditor::HasSelection() const
{
	return mState.mSelectionEnd > mState.mSelectionStart;
}

void TextEditor::Copy()
{
	if (HasSelection())
	{
		ImGui::SetClipboardText(GetSelectedText().c_str());
	}
	else
	{
		if (!mLines.empty())
		{
			std::string str;
			auto& line = mLines[GetActualCursorCoordinates().mLine];
			for (auto& g : line)
				str.push_back(g.mChar);
			ImGui::SetClipboardText(str.c_str());
		}
	}
}

void TextEditor::Cut()
{
	if (IsReadOnly())
	{
		Copy();
	}
	else
	{
		if (HasSelection())
		{
			UndoRecord u;
			u.mBefore = mState;
			u.mRemoved = GetSelectedText();
			u.mRemovedStart = mState.mSelectionStart;
			u.mRemovedEnd = mState.mSelectionEnd;

			Copy();
			DeleteSelection();

			u.mAfter = mState;
			AddUndo(u);
		}
	}
}

void TextEditor::Paste()
{
	if (IsReadOnly())
		return;

	auto clipText = ImGui::GetClipboardText();
	if (clipText != nullptr && strlen(clipText) > 0)
	{
		UndoRecord u;
		u.mBefore = mState;

		if (HasSelection())
		{
			u.mRemoved = GetSelectedText();
			u.mRemovedStart = mState.mSelectionStart;
			u.mRemovedEnd = mState.mSelectionEnd;
			DeleteSelection();
		}

		u.mAdded = clipText;
		u.mAddedStart = GetActualCursorCoordinates();

		InsertText(clipText);

		u.mAddedEnd = GetActualCursorCoordinates();
		u.mAfter = mState;
		AddUndo(u);
	}
}

bool TextEditor::CanUndo() const
{
	return !mReadOnly && mUndoIndex > 0;
}

bool TextEditor::CanRedo() const
{
	return !mReadOnly && mUndoIndex < (int)mUndoBuffer.size();
}

void TextEditor::Undo(int aSteps)
{
	while (CanUndo() && aSteps-- > 0)
		mUndoBuffer[--mUndoIndex].Undo(this);
}

void TextEditor::Redo(int aSteps)
{
	while (CanRedo() && aSteps-- > 0)
		mUndoBuffer[mUndoIndex++].Redo(this);
}

unsigned int TextEditor::GetUndoIndex()
{
	return mUndoIndex;
}

std::string TextEditor::GetText() const
{
	return GetText(Coordinates(), Coordinates((int)mLines.size(), 0));
}

std::vector<std::string> TextEditor::GetTextLines() const
{
	std::vector<std::string> result;

	result.reserve(mLines.size());

	for (auto& line : mLines)
	{
		std::string text;

		text.resize(line.size());

		for (size_t i = 0; i < line.size(); ++i)
			text[i] = line[i].mChar;

		result.emplace_back(std::move(text));
	}

	return result;
}

std::string TextEditor::GetSelectedText() const
{
	return GetText(mState.mSelectionStart, mState.mSelectionEnd);
}

std::string TextEditor::GetCurrentLineText()const
{
	auto lineLength = GetLineMaxColumn(mState.mCursorPosition.mLine);
	return GetText(
		Coordinates(mState.mCursorPosition.mLine, 0),
		Coordinates(mState.mCursorPosition.mLine, lineLength));
}

void TextEditor::ProcessInputs()
{
}

void TextEditor::Colorize(int aFromLine, int aLines)
{
	int toLine = aLines == -1 ? (int)mLines.size() : std::min((int)mLines.size(), aFromLine + aLines);
	mColorRangeMin = std::min(mColorRangeMin, aFromLine);
	mColorRangeMax = std::max(mColorRangeMax, toLine);
	mColorRangeMin = std::max(0, mColorRangeMin);
	mColorRangeMax = std::max(mColorRangeMin, mColorRangeMax);
	mCheckComments = true;
}

void TextEditor::ColorizeRange(int aFromLine, int aToLine)
{
	if (mLines.empty() || aFromLine >= aToLine)
		return;

	std::string buffer;
	std::cmatch results;
	std::string id;

	int endLine = std::max(0, std::min((int)mLines.size(), aToLine));
	for (int i = aFromLine; i < endLine; ++i)
	{
		auto& line = mLines[i];

		if (line.empty())
			continue;

		buffer.resize(line.size());
		for (size_t j = 0; j < line.size(); ++j)
		{
			auto& col = line[j];
			buffer[j] = col.mChar;
			col.mColorIndex = ImGuiCol_TextEditorDefault;
		}

		const char* bufferBegin = &buffer.front();
		const char* bufferEnd = bufferBegin + buffer.size();

		auto last = bufferEnd;

		for (auto first = bufferBegin; first != last; )
		{
			const char* token_begin = nullptr;
			const char* token_end = nullptr;
			ImGuiCol_ token_color = ImGuiCol_TextEditorDefault;

			bool hasTokenizeResult = false;

			if (mLanguageDefinition.mTokenize != nullptr)
			{
				if (mLanguageDefinition.mTokenize(first, last, token_begin, token_end, token_color))
					hasTokenizeResult = true;
			}

			if (hasTokenizeResult == false)
			{
				// todo : remove
				//printf("using regex for %.*s\n", first + 10 < last ? 10 : int(last - first), first);

				for (auto& p : mRegexList)
				{
					if (std::regex_search(first, last, results, p.first, std::regex_constants::match_continuous))
					{
						hasTokenizeResult = true;

						auto& v = *results.begin();
						token_begin = v.first;
						token_end = v.second;
						token_color = p.second;
						break;
					}
				}
			}

			if (hasTokenizeResult == false)
			{
				first++;
			}
			else
			{
				const size_t token_length = token_end - token_begin;

				if (token_color == ImGuiCol_TextEditorIdentifier)
				{
					id.assign(token_begin, token_end);

					// todo : allmost all language definitions use lower case to specify keywords, so shouldn't this use ::tolower ?
					if (!mLanguageDefinition.mCaseSensitive)
						std::transform(id.begin(), id.end(), id.begin(), ::toupper);

					if (!line[first - bufferBegin].mPreprocessor)
					{
						if (mLanguageDefinition.mKeywords.count(id) != 0)
							token_color = ImGuiCol_TextEditorKeyword;
						if (mLanguageDefinition.mSpecialKeywords.count(id) != 0)
							token_color = ImGuiCol_TextEditorSpecialKeyword;
						else if (mLanguageDefinition.mIdentifiers.count(id) != 0)
							token_color = ImGuiCol_TextEditorIdentifier; //Was Known Identifier
						else if (mLanguageDefinition.mPreprocIdentifiers.count(id) != 0)
							token_color = ImGuiCol_TextEditorPreprocessor; //Was Preproc Identifier
					}
					else
					{
						if (mLanguageDefinition.mPreprocIdentifiers.count(id) != 0)
							token_color = ImGuiCol_TextEditorPreprocessor; //Was Preproc Identifier
					}
				}

				for (size_t j = 0; j < token_length; ++j)
					line[(token_begin - bufferBegin) + j].mColorIndex = token_color;

				first = token_end;
			}
		}
	}
}

void TextEditor::ColorizeInternal()
{
	if (mLines.empty() || !mColorizerEnabled)
		return;

	if (mCheckComments)
	{
		auto endLine = mLines.size();
		auto endIndex = 0;
		auto commentStartLine = endLine;
		auto commentStartIndex = endIndex;
		auto withinString = false;
		auto withinSingleLineComment = false;
		auto withinPreproc = false;
		auto firstChar = true;			// there is no other non-whitespace characters in the line before
		auto concatenate = false;		// '\' on the very end of the line
		auto currentLine = 0;
		auto currentIndex = 0;
		while (currentLine < endLine || currentIndex < endIndex)
		{
			auto& line = mLines[currentLine];

			if (currentIndex == 0 && !concatenate)
			{
				withinSingleLineComment = false;
				withinPreproc = false;
				firstChar = true;
			}

			concatenate = false;

			if (!line.empty())
			{
				auto& g = line[currentIndex];
				auto c = g.mChar;

				if (c != mLanguageDefinition.mPreprocChar && !isspace(c))
					firstChar = false;

				if (currentIndex == (int)line.size() - 1 && line[line.size() - 1].mChar == '\\')
					concatenate = true;

				bool inComment = (commentStartLine < currentLine || (commentStartLine == currentLine && commentStartIndex <= currentIndex));

				if (withinString)
				{
					line[currentIndex].mMultiLineComment = inComment;

					if (c == '\"')
					{
						if (currentIndex + 1 < (int)line.size() && line[currentIndex + 1].mChar == '\"')
						{
							currentIndex += 1;
							if (currentIndex < (int)line.size())
								line[currentIndex].mMultiLineComment = inComment;
						}
						else
							withinString = false;
					}
					else if (c == '\\')
					{
						currentIndex += 1;
						if (currentIndex < (int)line.size())
							line[currentIndex].mMultiLineComment = inComment;
					}
				}
				else
				{
					if (firstChar && c == mLanguageDefinition.mPreprocChar)
						withinPreproc = true;

					if (c == '\"')
					{
						withinString = true;
						line[currentIndex].mMultiLineComment = inComment;
					}
					else
					{
						auto pred = [](const char& a, const Glyph& b) { return a == b.mChar; };
						auto from = line.begin() + currentIndex;
						auto& startStr = mLanguageDefinition.mCommentStart;
						auto& singleStartStr = mLanguageDefinition.mSingleLineComment;

						if (singleStartStr.size() > 0 &&
							currentIndex + singleStartStr.size() <= line.size() &&
							equals(singleStartStr.begin(), singleStartStr.end(), from, from + singleStartStr.size(), pred))
						{
							withinSingleLineComment = true;
						}
						else if (!withinSingleLineComment && currentIndex + startStr.size() <= line.size() &&
							equals(startStr.begin(), startStr.end(), from, from + startStr.size(), pred))
						{
							commentStartLine = currentLine;
							commentStartIndex = currentIndex;
						}

						inComment = inComment = (commentStartLine < currentLine || (commentStartLine == currentLine && commentStartIndex <= currentIndex));

						line[currentIndex].mMultiLineComment = inComment;
						line[currentIndex].mComment = withinSingleLineComment;

						auto& endStr = mLanguageDefinition.mCommentEnd;
						if (currentIndex + 1 >= (int)endStr.size() &&
							equals(endStr.begin(), endStr.end(), from + 1 - endStr.size(), from + 1, pred))
						{
							commentStartIndex = endIndex;
							commentStartLine = endLine;
						}
					}
				}
				line[currentIndex].mPreprocessor = withinPreproc;
				currentIndex += UTF8CharLength(c);
				if (currentIndex >= (int)line.size())
				{
					currentIndex = 0;
					++currentLine;
				}
			}
			else
			{
				currentIndex = 0;
				++currentLine;
			}
		}
		mCheckComments = false;
	}

	if (mColorRangeMin < mColorRangeMax)
	{
		const int increment = (mLanguageDefinition.mTokenize == nullptr) ? 10 : 10000;
		const int to = std::min(mColorRangeMin + increment, mColorRangeMax);
		ColorizeRange(mColorRangeMin, to);
		mColorRangeMin = to;

		if (mColorRangeMax == mColorRangeMin)
		{
			mColorRangeMin = std::numeric_limits<int>::max();
			mColorRangeMax = 0;
		}
		return;
	}
}

float TextEditor::TextDistanceToLineStart(const Coordinates& aFrom) const
{
	auto& line = mLines[aFrom.mLine];
	float distance = 0.0f;
	float spaceSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, " ", nullptr, nullptr).x;
	int colIndex = GetCharacterIndex(aFrom);
	for (size_t it = 0u; it < line.size() && it < colIndex; )
	{
		if (line[it].mChar == '\t')
		{
			distance = (1.0f + std::floor((1.0f + distance) / (float(mTabSize) * spaceSize))) * (float(mTabSize) * spaceSize);
			++it;
		}
		else
		{
			auto d = UTF8CharLength(line[it].mChar);
			char tempCString[7];
			int i = 0;
			for (; i < 6 && d-- > 0 && it < (int)line.size(); i++, it++)
				tempCString[i] = line[it].mChar;

			tempCString[i] = '\0';
			distance += ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, tempCString, nullptr, nullptr).x;
		}
	}

	return distance;
}

void TextEditor::EnsureCursorVisible()
{
	if (!mWithinRender)
	{
		mScrollToCursor = true;
		return;
	}

	float scrollX = ImGui::GetScrollX();
	float scrollY = ImGui::GetScrollY();

	auto height = ImGui::GetWindowHeight();
	auto width = ImGui::GetWindowWidth();

	auto top = 1 + (int)ceil(scrollY / mCharAdvance.y);
	auto bottom = (int)ceil((scrollY + height) / mCharAdvance.y);

	auto left = (int)ceil(scrollX / mCharAdvance.x);
	auto right = (int)ceil((scrollX + width) / mCharAdvance.x);

	auto pos = GetActualCursorCoordinates();
	auto len = TextDistanceToLineStart(pos);

	if (pos.mLine < top)
		ImGui::SetScrollY(std::max(0.0f, (pos.mLine - 1) * mCharAdvance.y));
	if (pos.mLine > bottom - 4)
		ImGui::SetScrollY(std::max(0.0f, (pos.mLine + 4) * mCharAdvance.y - height));
	if (len + mTextStart < left + 4)
		ImGui::SetScrollX(std::max(0.0f, len + mTextStart - 4));
	if (len + mTextStart > right - 4)
		ImGui::SetScrollX(std::max(0.0f, len + mTextStart + 4 - width));
}

int TextEditor::GetPageSize() const
{
	auto height = ImGui::GetWindowHeight() - 20.0f;
	return (int)floor(height / mCharAdvance.y);
}

TextEditor::UndoRecord::UndoRecord(
	const std::string& aAdded,
	const TextEditor::Coordinates aAddedStart,
	const TextEditor::Coordinates aAddedEnd,
	const std::string& aRemoved,
	const TextEditor::Coordinates aRemovedStart,
	const TextEditor::Coordinates aRemovedEnd,
	TextEditor::EditorState& aBefore,
	TextEditor::EditorState& aAfter)
	: mAdded(aAdded)
	, mAddedStart(aAddedStart)
	, mAddedEnd(aAddedEnd)
	, mRemoved(aRemoved)
	, mRemovedStart(aRemovedStart)
	, mRemovedEnd(aRemovedEnd)
	, mBefore(aBefore)
	, mAfter(aAfter)
{
	assert(mAddedStart <= mAddedEnd);
	assert(mRemovedStart <= mRemovedEnd);
}

void TextEditor::UndoRecord::Undo(TextEditor* aEditor)
{
	if (!mAdded.empty())
	{
		aEditor->DeleteRange(mAddedStart, mAddedEnd);
		aEditor->Colorize(mAddedStart.mLine - 1, mAddedEnd.mLine - mAddedStart.mLine + 2);
	}

	if (!mRemoved.empty())
	{
		auto start = mRemovedStart;
		aEditor->InsertTextAt(start, mRemoved.c_str());
		aEditor->Colorize(mRemovedStart.mLine - 1, mRemovedEnd.mLine - mRemovedStart.mLine + 2);
	}

	aEditor->mState = mBefore;
	aEditor->EnsureCursorVisible();

}

void TextEditor::UndoRecord::Redo(TextEditor* aEditor)
{
	if (!mRemoved.empty())
	{
		aEditor->DeleteRange(mRemovedStart, mRemovedEnd);
		aEditor->Colorize(mRemovedStart.mLine - 1, mRemovedEnd.mLine - mRemovedStart.mLine + 1);
	}

	if (!mAdded.empty())
	{
		auto start = mAddedStart;
		aEditor->InsertTextAt(start, mAdded.c_str());
		aEditor->Colorize(mAddedStart.mLine - 1, mAddedEnd.mLine - mAddedStart.mLine + 2);
	}

	aEditor->mState = mAfter;
	aEditor->EnsureCursorVisible();
}

static bool TokenizeCStyleString(const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end)
{
	const char* p = in_begin;

	if (*p == '"')
	{
		p++;

		while (p < in_end)
		{
			// handle end of string
			if (*p == '"')
			{
				out_begin = in_begin;
				out_end = p + 1;
				return true;
			}

			// handle escape character for "
			if (*p == '\\' && p + 1 < in_end && p[1] == '"')
				p++;

			p++;
		}
	}

	return false;
}

static bool TokenizeCStyleCharacterLiteral(const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end)
{
	const char* p = in_begin;

	if (*p == '\'')
	{
		p++;

		// handle escape characters
		if (p < in_end && *p == '\\')
			p++;

		if (p < in_end)
			p++;

		// handle end of character literal
		if (p < in_end && *p == '\'')
		{
			out_begin = in_begin;
			out_end = p + 1;
			return true;
		}
	}

	return false;
}

static bool TokenizeCStyleIdentifier(const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end)
{
	const char* p = in_begin;

	if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || *p == '_')
	{
		p++;

		while ((p < in_end) && ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') || *p == '_'))
			p++;

		out_begin = in_begin;
		out_end = p;
		return true;
	}

	return false;
}

static bool TokenizeCStyleNumber(const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end)
{
	const char* p = in_begin;

	const bool startsWithNumber = *p >= '0' && *p <= '9';

	if (*p != '+' && *p != '-' && !startsWithNumber)
		return false;

	p++;

	bool hasNumber = startsWithNumber;

	while (p < in_end && (*p >= '0' && *p <= '9'))
	{
		hasNumber = true;

		p++;
	}

	if (hasNumber == false)
		return false;

	bool isFloat = false;
	bool isHex = false;
	bool isBinary = false;

	if (p < in_end)
	{
		if (*p == '.')
		{
			isFloat = true;

			p++;

			while (p < in_end && (*p >= '0' && *p <= '9'))
				p++;
		}
		else if (*p == 'x' || *p == 'X')
		{
			// hex formatted integer of the type 0xef80

			isHex = true;

			p++;

			while (p < in_end && ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F')))
				p++;
		}
		else if (*p == 'b' || *p == 'B')
		{
			// binary formatted integer of the type 0b01011101

			isBinary = true;

			p++;

			while (p < in_end && (*p >= '0' && *p <= '1'))
				p++;
		}
	}

	if (isHex == false && isBinary == false)
	{
		// floating point exponent
		if (p < in_end && (*p == 'e' || *p == 'E'))
		{
			isFloat = true;

			p++;

			if (p < in_end && (*p == '+' || *p == '-'))
				p++;

			bool hasDigits = false;

			while (p < in_end && (*p >= '0' && *p <= '9'))
			{
				hasDigits = true;

				p++;
			}

			if (hasDigits == false)
				return false;
		}

		// single precision floating point type
		if (p < in_end && *p == 'f')
			p++;
	}

	if (isFloat == false)
	{
		// integer size type
		while (p < in_end && (*p == 'u' || *p == 'U' || *p == 'l' || *p == 'L'))
			p++;
	}

	out_begin = in_begin;
	out_end = p;
	return true;
}

static bool TokenizeCStylePunctuation(const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end)
{
	(void)in_end;

	switch (*in_begin)
	{
	case '[':
	case ']':
	case '{':
	case '}':
	case '!':
	case '%':
	case '^':
	case '&':
	case '*':
	case '(':
	case ')':
	case '-':
	case '+':
	case '=':
	case '~':
	case '|':
	case '<':
	case '>':
	case '?':
	case ':':
	case '/':
	case ';':
	case ',':
	case '.':
		out_begin = in_begin;
		out_end = in_begin + 1;
		return true;
	}

	return false;
}

const TextEditor::LanguageDefinition& TextEditor::LanguageDefinition::None()
{
	static bool inited = false;
	static LanguageDefinition langDef;
	if (!inited)
	{
		langDef.mCommentStart = std::string::npos;
		langDef.mCommentEnd = std::string::npos;
		langDef.mSingleLineComment = std::string::npos;

		langDef.mCaseSensitive = true;
		langDef.mAutoIndentation = true;

		langDef.mName = "None";

		inited = true;
	}
	return langDef;
}

const TextEditor::LanguageDefinition& TextEditor::LanguageDefinition::CPlusPlus()
{
	static bool inited = false;
	static LanguageDefinition langDef;
	if (!inited)
	{
		static const char* const cppKeywords[] = {
			"alignas", "alignof", "and", "and_eq", "asm", "atomic_cancel", "atomic_commit", "atomic_noexcept", "auto", "bitand", "bitor", "bool", "char", "char8_t", "char16_t", "char32_t", "class",
			"compl", "concept", "const", "constexpr", "const_cast", "decltype", "delete", "double", "dynamic_cast", "enum", "explicit", "export", "extern", "false", "float",
			"friend", "import", "inline", "int", "long", "module", "mutable", "namespace", "new", "noexcept", "not", "not_eq", "nullptr", "operator", "or", "or_eq", "private", "protected", "public",
			"register", "reinterpret_cast", "requires", "short", "signed", "sizeof", "static", "static_assert", "static_cast", "struct", "synchronized", "template", "this", "thread_local",
			"true", "typedef", "typeid", "typename", "union", "unsigned", "using", "virtual", "void", "volatile", "wchar_t", "xor", "xor_eq"
		};
		static const char* const cppKeywordsDeclarations[] = {
			/*alignas*/ "Specifies custom alignment of variables and user defined types.",
			/*alignof*/ "Returns the alignment, in bytes if the specified type.",
			/*and*/ "Logical AND operator returns [true] if both operands are [true] and returns [false] otherwise.\nAlternative operator for [&&].",
			/*and_eq*/ "Performs a Bitwise AND operation [&] and stores the result in the left operand.\nAlternative operator for [&=].",
			/*asm*/ "Embeds assembly language source code within a C++ program.",
			/*atomic_cancel*/ "Cancel transaction",
			/*atomic_commit*/ "Commit transaction.",
			/*atomic_noexcept*/ "Begin transaction",
			/*auto*/ "The auto keyword is a simple way to declare a variable that has a complicated type.\nThe data type is deduced automatically by the compiler.",
			/*bitand*/ "Bitwise AND. Returns 1 only if both bits are 1.\nAlternative operator for [&].",
			/*bitor*/ "Bitwise OR. Returns 1 if either of the bits is 1 and returns 0 if both the bits are 0 or 1.\nAlternative operator for [|].",
			/*bool*/ "Boolean data type. Use in declaration of data type. Conditional expressions take a bool value.\nValue can be [true] or [false].",
			/*char*/ "Used to store characters from the ASCII character set. Char is an 8-bit type.\nAn unsigned char is often used to represent a byte, which is not a built in data type.",
			/*char8_t*/ "8-bit wide unicode character.",
			/*char16_t*/ "16-bit wide unicode character.",
			/*char32_t*/ "32-bit wide unicode character.",
			/*class*/ "The class keyword declares a class type or defines an object of a class type.\nIt has a 'tag', or the type name given to the class, becoming a data type.\nThe 'base list' declares the classes or structures this class derives from.\nIncludes a 'member list' which  can be marked as [public], [private], or [protected].\n[private] by default.",
			/*compl*/ "Bitwise NOT, or complement. Changes each bit to its opposite, 0 becomes 1, and 1 becomes 0.\nAlternative operator for [~]",
			/*concept*/ "A concept is a named set associated with a constraint, which specifies the requirements on template arguments, selecting the most appropriate overload.",
			/*const*/ "The const keyword specifies to the compiler that the object or variable is not modifiable.\nDeclaring a const member function means that it is 'read only'.",
			/*constexpr*/ "A constant expression. Unlike const, constexpr can also be applied to functions and class constructors. Where possible, computed at compile time.",
			/*const_cast*/ "Removes the const, volatile and __unaligned attribute(s) from a class.",
			/*decltype*/ "The decltype type specifier yields the type of a specified expression.\nOften used with the [auto] keyword to declare a template function with return type depending on arguments.",
			/*delete*/ "Deallocates a block of memory.\nUse delete[] before a pointer to free an array.",
			/*double*/ "Floating-point number.\n15 - 16 significant digits. 8 bytes.",
			/*dynamic_cast*/ "Converts the operand expression to an object of type type-id.",
			/*enum*/ "An enumeration is a user defined data type. Used to assign names to integral constants.\nBy default, underlying type is [int], but signed and unsigned forms of [short], [long], [__int32], [__int64], or [char] can be used.",
			/*explicit*/ "The explicit keyword is used to mark constructors to not implicitly convert types.",
			/*export*/ "Used to mark a template definition exported, which allows for the same template to be declared but not defined, in other translational units.",
			/*extern*/ "Specifies that the symbol has external linkage. Linker looks for the definition in another translation unit.",
			/*false*/ "Boolean literal.",
			/*float*/ "Floating-point number.\n6 - 7 significant digits. 4 bytes.",
			/*friend*/ "Grants member level access to functions that are not members of a class, or to all members in a separate class.",
			/*import*/ "Imports a module unit / module partition / header unit.",
			/*inline*/ "Inline is used to specify a function defined in the body of a class declaration.",
			/*int*/ "Integer data type. Can be signed or unsigned.",
			/*long*/ "Long type modifier. Used for long integers.",
			/*module*/ "Declaration placed at the beginning of a module implementation file to specify that the file contents belongs to the named module.",
			/*mutable*/ "Allows for a value to be assigned to this data member from a [const] member function.\nOnly applies to non-static and non-const data members of a class.",
			/*namespace*/ "A namespace is a declarative region that provides a scope to the identifiers inside it.\nUsed to organize code into logical groups and prevent name collisions occurring.",
			/*new*/ "Allocates memory for an object or array of objects from the free store and returns a suitably typed, nonzero pointer to the object.",
			/*noexcept*/ "Specifies weather a function might throw exceptions. Takes in a constant expression.",
			/*not*/ "Logical NOT operator reverses meaning of its operand. Returns [true] if operand is [false] and returns [false] if operand is [true].\nAlternative operator for [!].",
			/*not_eq*/ "Returns a [bool] value. If both operands are not equal returns [true], otherwise returns [false].\nAlternative operator for [!=].",
			/*nullptr*/ "A null pointer constant.\nUsed to indicate that an object handle, interior pointer or native pointer doesn't point to an object.",
			/*operator*/ "Declares a function specifying what an 'operator symbol' means when applied to instances of a class.\nOverloads are differentiated based on the types of operands.",
			/*or*/ "Logical OR operator returns [true] if either or both operands are [true], otherwise returns [false]\\nAlternative operator for [||].",
			/*or_eq*/ "Performs a Bitwise OR operation [|] and stores the result in the left operand.\nAlternative operator for [|=].",
			/*private*/ "Access specifier keyword.\nWhen preceding a list of class members, indicates those members are only accessible from other members or friends of the class.",
			/*protected*/ "Access specifier keyword.\nSpecifies that protected class members can only be used by member functions that original declared them, friends of the class that originally declared them, classes derived with public or protected access from the class that originally declared these members or direct privately derives classes that also have private access to protected members.",
			/*public*/ "Access specifier keyword.\nWhen preceding a list of class members, indicates those members are accessible from any function.",
			/*register*/ "Register variables are stored in a register of the processor, rather than memory, making them faster to access compared to the [auto] keyword.",
			/*reinterpret_cast*/ "Allows any pointer to be converted into any other pointer type.\nAlso allows any integral type to be converted into any pointer type and vice versa.",
			/*requires*/ "Specifies a constant expression on template parameters that evaluate a requirement.",
			/*short*/ "Short type modifier. Used for small integers.",
			/*signed*/ "Signed type modifier.",
			/*sizeof*/ "Returns a [size_t] in number of bytes. Operand can be a type name, or an expression.",
			/*static*/ "Has a global lifetime, but is only visible within the block in which it was declared. Stored on the heap.",
			/*static_assert*/ "Tests assertion at compile time. If expression is [false], compiler displays specifies message if one is provided.",
			/*static_cast*/ "Converts an expression to the type of type-id based on the types that are present in the expression.", 
			/*struct*/ "The struct keyword declares a structure type or defines an object of a structure type.\nIt has a 'tag', or the type name given to the class, becoming a data type.\nThe 'base list' declares the classes or structures this class derives from.\nIncludes a 'member list' which  can be marked as [public], [private], or [protected].\n[public] by default.", 
			/*synchronized*/ "Executes the compound statement as if under a global lock. The end of each synchronized block synchronizes with the beginning of the next synchronized block in that order.",
			/*template*/ "A construct that generates an ordinary type or function at compile time based on the arguments the user supplies for the template parameters.",
			/*this*/ "Points to the object for which the member function is called.\nOnly accessible within non static member functions of a class, struct or union.",
			/*thread_local*/ "Declares that a variable is only accessible on the thread on which it was created. Each thread will have its own copy of the variable",
			/*true*/ "Boolean literal.",
			/*typedef*/ "A typedef declaration introduces a name that, with its scope, becomes a synonym for the type given by the type-declaration portion of the declaration.",
			/*typeid*/ "The typeid operator allows the type of an object to be determined at run time.",
			/*typename*/ "Used in template definitions, provides a hint to the compiler that an unknown identifier is a type.",
			/*union*/ "A union is a user-defined type in which all members share the same memory location. A union can contain no more than one object from its list of members. Only enough memory for its largest member. Includes a 'tag', or the type name given, and a 'member list'.",
			/*unsigned*/ "Unsigned type modifier.",
			/*using*/ "The using declaration introduces a name into the declarative region in which the using declaration appears.",
			/*virtual*/ "Declares a virtual function or a virtual base class.",
			/*void*/ "When used as a function type, specifies that function has no return value. When used in a parameter list, specifies it takes no parameters. When used in the declaration of a pointer, specifies that pointer is 'universal'.",
			/*volatile*/ "A type qualifier that can be used to declare that an object can be modified in the program by the hardware.",
			/*wchar_t*/ "The wchar_t type is used to represent a 16 - bit wide character used to store Unicode encoded characters.",
			/*xor*/ "Bitwise Exclusive OR. If the bit in one operand is 0 and the other is 1, the corresponding bit is set to 1, otherwise the corresponding bit is set to 0.\nAlternative operator for [^].",
			/*xor_eq*/ "Performs a Bitwise Exclusive OR operation [^] and stores the result in the left operand.\nAlternative operator for [^=]."
		};
		//for (auto& k : cppKeywords)
		//	langDef.mKeywords.insert(k);
		for (int i = 0; i < sizeof(cppKeywords) / sizeof(cppKeywords[0]); i++)
		{
			Identifier id;
			id.mDeclaration = cppKeywordsDeclarations[i];
			langDef.mKeywords.insert(std::make_pair(std::string(cppKeywords[i]), id));
		}

		static const char* const cppSpecialKeywords[] = {
			"break", "case", "catch", "continue", "default", "do", "else", "for", "goto", "if", "return", "switch", "throw", "try", "while"
		};
		static const char* const cppSpecialKeywordsDeclarations[] = {
			/*break*/ "The break statement ends execution of the nearest enclosing loop or conditional statement in which it appears.\nControl is passed to the statement that follows the ended statement.\nBreak only ends the statement it is in, not nested structures.",
			/*case*/ "A test for a possible value that is provided in a switch statement.\nData types include: [int], [char], [bool], [enum]",
			/*catch*/ "One or more catch blocks can follow a [try] block when handling an exception. Each block specifies the type of exception it can handle.",
			/*continue*/ "Continue forces transfer of control to the controlling expression of the smallest enclosing do, for or while loop. Any remaining statements in the current iteration are not executed.",
			/*default*/ "A default label is an optional label in a switch statement body.\n May only appear once in the statement body.",
			/*do*/ "Executes a statement repeatedly until the specified termination condition (the expression) evaluates to zero. Post loop condition check; therefore, the loop executes one or more times.",
			/*else*/ "The else statement follows an [if] statement, and the branch it contains is only executed if the [if] statements condition fails.",
			/*for*/ "Executes a statement repeatedly until the condition becomes false.\nContains an initial expression, condition expression and a loop expression.",
			/*goto*/ "Unconditionally transfers control to the statement labeled by the specified identifier.",
			/*if*/ "If condition evaluates to a non-zero (true) value, statement within branch is executed.\nIf condition succeeds next optional else is skipped.",
			/*return*/ "Terminates the execution of a function and returns control to the calling function. An expression is returned with the same type as the function, aside from [void].",
			/*switch*/ "Allows selection among multiple section of code depending on the value of an integral expression. The condition must have an integral type.\nThe body consists of multiple [case] labels and an optional [default] label. [Break] keyword must be used to end execution after [case].",
			/*throw*/ "A throw expression signals that an exceptional condition - often, and error - has occurred in a [try] block. An object of any type can be used as the operand of a [throw] expression that typically is used to communicate information about the error.",
			/*try*/ "A try block is used to enclose one or more statements that might throw an exception.",
			/*while*/ "Executes statement repeatedly until expression evaluates to zero. Pre loop condition check; therefore, the loop executes zero or more times.",
		};
		for (int i = 0; i < sizeof(cppSpecialKeywords) / sizeof(cppSpecialKeywords[0]); i++)
		{
			Identifier id;
			id.mDeclaration = cppSpecialKeywordsDeclarations[i];
			langDef.mSpecialKeywords.insert(std::make_pair(std::string(cppSpecialKeywords[i]), id));
		}

		static const char* const identifiers[] = {
			"abort", "abs", "acos", "asin", "atan", "atexit", "atof", "atoi", "atol", "atoll", "ceil", "clock", "cosh", "ctime", "div", "exit", "fabs", "floor", "fmod", "getchar", "getenv", "isalnum", "isalpha", "isdigit", "isgraph",
			"ispunct", "isspace", "isupper", "kbhit", "log10", "log2", "log", "memcmp", "modf", "pow", "printf", "sprintf", "snprintf", "putchar", "putenv", "puts", "rand", "remove", "rename", "sinh", "sqrt", "srand", "strcat", "strcmp", "strerror", "time", "tolower", "toupper",
			"std", "string", "wstring", "vector", "map", "unordered_map", "set", "unordered_set", "min", "max",
			"Dymatic", "DY_CORE_TRACE", "DY_CORE_INFO", "DY_CORE_WARN", "DY_CORE_ERROR", "DY_CORE_CRITICAL", "DY_TRACE", "DY_INFO", "DY_WARN", "DY_ERROR", "DY_CRITICAL", "DYCLASS", "DYFUNCTION", "DYPARAM",
			"glm", "ImGui"
		};
		static const char* const declarations[] = {
			/*abort*/ "void abort();\n[[noreturn]] void abort() noexcept;\n\nCauses abnormal program termination.",
			/*abs*/ "int abs( int n );\nlong abs( long n );\nlong long abs( long long n );\nstd::intmax_t abs( std::intmax_t n );\n\nComputes the absolute value of an integer number, if representable.",
			/*acos*/ "float acos ( float arg );\ndouble acos ( double arg );\nlong double acos ( long double arg );\ndouble acos ( IntegralType arg );\n\nComputes the principal value of the arc cosine of arg.",
			/*asin*/ "float asin ( float arg );\ndouble asin ( double arg );\nlong double asin ( long double arg );\ndouble asin ( IntegralType arg );\n\nComputes the principal value of the arc sine of arg.",
			/*atan*/ "float atan ( float arg );\ndouble atan ( double arg );\nlong double atan ( long double arg );\ndouble atan ( IntegralType arg );\n\nComputes the principal value of the arc tangent of arg.",
			/*atexit*/ "int atexit( /*atexit-handler*/* func ) noexcept;\n\nRegisters the function pointed to by func to be called on normal program termination (via [std::exit()] or returning from the main function).",
			/*atof*/ "double atof( const char *str );\n\nInterprets a floating point value in a byte string pointed to by str.",
			/*atoi*/ "int atoi( const char *str );\n\nInterprets an integer value in a byte string pointed to by str.",
			/*atol*/ "long atol( const char *str );\n\nInterprets a long value in a byte string pointed to by str.",
			/*atoll*/ "long long atol( const char *str );\n\nInterprets a long long value in a byte string pointed to by str.",
			/*ceil*/ "float ceil ( float arg );\ndouble ceil ( double arg );\nlong double ceil ( long double arg );\ndouble ceil ( IntegralType arg );\n\nComputes the smallest integer value not less than arg.",
			/*clock*/ "std::clock_t clock();\n\nReturns the approximate processor time used by the process since the beginning of an implementation-defined era related to the program's execution.",
			/*cosh*/ "float cosh ( float arg );\ndouble cosh ( double arg );\nlong double cosh ( long double arg );\ndouble cosh ( IntegralType arg );\n\nComputes the hyperbolic cosine of arg.",
			/*ctime*/ "char* ctime( const std::time_t* time );\n\nConverts given time since epoch to a calendar local time and then to a textual representation, as if by calling [std::asctime]([std::localtime(time)]).\nThe resulting string has the following format: Www Mmm dd hh : mm:ss yyyy",
			/*div*/ "std::div_t div( int x, int y );\nstd::ldiv_t div( long x, long y );\nstd::lldiv_t div( long long x, long long y );\nstd::imaxdiv_t div( std::intmax_t x, std::intmax_t y );\n\nComputes both the quotient and the remainder of the division of the numerator x by the denominator y.",
			/*exit*/ "[[noreturn]] void exit( int exit_code );\n\nCauses normal program termination to occur. Several cleanup steps are performed.",
			/*fabs*/ "float fabs ( float arg );\ndouble fabs ( double arg );\nlong double fabs ( long double arg );\ndouble fabs ( IntegralType arg );\n\nComputes the absolute value of a floating point value arg.",
			/*floor*/ "float floor ( float arg );\ndouble floor ( double arg );\nlong double floor ( long double arg );\ndouble floor ( IntegralType arg );\n\nComputes the largest integer value not greater than arg.",
			/*fmod*/ "float fmod ( float x, float y );\ndouble fmod ( double x, double y );\nlong double fmod ( long double x, long double y );\nPromoted fmod ( Arithmetic1 x, Arithmetic2 y );\n\nComputes the floating-point remainder of the division operation x/y.",
			/*getchar*/ "int getchar();\n\nReads the next character from [stdin]. Equivalent to [std::getc(stdin)].",
			/*getenv*/ "char* getenv( const char* env_var );\n\nSearches the environment list provided by the host environment (the OS), for a string that matches the C string pointed to by env_var and returns a pointer to the C string that is associated with the matched environment list member.",
			/*isalnum*/ "int isalnum( int ch );\n\nChecks if the given character is an alphanumeric character as classified by the current C locale. In the default locale, the following characters are alphanumeric:\n    - digits (0123456789)\n    - uppercase letters (ABCDEFGHIJKLMNOPQRSTUVWXYZ)\n    - lowercase letters (abcdefghijklmnopqrstuvwxyz)",
			/*isalpha*/ "int isalpha( int ch );\n\nChecks if the given character is an alphabetic character as classified by the currently installed C locale. In the default locale, the following characters are alphabetic:\n    - uppercase letters (ABCDEFGHIJKLMNOPQRSTUVWXYZ)\n    - lowercase letters (abcdefghijklmnopqrstuvwxyz)",
			/*isdigit*/ "int isdigit( int ch );\n\nChecks if the given character is one of the 10 decimal digits: 0123456789.",
			/*isgraph*/ "int isgraph( int ch );\n\nChecks if the given character is graphic (has a graphical representation) as classified by the currently installed C locale. In the default C locale, the following characters are graphic:\n    - digits (0123456789)\n    - uppercase letters (ABCDEFGHIJKLMNOPQRSTUVWXYZ)\n    - lowercase letters (abcdefghijklmnopqrstuvwxyz)\n    - punctuation characters (!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~)",
			/*ispunct*/ "int ispunct( int ch );\n\nChecks if the given character is a punctuation character as classified by the current C locale. The default C locale classifies the characters !\"#$ % &'()*+,-./:;<=>?@[\\]^_`{|}~ as punctuation.",
			/*isspace*/ "int isspace( int ch );\n\nChecks if the given character is whitespace character as classified by the currently installed C locale. In the default locale, the whitespace characters are the following:\n    - space(0x20, ' ')\n    - form feed(0x0c, '\\f')\n    - line feed(0x0a, '\\n')\n    - carriage return (0x0d, '\\r')\n    - horizontal tab(0x09, '\\t')\n    - vertical tab(0x0b, '\\v')",
			/*isupper*/ "int isupper( int ch );\n\nChecks if the given character is an uppercase character as classified by the currently installed C locale. In the default \"C\" locale, isupper returns a nonzero value only for the uppercase letters (ABCDEFGHIJKLMNOPQRSTUVWXYZ).\nIf isupper returns a nonzero value, it is guaranteed that iscntrl, isdigit, ispunct,and isspace return zero for the same character in the same C locale.",
			/*kbhit*/ "Note : kbhit() is not a standard library function and should be avoided.",
			/*log10*/ "float log10 ( float arg );\ndouble log10 ( double arg );\nlong double log10 ( long double arg );\ndouble log10 ( IntegralType arg );\n\nComputes the common (base-10) logarithm of arg.",
			/*log2*/ "float log2 ( float arg );\ndouble log2 ( double arg );\nlong double log2 ( long double arg );\ndouble log2 ( IntegralType arg );\n\nComputes the binary (base-2) logarithm of arg.",
			/*log*/ "float log ( float arg );\ndouble log ( double arg );\nlong double log ( long double arg );\ndouble log ( IntegralType arg );\n\nComputes the natural (base e) logarithm of arg.",
			/*memcmp*/ "int memcmp( const void* lhs, const void* rhs, std::size_t count );\n\nReinterprets the objects pointed to by lhs and rhs as arrays of unsigned char and compares the first count characters of these arrays. The comparison is done lexicographically.",
			/*modf*/ "float modf ( float x, float* iptr );\ndouble modf ( double x, double* iptr );\nlong double modf ( long double x, long double* iptr );\n\nDecomposes given floating point value x into integral and fractional parts, each having the same type and sign as x. The integral part (in floating-point format) is stored in the object pointed to by iptr.",
			/*pow*/ "float pow ( float base, float exp );\ndouble pow ( double base, double exp );\nlong double pow ( long double base, long double exp );\ndouble pow ( double base, int iexp );\nlong double pow ( long double base, int iexp );\n\nComputes the value of base raised to the power exp or iexp.",
			/*printf*/ "int printf( const char* format, ... );\n\nWrites the results to stdout.",
			/*sprintf*/ "int sprintf( char* buffer, const char* format, ... );\n\nWrites the results to a character string buffer.",
			/*snprintf*/ "int snprintf( char* buffer, std::size_t buf_size, const char* format, ... );\n\nWrites the results to a character string buffer. At most buf_size - 1 characters are written. The resulting character string will be terminated with a null character, unless buf_size is zero. If buf_size is zero, nothing is written and buffer may be a null pointer, however the return value (number of bytes that would be written not including the null terminator) is still calculated and returned.",
			/*putchar*/ "int putchar( int ch );\n\nWrites a character ch to stdout. Internally, the character is converted to unsigned char just before being written.",
			/*putenv*/ "Note: putenv() i a C function, not a C++ function.\nA better solution is [setenv].",
			/*puts*/ "int puts( const char *str );\n\nWrites every character from the null-terminated string str and one additional newline character '\\n' to the output stream stdout, as if by repeatedly executing std::fputc.\nThe terminating null character from str is not written.",
			/*rand*/ "int rand();\n\nReturns a pseudo-random integral value between 0 and RAND_MAX (0 and RAND_MAX included).",
			/*remove*/ "template< class ForwardIt, class T >\nForwardIt remove(ForwardIt first, ForwardIt last, const T & value);\n\nRemoves all elements that are equal to value, using operator== to compare them.",
			/*rename*/ "int rename( const char *old_filename, const char *new_filename );\n\nChanges the filename of a file. The file is identified by character string pointed to by old_filename. The new filename is identified by character string pointed to by new_filename.",
			/*sinh*/ "float sinh ( float arg );\ndouble sinh ( double arg );\nlong double sinh ( long double arg );\ndouble sinh ( IntegralType arg );\n\nComputes the hyperbolic sine of arg.",
			/*sqrt*/ "float sqrt ( float arg );\ndouble sqrt ( double arg );\nlong double sqrt ( long double arg );\ndouble sqrt ( IntegralType arg );\n\nComputes the square root of arg.",
			/*srand*/ "void srand( unsigned seed );\n\nSeeds the pseudo-random number generator used by std::rand() with the value seed.",
			/*strcat*/ "char *strcat( char *dest, const char *src );\n\nAppends a copy of the character string pointed to by src to the end of the character string pointed to by dest. The character src[0] replaces the null terminator at the end of dest. The resulting byte string is null-terminated.",
			/*strcmp*/ "int strcmp( const char *lhs, const char *rhs );\n\nCompares two null-terminated byte strings lexicographically.",
			/*strerror*/ "char* strerror( int errnum );\n\nReturns a pointer to the textual description of the system error code errnum, identical to the description that would be printed by std::perror().",
			/*time*/ "std::time_t time( std::time_t* arg );\n\nReturns the current calendar time encoded as a std::time_t object, and also stores it in the object pointed to by arg, unless arg is a null pointer.",
			/*tolower*/ "int tolower( int ch );\n\nConverts the given character to lowercase according to the character conversion rules defined by the currently installed C locale.\nIn the default \"C\" locale, the following uppercase letters ABCDEFGHIJKLMNOPQRSTUVWXYZ are replaced with respective lowercase letters abcdefghijklmnopqrstuvwxyz.",
			/*toupper*/ "int toupper( int ch );\n\nConverts the given character to uppercase according to the character conversion rules defined by the currently installed C locale.\nIn the default \"C\" locale, the following lowercase letters abcdefghijklmnopqrstuvwxyz are replaced with respective uppercase letters ABCDEFGHIJKLMNOPQRSTUVWXYZ.",
			/*std*/ "The C++ standard library [namespace].",
			/*string*/ "std::basic_string<char>\n\nStores and manipulates sequences of char-like objects.",
			/*wstring*/ "std::basic_string<wchar_t>\n\nStores and manipulates sequences of char-like objects.",
			/*vector*/ "template<class T, class Allocator = std::allocator<T>> class vector;\n\nstd::vector is a sequence container that encapsulates dynamic size arrays.",
			/*map*/ "template<class Key, class T, class Compare = std::less<Key>, class Allocator = std::allocator<std::pair<const Key, T> >> class map;\n\nstd::map is a sorted associative container that contains key - value pairs with unique keys.",
			/*unordered_map*/ "template< class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>, class Allocator = std::allocator< std::pair<const Key, T> >> class unordered_map;\n\nUnordered map is an associative container that contains key - value pairs with unique keys.Search, insertion,and removal of elements have average constant - time complexity.\nInternally, the elements are not sorted in any particular order, but organized into buckets.",
			/*set*/ "template<class Key, class Compare = std::less<Key>, class Allocator = std::allocator<Key>> class set;\n\nstd::set is an associative container that contains a sorted set of unique objects of type Key.",
			/*unordered_set*/ "template<class Key, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>, class Allocator = std::allocator<Key>> class unordered_set;\n\nUnordered set is an associative container that contains a set of unique objects of type Key.Search, insertion,and removal have average constant - time complexity.\nInternally, the elements are not sorted in any particular order, but organized into buckets.",
			/*min*/ "template< class T >\nconst T & min(const T & a, const T & b);\ntemplate< class T, class Compare >\nconst T & min(const T & a, const T & b, Compare comp);\ntemplate< class T >\nT min(std::initializer_list<T> ilist);\ntemplate< class T, class Compare >\nT min(std::initializer_list<T> ilist, Compare comp);\n\nReturns the smaller of the given values. All have [constexpr] alternatives.",
			/*max*/ "template< class T >\nconst T & max(const T & a, const T & b);\ntemplate< class T, class Compare >\nconst T & max(const T & a, const T & b, Compare comp);\ntemplate< class T >\nT max(std::initializer_list<T> ilist);\ntemplate< class T, class Compare >\nT max(std::initializer_list<T> ilist, Compare comp);\n\nReturns the greater of the given values. All have [constexpr] alternatives.",
			// Dymatic Identifiers
			/*Dymatic*/ "Dymatic Engine [namespace].",
			/*DY_CORE_TRACE*/	"Outputs message with a varying number of arguments to the CORE [Dymatic] Log.\n\n'Trace' severity.\n\nNote: Internal use only.",
			/*DY_CORE_INFO*/	"Outputs message with a varying number of arguments to the CORE [Dymatic] Log.\n\n'Info' severity.\n\nNote: Internal use only.",
			/*DY_CORE_WARN*/	"Outputs message with a varying number of arguments to the CORE [Dymatic] Log.\n\n'Warning' severity.\n\nNote: Internal use only.",
			/*DY_CORE_ERROR*/	"Outputs message with a varying number of arguments to the CORE [Dymatic] Log.\n\n'Error' severity.\n\nNote: Internal use only.",
			/*DY_CORE_CRITICAL*/"Outputs message with a varying number of arguments to the CORE [Dymatic] Log.\n\n'Critical' severity.\n\nNote: Internal use only.",
			/*DY_TRACE*/		 "Outputs message with a varying number of arguments to the APPLICATION [Dymatic] Log.\n\n'Trace' severity.",
			/*DY_INFO*/			 "Outputs message with a varying number of arguments to the APPLICATION [Dymatic] Log.\n\n'Info' severity.",
			/*DY_WARN*/			 "Outputs message with a varying number of arguments to the APPLICATION [Dymatic] Log.\n\n'Warning' severity.",
			/*DY_ERROR*/		 "Outputs message with a varying number of arguments to the APPLICATION [Dymatic] Log.\n\n'Error' severity.",
			/*DY_CRITICAL*/		 "Outputs message with a varying number of arguments to the APPLICATION [Dymatic] Log.\n\n'Critical' severity.",
			/*DYCLASS*/		 "Marks the following class to be interpreted by the Dymatic Parser and imported into the node editor.\nMetadata can be passed in using the format: {data_name} = {data_value}",
			/*DYFUNCTION*/		 "Marks the following function to be interpreted by the Dymatic Parser and imported into the node editor.\nMetadata can be passed in using the format: {data_name} = {data_value}",
			/*DYPARAM*/		 "Marks the following parameter to be interpreted by the Dymatic Parser and imported into the node editor.\nMetadata can be passed in using the format: {data_name} = {data_value}",
			/*glm*/ "OpenGL Mathematics [namespace].",
			/*ImGui*/ "Dear ImGui [namespace].\n\GUI used internally to render [Dymatic] tools."
		};
		//for (auto& k : identifiers)
		//{
		//	Identifier id;
		//	id.mDeclaration = "Built-in function";
		//	langDef.mIdentifiers.insert(std::make_pair(std::string(k), id));
		//}
		for (int i = 0; i < sizeof(identifiers) / sizeof(identifiers[0]); i++)
		{
			Identifier id;
			id.mDeclaration = declarations[i];
			langDef.mIdentifiers.insert(std::make_pair(std::string(identifiers[i]), id));
		}

		// Custom Preprocessor Identifiers
		static const char* const preprocessorIdentifiers[] = {
			"if", "elif", "else", "endif",
			"ifdef", "ifndef", "define", "undef",
			"include", "line", "error", "pragma"
		};

		//Declaration definitions taken from https://cppreference.com
		static const char* const preprocessorDeclarations[] = {
			"If the expression evaluates to nonzero value, the controlled code block is included and skipped otherwise.", "An #else statement with an #if condition. If the expression evaluates to nonzero value, the controlled code block is included and skipped otherwise. There can be any number of #elif s", "If all previous #if and #elif statements fail, the controlled code block is included and skipped otherwise.", "The #endif directive terminates the conditional preprocessing block.\nCan be included after #if, #elif, or #else.",
			"Checks if the identifier was defined as a macro name.", "Checks if the identifier was not defined as a macro name.", "This directive defines the identifier as macro. The #define directive defines a function-like macro with variable number of arguments, but no regular arguments.", "The #undef directive undefines the identifier, that is cancels previous definition of the identifier by #define directive. If the identifier does not have associated macro, the directive is ignored.",
			"Includes source file, identified by filename into the current source file at the line immediately after the directive. Any preprocessing tokens (macro constants or expressions) are allowed as arguments to #include", "Changes the current preprocessor line number to lineno. Expansions of the macro __LINE__ beyond this point will expand to lineno plus the number of actual source code lines encountered since.", "After encountering the #error directive, an implementation displays the diagnostic message error_message and renders the program ill-formed (the compilation stops).", 
			"Pragma directive controls implementation-specific behavior of the compiler, such as disabling compiler warnings or changing alignment requirements. Any pragma that is not recognized is ignored."
		};

		for (int i = 0; i < sizeof(preprocessorIdentifiers) / sizeof(preprocessorIdentifiers[0]); i++)
		{
			Identifier id;
			id.mDeclaration = preprocessorDeclarations[i];
			langDef.mPreprocIdentifiers.insert(std::make_pair(std::string(preprocessorIdentifiers[i]), id));
		}
		// End Custom Code

		langDef.mTokenize = [](const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end, ImGuiCol_& paletteIndex) -> bool
		{
			paletteIndex = ImGuiCol_COUNT;

			while (in_begin < in_end && isascii(*in_begin) && isblank(*in_begin))
				in_begin++;

			if (in_begin == in_end)
			{
				out_begin = in_end;
				out_end = in_end;
				paletteIndex = ImGuiCol_TextEditorDefault;
			}
			else if (TokenizeCStyleString(in_begin, in_end, out_begin, out_end))
				paletteIndex = ImGuiCol_TextEditorString;
			else if (TokenizeCStyleCharacterLiteral(in_begin, in_end, out_begin, out_end))
				paletteIndex = ImGuiCol_TextEditorCharLiteral;
			else if (TokenizeCStyleIdentifier(in_begin, in_end, out_begin, out_end))
				paletteIndex = ImGuiCol_TextEditorIdentifier;
			else if (TokenizeCStyleNumber(in_begin, in_end, out_begin, out_end))
				paletteIndex = ImGuiCol_TextEditorNumber;
			else if (TokenizeCStylePunctuation(in_begin, in_end, out_begin, out_end))
				paletteIndex = ImGuiCol_TextEditorPunctuation;

			return paletteIndex != ImGuiCol_COUNT;
		};

		//Custom
		//langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("namespacee[ \\t]*[\\n\\t]*(\\w+)", PaletteIndex::ErrorMarker));
		

		langDef.mCommentStart = "/*";
		langDef.mCommentEnd = "*/";
		langDef.mSingleLineComment = "//";

		langDef.mCaseSensitive = true;
		langDef.mAutoIndentation = true;

		langDef.mName = "C++";

		inited = true;
	}
	return langDef;
}

const TextEditor::LanguageDefinition& TextEditor::LanguageDefinition::HLSL()
{
	static bool inited = false;
	static LanguageDefinition langDef;
	if (!inited)
	{
		static const char* const keywords[] = {
			"AppendStructuredBuffer", "asm", "asm_fragment", "BlendState", "bool", "break", "Buffer", "ByteAddressBuffer", "case", "cbuffer", "centroid", "class", "column_major", "compile", "compile_fragment",
			"CompileShader", "const", "continue", "ComputeShader", "ConsumeStructuredBuffer", "default", "DepthStencilState", "DepthStencilView", "discard", "do", "double", "DomainShader", "dword", "else",
			"export", "extern", "false", "float", "for", "fxgroup", "GeometryShader", "groupshared", "half", "Hullshader", "if", "in", "inline", "inout", "InputPatch", "int", "interface", "line", "lineadj",
			"linear", "LineStream", "matrix", "min16float", "min10float", "min16int", "min12int", "min16uint", "namespace", "nointerpolation", "noperspective", "NULL", "out", "OutputPatch", "packoffset",
			"pass", "pixelfragment", "PixelShader", "point", "PointStream", "precise", "RasterizerState", "RenderTargetView", "return", "register", "row_major", "RWBuffer", "RWByteAddressBuffer", "RWStructuredBuffer",
			"RWTexture1D", "RWTexture1DArray", "RWTexture2D", "RWTexture2DArray", "RWTexture3D", "sample", "sampler", "SamplerState", "SamplerComparisonState", "shared", "snorm", "stateblock", "stateblock_state",
			"static", "string", "struct", "switch", "StructuredBuffer", "tbuffer", "technique", "technique10", "technique11", "texture", "Texture1D", "Texture1DArray", "Texture2D", "Texture2DArray", "Texture2DMS",
			"Texture2DMSArray", "Texture3D", "TextureCube", "TextureCubeArray", "true", "typedef", "triangle", "triangleadj", "TriangleStream", "uint", "uniform", "unorm", "unsigned", "vector", "vertexfragment",
			"VertexShader", "void", "volatile", "while",
			"bool1","bool2","bool3","bool4","double1","double2","double3","double4", "float1", "float2", "float3", "float4", "int1", "int2", "int3", "int4", "in", "out", "inout",
			"uint1", "uint2", "uint3", "uint4", "dword1", "dword2", "dword3", "dword4", "half1", "half2", "half3", "half4",
			"float1x1","float2x1","float3x1","float4x1","float1x2","float2x2","float3x2","float4x2",
			"float1x3","float2x3","float3x3","float4x3","float1x4","float2x4","float3x4","float4x4",
			"half1x1","half2x1","half3x1","half4x1","half1x2","half2x2","half3x2","half4x2",
			"half1x3","half2x3","half3x3","half4x3","half1x4","half2x4","half3x4","half4x4",
		};
		//for (auto& k : keywords)
		//	langDef.mKeywords.insert(k);
		for (auto& k : keywords)
		{
			Identifier id;
			id.mDeclaration = "Keyword";
			langDef.mKeywords.insert(std::make_pair(std::string(k), id));
		}

		static const char* const identifiers[] = {
			"abort", "abs", "acos", "all", "AllMemoryBarrier", "AllMemoryBarrierWithGroupSync", "any", "asdouble", "asfloat", "asin", "asint", "asint", "asuint",
			"asuint", "atan", "atan2", "ceil", "CheckAccessFullyMapped", "clamp", "clip", "cos", "cosh", "countbits", "cross", "D3DCOLORtoUBYTE4", "ddx",
			"ddx_coarse", "ddx_fine", "ddy", "ddy_coarse", "ddy_fine", "degrees", "determinant", "DeviceMemoryBarrier", "DeviceMemoryBarrierWithGroupSync",
			"distance", "dot", "dst", "errorf", "EvaluateAttributeAtCentroid", "EvaluateAttributeAtSample", "EvaluateAttributeSnapped", "exp", "exp2",
			"f16tof32", "f32tof16", "faceforward", "firstbithigh", "firstbitlow", "floor", "fma", "fmod", "frac", "frexp", "fwidth", "GetRenderTargetSampleCount",
			"GetRenderTargetSamplePosition", "GroupMemoryBarrier", "GroupMemoryBarrierWithGroupSync", "InterlockedAdd", "InterlockedAnd", "InterlockedCompareExchange",
			"InterlockedCompareStore", "InterlockedExchange", "InterlockedMax", "InterlockedMin", "InterlockedOr", "InterlockedXor", "isfinite", "isinf", "isnan",
			"ldexp", "length", "lerp", "lit", "log", "log10", "log2", "mad", "max", "min", "modf", "msad4", "mul", "noise", "normalize", "pow", "printf",
			"Process2DQuadTessFactorsAvg", "Process2DQuadTessFactorsMax", "Process2DQuadTessFactorsMin", "ProcessIsolineTessFactors", "ProcessQuadTessFactorsAvg",
			"ProcessQuadTessFactorsMax", "ProcessQuadTessFactorsMin", "ProcessTriTessFactorsAvg", "ProcessTriTessFactorsMax", "ProcessTriTessFactorsMin",
			"radians", "rcp", "reflect", "refract", "reversebits", "round", "rsqrt", "saturate", "sign", "sin", "sincos", "sinh", "smoothstep", "sqrt", "step",
			"tan", "tanh", "tex1D", "tex1D", "tex1Dbias", "tex1Dgrad", "tex1Dlod", "tex1Dproj", "tex2D", "tex2D", "tex2Dbias", "tex2Dgrad", "tex2Dlod", "tex2Dproj",
			"tex3D", "tex3D", "tex3Dbias", "tex3Dgrad", "tex3Dlod", "tex3Dproj", "texCUBE", "texCUBE", "texCUBEbias", "texCUBEgrad", "texCUBElod", "texCUBEproj", "transpose", "trunc"
		};
		for (auto& k : identifiers)
		{
			Identifier id;
			id.mDeclaration = "Built-in function";
			langDef.mIdentifiers.insert(std::make_pair(std::string(k), id));
		}

		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("[ \\t]*#[ \\t]*[a-zA-Z_]+", ImGuiCol_TextEditorPreprocessor));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("L?\\\"(\\\\.|[^\\\"])*\\\"", ImGuiCol_TextEditorString));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("\\'\\\\?[^\\']\\'", ImGuiCol_TextEditorCharLiteral));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", ImGuiCol_TextEditorNumber));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", ImGuiCol_TextEditorNumber));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("0[0-7]+[Uu]?[lL]?[lL]?", ImGuiCol_TextEditorNumber));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", ImGuiCol_TextEditorNumber));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("[a-zA-Z_][a-zA-Z0-9_]*", ImGuiCol_TextEditorIdentifier));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", ImGuiCol_TextEditorPunctuation));

		langDef.mCommentStart = "/*";
		langDef.mCommentEnd = "*/";
		langDef.mSingleLineComment = "//";

		langDef.mCaseSensitive = true;
		langDef.mAutoIndentation = true;

		langDef.mName = "HLSL";

		inited = true;
	}
	return langDef;
}

const TextEditor::LanguageDefinition& TextEditor::LanguageDefinition::GLSL()
{
	static bool inited = false;
	static LanguageDefinition langDef;
	if (!inited)
	{
		static const char* const keywords[] = {
			"auto", "break", "case", "char", "const", "continue", "default", "do", "double", "else", "enum", "extern", "float", "for", "goto", "if", "inline", "int", "long", "register", "restrict", "return", "short",
			"signed", "sizeof", "static", "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while", "_Alignas", "_Alignof", "_Atomic", "_Bool", "_Complex", "_Generic", "_Imaginary",
			"_Noreturn", "_Static_assert", "_Thread_local"
		};
		//for (auto& k : keywords)
		//	langDef.mKeywords.insert(k);
		for (auto& k : keywords)
		{
			Identifier id;
			id.mDeclaration = "Keyword";
			langDef.mKeywords.insert(std::make_pair(std::string(k), id));
		}

		static const char* const identifiers[] = {
			"abort", "abs", "acos", "asin", "atan", "atexit", "atof", "atoi", "atol", "ceil", "clock", "cosh", "ctime", "div", "exit", "fabs", "floor", "fmod", "getchar", "getenv", "isalnum", "isalpha", "isdigit", "isgraph",
			"ispunct", "isspace", "isupper", "kbhit", "log10", "log2", "log", "memcmp", "modf", "pow", "putchar", "putenv", "puts", "rand", "remove", "rename", "sinh", "sqrt", "srand", "strcat", "strcmp", "strerror", "time", "tolower", "toupper"
		};
		for (auto& k : identifiers)
		{
			Identifier id;
			id.mDeclaration = "Built-in function";
			langDef.mIdentifiers.insert(std::make_pair(std::string(k), id));
		}

		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("[ \\t]*#[ \\t]*[a-zA-Z_]+", ImGuiCol_TextEditorPreprocessor));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("L?\\\"(\\\\.|[^\\\"])*\\\"", ImGuiCol_TextEditorString));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("\\'\\\\?[^\\']\\'", ImGuiCol_TextEditorCharLiteral));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", ImGuiCol_TextEditorNumber));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", ImGuiCol_TextEditorNumber));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("0[0-7]+[Uu]?[lL]?[lL]?", ImGuiCol_TextEditorNumber));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", ImGuiCol_TextEditorNumber));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("[a-zA-Z_][a-zA-Z0-9_]*", ImGuiCol_TextEditorIdentifier));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", ImGuiCol_TextEditorPunctuation));

		langDef.mCommentStart = "/*";
		langDef.mCommentEnd = "*/";
		langDef.mSingleLineComment = "//";

		langDef.mCaseSensitive = true;
		langDef.mAutoIndentation = true;

		langDef.mName = "GLSL";

		inited = true;
	}
	return langDef;
}

const TextEditor::LanguageDefinition& TextEditor::LanguageDefinition::C()
{
	static bool inited = false;
	static LanguageDefinition langDef;
	if (!inited)
	{
		static const char* const keywords[] = {
			"auto", "break", "case", "char", "const", "continue", "default", "do", "double", "else", "enum", "extern", "float", "for", "goto", "if", "inline", "int", "long", "register", "restrict", "return", "short",
			"signed", "sizeof", "static", "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while", "_Alignas", "_Alignof", "_Atomic", "_Bool", "_Complex", "_Generic", "_Imaginary",
			"_Noreturn", "_Static_assert", "_Thread_local"
		};
		//for (auto& k : keywords)
		//	langDef.mKeywords.insert(k);
		for (auto& k : keywords)
		{
			Identifier id;
			id.mDeclaration = "Keyword";
			langDef.mKeywords.insert(std::make_pair(std::string(k), id));
		}

		static const char* const identifiers[] = {
			"abort", "abs", "acos", "asin", "atan", "atexit", "atof", "atoi", "atol", "ceil", "clock", "cosh", "ctime", "div", "exit", "fabs", "floor", "fmod", "getchar", "getenv", "isalnum", "isalpha", "isdigit", "isgraph",
			"ispunct", "isspace", "isupper", "kbhit", "log10", "log2", "log", "memcmp", "modf", "pow", "putchar", "putenv", "puts", "rand", "remove", "rename", "sinh", "sqrt", "srand", "strcat", "strcmp", "strerror", "time", "tolower", "toupper"
		};
		for (auto& k : identifiers)
		{
			Identifier id;
			id.mDeclaration = "Built-in function";
			langDef.mIdentifiers.insert(std::make_pair(std::string(k), id));
		}

		langDef.mTokenize = [](const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end, ImGuiCol_& paletteIndex) -> bool
		{
			paletteIndex = ImGuiCol_COUNT;

			while (in_begin < in_end && isascii(*in_begin) && isblank(*in_begin))
				in_begin++;

			if (in_begin == in_end)
			{
				out_begin = in_end;
				out_end = in_end;
				paletteIndex = ImGuiCol_TextEditorDefault;
			}
			else if (TokenizeCStyleString(in_begin, in_end, out_begin, out_end))
				paletteIndex = ImGuiCol_TextEditorString;
			else if (TokenizeCStyleCharacterLiteral(in_begin, in_end, out_begin, out_end))
				paletteIndex = ImGuiCol_TextEditorCharLiteral;
			else if (TokenizeCStyleIdentifier(in_begin, in_end, out_begin, out_end))
				paletteIndex = ImGuiCol_TextEditorIdentifier;
			else if (TokenizeCStyleNumber(in_begin, in_end, out_begin, out_end))
				paletteIndex = ImGuiCol_TextEditorNumber;
			else if (TokenizeCStylePunctuation(in_begin, in_end, out_begin, out_end))
				paletteIndex = ImGuiCol_TextEditorPunctuation;

			return paletteIndex != ImGuiCol_COUNT;
		};

		langDef.mCommentStart = "/*";
		langDef.mCommentEnd = "*/";
		langDef.mSingleLineComment = "//";

		langDef.mCaseSensitive = true;
		langDef.mAutoIndentation = true;

		langDef.mName = "C";

		inited = true;
	}
	return langDef;
}

const TextEditor::LanguageDefinition& TextEditor::LanguageDefinition::SQL()
{
	static bool inited = false;
	static LanguageDefinition langDef;
	if (!inited)
	{
		static const char* const keywords[] = {
			"ADD", "EXCEPT", "PERCENT", "ALL", "EXEC", "PLAN", "ALTER", "EXECUTE", "PRECISION", "AND", "EXISTS", "PRIMARY", "ANY", "EXIT", "PRINT", "AS", "FETCH", "PROC", "ASC", "FILE", "PROCEDURE",
			"AUTHORIZATION", "FILLFACTOR", "PUBLIC", "BACKUP", "FOR", "RAISERROR", "BEGIN", "FOREIGN", "READ", "BETWEEN", "FREETEXT", "READTEXT", "BREAK", "FREETEXTTABLE", "RECONFIGURE",
			"BROWSE", "FROM", "REFERENCES", "BULK", "FULL", "REPLICATION", "BY", "FUNCTION", "RESTORE", "CASCADE", "GOTO", "RESTRICT", "CASE", "GRANT", "RETURN", "CHECK", "GROUP", "REVOKE",
			"CHECKPOINT", "HAVING", "RIGHT", "CLOSE", "HOLDLOCK", "ROLLBACK", "CLUSTERED", "IDENTITY", "ROWCOUNT", "COALESCE", "IDENTITY_INSERT", "ROWGUIDCOL", "COLLATE", "IDENTITYCOL", "RULE",
			"COLUMN", "IF", "SAVE", "COMMIT", "IN", "SCHEMA", "COMPUTE", "INDEX", "SELECT", "CONSTRAINT", "INNER", "SESSION_USER", "CONTAINS", "INSERT", "SET", "CONTAINSTABLE", "INTERSECT", "SETUSER",
			"CONTINUE", "INTO", "SHUTDOWN", "CONVERT", "IS", "SOME", "CREATE", "JOIN", "STATISTICS", "CROSS", "KEY", "SYSTEM_USER", "CURRENT", "KILL", "TABLE", "CURRENT_DATE", "LEFT", "TEXTSIZE",
			"CURRENT_TIME", "LIKE", "THEN", "CURRENT_TIMESTAMP", "LINENO", "TO", "CURRENT_USER", "LOAD", "TOP", "CURSOR", "NATIONAL", "TRAN", "DATABASE", "NOCHECK", "TRANSACTION",
			"DBCC", "NONCLUSTERED", "TRIGGER", "DEALLOCATE", "NOT", "TRUNCATE", "DECLARE", "NULL", "TSEQUAL", "DEFAULT", "NULLIF", "UNION", "DELETE", "OF", "UNIQUE", "DENY", "OFF", "UPDATE",
			"DESC", "OFFSETS", "UPDATETEXT", "DISK", "ON", "USE", "DISTINCT", "OPEN", "USER", "DISTRIBUTED", "OPENDATASOURCE", "VALUES", "DOUBLE", "OPENQUERY", "VARYING","DROP", "OPENROWSET", "VIEW",
			"DUMMY", "OPENXML", "WAITFOR", "DUMP", "OPTION", "WHEN", "ELSE", "OR", "WHERE", "END", "ORDER", "WHILE", "ERRLVL", "OUTER", "WITH", "ESCAPE", "OVER", "WRITETEXT"
		};

		//for (auto& k : keywords)
		//	langDef.mKeywords.insert(k);
		for (auto& k : keywords)
		{
			Identifier id;
			id.mDeclaration = "Keyword";
			langDef.mKeywords.insert(std::make_pair(std::string(k), id));
		}

		static const char* const identifiers[] = {
			"ABS",  "ACOS",  "ADD_MONTHS",  "ASCII",  "ASCIISTR",  "ASIN",  "ATAN",  "ATAN2",  "AVG",  "BFILENAME",  "BIN_TO_NUM",  "BITAND",  "CARDINALITY",  "CASE",  "CAST",  "CEIL",
			"CHARTOROWID",  "CHR",  "COALESCE",  "COMPOSE",  "CONCAT",  "CONVERT",  "CORR",  "COS",  "COSH",  "COUNT",  "COVAR_POP",  "COVAR_SAMP",  "CUME_DIST",  "CURRENT_DATE",
			"CURRENT_TIMESTAMP",  "DBTIMEZONE",  "DECODE",  "DECOMPOSE",  "DENSE_RANK",  "DUMP",  "EMPTY_BLOB",  "EMPTY_CLOB",  "EXP",  "EXTRACT",  "FIRST_VALUE",  "FLOOR",  "FROM_TZ",  "GREATEST",
			"GROUP_ID",  "HEXTORAW",  "INITCAP",  "INSTR",  "INSTR2",  "INSTR4",  "INSTRB",  "INSTRC",  "LAG",  "LAST_DAY",  "LAST_VALUE",  "LEAD",  "LEAST",  "LENGTH",  "LENGTH2",  "LENGTH4",
			"LENGTHB",  "LENGTHC",  "LISTAGG",  "LN",  "LNNVL",  "LOCALTIMESTAMP",  "LOG",  "LOWER",  "LPAD",  "LTRIM",  "MAX",  "MEDIAN",  "MIN",  "MOD",  "MONTHS_BETWEEN",  "NANVL",  "NCHR",
			"NEW_TIME",  "NEXT_DAY",  "NTH_VALUE",  "NULLIF",  "NUMTODSINTERVAL",  "NUMTOYMINTERVAL",  "NVL",  "NVL2",  "POWER",  "RANK",  "RAWTOHEX",  "REGEXP_COUNT",  "REGEXP_INSTR",
			"REGEXP_REPLACE",  "REGEXP_SUBSTR",  "REMAINDER",  "REPLACE",  "ROUND",  "ROWNUM",  "RPAD",  "RTRIM",  "SESSIONTIMEZONE",  "SIGN",  "SIN",  "SINH",
			"SOUNDEX",  "SQRT",  "STDDEV",  "SUBSTR",  "SUM",  "SYS_CONTEXT",  "SYSDATE",  "SYSTIMESTAMP",  "TAN",  "TANH",  "TO_CHAR",  "TO_CLOB",  "TO_DATE",  "TO_DSINTERVAL",  "TO_LOB",
			"TO_MULTI_BYTE",  "TO_NCLOB",  "TO_NUMBER",  "TO_SINGLE_BYTE",  "TO_TIMESTAMP",  "TO_TIMESTAMP_TZ",  "TO_YMINTERVAL",  "TRANSLATE",  "TRIM",  "TRUNC", "TZ_OFFSET",  "UID",  "UPPER",
			"USER",  "USERENV",  "VAR_POP",  "VAR_SAMP",  "VARIANCE",  "VSIZE "
		};
		for (auto& k : identifiers)
		{
			Identifier id;
			id.mDeclaration = "Built-in function";
			langDef.mIdentifiers.insert(std::make_pair(std::string(k), id));
		}

		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("L?\\\"(\\\\.|[^\\\"])*\\\"", ImGuiCol_TextEditorString));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("\\\'[^\\\']*\\\'", ImGuiCol_TextEditorString));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", ImGuiCol_TextEditorNumber));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", ImGuiCol_TextEditorNumber));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("0[0-7]+[Uu]?[lL]?[lL]?", ImGuiCol_TextEditorNumber));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", ImGuiCol_TextEditorNumber));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("[a-zA-Z_][a-zA-Z0-9_]*", ImGuiCol_TextEditorIdentifier));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", ImGuiCol_TextEditorPunctuation));

		langDef.mCommentStart = "/*";
		langDef.mCommentEnd = "*/";
		langDef.mSingleLineComment = "//";

		langDef.mCaseSensitive = false;
		langDef.mAutoIndentation = false;

		langDef.mName = "SQL";

		inited = true;
	}
	return langDef;
}

const TextEditor::LanguageDefinition& TextEditor::LanguageDefinition::AngelScript()
{
	static bool inited = false;
	static LanguageDefinition langDef;
	if (!inited)
	{
		static const char* const keywords[] = {
			"and", "abstract", "auto", "bool", "break", "case", "cast", "class", "const", "continue", "default", "do", "double", "else", "enum", "false", "final", "float", "for",
			"from", "funcdef", "function", "get", "if", "import", "in", "inout", "int", "interface", "int8", "int16", "int32", "int64", "is", "mixin", "namespace", "not",
			"null", "or", "out", "override", "private", "protected", "return", "set", "shared", "super", "switch", "this ", "true", "typedef", "uint", "uint8", "uint16", "uint32",
			"uint64", "void", "while", "xor"
		};

		//for (auto& k : keywords)
		//	langDef.mKeywords.insert(k);
		for (auto& k : keywords)
		{
			Identifier id;
			id.mDeclaration = "Keyword";
			langDef.mKeywords.insert(std::make_pair(std::string(k), id));
		}

		static const char* const identifiers[] = {
			"cos", "sin", "tab", "acos", "asin", "atan", "atan2", "cosh", "sinh", "tanh", "log", "log10", "pow", "sqrt", "abs", "ceil", "floor", "fraction", "closeTo", "fpFromIEEE", "fpToIEEE",
			"complex", "opEquals", "opAddAssign", "opSubAssign", "opMulAssign", "opDivAssign", "opAdd", "opSub", "opMul", "opDiv"
		};
		for (auto& k : identifiers)
		{
			Identifier id;
			id.mDeclaration = "Built-in function";
			langDef.mIdentifiers.insert(std::make_pair(std::string(k), id));
		}

		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("L?\\\"(\\\\.|[^\\\"])*\\\"", ImGuiCol_TextEditorString));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("\\'\\\\?[^\\']\\'", ImGuiCol_TextEditorString));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", ImGuiCol_TextEditorNumber));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", ImGuiCol_TextEditorNumber));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("0[0-7]+[Uu]?[lL]?[lL]?", ImGuiCol_TextEditorNumber));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", ImGuiCol_TextEditorNumber));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("[a-zA-Z_][a-zA-Z0-9_]*", ImGuiCol_TextEditorIdentifier));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", ImGuiCol_TextEditorPunctuation));

		langDef.mCommentStart = "/*";
		langDef.mCommentEnd = "*/";
		langDef.mSingleLineComment = "//";

		langDef.mCaseSensitive = true;
		langDef.mAutoIndentation = true;

		langDef.mName = "AngelScript";

		inited = true;
	}
	return langDef;
}

const TextEditor::LanguageDefinition& TextEditor::LanguageDefinition::Lua()
{
	static bool inited = false;
	static LanguageDefinition langDef;
	if (!inited)
	{
		static const char* const keywords[] = {
			"and", "break", "do", "", "else", "elseif", "end", "false", "for", "function", "if", "in", "", "local", "nil", "not", "or", "repeat", "return", "then", "true", "until", "while"
		};

		//for (auto& k : keywords)
		//	langDef.mKeywords.insert(k);
		for (auto& k : keywords)
		{
			Identifier id;
			id.mDeclaration = "Keyword";
			langDef.mKeywords.insert(std::make_pair(std::string(k), id));
		}

		static const char* const identifiers[] = {
			"assert", "collectgarbage", "dofile", "error", "getmetatable", "ipairs", "loadfile", "load", "loadstring",  "next",  "pairs",  "pcall",  "print",  "rawequal",  "rawlen",  "rawget",  "rawset",
			"select",  "setmetatable",  "tonumber",  "tostring",  "type",  "xpcall",  "_G",  "_VERSION","arshift", "band", "bnot", "bor", "bxor", "btest", "extract", "lrotate", "lshift", "replace",
			"rrotate", "rshift", "create", "resume", "running", "status", "wrap", "yield", "isyieldable", "debug","getuservalue", "gethook", "getinfo", "getlocal", "getregistry", "getmetatable",
			"getupvalue", "upvaluejoin", "upvalueid", "setuservalue", "sethook", "setlocal", "setmetatable", "setupvalue", "traceback", "close", "flush", "input", "lines", "open", "output", "popen",
			"read", "tmpfile", "type", "write", "close", "flush", "lines", "read", "seek", "setvbuf", "write", "__gc", "__tostring", "abs", "acos", "asin", "atan", "ceil", "cos", "deg", "exp", "tointeger",
			"floor", "fmod", "ult", "log", "max", "min", "modf", "rad", "random", "randomseed", "sin", "sqrt", "string", "tan", "type", "atan2", "cosh", "sinh", "tanh",
			"pow", "frexp", "ldexp", "log10", "pi", "huge", "maxinteger", "mininteger", "loadlib", "searchpath", "seeall", "preload", "cpath", "path", "searchers", "loaded", "module", "require", "clock",
			"date", "difftime", "execute", "exit", "getenv", "remove", "rename", "setlocale", "time", "tmpname", "byte", "char", "dump", "find", "format", "gmatch", "gsub", "len", "lower", "match", "rep",
			"reverse", "sub", "upper", "pack", "packsize", "unpack", "concat", "maxn", "insert", "pack", "unpack", "remove", "move", "sort", "offset", "codepoint", "char", "len", "codes", "charpattern",
			"coroutine", "table", "io", "os", "string", "utf8", "bit32", "math", "debug", "package"
		};
		for (auto& k : identifiers)
		{
			Identifier id;
			id.mDeclaration = "Built-in function";
			langDef.mIdentifiers.insert(std::make_pair(std::string(k), id));
		}

		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("L?\\\"(\\\\.|[^\\\"])*\\\"", ImGuiCol_TextEditorString));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("\\\'[^\\\']*\\\'", ImGuiCol_TextEditorString));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", ImGuiCol_TextEditorNumber));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", ImGuiCol_TextEditorNumber));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", ImGuiCol_TextEditorNumber));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("[a-zA-Z_][a-zA-Z0-9_]*", ImGuiCol_TextEditorIdentifier));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, ImGuiCol_>("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", ImGuiCol_TextEditorPunctuation));

		langDef.mCommentStart = "--[[";
		langDef.mCommentEnd = "]]";
		langDef.mSingleLineComment = "--";

		langDef.mCaseSensitive = true;
		langDef.mAutoIndentation = false;

		langDef.mName = "Lua";

		inited = true;
	}
	return langDef;
}
}

namespace Dymatic {

	static ImU32   TabBarCalcTabID(ImGuiTabBar* tab_bar, const char* label)
	{
		if (tab_bar->Flags & ImGuiTabBarFlags_DockNode)
		{
			ImGuiID id = ImHashStr(label);
			ImGui::KeepAliveID(id);
			return id;
		}
		else
		{
			ImGuiWindow* window = GImGui->CurrentWindow;
			return window->GetID(label);
		}
	}

	TextEditorPannel::TextEditorPannel()
	{
	}

	void TextEditorPannel::OnImGuiRender()
	{
		auto& io = ImGui::GetIO();

		if (m_TextEditorVisible)
		{
			for (int i = 0; i < m_TextEditors.size(); i++)
			{
				if (m_TextEditors[i].pendingClose && !ImGui::IsPopupOpen("Unsaved Changes##TextEditor"))
				{
					ImGui::OpenPopup("Unsaved Changes##TextEditor");
				}
			}
			if (ImGui::BeginPopupModal("Unsaved Changes##TextEditor"))
			{
				ImGui::Text("Unsaved changes to:");
				for (int i = 0; i < m_TextEditors.size(); i++)
				{
					if (m_TextEditors[i].pendingClose)
					{
						ImGui::BulletText(m_TextEditors[i].Filename.c_str());
					}
				}
				if (ImGui::Button("Save##UnsavedChangesText")) { for (int i = 0; i < m_TextEditors.size(); i++) { if (m_TextEditors[i].pendingClose) { if (SaveTextFileByReference(&m_TextEditors[i])) { m_TextEditors.erase(m_TextEditors.begin() + i); i--; } else { break; } } } ImGui::CloseCurrentPopup(); }
				ImGui::SameLine();
				if (ImGui::Button("Discard##UnsavedChangesText")) { for (int i = 0; i < m_TextEditors.size(); i++) { if (m_TextEditors[i].pendingClose) { m_TextEditors.erase(m_TextEditors.begin() + i); i--; } } ImGui::CloseCurrentPopup(); }
				ImGui::SameLine();
				if (ImGui::Button("Cancel##UnsavedChangesText")) { for (int i = 0; i < m_TextEditors.size(); i++) { if (m_TextEditors[i].pendingClose) { m_TextEditors[i].pendingClose = false; } } ImGui::CloseCurrentPopup(); }

				ImGui::EndPopup();
			}
			

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
			ImGui::Begin((std::string(CHARACTER_WINDOW_ICON_TEXT_EDITOR) + " Text Editor").c_str(), &m_TextEditorVisible, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoNav);
			ImGui::PopStyleVar();
			ImGui::BeginMenuBar();
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("New")) { NewTextFile(); }
				if (ImGui::MenuItem("Open...")) { OpenTextFile(); }
				if (ImGui::MenuItem("Save", "", false, !m_TextEditors.empty())) { SaveTextFile(); }
				if (ImGui::MenuItem("Save As...", "", false, !m_TextEditors.empty())) { SaveAsTextFile(); }
				if (ImGui::MenuItem("Save All", "", false, !m_TextEditors.empty())) { for (auto& textEditor : m_TextEditors) { SaveTextFileByReference(&textEditor); } }
				ImGui::Separator();
				if (ImGui::MenuItem("Close All Tabs", "", false, !m_TextEditors.empty())) { for (int i = 0; i < m_TextEditors.size(); i++)
				{
					if (DeleteTextEditor(&m_TextEditors[i])) { i--; }
				} }
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Edit"))
			{
				if (ImGui::MenuItem("Undo", "Ctrl+Z")) { for (int i = 0; i < m_TextEditors.size(); i++) { if (m_TextEditors[i].ID == m_SelectedEditor->ID) { m_TextEditors[i].textEditor.Undo(); } } }
				if (ImGui::MenuItem("Redo", "Ctrl+Shift+Z")) { for (int i = 0; i < m_TextEditors.size(); i++) { if (m_TextEditors[i].ID == m_SelectedEditor->ID) { m_TextEditors[i].textEditor.Redo(); } } }
				
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("View"))
			{
				if (ImGui::MenuItem("Show Whitespaces", "", m_ShowWhitespaces)) { SetShowWhitespaces(!m_ShowWhitespaces); }
				if (ImGui::MenuItem("Zoom In", "Ctrl+Scroll"))  { Zoom( 1.0f); }
				if (ImGui::MenuItem("Zoom Out", "Ctrl+Scroll")) { Zoom(-1.0f); }
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();

			if (io.KeyCtrl)
				Zoom(io.MouseWheel);

			ImGui::BeginChild("##TextEditorDropArea");
	
			// Internal Code (Based on BeginTabBar())
			ImGuiTabBarFlags tab_bar_flags = (ImGuiTabBarFlags_FittingPolicyScroll) | ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_AutoSelectNewTabs;

			ImGuiContext& g = *GImGui;
			ImGuiWindow* window = g.CurrentWindow;

			ImGuiID id = window->GetID("##TextEditorTabs");
			ImGuiTabBar* tab_bar = g.TabBars.GetOrAddByKey(id);
			ImRect tab_bar_bb = ImRect(window->DC.CursorPos.x, window->DC.CursorPos.y, window->WorkRect.Max.x, window->DC.CursorPos.y + g.FontSize + g.Style.FramePadding.y * 2);
			tab_bar->ID = id;
			if (ImGui::BeginTabBarEx(tab_bar, tab_bar_bb, tab_bar_flags | ImGuiTabBarFlags_IsFocused, NULL))
				//--- End Internal Code ---//
			// {ORIGINAL} if (ImGui::BeginTabBar("##TextEditorTabs", tab_bar_flags))
			{
				// Submit Tabs
				bool closeTabsRight = m_CloseToRightIndex != -1;
				bool closeTabsOther = m_CloseOtherIndex != -1;
				bool closeAll = m_CloseAll;
				for (int n = 0; n < m_TextEditors.size(); n++)
				{
					ImGui::PushID(m_TextEditors[n].ID);
					ImGuiTabItemFlags tab_flags = (m_TextEditors[n].IsDirty() ? ImGuiTabItemFlags_UnsavedDocument : 0) | ImGuiTabItemFlags_NoTooltip;
					bool open = true;
					bool visible = ImGui::BeginTabItem(m_TextEditors[n].Filename.c_str(), &open, tab_flags | (m_TextEditors[n].SetSelected ? ImGuiTabItemFlags_SetSelected : 0));

					if (ImGui::BeginPopupContextItem("##TabContext"))
					{
						if (ImGui::MenuItem("Save")) { SaveTextFileByReference(&m_TextEditors[n]); }
						if (ImGui::MenuItem("Save All")) { for (auto& textEditor : m_TextEditors) { SaveTextFileByReference(&textEditor); } }
						if (ImGui::MenuItem("Close")) { open = false; }
						//if (ImGui::MenuItem("Close All")) { for (int i = 0; i < m_TextEditors.size(); i++) { if (m_TextEditors[i].ID == m_TextEditors[n].ID) { open = false; } else { if (DeleteTextEditor(&m_TextEditors[i])) { i--; } } } }
						if (ImGui::MenuItem("Close All")) { m_CloseAll = true; }
						//if (ImGui::MenuItem("Close All But This")) { for (int i = 0; i < m_TextEditors.size(); i++) { if (m_TextEditors[i].ID != m_TextEditors[n].ID) { if (DeleteTextEditor(&m_TextEditors[i])) { i--; } } } }
						if (ImGui::MenuItem("Close All But This")) { m_CloseOtherIndex = m_TextEditors[n].ID; }
						if (ImGui::MenuItem("Close All To Right")) { m_CloseToRightIndex = tab_bar->LastTabItemIdx; }

						ImGui::EndPopup();
					}

					m_TextEditors[n].SetSelected = false;
					m_TextEditors[n].InternalID = tab_bar->Tabs[tab_bar->LastTabItemIdx].ID;
					if (ImGui::GetCurrentContext()->HoveredId == ImGui::GetItemID() && !ImGui::IsItemActive() && ImGui::GetCurrentContext()->HoveredIdNotActiveTimer > 0.50f && ImGui::IsItemHovered())
					{
						auto label = (m_TextEditors[n].Filepath + ((m_TextEditors[n].Language != "") ? (" [" + m_TextEditors[n].Language + "]") : ("")));
						ImGui::SetTooltip("%.*s", (int)(ImGui::FindRenderedTextEnd(label.c_str()) - label.c_str()), label.c_str());
					}
					if (!open || (closeTabsRight && tab_bar->LastTabItemIdx > m_CloseToRightIndex) || (closeAll) || (closeTabsOther && m_CloseOtherIndex != m_TextEditors[n].ID)) 
					{
						if (DeleteTextEditor(&m_TextEditors[n])) { n--; }
					}
					else if (visible)
					{
						m_SelectedEditor = &m_TextEditors[n];
						ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[2]);
						m_TextEditors[n].textEditor.Render("##TextEditorWindow", m_Zoom);
						ImGui::PopFont();
						if (ImGui::BeginPopupContextItem("##EditorContextItem"))
						{
							if (ImGui::MenuItem("Copy")) { m_TextEditors[n].textEditor.Copy(); }
							if (ImGui::MenuItem("Cut")) { m_TextEditors[n].textEditor.Cut(); }
							if (ImGui::MenuItem("Paste")) { m_TextEditors[n].textEditor.Paste(); };
							if (ImGui::MenuItem("Duplicate Line")) { m_TextEditors[n].textEditor.Duplicate(); }
							if (ImGui::MenuItem("Swap Line Up")) { m_TextEditors[n].textEditor.SwapLineUp(); }
							if (ImGui::MenuItem("Swap Line Down")) { m_TextEditors[n].textEditor.SwapLineDown(); }

							ImGui::EndPopup();
						}
					}
	
					if (visible)
					{
						ImGui::EndTabItem();
					}
					ImGui::PopID();
				}
				if (closeTabsRight) { m_CloseToRightIndex = -1; }
				if (closeTabsOther) { m_CloseOtherIndex = -1; }
				if (closeAll) { m_CloseAll = false; }
	
				ImGui::EndTabBar();
			}
	
			ImGui::EndChild();

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					const char* filepath = (const char*)payload->Data;
					OpenTextFileByFilepath(filepath);
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::End();
		}
	}

	void TextEditorPannel::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);

		dispatcher.Dispatch<MouseScrolledEvent>(DY_BIND_EVENT_FN(TextEditorPannel::OnMouseScrolled));
	}

	bool TextEditorPannel::OnMouseScrolled(MouseScrolledEvent& e)
	{
		return false;
	}

	void TextEditorPannel::SwitchCStyleHeader()
	{
		if (m_SelectedEditor != NULL && m_TextEditorVisible)
		{
			if (m_SelectedEditor->Filepath.find(".") != std::string::npos)
			{
				std::string name = m_SelectedEditor->Filepath.substr(0, m_SelectedEditor->Filepath.find_last_of("."));
				std::string extension = m_SelectedEditor->Filepath.substr(m_SelectedEditor->Filepath.find_last_of("."));
				std::string lookForFile = "";
				if (extension == ".cpp") { lookForFile = name + ".h"; }
				else if (extension == ".h") { lookForFile = name + ".cpp"; }

				if (lookForFile != "")
				{
					struct stat buffer;
					bool exists = (stat(lookForFile.c_str(), &buffer) == 0);
					if (exists) { OpenTextFileByFilepath(lookForFile); }
				}
			}
		}
	}

	static std::string GetFilenameFromFilepath(std::string filepath)
	{
		if (filepath.find("\\") != std::string::npos) { filepath = filepath.substr(filepath.find_last_of("\\") + 1); }
		return filepath;
	}
	
	void TextEditorPannel::NewTextFile()
	{
		auto id = GetNextTextEditorID();

		int number = 0;
		bool found = false;
		while (!found)
		{
			number++;
			found = true;
			for (auto const & textEditor : m_TextEditors)
			{
				if (textEditor.Filename == "Untitled-" + std::to_string(number))
				{
					found = false;
					break;
				}
			}
		}
		m_TextEditors.insert(m_TextEditors.begin(), TextEditorInformation(id, ("Untitled-" + std::to_string(number)), true));
		m_TextEditors[0].textEditor.SetShowWhitespaces(m_ShowWhitespaces);
		m_TextEditors[0].textEditor.SetZoom(&m_Zoom);
		m_TextEditors[0].SetSelected = true;
	}
	
	void TextEditorPannel::OpenTextFile()
	{
		std::string filepath = FileDialogs::OpenFile("");
		if (!filepath.empty())
		{
			OpenTextFileByFilepath(filepath);
		}
	}
	
	void TextEditorPannel::OpenTextFileByFilepath(std::string filepath)
	{
		for (int i = 0; i < m_TextEditors.size(); i++)
		{
			if (m_TextEditors[i].Filepath == filepath)
			{
				m_TextEditors[i].SetSelected = true;
				return;
			}
		}
		auto id = GetNextTextEditorID();
		m_TextEditors.insert(m_TextEditors.begin(), TextEditorInformation(id, filepath));
		m_TextEditors[0].textEditor.SetShowWhitespaces(m_ShowWhitespaces);
		m_TextEditors[0].textEditor.SetZoom(&m_Zoom);
		m_TextEditors[0].SetSelected = true;
		UpdateLanguage(&m_TextEditors[0]);
	}

	void TextEditorPannel::SaveTextFile()
	{
		SaveTextFileByReference(m_SelectedEditor);
	}

	bool TextEditorPannel::SaveTextFileByReference(TextEditorInformation* reference)
	{
		if (!reference->Untitled)
		{
			if (!reference->Filepath.empty()) { SaveFileToFilePath(reference->Filepath); }
			return true;
		}
		else
		{
			return SaveAsTextFile();
		}
	}

	bool TextEditorPannel::SaveAsTextFile()
	{
		std::string filepath = FileDialogs::SaveFile("");
		if (!filepath.empty())
		{
			SaveFileToFilePath(filepath);
			return true;
		}	
		return false;
	}

	void TextEditorPannel::SaveFileToFilePath(std::string filepath)
	{
		std::ofstream outfile;

		outfile.open(filepath);
		outfile << m_SelectedEditor->textEditor.GetText();

		m_SelectedEditor->SavedUndoIndex = m_SelectedEditor->textEditor.GetUndoIndex();
		m_SelectedEditor->Filepath = filepath;
		m_SelectedEditor->Filename = GetFilenameFromFilepath(filepath);
		m_SelectedEditor->Untitled = false;
		UpdateLanguage(m_SelectedEditor);
	}

	bool TextEditorPannel::DeleteTextEditor(TextEditorInformation* reference)
	{
		for (int i = 0; i < m_TextEditors.size(); i++)
		{
			if (m_TextEditors[i].ID == reference->ID)
			{
				if (reference->IsDirty())
				{
					reference->pendingClose = true;
					return false;
				}
				else
				{
					m_TextEditors.erase(m_TextEditors.begin() + i);
					return true;
				}
			}
		}
	}

	void TextEditorPannel::UpdateLanguage(TextEditorInformation* reference)
	{
		if (reference->Filename.find_last_of(".") != std::string::npos)
		{
			std::string suffix = reference->Filename.substr(reference->Filename.find_last_of("."));
			     if (suffix == ".cpp")		{ reference->textEditor.SetLanguageDefinition(TextEditorInternal::TextEditor::LanguageDefinition::CPlusPlus()); reference->Language = "C++"; }
			else if (suffix == ".pch")		{ reference->textEditor.SetLanguageDefinition(TextEditorInternal::TextEditor::LanguageDefinition::CPlusPlus()); reference->Language = "Precompiled Header"; }
			else if (suffix == ".hpp")		{ reference->textEditor.SetLanguageDefinition(TextEditorInternal::TextEditor::LanguageDefinition::CPlusPlus()); reference->Language = "C++ Header"; }
			else if (suffix == ".h")		{ reference->textEditor.SetLanguageDefinition(TextEditorInternal::TextEditor::LanguageDefinition::CPlusPlus()); reference->Language = "Header"; }
			else if (suffix == ".c")		{ reference->textEditor.SetLanguageDefinition(TextEditorInternal::TextEditor::LanguageDefinition::C()); reference->Language = "C"; }
			else if (suffix == ".hlsl")		{ reference->textEditor.SetLanguageDefinition(TextEditorInternal::TextEditor::LanguageDefinition::HLSL()); reference->Language = "HLSL"; }
			else if (suffix == ".glsl")		{ reference->textEditor.SetLanguageDefinition(TextEditorInternal::TextEditor::LanguageDefinition::GLSL()); reference->Language = "GLSL"; }
			else if (suffix == ".sql")		{ reference->textEditor.SetLanguageDefinition(TextEditorInternal::TextEditor::LanguageDefinition::SQL()); reference->Language = "SQL"; }
			else if (suffix == ".as")		{ reference->textEditor.SetLanguageDefinition(TextEditorInternal::TextEditor::LanguageDefinition::AngelScript()); reference->Language = "Angel Script"; }
			else if (suffix == ".lua")		{ reference->textEditor.SetLanguageDefinition(TextEditorInternal::TextEditor::LanguageDefinition::Lua()); reference->Language = "Lua"; }
			else if (suffix == ".node")		{ reference->textEditor.SetLanguageDefinition(TextEditorInternal::TextEditor::LanguageDefinition::None()); reference->Language = "Dymatic Node Graph"; }
			else							
			{
				reference->textEditor.SetLanguageDefinition(TextEditorInternal::TextEditor::LanguageDefinition::None());
				reference->Language = "";
				if (suffix == ".txt") { reference->Language = "Text"; }
			}
		}
		else
		{
			reference->textEditor.SetLanguageDefinition(TextEditorInternal::TextEditor::LanguageDefinition::None());
			reference->Language = "File";
		}
	}

	void TextEditorPannel::SetShowWhitespaces(bool show)
	{
		m_ShowWhitespaces = show;
		for (auto& textEditor : m_TextEditors)
		{
			textEditor.textEditor.SetShowWhitespaces(m_ShowWhitespaces);
		}
	}

	TextEditorInformation::TextEditorInformation(int id, std::string filepath, bool untitled)
		: ID(id), Filepath(filepath), Untitled(untitled)
	{
		Filename = GetFilenameFromFilepath(filepath);
	
		std::string result;
		std::ifstream in(filepath, std::ios::in | std::ios::binary); // ifstream closes itself due to RAII
		if (in)
		{
			in.seekg(0, std::ios::end);
			size_t size = in.tellg();
			if (size != -1)
			{
				result.resize(size);
				in.seekg(0, std::ios::beg);
				in.read(&result[0], size);
			}
			else
			{
				DY_CORE_ASSERT("Could not read from file!");
			}
	
			textEditor.InsertText(result);
			textEditor.SetCursorPosition({}); // Sets cursor position to beginning of document when initially opened.
		}
	}

}