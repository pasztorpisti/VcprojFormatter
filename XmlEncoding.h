//
// Copyright (c) Istvan Pasztor
// This source has been published on www.codeproject.com under the CPOL license.
//
#ifndef __XML_ENCODING_H__
#define __XML_ENCODING_H__
#pragma once


enum ENewLineMode
{
	eNLM_CR,
	eNLM_CRLF,
	eNLM_LF,
	eNLM_LFCR,
	eNLM_Auto,

	eNLM_Last,
};
// Used for commandline parsing.
const wchar_t* GetName(ENewLineMode);
// Returns eNLM_Last on error.
ENewLineMode NewLineModeFromName(const wchar_t* new_line_mode_name);
const wchar_t* ToString(ENewLineMode);
// Returns  eNLM_Auto if the specified text does not contain any newline characters.
ENewLineMode DetectNewLineMode(const wchar_t* s_begin, const wchar_t* s_end);


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------


struct IEncoding
{
	// Must return at least 1.
	virtual int GetNameCount() const = 0;
	// GetName(0) always exists, it is the default name.
	virtual const wchar_t* GetName(int name_index) const = 0;

	// For codepages this checks if the windows WideCharToMultiByte() function
	// can determine the length of the encoded space character.
	virtual bool IsAvailable() const = 0;

	// Returns -1 on error, the number of converted bytes otherwise. If byte_count is zero, then returns
	// the number of bytes required for the conversion.
	virtual int UTF16ToBytes(const wchar_t* utf16le_str, int utf16_chars, char* bytes, int byte_count, wstring* error_message=NULL) const = 0;
	// Returns -1 on error, the number of converted utf16_chars otherwise. If utf16_chars is zero, then
	// returns the number of utf16_chars required for the conversion.
	virtual int BytesToUTF16(const char* bytes, int byte_count, wchar_t* utf16le_str, int utf16_chars, wstring* error_message=NULL) const = 0;

	virtual int GetBOMSizeBytes() const				{ return 0; }
	virtual const char* GetBOM() const				{ return NULL; }

	enum EFlags
	{
		eF_CanRepresentAllUniChars = 1,		// for utf encodings
	};
	virtual int GetFlags() const					{ return 0; }
};

class CEncodings
{
public:
	static const CEncodings& GetInstance()
	{
		static const CEncodings g_Instance;
		return g_Instance;
	}
	IEncoding* FindEncoding(const wchar_t* name) const;
	const std::vector<IEncoding*>& GetAllEncodings() const { return m_Encodings; }

private:
	CEncodings();
	void AddEncoding(IEncoding* encoding);

private:
	// encoding names are stored in lowercase to this map
	std::map<wstring,IEncoding*> m_NameToEncoding;
	std::vector<IEncoding*> m_Encodings;
};


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------


struct SXmlDeclarationAttrib
{
	wstring name;
	wstring value;
	SXmlDeclarationAttrib(const wstring& _name, const wstring& _value) : name(_name), value(_value) {}
};
struct SXmlDeclarationAttribs : public std::vector<SXmlDeclarationAttrib>
{
	bool GetAttrib(const wstring& name, wstring& value) const;
	void SetAttrib(const wstring& name, const wstring& value);
	wstring GetAsXmlDeclaration() const;
};


class CXmlTextCodec
{
public:
	CXmlTextCodec();

	const wstring& GetXmlBody() const								{ return m_XmlBody; }
	void SetXmlBody(const wstring& s)								{ m_XmlBody = s; }

	IEncoding* GetEncoding() const									{ return m_Encoding; }
	void SetEncoding(IEncoding* encoding)							{ m_Encoding = encoding; }

	SXmlDeclarationAttribs& GetXmlDeclarationAttributes()			{ return m_XmlDeclarationAttributes; }
	const SXmlDeclarationAttribs& GetXmlDeclarationAttributes() const	{ return m_XmlDeclarationAttributes; }

	// Fills this object with the xml file data. Decodes the xml data to utf16.
	bool DecodeXmlFileData(const void* data, int data_size);
	// Encodes the current utf16 xml data and settings of this object and returns the encoded
	// xml file data. The data contains the BOM if required, the xml declaration, and the xml data.
	static bool EncodeXmlFileData(const SXmlDeclarationAttribs& attribs, const wstring& xml_body,
		IEncoding* encoding, ENewLineMode newline_mode, std::vector<char>& data, wstring* error_message);

	const wstring& GetErrorMessage() const							{ return m_ErrorMessage; }

private:
	bool Error(const wchar_t* error_mesasge);

private:
	wstring m_XmlBody;
	IEncoding* m_Encoding;
	SXmlDeclarationAttribs m_XmlDeclarationAttributes;
	wstring m_ErrorMessage;
};


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------


// This can tell you which utf16 codepoints need to be stored as xml character references
// in the xml when a specific codepage is used for encoding.
class CXmlCharacterReferenceMap
{
public:
	CXmlCharacterReferenceMap();
	// safe_encoding==true is used only with windows codepages and it forces character
	// codes above 0x7F in the codepage to be encoded as xml character references.
	void SetEncoding(IEncoding* encoding, bool safe_encoding=false);
	// Tells you which utf16 codepoints have to be represented as xml
	// character references in the value of xml attributes.

	enum ECharRefMode { eCRM_NoRef, eCRM_NoRefNonCharacter, eCRM_RefDecimal, eCRM_RefHex, eCRM_RefHexNonCharacter, eCRM_RefSurrogate, eCRM_NoRefSurrogate };
	char CharacterRefType(wchar_t c) const { return m_Map[c]; }

private:
	void SetupStaticRefTypes(bool can_represent_all_unichars);

private:
	char m_Map[0x10000];
};


#endif
