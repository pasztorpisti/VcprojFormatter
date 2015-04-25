//
// Copyright (c) Istvan Pasztor
// This source has been published on www.codeproject.com under the CPOL license.
//
#ifndef __VCPROJ_H__
#define __VCPROJ_H__
#pragma once


#include "XmlEncoding.h"


// Defines a position in a file. Tabsize is treated as 4.
struct SXmlFileCursor
{
	int line;			// zero based
	int column;			// zero based
	SXmlFileCursor() : line(0), column(0) {}
};

struct SXmlNode : public refcounted
{
	wstring name;
	SXmlFileCursor file_pos;
};

struct SXmlAttrib : public SXmlNode
{
	wstring value;

	void ToString(wstring& s, const CXmlCharacterReferenceMap& crm) const;
	int Compare(const SXmlAttrib& other) const;
};
typedef smartptr<SXmlAttrib> TXmlAttribPtr;
typedef std::vector<TXmlAttribPtr> TXmlAttribPtrVec;

struct SXmlElement;
typedef smartptr<SXmlElement> TXmlElementPtr;
typedef std::vector<TXmlElementPtr> TXmlElementPtrVec;

struct SXmlElement : public SXmlNode
{
	TXmlAttribPtrVec attributes;
	TXmlElementPtrVec children;

	void ToString(wstring& s, const CXmlCharacterReferenceMap& crm, const wchar_t* newline=L"\r\n", int indent=0) const;
	int Compare(const SXmlElement& other) const;
	void SortAttributes();
	void SortChildElements();
	void SortSubTree();
};


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------


class CVcprojFile
{
public:
	CVcprojFile();

	bool LoadVcprojFile(const wchar_t* filepath, ENewLineMode newline_mode=eNLM_Auto);
	bool SaveVcprojFile(const wchar_t* filepath, DWORD file_attributes, bool safe_encoding);
	const wstring& GetErrorMessage() const				{ return m_ErrorMessage; }

	ENewLineMode GetNewLineMode() const					{ return m_NewLineMode; }
	void SetNewLineMode(ENewLineMode);
	IEncoding* GetEncoding() const						{ return m_Encoding; }
	void SetEncoding(IEncoding*);

	SXmlElement* GetRoot()								{ return m_Root; }
	void SetDecimalPoint(wchar_t decimal_point);

private:
	bool Error(const wchar_t* fmtstr, ...);
	bool WinError(const wchar_t* fmtstr, ...);

private:
	wstring m_ErrorMessage;

	SXmlDeclarationAttribs m_XmlDeclarationAttribs;
	ENewLineMode m_NewLineMode;
	IEncoding* m_Encoding;
	TXmlElementPtr m_Root;
};


#endif
