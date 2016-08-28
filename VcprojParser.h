#pragma once


#include "Vcproj.h"


// A class that parses the vcproj xml. At this point the xml declaration is already processed
// and the remaining xml data is converted to utf16 because this class works with utf16 only!!!
class CVcprojParser
{
public:
	CVcprojParser();
	// Returns NULL on error, in this case you can call GetError() to find out more.
	// The text content pointed by vcproj_contents should not contain the encoding
	// <?xml version="1.0" encoding="Windows-1252"?>
	SXmlElement* Parse(const wchar_t* vcproj_contents, const wchar_t* vcproj_contents_end);
	// Call this if Parse() returns NULL.
	void GetError(wstring& error_message, SXmlFileCursor& file_pos) const;

private:
	bool Error(const wchar_t* error_message);
	int CalculateCurrentColumn() const;
	void GetCurrentFilePos(SXmlFileCursor& file_pos) const;

	wchar_t PreviewChar() const;
	wchar_t PreviewChar2() const;
	void SetPos(const wchar_t* pos);
	bool SkipChar();
	bool ConsumeChar(wchar_t c);
	void SkipSpaces();
	bool SkipSpacesAndConsumeChar(wchar_t c);

	bool Name(wstring& name);
	bool DerefAttribValueString(const wchar_t* val_begin, const wchar_t* val_end, wstring& s);
	bool AttribValue(wstring& value);
	bool Attrib(SXmlAttrib& attrib);
	bool ElementContents(SXmlElement& element);
	bool ElementCloseTag(SXmlElement& element);
	bool Element(SXmlElement& element);

private:
	const wchar_t* m_Begin;
	const wchar_t* m_End;
	const wchar_t* m_Pos;

	const wchar_t* m_CurrentLineBegin;
	int m_CurrentLineNumber;
	enum { NEWLINE_NONE, NEWLINE_CR, NEWLINE_LF } m_PrevNewLineChar;

	wstring m_Error;
};
