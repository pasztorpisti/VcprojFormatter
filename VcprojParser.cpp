//
// Copyright (c) Istvan Pasztor
// This source has been published on www.codeproject.com under the CPOL license.
//
#include "stdafx.h"
#include "VcprojParser.h"


ILINE static void UnicodeCharToUTF16(int c, wstring& str)
{
	assert(c<=0x10FFFF);
	assert((c&0xFFFF)!=0xFFFF && (c&0xFFFF)!=0xFFFE);
	assert(c<0xFDD0 || c>0xFDEF);
	if (c >= 0x10000)
	{
		// have to use a surrogate pair
		c -= 0x10000;
		str.push_back((wchar_t)(0xD800 | (c >> 10)));
		str.push_back((wchar_t)(0xDC00 | (c & 0x3FF)));
	}
	else
	{
		str.push_back((wchar_t)c);
	}
}

// appends the result char(s) to str
ILINE static bool HexStrToChar(const wchar_t* s, const wchar_t* s_end, wstring& str)
{
	if (s >= s_end)
		return false;
	int val = 0;
	for (; s<s_end; ++s)
	{
		wchar_t b = *s;
		if ((b>=L'0') & (b<=L'9'))
			b -= L'0';
		else if ((b>=L'a') & (b<=L'f'))
			b = b - L'a' + 10;
		else if ((b>=L'A') & (b<=L'F'))
			b = b - L'A' + 10;
		else
			return false;
		val = val << 4;
		val |= b;
		if (val > 0x10FFFF)
			return false;
	}
	UnicodeCharToUTF16(val, str);
	return true;
}

// appends the result char(s) to str
ILINE static bool DecStrToChar(const wchar_t* s, const wchar_t* s_end, wstring& str)
{
	if (s >= s_end)
		return false;
	int val = 0;
	for (; s<s_end; ++s)
	{
		if ((*s<L'0') | (*s>L'9'))
			return false;
		val *= 10;
		val += *s - L'0';
		if (val > 0x10FFFF)
			return false;
	}
	UnicodeCharToUTF16(val, str);
	return true;
}

// appends the unescaped character sequence to str
ILINE static bool UnEscape(const wchar_t* s, const wchar_t* s_end, wstring& str)
{
	size_t size = s_end - s;
	assert(size >= 2);
	if (size < 2)
		return false;

	if (s[0] != L'#')
	{
		switch (size)
		{
		case 2:
			if (s[0]==L'l' && s[1]==L't')
			{
				str.push_back(L'<');
				return true;
			}
			if (s[0]==L'g' && s[1]==L't')
			{
				str.push_back(L'>');
				return true;
			}
			break;
		case 3:
			if (s[0]==L'a' && s[1]==L'm' && s[2]==L'p')
			{
				str.push_back(L'&');
				return true;
			}
			break;
		case 4:
			if (s[0]==L'a' && s[1]==L'p' && s[2]==L'o' && s[3]==L's')
			{
				str.push_back(L'\'');
				return true;
			}
			if (s[0]==L'q' && s[1]==L'u' && s[2]==L'o' && s[3]==L't')
			{
				str.push_back(L'"');
				return true;
			}
			break;
		}
	}
	else
	{
		if (size < 2)
			return false;
		if ((s[1] == L'x') | (s[1] == L'X'))
		{
			if (size < 3)
				return false;
			return HexStrToChar(s+2, s_end, str);
		}
		else
		{
			return DecStrToChar(s+1, s_end, str);
		}
	}

	return false;
}

ILINE static bool IsSpace(wchar_t c)
{
	return (c==L' ') | (c==L'\t') | (c==L'\r') | (c==L'\n');
}

ILINE static bool IsXmlTokenChar(wchar_t c)
{
	return (c==L'<') | (c==L'=') | (c==L'/') | (c==L'>');
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------


CVcprojParser::CVcprojParser()
: m_Begin(NULL)
, m_End(NULL)
, m_Pos(NULL)
, m_CurrentLineBegin(NULL)
, m_CurrentLineNumber(0)
, m_PrevNewLineChar(NEWLINE_NONE)
{
}

SXmlElement* CVcprojParser::Parse(const wchar_t* vcproj_contents, const wchar_t* vcproj_contents_end)
{
	m_Begin = vcproj_contents;
	m_Pos = m_Begin;
	m_End = vcproj_contents_end;
	m_CurrentLineBegin = m_Begin;

	m_Error.clear();
	m_CurrentLineNumber = 0;
	m_PrevNewLineChar = NEWLINE_NONE;

	TXmlElementPtr root = new SXmlElement;
	SkipSpaces();
	if (!Element(*root))
		return NULL;
	return root.ReleasePtr();
}

void CVcprojParser::GetError(wstring& error_message, SXmlFileCursor& file_pos) const
{
	error_message = m_Error;
	GetCurrentFilePos(file_pos);
}

bool CVcprojParser::Error(const wchar_t* error_message)
{
	if (m_Error.empty())
		m_Error = error_message;
	return false;
}

int CVcprojParser::CalculateCurrentColumn() const
{
	const int TAB_SIZE = 4;
	int column = 0;
	for (const wchar_t* p=m_CurrentLineBegin; p<m_Pos; ++p)
	{
		if (p[0] == '\t')
		{
			column += TAB_SIZE;
			column -= column % TAB_SIZE;
		}
		else
		{
			if ((p[0]<0xDC00) | (p[0]>0xDFFF))
				++column;
		}
	}
	return column;
}

void CVcprojParser::GetCurrentFilePos(SXmlFileCursor& file_pos) const
{
	file_pos.line = m_CurrentLineNumber;
	file_pos.column = CalculateCurrentColumn();
}

wchar_t CVcprojParser::PreviewChar() const
{
	if (m_Pos < m_End)
		return m_Pos[0];
	return 0xFFFF;
}

wchar_t CVcprojParser::PreviewChar2() const
{
	if (m_Pos+1 < m_End)
		return m_Pos[1];
	return 0xFFFF;
}

void CVcprojParser::SetPos(const wchar_t* pos)
{
	// maintaining the current line
	for (const wchar_t* p=m_Pos; p<pos; ++p)
	{
		if (p[0] == L'\r')
		{
			if (m_PrevNewLineChar == NEWLINE_LF)
			{
				m_PrevNewLineChar = NEWLINE_NONE;
			}
			else
			{
				++m_CurrentLineNumber;
				m_PrevNewLineChar = NEWLINE_CR;
			}
			m_CurrentLineBegin = p + 1;
		}
		else if (p[0] == L'\n')
		{
			if (m_PrevNewLineChar == NEWLINE_CR)
			{
				m_PrevNewLineChar = NEWLINE_NONE;
			}
			else
			{
				++m_CurrentLineNumber;
				m_PrevNewLineChar = NEWLINE_LF;
			}
			m_CurrentLineBegin = p + 1;
		}
		else
		{
			m_PrevNewLineChar = NEWLINE_NONE;
		}
	}
	m_Pos = pos;
}

bool CVcprojParser::SkipChar()
{
	if (m_Pos >= m_End)
		return false;
	SetPos(m_Pos + 1);
	return true;
}

bool CVcprojParser::ConsumeChar(wchar_t c)
{
	if (PreviewChar() != c)
		return false;
	SetPos(m_Pos + 1);
	return true;
}

void CVcprojParser::SkipSpaces()
{
	const wchar_t* p;
	for (p=m_Pos; p<m_End; ++p)
	{
		if (!IsSpace(p[0]))
			break;
	}
	SetPos(p);
}

bool CVcprojParser::SkipSpacesAndConsumeChar(wchar_t c)
{
	SkipSpaces();
	return ConsumeChar(c);
}

bool CVcprojParser::Name(wstring& name)
{
	const wchar_t* p;
	for (p=m_Pos; p<m_End; ++p)
	{
		if (IsSpace(p[0]) || IsXmlTokenChar(p[0]))
			break;
	}
	if (p == m_Pos)
		return false;
	name.assign(m_Pos, p);
	SetPos(p);
	return true;
}

bool CVcprojParser::DerefAttribValueString(const wchar_t* val_begin, const wchar_t* val_end, wstring& s)
{
	const wchar_t* p = val_begin;
	while (p < val_end)
	{
		for (p=val_begin; p<val_end; ++p)
		{
			if (p[0] == L'&')
				break;
		}
		s.append(val_begin, p);
		if (p >= val_end)
			return true;

		val_begin = p;
		for (p=p+1; p<val_end; ++p)
		{
			if (p[0] == L';')
				break;
		}
		if (p >= val_end)
		{
			SetPos(p);
			return Error(L"Expected ';' to close the '&'");
		}
		if (!UnEscape(val_begin+1, p, s))
		{
			SetPos(val_begin);
			return Error(L"Invalid character reference!");
		}
		val_begin = p + 1;
	}

	return true;
}

bool CVcprojParser::AttribValue(wstring& value)
{
	if (!SkipSpacesAndConsumeChar(L'"'))
		return Error(L"Expected '\"'");
	const wchar_t* p;
	for (p=m_Pos; p<m_End; ++p)
	{
		if (p[0] == L'"')
		{
			if (!DerefAttribValueString(m_Pos, p, value))
				return false;
			SetPos(p+1);
			return true;
		}
	}
	return Error(L"Expected a closing '\"'");
}

bool CVcprojParser::Attrib(SXmlAttrib& attrib)
{
	GetCurrentFilePos(attrib.file_pos);
	if (!Name(attrib.name))
		return Error(L"Expected an attribute name");
	if (!SkipSpacesAndConsumeChar(L'='))
		return Error(L"Expected '='");
	SkipSpaces();
	return AttribValue(attrib.value);
}

bool CVcprojParser::ElementContents(SXmlElement& element)
{
	while (1)
	{
		SkipSpaces();
		if (PreviewChar() != L'<')
			return Error(L"Expected '<'");
		if (PreviewChar2() == L'/')
			return true;
		element.children.push_back(new SXmlElement());
		if (!Element(*element.children.back()))
			return false;
	}
	return false;
}

bool CVcprojParser::ElementCloseTag(SXmlElement& element)
{
	if (!ConsumeChar(L'<') || !ConsumeChar(L'/'))
		return Error(L"Expected \"</\"");
	SkipSpaces();
	wstring name;
	if (!Name(name) || name!=element.name)
		return Error(L"Expected element name");
	if (!SkipSpacesAndConsumeChar(L'>'))
		return Error(L"Expected '>'");
	return true;
}

bool CVcprojParser::Element(SXmlElement& element)
{
	GetCurrentFilePos(element.file_pos);
	if (!ConsumeChar(L'<'))
		return Error(L"Expected '<'");

	SkipSpaces();
	if (!Name(element.name))
		return Error(L"Expected an element name.");

	while (1)
	{
		SkipSpaces();
		if (IsXmlTokenChar(PreviewChar()))
			break;
		TXmlAttribPtr attrib = new SXmlAttrib;
		if (!Attrib(*attrib))
			return false;
		element.attributes.push_back(attrib);
	}

	switch (PreviewChar())
	{
	case L'/':
		// no children
		SkipChar();
		if (!ConsumeChar(L'>'))
			return Error(L"Expected and '>' char.");
		return true;

	case L'>':
		// possible element content and/or child elements
		SkipChar();
		if (!ElementContents(element))
			return false;
		return ElementCloseTag(element);

	default:
		return Error(L"Expected \">\" or \"/>\".");
	}

	return false;
}
