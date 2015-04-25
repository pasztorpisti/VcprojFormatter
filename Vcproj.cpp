//
// Copyright (c) Istvan Pasztor
// This source has been published on www.codeproject.com under the CPOL license.
//
#include "stdafx.h"
#include "Vcproj.h"
#include "VcprojParser.h"


//-------------------------------------------------------------------------------------------------
// CXsdChoiceStrictOrdering
//-------------------------------------------------------------------------------------------------


// When the vcproj xsd defines elements with an <xs:choice/> definition then these elements can
// appear in arbitrarily mixed order in the xml. This class defines a strict order between
// these elements. For example if you have an X and Y element in your <xs:choice/> definition then
// this class can  define so that all X elements should come before all of the Y elements.
// Reference XSD file: http://msdn.microsoft.com/en-us/library/y4sy8216%28VS.90%29.aspx
class CXsdChoiceStrictOrdering
{
public:
	static const CXsdChoiceStrictOrdering& GetInstance() { return g_Instance; }
	// Returns false if the elements are not present in the same <xs:choice/> block in the xsd,
	// fills in the e1_less_e2 parameter with the result of e1<e2 otherwise.
	bool AreElementsRelated(const wchar_t* e1, const wchar_t* e2, bool& e1_less_e2) const;

private:
	void AddXsdChoiceGroup(const wchar_t* group[], int group_size);
	CXsdChoiceStrictOrdering();

private:
	typedef std::map<wstring, bool> TLessE2Map;
	typedef std::map<wstring, TLessE2Map> TE1LessE2Map;
	TE1LessE2Map m_E1LessE2;
	static const CXsdChoiceStrictOrdering g_Instance;
};
const CXsdChoiceStrictOrdering CXsdChoiceStrictOrdering::g_Instance;

bool CXsdChoiceStrictOrdering::AreElementsRelated(const wchar_t* e1, const wchar_t* e2, bool& e1_less_e2) const
{
	TE1LessE2Map::const_iterator it = m_E1LessE2.find(e1);
	if (it == m_E1LessE2.end())
		return false;
	TLessE2Map::const_iterator it2 = it->second.find(e2);
	if (it2 == it->second.end())
		return false;
	e1_less_e2 = it2->second;
	return true;
}

void CXsdChoiceStrictOrdering::AddXsdChoiceGroup(const wchar_t* group[], int group_size)
{
	for (int i=0; i<group_size; ++i)
	{
		const wchar_t* e1 = group[i];
		TLessE2Map& other_elements = m_E1LessE2[e1];
		for (int j=0; j<group_size; ++j)
		{
			if (i == j)
				continue;
			const wchar_t* e2 = group[j];
			other_elements[e2] = i < j;
		}
	}
}

CXsdChoiceStrictOrdering::CXsdChoiceStrictOrdering()
{
	// Each line defines a list of element names that are grouped together in an <xs:choice/> definitoin.
	static const wchar_t* XSD_CHOICE_DEFINITIONS[] =
	{
		L"AssemblyReference", L"ActiveXReference", L"ProjectReference", NULL,
		L"DefaultToolFile", L"ToolFile", NULL,
		L"File", L"FileConfiguration", NULL,
		L"Filter", L"File", NULL,
		NULL,
	};

	const wchar_t** group_begin = XSD_CHOICE_DEFINITIONS;
	while (group_begin[0])
	{
		const wchar_t** group_end = group_begin + 1;
		while (group_end[0])
			++group_end;
		AddXsdChoiceGroup(group_begin, (int)(group_end-group_begin));
		group_begin = group_end + 1;
	}
}


//-------------------------------------------------------------------------------------------------
// SXmlAttrib
//-------------------------------------------------------------------------------------------------


static void StringToXmlValue(wstring& t, const wchar_t* s_begin, const wchar_t* s_end, const CXmlCharacterReferenceMap& crm)
{
	for (const wchar_t* s=s_begin; s<s_end; ++s)
	{
		wchar_t c = *s;
		switch (c)
		{
		case '&':
			t.append(L"&amp;");
			break;
		case '\'':
			t.append(L"&apos;");
			break;
		case '"':
			t.append(L"&quot;");
			break;
		case '<':
			t.append(L"&lt;");
			break;
		case '>':
			t.append(L"&gt;");
			break;
		default:
			switch (crm.CharacterRefType(c))
			{
			case CXmlCharacterReferenceMap::eCRM_NoRefNonCharacter:
				assert(0);
			case CXmlCharacterReferenceMap::eCRM_NoRef:
				t.push_back(c);
				break;
			case CXmlCharacterReferenceMap::eCRM_RefDecimal:
				{
					wchar_t buf[0x20];
					swprintf(buf, sizeof(buf)/sizeof(buf[0]), L"&#%d;", c);
					t.append(buf);
				}
				break;
			default:
			case CXmlCharacterReferenceMap::eCRM_RefHexNonCharacter:
				assert(0);
			case CXmlCharacterReferenceMap::eCRM_RefHex:
				{
					wchar_t buf[0x20];
					swprintf(buf, sizeof(buf)/sizeof(buf[0]), L"&#x%02X;", c);
					t.append(buf);
				}
				break;
			case CXmlCharacterReferenceMap::eCRM_RefSurrogate:
				assert(s+1 < s_end && (s[1]&~0x3FF)==0xDC00);
				if (s+1 < s_end && (s[1]&~0x3FF)==0xDC00)
				{
					int u = (((int)c & 0x3FF) << 10) | (int)(s[1] & 0x3FF);
					u += 0x10000;
					++s;
					wchar_t buf[0x20];
					swprintf(buf, sizeof(buf)/sizeof(buf[0]), L"&#x%02X;", u);
					t.append(buf);
					break;
				}
				else
				{
					wchar_t buf[0x20];
					swprintf(buf, sizeof(buf)/sizeof(buf[0]), L"&#x%02X;", c);
					t.append(buf);
				}
				break;
			case CXmlCharacterReferenceMap::eCRM_NoRefSurrogate:
				t.push_back(c);
				assert(s+1 < s_end && (s[1]&~0x3FF)==0xDC00);
				if (s+1 < s_end && (s[1]&~0x3FF)==0xDC00)
					t.push_back(*(++s));
				break;
			}
			break;
		}
	}
}

void SXmlAttrib::ToString(wstring& s, const CXmlCharacterReferenceMap& crm) const
{
	s.append(name);
	s.append(L"=\"");
	StringToXmlValue(s, value.data(), value.data()+value.size(), crm);
	s.push_back('"');
}

int SXmlAttrib::Compare(const SXmlAttrib& other) const
{
	if (name == other.name)
		return value.compare(other.value);
	if (name == L"Name")
		return -1;
	if (other.name == L"Name")
		return 1;
	return name.compare(other.name);
}

struct SXmlAttribPtr_less
{
	bool operator()(const TXmlAttribPtr& a1, const TXmlAttribPtr& a2) const
	{
		return a1->Compare(*a2) < 0;
	}
};


//-------------------------------------------------------------------------------------------------
// SXmlElement
//-------------------------------------------------------------------------------------------------


void SXmlElement::ToString(wstring& s, const CXmlCharacterReferenceMap& crm, const wchar_t* newline, int indent) const
{
	wstring indent_str(indent, L'\t');
	s.append(indent_str);
	s.push_back(L'<');
	s.append(name);

	if (!attributes.empty())
	{
		if (attributes.size() == 1)
		{
			s.push_back(L' ');
			attributes[0]->ToString(s, crm);
		}
		else
		{
			s.append(newline);
			wstring indent_str2 = indent_str + L"\t";
			s.append(indent_str2);
			for (TXmlAttribPtrVec::const_iterator it=attributes.begin(),eit=attributes.end(); it!=eit; ++it)
			{
				(*it)->ToString(s, crm);
				s.append(newline);
				s.append(indent_str2);
			}
		}
	}

	if (children.empty())
	{
		s.append(L"/>");
		s.append(newline);
	}
	else
	{
		s.append(L">");
		s.append(newline);
		for (TXmlElementPtrVec::const_iterator it=children.begin(),eit=children.end(); it!=eit; ++it)
			(*it)->ToString(s, crm, newline, indent+1);
		s.append(indent_str);
		s.append(L"</");
		s.append(name);
		s.append(L">");
		s.append(newline);
	}
}

int SXmlElement::Compare(const SXmlElement& other) const
{
	int cmp_name = name.compare(other.name);
	if (cmp_name == 0)
	{
		size_t size_1 = attributes.size();
		size_t size_2 = other.attributes.size();
		size_t attrib_count = min(size_1, size_2);
		for (size_t i=0; i<attrib_count; ++i)
		{
			if (int res = attributes[i]->Compare(*other.attributes[i]))
				return res;
		}
		if (size_1 < size_2)
			return -1;
		if (size_1 == size_2)
			return 0;
		return 1;
	}
	else
	{
		bool cmp_xsd_choice_members;
		bool exchangeable = CXsdChoiceStrictOrdering::GetInstance().AreElementsRelated(name.c_str(), other.name.c_str(), cmp_xsd_choice_members);
		if (exchangeable)
			return cmp_xsd_choice_members ? -1 : 1;
		assert(0);
		return cmp_name;
	}
}

struct SXmlElementPtr_less
{
	bool operator()(const TXmlElementPtr& e1, const TXmlElementPtr& e2) const
	{
		return e1->Compare(*e2) < 0;
	}
};

void SXmlElement::SortAttributes()
{
	std::stable_sort(attributes.begin(), attributes.end(), SXmlAttribPtr_less());
}

void SXmlElement::SortChildElements()
{
	size_t first = 0;
	for (size_t last=1,count=children.size(); last<=count; ++last)
	{
		bool do_sort = last == count;
		if (!do_sort)
		{
			// We allow sorting consequent elements with the same name, this handles the <xs:sequence/> nodes of the vcproj xsd.
			if (children[first]->name != children[last]->name)
			{
				// We allow sorting of consequent elements with different names only if their names are declared with <xs:choice/> in the vcproj xsd.
				const wchar_t* e1 = children[first]->name.c_str();
				const wchar_t* e2 = children[last]->name.c_str();
				bool tmp;
				do_sort = !CXsdChoiceStrictOrdering::GetInstance().AreElementsRelated(e1, e2, tmp);
			}
		}

		if (do_sort)
		{
			if (last-first > 1)
				std::stable_sort(children.begin()+first, children.begin()+last, SXmlElementPtr_less());
			first = last;
		}
	}
}

void SXmlElement::SortSubTree()
{
	for (TXmlElementPtrVec::iterator it=children.begin(),eit=children.end(); it!=eit; ++it)
		(*it)->SortSubTree();
	SortAttributes();
	SortChildElements();
}


//-------------------------------------------------------------------------------------------------
// CVcprojFile
//-------------------------------------------------------------------------------------------------


CVcprojFile::CVcprojFile()
: m_NewLineMode(eNLM_Last)
, m_Encoding(NULL)
{
}

bool CVcprojFile::LoadVcprojFile(const wchar_t* filepath, ENewLineMode newline_mode)
{
	m_ErrorMessage.clear();

	SWinHandle handle = CreateFile(filepath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (handle == INVALID_HANDLE_VALUE)
		return WinError(L"Error opening file!");
	LARGE_INTEGER file_size;
	if (!GetFileSizeEx(handle, &file_size))
		return WinError(L"Error retrieving file size!");
	if (file_size.HighPart || file_size.LowPart>=0x80000000)
		return Error(L"File is too big!");

	std::vector<char> buf;
	buf.resize(file_size.LowPart);

	DWORD read;
	if (!ReadFile(handle, &buf[0], file_size.LowPart, &read, NULL) || read!=file_size.LowPart)
		return WinError(L"Error reading file!");
	handle.Close();

	CXmlTextCodec decoder;
	if (!decoder.DecodeXmlFileData(&buf[0], (int)file_size.LowPart))
		return Error(decoder.GetErrorMessage().c_str());

	CVcprojParser parser;
	const wstring& xml_body = decoder.GetXmlBody();
	m_Root = parser.Parse(xml_body.data(), xml_body.data()+xml_body.size());
	if (!m_Root)
	{
		wstring error_message;
		SXmlFileCursor file_pos;
		parser.GetError(error_message, file_pos);
		return Error(L"[line=%d, column=%d] %s", file_pos.line+1, file_pos.column+1, error_message.c_str());
	}

	m_XmlDeclarationAttribs.swap(decoder.GetXmlDeclarationAttributes());
	assert(newline_mode != eNLM_Last);
	if (newline_mode==eNLM_Auto || newline_mode==eNLM_Last)
	{
		m_NewLineMode = DetectNewLineMode(xml_body.data(), xml_body.data()+xml_body.size());
		if (m_NewLineMode==eNLM_Auto || m_NewLineMode==eNLM_Last)
			m_NewLineMode = eNLM_CRLF;
	}
	else
	{
		m_NewLineMode = newline_mode;
	}
	m_Encoding = decoder.GetEncoding();

	if (m_Root->name != L"VisualStudioProject")
		return Error(L"The root element is not \"VisualStudioProject\"!");
	return true;
}

bool CVcprojFile::SaveVcprojFile(const wchar_t* filepath, DWORD file_attributes, bool safe_encoding)
{
	assert(m_NewLineMode!=eNLM_Auto && m_NewLineMode!=eNLM_Last);
	assert(m_Encoding);
	m_ErrorMessage.clear();
	if (!m_Root)
		return Error(L"No vcproj data to save!");
	if (m_NewLineMode==eNLM_Auto || m_NewLineMode==eNLM_Last)
		return Error(L"Can't save vcproj with invalid newline mode: %s", GetName(m_NewLineMode));
	if (!m_Encoding)
		return Error(L"Can't save vcproj without an encoding!");

	CXmlCharacterReferenceMap crm;
	crm.SetEncoding(m_Encoding, safe_encoding);

	wstring xml_body;
	m_Root->ToString(xml_body, crm, ToString(m_NewLineMode));

	std::vector<char> data;
	if (!CXmlTextCodec::EncodeXmlFileData(m_XmlDeclarationAttribs, xml_body, m_Encoding, m_NewLineMode, data, &m_ErrorMessage))
		return false;

	SWinHandle handle = CreateFile(filepath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, file_attributes, NULL);
	if (handle == INVALID_HANDLE_VALUE)
		return WinError(L"Error opening file for writing!");

	DWORD written;
	if (!WriteFile(handle, &data[0], (DWORD)data.size(), &written, NULL) || written!=(DWORD)data.size())
		return Error(L"Error writing file!");

	return true;
}

void CVcprojFile::SetNewLineMode(ENewLineMode newline_mode)
{
	if (newline_mode==eNLM_Auto || newline_mode==eNLM_Last)
		return;
	m_NewLineMode = newline_mode;
}

void CVcprojFile::SetEncoding(IEncoding* encoding)
{
	if (!encoding)
		return;
	m_Encoding = encoding;
}

void CVcprojFile::SetDecimalPoint(wchar_t decimal_point)
{
	assert(m_Root);
	if (!m_Root)
		return;
	for (TXmlAttribPtrVec::iterator it=m_Root->attributes.begin(),eit=m_Root->attributes.end(); it!=eit; ++it)
	{
		SXmlAttrib& attr = **it;
		if (attr.name == L"Version")
		{
			for (size_t i=0,e=attr.value.size(); i<e; ++i)
			{
				if (attr.value[i]==',' || attr.value[i]=='.')
					attr.value[i] = decimal_point;
			}
			break;
		}
	}
}

bool CVcprojFile::Error(const wchar_t* fmtstr, ...)
{
	if (!m_ErrorMessage.empty())
		return false;
	wchar_t buf[0x100];
	va_list args;
	va_start(args, fmtstr);
	vswprintf(buf, sizeof(buf)/sizeof(buf[0]), fmtstr, args);
	va_end(args);
	m_ErrorMessage = buf;
	return false;
}

bool CVcprojFile::WinError(const wchar_t* fmtstr, ...)
{
	if (!m_ErrorMessage.empty())
		return false;
	DWORD last_error = GetLastError();
	wchar_t buf[0x100];
	va_list args;
	va_start(args, fmtstr);
	vswprintf(buf, sizeof(buf)/sizeof(buf[0]), fmtstr, args);
	va_end(args);
	m_ErrorMessage = buf;
	swprintf(buf, sizeof(buf)/sizeof(buf[0]), L" [LastError: %d] %s\n", last_error, LastErrorToString(last_error).c_str());
	m_ErrorMessage.append(buf);
	return false;
}
