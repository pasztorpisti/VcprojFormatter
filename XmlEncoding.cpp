//
// Copyright (c) Istvan Pasztor
// This source has been published on www.codeproject.com under the CPOL license.
//
#include "stdafx.h"
#include "XmlEncoding.h"


const wchar_t* GetName(ENewLineMode newline_mode)
{
	switch (newline_mode)
	{
	case eNLM_CR: return L"CR";
	case eNLM_CRLF: return L"CRLF";
	case eNLM_LF: return L"LF";
	case eNLM_LFCR: return L"LFCR";
	case eNLM_Auto: return L"AUTO";
	default: assert(0); return L"UNKNOWN_NEWLINE_MODE";
	}
}

ENewLineMode NewLineModeFromName(const wchar_t* new_line_mode_name)
{
	for (int i=0; i<eNLM_Last; ++i)
	{
		ENewLineMode nlm = (ENewLineMode)i;
		if (0 == _wcsicmp(new_line_mode_name, GetName(nlm)))
			return nlm;
	}
	return eNLM_Last;
}

const wchar_t* ToString(ENewLineMode newline_mode)
{
	switch (newline_mode)
	{
	case eNLM_CR: return L"\r";
	case eNLM_CRLF: return L"\r\n";
	case eNLM_LF: return L"\n";
	case eNLM_LFCR: return L"\n\r";
	default: assert(0); return L"";
	}
}

ENewLineMode DetectNewLineMode(const wchar_t* s_begin, const wchar_t* s_end)
{
	for (const wchar_t* s=s_begin; s<s_end; ++s)
	{
		if (s[0] == 10)
		{
			if (s+1 < s_end)
			{
				if (s[1] == 13)
					return eNLM_LFCR;
			}
			return eNLM_LF;
		}
		else if (s[0] == 13)
		{
			if (s+1 < s_end)
			{
				if (s[1] == 10)
					return eNLM_CRLF;
			}
			return eNLM_CR;
		}
	}
	return eNLM_Auto;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------


static const wchar_t* UTF16LE_NAMES[] = { L"UTF-16", L"UTF16" };
class UTF16LE_Encoding : public IEncoding
{
public:
	virtual int GetNameCount() const							{ return sizeof(UTF16LE_NAMES) / sizeof(UTF16LE_NAMES[0]); }
	virtual const wchar_t* GetName(int name_index) const		{ return UTF16LE_NAMES[name_index]; }
	virtual bool IsAvailable() const							{ return true; }
	virtual int UTF16ToBytes(const wchar_t* utf16le_str, int utf16_chars, char* bytes, int byte_count, wstring* error_message=NULL) const;
	virtual int BytesToUTF16(const char* bytes, int byte_count, wchar_t* utf16le_str, int utf16_chars, wstring* error_message=NULL) const;
	virtual int GetBOMSizeBytes() const				{ return 2; }
	virtual const char* GetBOM() const				{ return "\xFF\xFE"; }
	virtual int GetFlags() const					{ return eF_CanRepresentAllUniChars; }
};

int UTF16LE_Encoding::UTF16ToBytes(const wchar_t* utf16le_str, int utf16_chars, char* bytes, int byte_count, wstring* error_message) const
{
	if (utf16_chars <= 0)
		return utf16_chars;
	int res = utf16_chars * 2;
	if (byte_count > 0)
	{
		if (byte_count < res)
		{
			if (error_message)
				*error_message = L"Destination buffer is too small!";
			return -1;
		}
		memcpy(bytes, utf16le_str, res);
	}
	return res;
}

int UTF16LE_Encoding::BytesToUTF16(const char* bytes, int byte_count, wchar_t* utf16le_str, int utf16_chars, wstring* error_message) const
{
	if (byte_count <= 0)
		return byte_count;
	int res = byte_count / 2;
	if (utf16_chars > 0)
	{
		if (utf16_chars < res)
		{
			if (error_message)
				*error_message = L"Destination buffer is too small!";
			return -1;
		}
		memcpy(utf16le_str, bytes, byte_count & ~1);
	}
	return res;
}

//-------------------------------------------------------------------------------------------------

class Codepage_Encoding : public IEncoding
{
public:
	Codepage_Encoding(UINT codepage=0, const wchar_t** names=NULL, int name_count=0)
		: m_Codepage(codepage), m_Names(names), m_NameCount(name_count) {}
	virtual int GetNameCount() const							{ return m_NameCount; }
	virtual const wchar_t* GetName(int name_index) const		{ return m_Names[name_index]; }
	virtual bool IsAvailable() const;
	virtual int UTF16ToBytes(const wchar_t* utf16le_str, int utf16_chars, char* bytes, int byte_count, wstring* error_message=NULL) const;
	virtual int BytesToUTF16(const char* bytes, int byte_count, wchar_t* utf16le_str, int utf16_chars, wstring* error_message=NULL) const;

private:
	UINT m_Codepage;
	const wchar_t** m_Names;
	int m_NameCount;
};

bool Codepage_Encoding::IsAvailable() const
{
	wchar_t c = L' ';
	int res = WideCharToMultiByte(m_Codepage, 0, &c, 1, NULL, 0, NULL, NULL);
	return res > 0;
}

int Codepage_Encoding::UTF16ToBytes(const wchar_t* utf16le_str, int utf16_chars, char* bytes, int byte_count, wstring* error_message) const
{
	if (utf16_chars <= 0)
	{
		if (utf16_chars<0 && error_message)
			*error_message = L"Negative destination buffer size!!!";
		return utf16_chars;
	}
	int res = WideCharToMultiByte(m_Codepage, 0, utf16le_str, utf16_chars, bytes, byte_count, NULL, NULL);
	if (res <= 0)
	{
		if (error_message)
		{
			*error_message = L"Error encoding UTF-16 text with the specified encoding! ";
			*error_message += LastErrorToString(GetLastError());
		}
		return -1;
	}
	return res;
}

int Codepage_Encoding::BytesToUTF16(const char* bytes, int byte_count, wchar_t* utf16le_str, int utf16_chars, wstring* error_message) const
{
	if (byte_count <= 0)
	{
		if (byte_count<0 && error_message)
			*error_message = L"Negative destination buffer size!!!";
		return byte_count;
	}
	int res = MultiByteToWideChar(m_Codepage, MB_ERR_INVALID_CHARS, bytes, byte_count, utf16le_str, utf16_chars);
	if (res <= 0)
	{
		if (error_message)
		{
			*error_message = L"Error decoding to UTF-16 with the specified encoding! ";
			*error_message += LastErrorToString(GetLastError());
		}
		return -1;
	}
	return res;
}

//-------------------------------------------------------------------------------------------------

static const wchar_t* UTF8_NAMES[] = { L"UTF-8", L"UTF8", L"Windows-65001" };
class UTF8_Encoding : public Codepage_Encoding
{
public:
	UTF8_Encoding() : Codepage_Encoding(65001, UTF8_NAMES, sizeof(UTF8_NAMES)/sizeof(UTF8_NAMES[0])) {}
	virtual int GetBOMSizeBytes() const				{ return 3; }
	virtual const char* GetBOM() const				{ return "\xEF\xBB\xBF"; }
	virtual int GetFlags() const					{ return eF_CanRepresentAllUniChars; }
};

//-------------------------------------------------------------------------------------------------

CEncodings::CEncodings()
{
	static UTF16LE_Encoding utf16le_encoding;
	AddEncoding(&utf16le_encoding);
	static UTF8_Encoding utf8_encoding;
	AddEncoding(&utf8_encoding);

#define REGISTER_CODEPAGE(codepage, ...) \
	static const wchar_t* CP##codepage##_names[] = { __VA_ARGS__ }; \
	static Codepage_Encoding CP##codepage##_encoding(codepage, CP##codepage##_names, \
		sizeof(CP##codepage##_names)/sizeof(CP##codepage##_names[0])); \
	AddEncoding(&CP##codepage##_encoding);

	 REGISTER_CODEPAGE(20127, L"ASCII", L"us", L"US-ASCII", L"csASCII", L"Windows-20127", L"646", L"ANSI_X3.4-1968", L"ANSI_X3.4-1986", L"ascii7", L"cp367", L"ibm-367", L"IBM367", L"iso-ir-6", L"ISO646-US", L"iso_646.irv:1983", L"ISO_646.irv:1991");
	 REGISTER_CODEPAGE(1250, L"Windows-1250", L"cp1250", L"ibm-1250", L"ibm-1250_P100-1995", L"ibm-5346", L"ibm-5346_P100-1998");
	 REGISTER_CODEPAGE(1251, L"Windows-1251", L"cp1251", L"ibm-1251", L"ibm-1251_P100-1995", L"ibm-5347", L"ibm-5347_P100-1998");
	 REGISTER_CODEPAGE(1252, L"Windows-1252", L"cp1252", L"ibm-1252", L"ibm-1252_P100-2000", L"ibm-5348", L"ibm-5348_P100-1997");
	 REGISTER_CODEPAGE(1253, L"Windows-1253", L"cp1253", L"ibm-1253", L"ibm-1253_P100-1995", L"ibm-5349", L"ibm-5349_P100-1998");
	 REGISTER_CODEPAGE(1254, L"Windows-1254", L"cp1254", L"ibm-1254", L"ibm-1254_P100-1995", L"ibm-5350", L"ibm-5350_P100-1998");
	 REGISTER_CODEPAGE(1255, L"Windows-1255", L"cp1255", L"ibm-1255", L"ibm-1255_P100-1995", L"ibm-9447", L"ibm-9447_P100-2002");
	 REGISTER_CODEPAGE(1256, L"Windows-1256", L"cp1256", L"ibm-1256", L"ibm-1256_P110-1997", L"ibm-9448", L"ibm-9448_X100-2005", L"x-windows-1256S");
	 REGISTER_CODEPAGE(1257, L"Windows-1257", L"cp1257", L"ibm-1257", L"ibm-1257_P100-1995", L"ibm-9449", L"ibm-9449_P100-2002");
	 REGISTER_CODEPAGE(1258, L"Windows-1258", L"cp1258", L"ibm-1258", L"ibm-1258_P100-1997", L"ibm-5354", L"ibm-5354_P100-1998");
	 REGISTER_CODEPAGE(37, L"IBM037", L"037", L"cp037", L"cpibm37", L"csIBM037", L"ebcdic-cp-ca", L"ebcdic-cp-nl", L"ebcdic-cp-us", L"ebcdic-cp-wt", L"ibm-37", L"ibm-37_P100-1995");
	REGISTER_CODEPAGE(256, L"ibm-256", L"ibm-256_P100-1995");
	REGISTER_CODEPAGE(259, L"csIBMSymbols", L"ibm-259", L"ibm-259_P100-1995", L"IBM-Symbols");
	//REGISTER_CODEPAGE(273, L"IBM273", L"273", L"CP273", L"csIBM273", L"ibm-273", L"ibm-273_P100-1995");
	REGISTER_CODEPAGE(274, L"IBM274", L"CP274", L"csIBM274", L"EBCDIC-BE", L"ibm-274", L"ibm-274_P100-2000");
	REGISTER_CODEPAGE(275, L"IBM275", L"cp275", L"csIBM275", L"EBCDIC-BR", L"ibm-275", L"ibm-275_P100-1995");
	//REGISTER_CODEPAGE(277, L"IBM277", L"277", L"cp277", L"csIBM277", L"EBCDIC-CP-DK", L"EBCDIC-CP-NO", L"ibm-277", L"ibm-277_P100-1995");
	//REGISTER_CODEPAGE(278, L"IBM278", L"278", L"cp278", L"csIBM278", L"ebcdic-cp-fi", L"ebcdic-cp-se", L"ebcdic-sv", L"ibm-278", L"ibm-278_P100-1995");
	//REGISTER_CODEPAGE(280, L"IBM280", L"280", L"CP280", L"csIBM280", L"ebcdic-cp-it", L"ibm-280", L"ibm-280_P100-1995");
	//REGISTER_CODEPAGE(284, L"IBM284", L"284", L"CP284", L"cpibm284", L"csIBM284", L"ebcdic-cp-es", L"ibm-284", L"ibm-284_P100-1995");
	//REGISTER_CODEPAGE(285, L"IBM285", L"285", L"CP285", L"cpibm285", L"csIBM285", L"ebcdic-cp-gb", L"ebcdic-gb", L"ibm-285", L"ibm-285_P100-1995");
	REGISTER_CODEPAGE(286, L"ibm-286", L"csEBCDICATDEA", L"EBCDIC-AT-DE-A", L"ibm-286_P100-2003");
	//REGISTER_CODEPAGE(290, L"IBM290", L"cp290", L"csIBM290", L"EBCDIC-JP-kana", L"ibm-290", L"ibm-290_P100-1995");
	REGISTER_CODEPAGE(293, L"ibm-293", L"ibm-293_P100-1995");
	//REGISTER_CODEPAGE(297, L"IBM297", L"297", L"cp297", L"cpibm297", L"csIBM297", L"ebcdic-cp-fr", L"ibm-297", L"ibm-297_P100-1995");
	REGISTER_CODEPAGE(300, L"ibm-300", L"ibm-300_P120-2006", L"x-IBM300");
	REGISTER_CODEPAGE(301, L"ibm-301", L"ibm-301_P110-1997", L"x-IBM301");
	//REGISTER_CODEPAGE(420, L"IBM420", L"420", L"cp420", L"csIBM420", L"ebcdic-cp-ar1", L"ibm-420", L"ibm-420_X120-1999");
	//REGISTER_CODEPAGE(424, L"IBM424", L"424", L"cp424", L"csIBM424", L"ebcdic-cp-he", L"ibm-424", L"ibm-424_P100-1995");
	REGISTER_CODEPAGE(425, L"ibm-425", L"ibm-425_P101-2000");
	 REGISTER_CODEPAGE(437, L"IBM437", L"Windows-437", L"437", L"cp437", L"csPC8CodePage437", L"ibm-437", L"ibm-437_P100-1995");
	 REGISTER_CODEPAGE(500, L"IBM500", L"CP500", L"csIBM500", L"ebcdic-cp-be", L"ebcdic-cp-ch", L"ibm-500", L"ibm-500_P100-1995");
	 REGISTER_CODEPAGE(708, L"ASMO-708");
	 REGISTER_CODEPAGE(720, L"DOS-720", L"Windows-720", L"ibm-720", L"ibm-720_P100-1997", L"x-IBM720");
	 REGISTER_CODEPAGE(737, L"IBM737", L"Windows-737", L"737", L"cp737", L"ibm-737", L"ibm-737_P100-1997", L"x-IBM737");
	 REGISTER_CODEPAGE(775, L"IBM775", L"Windows-775", L"775", L"cp775", L"csPC775Baltic", L"ibm-775", L"ibm-775_P100-1996");
	REGISTER_CODEPAGE(803, L"ibm-803", L"ibm-803_P100-1999");
	REGISTER_CODEPAGE(806, L"ibm-806", L"ibm-806_P100-1998");
	REGISTER_CODEPAGE(808, L"ibm-808", L"ibm-808_P100-1999", L"x-IBM808");
	REGISTER_CODEPAGE(813, L"813", L"8859_7", L"cp813", L"ibm-813", L"ibm-813_P100-1995");
	//REGISTER_CODEPAGE(819, L"ISO-8859-1", L"819", L"8859_1", L"cp819", L"csISOLatin1", L"ibm-819", L"IBM819", L"iso-ir-100", L"ISO_8859-1:1987", L"l1", L"latin1");
	REGISTER_CODEPAGE(833, L"ibm-833", L"ibm-833_P100-1995", L"x-IBM833");
	REGISTER_CODEPAGE(834, L"ibm-834", L"ibm-834_P100-1995", L"x-IBM834");
	REGISTER_CODEPAGE(835, L"ibm-835", L"ibm-835_P100-1995", L"x-IBM835");
	REGISTER_CODEPAGE(836, L"ibm-836", L"ibm-836_P100-1995", L"x-IBM836");
	REGISTER_CODEPAGE(837, L"ibm-837", L"ibm-837_P100-1995", L"x-IBM837");
	REGISTER_CODEPAGE(848, L"ibm-848", L"ibm-848_P100-1999");
	REGISTER_CODEPAGE(849, L"ibm-849", L"ibm-849_P100-1999");
	 REGISTER_CODEPAGE(850, L"IBM850", L"Windows-850", L"850", L"cp850", L"csPC850Multilingual", L"ibm-850", L"ibm-850_P100-1995");
	 REGISTER_CODEPAGE(851, L"IBM851", L"851", L"cp851", L"csPC851", L"ibm-851", L"ibm-851_P100-1995");
	 REGISTER_CODEPAGE(852, L"IBM852", L"Windows-852", L"852", L"cp852", L"csPCp852", L"ibm-852", L"ibm-852_P100-1995");
	 REGISTER_CODEPAGE(855, L"IBM855", L"Windows-855", L"855", L"cp855", L"csIBM855", L"csPCp855", L"ibm-855", L"ibm-855_P100-1995");
	 REGISTER_CODEPAGE(856, L"IBM856", L"856", L"cp856", L"ibm-856", L"ibm-856_P100-1995", L"x-IBM856");
	 REGISTER_CODEPAGE(857, L"IBM857", L"Windows-857", L"857", L"cp857", L"csIBM857", L"ibm-857", L"ibm-857_P100-1995");
	 REGISTER_CODEPAGE(858, L"IBM00858", L"Windows-858", L"CCSID00858", L"CP00858", L"cp858", L"ibm-858", L"ibm-858_P100-1997", L"PC-Multilingual-850+euro");
	REGISTER_CODEPAGE(859, L"ibm-859", L"ibm-859_P100-1999", L"x-IBM859");
	 REGISTER_CODEPAGE(860, L"IBM860", L"860", L"cp860", L"csIBM860", L"ibm-860", L"ibm-860_P100-1995");
	 REGISTER_CODEPAGE(861, L"IBM861", L"Windows-861", L"861", L"cp-is", L"cp861", L"csIBM861", L"ibm-861", L"ibm-861_P100-1995");
	 REGISTER_CODEPAGE(862, L"DOS-862", L"Windows-862", L"862", L"cp862", L"csPC862LatinHebrew", L"ibm-862", L"ibm-862_P100-1995", L"IBM862");
	 REGISTER_CODEPAGE(863, L"IBM863", L"863", L"cp863", L"csIBM863", L"ibm-863", L"ibm-863_P100-1995");
	 REGISTER_CODEPAGE(864, L"IBM864", L"cp864", L"csIBM864", L"ibm-864", L"ibm-864_X110-1999");
	 REGISTER_CODEPAGE(865, L"IBM865", L"865", L"cp865", L"csIBM865", L"ibm-865", L"ibm-865_P100-1995");
	 REGISTER_CODEPAGE(866, L"cp866", L"IBM866", L"Windows-866", L"866", L"csIBM866", L"ibm-866", L"ibm-866_P100-1995");
	REGISTER_CODEPAGE(867, L"ibm-867", L"ibm-867_P100-1998", L"x-IBM867");
	 REGISTER_CODEPAGE(868, L"IBM868", L"868", L"cp-ar", L"CP868", L"csIBM868", L"ibm-868", L"ibm-868_P100-1995");
	 REGISTER_CODEPAGE(869, L"IBM869", L"Windows-869", L"869", L"cp-gr", L"cp869", L"csIBM869", L"ibm-869", L"ibm-869_P100-1995");
	 REGISTER_CODEPAGE(870, L"IBM870", L"CP870", L"csIBM870", L"ebcdic-cp-roece", L"ebcdic-cp-yu", L"ibm-870", L"ibm-870_P100-1995");
	//REGISTER_CODEPAGE(871, L"IBM871", L"871", L"CP871", L"csIBM871", L"ebcdic-cp-is", L"ebcdic-is", L"ibm-871", L"ibm-871_P100-1995");
	REGISTER_CODEPAGE(872, L"ibm-872", L"ibm-872_P100-1999");
	 REGISTER_CODEPAGE(874, L"Windows-874", L"Windows-874-2000", L"MS874", L"TIS-620", L"x-windows-874");
	 REGISTER_CODEPAGE(875, L"cp875", L"875", L"ibm-875", L"ibm-875_P100-1995", L"IBM875", L"x-IBM875");
	REGISTER_CODEPAGE(896, L"ibm-896", L"ibm-896_P100-1995");
	REGISTER_CODEPAGE(897, L"ibm-897", L"csHalfWidthKatakana", L"ibm-897_P100-1995", L"JIS_X0201", L"x-IBM897", L"X0201");
	REGISTER_CODEPAGE(901, L"ibm-901", L"ibm-901_P100-1999");
	REGISTER_CODEPAGE(902, L"ibm-902", L"ibm-902_P100-1999");
	REGISTER_CODEPAGE(916, L"cp916", L"916", L"ibm-916", L"ibm-916_P100-1995");
	REGISTER_CODEPAGE(918, L"IBM918", L"CP918", L"csIBM918", L"ebcdic-cp-ar2", L"ibm-918", L"ibm-918_P100-1995");
	REGISTER_CODEPAGE(922, L"IBM922", L"cp922", L"922", L"ibm-922", L"ibm-922_P100-1999", L"x-IBM922");
	//REGISTER_CODEPAGE(924, L"IBM00924", L"ibm-924", L"CCSID00924", L"CP00924", L"ebcdic-Latin9--euro", L"ibm-924_P100-1998");
	REGISTER_CODEPAGE(926, L"ibm-926", L"ibm-926_P100-2000");
	REGISTER_CODEPAGE(927, L"ibm-927", L"ibm-927_P100-1995", L"x-IBM927");
	REGISTER_CODEPAGE(928, L"ibm-928", L"ibm-928_P100-1995");
	 REGISTER_CODEPAGE(932, L"shift_jis", L"shift-jis", L"Windows-932", L"cp932", L"Windows-31j", L"cp943c", L"csShiftJIS", L"csWindows31J", L"ibm-932", L"ibm-942", L"ibm-942_P12A-1999", L"ibm-943_P15A-2003", L"MS_Kanji", L"x-IBM942", L"x-IBM942C", L"x-JISAutoDetect", L"x-ms-cp932", L"x-MS932_0213", L"x-sjis");
	REGISTER_CODEPAGE(933, L"ibm-933", L"933", L"cp933", L"ibm-933_P110-1995", L"x-IBM933");
	REGISTER_CODEPAGE(935, L"ibm-935", L"935", L"cp935", L"ibm-935_P110-1999", L"x-IBM935");
	 REGISTER_CODEPAGE(936, L"gb2312", L"csGB2312", L"Windows-936", L"Windows-936-2000", L"CP936", L"GBK", L"MS936");
	REGISTER_CODEPAGE(937, L"ibm-937", L"937", L"cp937", L"ibm-937_P110-1999", L"x-IBM937");
	REGISTER_CODEPAGE(941, L"ibm-941", L"ibm-941_P13A-2001");
	REGISTER_CODEPAGE(943, L"ibm-943", L"943", L"cp943", L"ibm-943_P130-1999", L"x-IBM943");
	REGISTER_CODEPAGE(944, L"ibm-944", L"ibm-944_P100-1995");
	REGISTER_CODEPAGE(946, L"ibm-946", L"ibm-946_P100-1995");
	REGISTER_CODEPAGE(947, L"ibm-947", L"ibm-947_P100-1995", L"x-IBM947");
	REGISTER_CODEPAGE(948, L"ibm-948", L"ibm-948_P110-1999", L"x-IBM948");
	 REGISTER_CODEPAGE(949, L"ks_c_5601-1987", L"Windows-949", L"Windows-949-2000", L"949", L"cp949", L"csKSC56011987", L"ibm-949", L"ibm-949_P110-1999", L"iso-ir-149", L"korean", L"KS_C_5601-1989", L"KSC_5601", L"ms949", L"x-IBM949", L"x-KSC5601");
	 REGISTER_CODEPAGE(950, L"big5", L"Windows-950", L"Windows-950-2000", L"950", L"cp950", L"csBig5", L"ibm-950", L"ibm-950_P110-1999", L"x-IBM950", L"x-windows-950");
	REGISTER_CODEPAGE(951, L"ibm-951", L"ibm-951_P100-1995", L"x-IBM951");
	REGISTER_CODEPAGE(952, L"ibm-952", L"ibm-952_P110-1997");
	REGISTER_CODEPAGE(953, L"ibm-953", L"ibm-953_P100-2000", L"JIS_X0212-1990");
	REGISTER_CODEPAGE(954, L"ibm-954", L"eucjis", L"ibm-954_P101-2007", L"x-IBM954", L"x-IBM954C");
	REGISTER_CODEPAGE(955, L"ibm-955", L"ibm-955_P110-1997");
	REGISTER_CODEPAGE(964, L"ibm-964", L"964", L"cp964", L"ibm-964_P110-1999", L"x-IBM964");
	REGISTER_CODEPAGE(971, L"ibm-971", L"x-IBM971");
	REGISTER_CODEPAGE(1004, L"ibm-1004", L"ibm-1004_P100-1995");
	REGISTER_CODEPAGE(1006, L"IBM1006", L"1006", L"cp1006", L"ibm-1006", L"ibm-1006_P100-1995", L"x-IBM1006");
	REGISTER_CODEPAGE(1008, L"ibm-1008", L"ibm-1008_P100-1995");
	REGISTER_CODEPAGE(1009, L"ibm-1009", L"ibm-1009_P100-1995");
	REGISTER_CODEPAGE(1010, L"ibm-1010", L"csISO69French", L"fr", L"ibm-1010_P100-1995", L"iso-ir-69", L"ISO646-FR", L"NF_Z_62-010");
	REGISTER_CODEPAGE(1011, L"ibm-1011", L"csISO21German", L"de", L"DIN_66003", L"ibm-1011_P100-1995", L"iso-ir-21", L"ISO646-DE");
	REGISTER_CODEPAGE(1012, L"ibm-1012", L"csISO15Italian", L"ibm-1012_P100-1995", L"iso-ir-15", L"ISO646-IT", L"IT");
	REGISTER_CODEPAGE(1013, L"ibm-1013", L"BS_4730", L"csISO4UnitedKingdom", L"gb", L"ibm-1013_P100-1995", L"iso-ir-4", L"ISO646-GB", L"uk");
	REGISTER_CODEPAGE(1014, L"ibm-1014", L"csISO85Spanish2", L"ES2", L"ibm-1014_P100-1995", L"iso-ir-85", L"ISO646-ES2");
	REGISTER_CODEPAGE(1015, L"ibm-1015", L"csISO84Portuguese2", L"ibm-1015_P100-1995", L"iso-ir-84", L"ISO646-PT2", L"PT2");
	REGISTER_CODEPAGE(1016, L"ibm-1016", L"csISO60DanishNorwegian", L"csISO60Norwegian1", L"ibm-1016_P100-1995", L"iso-ir-60", L"ISO646-NO", L"no", L"NS_4551-1");
	REGISTER_CODEPAGE(1017, L"ibm-1017", L"ibm-1017_P100-1995");
	REGISTER_CODEPAGE(1018, L"ibm-1018", L"csISO10Swedish", L"FI", L"ibm-1018_P100-1995", L"iso-ir-10", L"ISO646-FI", L"ISO646-SE", L"se", L"SEN_850200_B");
	REGISTER_CODEPAGE(1019, L"ibm-1019", L"ibm-1019_P100-1995");
	REGISTER_CODEPAGE(1020, L"ibm-1020", L"ca", L"csa7-1", L"CSA_Z243.4-1985-1", L"csISO121Canadian1", L"ibm-1020_P100-2003", L"iso-ir-121", L"ISO646-CA");
	REGISTER_CODEPAGE(1021, L"ibm-1021", L"ibm-1021_P100-2003");
	REGISTER_CODEPAGE(1023, L"ibm-1023", L"csISO17Spanish", L"ES", L"ibm-1023_P100-2003", L"iso-ir-17", L"ISO646-ES");
	//REGISTER_CODEPAGE(1025, L"cp1025", L"ibm-1025", L"1025", L"ibm-1025_P100-1995", L"x-IBM1025");
	 REGISTER_CODEPAGE(1026, L"IBM1026", L"ibm-1026", L"1026", L"CP1026", L"csIBM1026", L"ibm-1026_P100-1995");
	REGISTER_CODEPAGE(1027, L"ibm-1027", L"ibm-1027_P100-1995", L"x-IBM1027");
	REGISTER_CODEPAGE(1041, L"ibm-1041", L"ibm-1041_P100-1995", L"x-IBM1041");
	REGISTER_CODEPAGE(1043, L"ibm-1043", L"ibm-1043_P100-1995", L"x-IBM1043");
	REGISTER_CODEPAGE(1046, L"ibm-1046", L"ibm-1046_X110-1999", L"x-IBM1046", L"x-IBM1046S");
	 REGISTER_CODEPAGE(1047, L"IBM1047", L"ibm01047", L"1047", L"cp1047", L"ibm-1047", L"ibm-1047_P100-1995");
	REGISTER_CODEPAGE(1051, L"ibm-1051", L"csHPRoman8", L"hp-roman8", L"ibm-1051_P100-1995", L"r8", L"roman8");
	REGISTER_CODEPAGE(1088, L"ibm-1088", L"ibm-1088_P100-1995", L"x-IBM1088");
	REGISTER_CODEPAGE(1097, L"ibm-1097", L"1097", L"cp1097", L"ibm-1097_P100-1995", L"x-IBM1097");
	REGISTER_CODEPAGE(1098, L"ibm-1098", L"1098", L"cp1098", L"ibm-1098_P100-1995", L"IBM1098", L"x-IBM1098");
	REGISTER_CODEPAGE(1100, L"ibm-1100", L"csDECMCS", L"dec", L"DEC-MCS", L"ibm-1100_P100-2003");
	REGISTER_CODEPAGE(1101, L"ibm-1101", L"ibm-1101_P100-2003");
	REGISTER_CODEPAGE(1102, L"ibm-1102", L"ibm-1102_P100-2003");
	REGISTER_CODEPAGE(1103, L"ibm-1103", L"ibm-1103_P100-2003");
	REGISTER_CODEPAGE(1104, L"ibm-1104", L"csISO25French", L"ibm-1104_P100-2003", L"iso-ir-25", L"ISO646-FR1");
	REGISTER_CODEPAGE(1105, L"ibm-1105", L"ibm-1105_P100-2003");
	REGISTER_CODEPAGE(1106, L"ibm-1106", L"ibm-1106_P100-2003");
	REGISTER_CODEPAGE(1107, L"ibm-1107", L"csISO646Danish", L"dk", L"DS_2089", L"ibm-1107_P100-2003", L"ISO646-DK");
	REGISTER_CODEPAGE(1112, L"ibm-1112", L"1112", L"cp1112", L"ibm-1112_P100-1995", L"x-IBM1112");
	REGISTER_CODEPAGE(1114, L"ibm-1114", L"ibm-1114_P100-2001", L"x-IBM1114");
	REGISTER_CODEPAGE(1115, L"ibm-1115", L"ibm-1115_P100-1995", L"x-IBM1115");
	REGISTER_CODEPAGE(1122, L"ibm-1122", L"1122", L"cp1122", L"ibm-1122_P100-1999", L"x-IBM1122");
	REGISTER_CODEPAGE(1123, L"ibm-1123", L"1123", L"cp1123", L"ibm-1123_P100-1995", L"x-IBM1123");
	REGISTER_CODEPAGE(1124, L"ibm-1124", L"cp1124", L"1124", L"ibm-1124_P100-1996", L"x-IBM1124");
	REGISTER_CODEPAGE(1125, L"ibm-1125", L"ibm-1125_P100-1997");
	REGISTER_CODEPAGE(1127, L"ibm-1127", L"ibm-1127_P100-2004");
	REGISTER_CODEPAGE(1129, L"ibm-1129", L"ibm-1129_P100-1997");
	REGISTER_CODEPAGE(1130, L"ibm-1130", L"ibm-1130_P100-1997");
	REGISTER_CODEPAGE(1131, L"ibm-1131", L"ibm-1131_P100-1997");
	REGISTER_CODEPAGE(1132, L"ibm-1132", L"ibm-1132_P100-1998");
	REGISTER_CODEPAGE(1133, L"ibm-1133", L"ibm-1133_P100-1997");
	REGISTER_CODEPAGE(1137, L"ibm-1137", L"ibm-1137_P100-1999");
	 REGISTER_CODEPAGE(1140, L"IBM01140", L"CCSID01140", L"CP01140", L"cp1140", L"ebcdic-us-37+euro", L"ibm-1140", L"ibm-1140_P100-1997");
	 REGISTER_CODEPAGE(1141, L"IBM01141", L"CCSID01141", L"CP01141", L"cp1141", L"ebcdic-de-273+euro", L"ibm-1141", L"ibm-1141_P100-1997");
	 REGISTER_CODEPAGE(1142, L"IBM01142", L"CCSID01142", L"CP01142", L"cp1142", L"ebcdic-dk-277+euro", L"ebcdic-no-277+euro", L"ibm-1142", L"ibm-1142_P100-1997");
	 REGISTER_CODEPAGE(1143, L"IBM01143", L"CCSID01143", L"CP01143", L"cp1143", L"ebcdic-fi-278+euro", L"ebcdic-se-278+euro", L"ibm-1143", L"ibm-1143_P100-1997");
	 REGISTER_CODEPAGE(1144, L"IBM01144", L"CCSID01144", L"CP01144", L"cp1144", L"ebcdic-it-280+euro", L"ibm-1144", L"ibm-1144_P100-1997");
	 REGISTER_CODEPAGE(1145, L"IBM01145", L"CCSID01145", L"CP01145", L"cp1145", L"ebcdic-es-284+euro", L"ibm-1145", L"ibm-1145_P100-1997");
	 REGISTER_CODEPAGE(1146, L"IBM01146", L"CCSID01146", L"CP01146", L"cp1146", L"ebcdic-gb-285+euro", L"ibm-1146", L"ibm-1146_P100-1997");
	 REGISTER_CODEPAGE(1147, L"IBM01147", L"CCSID01147", L"CP01147", L"cp1147", L"ebcdic-fr-297+euro", L"ibm-1147", L"ibm-1147_P100-1997");
	 REGISTER_CODEPAGE(1148, L"IBM01148", L"CCSID01148", L"CP01148", L"cp1148", L"ebcdic-international-500+euro", L"ibm-1148", L"ibm-1148_P100-1997");
	 REGISTER_CODEPAGE(1149, L"IBM01149", L"CCSID01149", L"CP01149", L"cp1149", L"ebcdic-is-871+euro", L"ibm-1149", L"ibm-1149_P100-1997");
	REGISTER_CODEPAGE(1153, L"ibm-1153", L"ibm-1153_P100-1999", L"IBM1153", L"x-IBM1153");
	REGISTER_CODEPAGE(1154, L"ibm-1154", L"ibm-1154_P100-1999");
	REGISTER_CODEPAGE(1155, L"ibm-1155", L"ibm-1155_P100-1999");
	REGISTER_CODEPAGE(1156, L"ibm-1156", L"ibm-1156_P100-1999");
	REGISTER_CODEPAGE(1157, L"ibm-1157", L"ibm-1157_P100-1999");
	REGISTER_CODEPAGE(1158, L"ibm-1158", L"ibm-1158_P100-1999");
	REGISTER_CODEPAGE(1160, L"ibm-1160", L"ibm-1160_P100-1999");
	REGISTER_CODEPAGE(1161, L"ibm-1161", L"ibm-1161_P100-1999");
	REGISTER_CODEPAGE(1162, L"ibm-1162", L"ibm-1162_P100-1999");
	REGISTER_CODEPAGE(1163, L"ibm-1163", L"ibm-1163_P100-1999");
	REGISTER_CODEPAGE(1164, L"ibm-1164", L"ibm-1164_P100-1999");
	REGISTER_CODEPAGE(1165, L"ibm-1165", L"ibm-1165_P101-2000");
	REGISTER_CODEPAGE(1166, L"ibm-1166", L"ibm-1166_P100-2002");
	REGISTER_CODEPAGE(1167, L"ibm-1167", L"ibm-1167_P100-2002", L"x-KOI8_RU");
	REGISTER_CODEPAGE(1174, L"ibm-1174", L"csKZ1048", L"ibm-1174_X100-2007", L"KZ-1048", L"RK1048", L"STRK1048-2002");
	//REGISTER_CODEPAGE(1200, L"utf-16", L"Windows-1200", L"ibm-1202", L"ibm-1203", L"ibm-13490", L"ibm-13491", L"ibm-17586", L"ibm-17587", L"ibm-21682", L"ibm-21683", L"ibm-25778", L"ibm-25779", L"ibm-29874", L"ibm-29875", L"UnicodeLittleUnmarked");
	//REGISTER_CODEPAGE(1201, L"unicodeFFFE", L"Windows-1201", L"ibm-1200", L"ibm-1201", L"ibm-13488", L"ibm-13489", L"ibm-17584", L"ibm-17585", L"ibm-21680", L"ibm-21681", L"ibm-25776", L"ibm-25777", L"ibm-29872", L"ibm-29873", L"ibm-61955", L"ibm-61956", L"UnicodeBigUnmarked");
	REGISTER_CODEPAGE(1205, L"ibm-1204", L"ibm-1205", L"ISO-10646-UCS-2");
	REGISTER_CODEPAGE(1213, L"ibm-1212", L"ibm-1213", L"SCSU");
	REGISTER_CODEPAGE(1215, L"ibm-1215", L"ibm-1214", L"BOCU-1", L"csBOCU-1");
	REGISTER_CODEPAGE(1235, L"ibm-1235", L"ibm-1234");
	REGISTER_CODEPAGE(1237, L"ibm-1237", L"ibm-1236", L"ISO-10646-UCS-4");
	REGISTER_CODEPAGE(1276, L"ibm-1276", L"Adobe-Standard-Encoding", L"csAdobeStandardEncoding", L"ibm-1276_P100-1995");
	REGISTER_CODEPAGE(1277, L"ibm-1277", L"ibm-1277_P100-1995");
	REGISTER_CODEPAGE(1350, L"ibm-1350", L"eucJP-Open", L"ibm-1350_P110-1997", L"x-eucJP-Open");
	REGISTER_CODEPAGE(1351, L"ibm-1351", L"ibm-1351_P110-1997", L"x-IBM1351");
	 REGISTER_CODEPAGE(1361, L"Johab");
	REGISTER_CODEPAGE(1362, L"ibm-1362", L"ibm-1362_P110-1999", L"x-IBM1362");
	REGISTER_CODEPAGE(1363, L"ibm-1363", L"cp1363", L"ibm-1363_P110-1997", L"ibm-1363_P11B-1998", L"x-IBM1363", L"x-IBM1363C");
	REGISTER_CODEPAGE(1364, L"ibm-1364", L"ibm-1364_P110-2007", L"x-IBM1364");
	REGISTER_CODEPAGE(1370, L"ibm-1370", L"ibm-1370_P100-1999", L"x-IBM1370");
	REGISTER_CODEPAGE(1371, L"ibm-1371", L"ibm-1371_P100-1999", L"x-IBM1371");
	REGISTER_CODEPAGE(1373, L"ibm-1373", L"ibm-1373_P100-2002");
	REGISTER_CODEPAGE(1375, L"ibm-1375", L"Big5-HKSCS", L"big5hk", L"ibm-1375_P100-2007");
	REGISTER_CODEPAGE(1380, L"ibm-1380", L"ibm-1380_P100-1995", L"x-IBM1380");
	REGISTER_CODEPAGE(1381, L"ibm-1381", L"1381", L"cp1381", L"ibm-1381_P110-1999", L"x-IBM1381");
	REGISTER_CODEPAGE(1382, L"ibm-1382", L"ibm-1382_P100-1995", L"x-IBM1382");
	REGISTER_CODEPAGE(1383, L"ibm-1383", L"1383", L"cp1383", L"ibm-1383_P110-1999");
	REGISTER_CODEPAGE(1385, L"ibm-1385", L"ibm-9577", L"ibm-9577_P100-2001", L"x-IBM1385");
	REGISTER_CODEPAGE(1386, L"ibm-1386", L"ibm-1386_P100-2001");
	REGISTER_CODEPAGE(1390, L"ibm-1390", L"ibm-1390_P110-2003", L"x-IBM1390");
	REGISTER_CODEPAGE(1399, L"ibm-1399", L"ibm-1399_P110-2003", L"x-IBM1399");
	REGISTER_CODEPAGE(4517, L"ibm-4517", L"ibm-4517_P100-2005");
	REGISTER_CODEPAGE(4899, L"ibm-4899", L"ibm-4899_P100-1998");
	REGISTER_CODEPAGE(4909, L"ibm-4909", L"ibm-4909_P100-1999");
	REGISTER_CODEPAGE(4930, L"ibm-4930", L"ibm-4930_P110-1999");
	REGISTER_CODEPAGE(4933, L"ibm-4933", L"ibm-4933_P100-2002");
	REGISTER_CODEPAGE(4948, L"ibm-4948", L"ibm-4948_P100-1995");
	REGISTER_CODEPAGE(4951, L"ibm-4951", L"ibm-4951_P100-1995");
	REGISTER_CODEPAGE(4952, L"ibm-4952", L"ibm-4952_P100-1995");
	REGISTER_CODEPAGE(4960, L"ibm-4960", L"ibm-4960_P100-1995");
	REGISTER_CODEPAGE(4971, L"ibm-4971", L"ibm-4971_P100-1999");
	REGISTER_CODEPAGE(5026, L"ibm-5026", L"930", L"cp930", L"ibm-930", L"ibm-930_P120-1999", L"IBM930", L"x-IBM930", L"x-IBM930A");
	REGISTER_CODEPAGE(5035, L"ibm-5035", L"939", L"cp939", L"ibm-931", L"ibm-939", L"ibm-939_P120-1999", L"IBM939", L"x-IBM939", L"x-IBM939A");
	REGISTER_CODEPAGE(5039, L"ibm-5039", L"ibm-5039_P11A-1998");
	REGISTER_CODEPAGE(5048, L"ibm-5048", L"ibm-5048_P100-1995");
	REGISTER_CODEPAGE(5049, L"ibm-5049", L"ibm-5049_P100-1995");
	REGISTER_CODEPAGE(5050, L"ibm-5050", L"33722", L"cp33722", L"ibm-33722", L"ibm-33722_P120-1999", L"x-IBM33722", L"x-IBM33722A", L"x-IBM33722C");
	REGISTER_CODEPAGE(5054, L"ibm-5054", L"csJISEncoding", L"ISO-2022-JP-1", L"JIS_Encoding", L"x-windows-50221");
	REGISTER_CODEPAGE(5067, L"ibm-5067", L"ibm-5067_P100-1995");
	REGISTER_CODEPAGE(5104, L"ibm-5104", L"ibm-5104_X110-1999");
	REGISTER_CODEPAGE(5123, L"ibm-5123", L"ibm-5123_P100-1999");
	REGISTER_CODEPAGE(5351, L"ibm-5351", L"ibm-5351_P100-1998");
	REGISTER_CODEPAGE(5352, L"ibm-5352", L"ibm-5352_P100-1998");
	REGISTER_CODEPAGE(5353, L"ibm-5353", L"ibm-5353_P100-1998");
	REGISTER_CODEPAGE(5471, L"ibm-5471", L"ibm-5471_P100-2006", L"MS950_HKSCS", L"x-MS950-HKSCS");
	REGISTER_CODEPAGE(5478, L"ibm-5478", L"chinese", L"csISO58GB231280", L"GB_2312-80", L"ibm-5478_P100-1995", L"iso-ir-58");
	REGISTER_CODEPAGE(8482, L"ibm-8482", L"ibm-8482_P100-1999");
	REGISTER_CODEPAGE(8612, L"ibm-8612", L"ibm-8612_P100-1995");
	REGISTER_CODEPAGE(9027, L"ibm-9027", L"ibm-9027_P100-1999");
	REGISTER_CODEPAGE(9048, L"ibm-9048", L"ibm-9048_P100-1998");
	REGISTER_CODEPAGE(9056, L"ibm-9056", L"ibm-9056_P100-1995");
	REGISTER_CODEPAGE(9061, L"ibm-9061", L"ibm-9061_P100-1999");
	REGISTER_CODEPAGE(9066, L"ibm-9066", L"cp874", L"ibm-874", L"ibm-874_P100-1995", L"tis620.2533", L"x-IBM874");
	REGISTER_CODEPAGE(9067, L"ibm-9067", L"ibm-9067_X100-2005");
	REGISTER_CODEPAGE(9145, L"ibm-9145", L"ibm-9145_P110-1997");
	REGISTER_CODEPAGE(9238, L"ibm-9238", L"ibm-9238_X110-1999");
	REGISTER_CODEPAGE(9400, L"CESU-8", L"ibm-9400");
	REGISTER_CODEPAGE(9424, L"ibm-1232", L"ibm-1233", L"ibm-9424");
	REGISTER_CODEPAGE(9580, L"ibm-1388", L"ibm-1388_P103-2001", L"ibm-9580", L"x-IBM1388");
	 REGISTER_CODEPAGE(10000, L"macintosh", L"Windows-10000", L"csMacintosh", L"mac", L"macos-0_2-10.2", L"macroman", L"x-macroman");
	 REGISTER_CODEPAGE(10001, L"x-mac-japanese");
	 REGISTER_CODEPAGE(10002, L"x-mac-chinesetrad");
	 REGISTER_CODEPAGE(10003, L"x-mac-korean");
	 REGISTER_CODEPAGE(10004, L"x-mac-arabic");
	 REGISTER_CODEPAGE(10005, L"x-mac-hebrew");
	 REGISTER_CODEPAGE(10006, L"x-mac-greek", L"Windows-10006", L"macos-6_2-10.4", L"x-MacGreek");
	 REGISTER_CODEPAGE(10007, L"x-mac-cyrillic", L"Windows-10007", L"macos-7_3-10.2", L"x-MacCyrillic", L"x-MacUkraine");
	 REGISTER_CODEPAGE(10008, L"x-mac-chinesesimp");
	 REGISTER_CODEPAGE(10010, L"x-mac-romanian");
	 REGISTER_CODEPAGE(10017, L"x-mac-ukrainian");
	 REGISTER_CODEPAGE(10021, L"x-mac-thai");
	 REGISTER_CODEPAGE(10029, L"x-mac-ce", L"Windows-10029", L"macos-29-10.2", L"x-mac-centraleurroman", L"x-MacCentralEurope");
	 REGISTER_CODEPAGE(10079, L"x-mac-icelandic");
	 REGISTER_CODEPAGE(10081, L"x-mac-turkish", L"Windows-10081", L"macos-35-10.2", L"x-MacTurkish");
	 REGISTER_CODEPAGE(10082, L"x-mac-croatian");
	 //REGISTER_CODEPAGE(12000, L"utf-32");
	 //REGISTER_CODEPAGE(12001, L"utf-32BE");
	REGISTER_CODEPAGE(12712, L"ibm-12712", L"ibm-12712_P100-1998");
	REGISTER_CODEPAGE(13125, L"ibm-13125", L"ibm-13125_P100-1997");
	REGISTER_CODEPAGE(13140, L"ibm-13140", L"ibm-13140_P101-2000");
	REGISTER_CODEPAGE(13218, L"ibm-13218", L"ibm-13218_P100-1996");
	REGISTER_CODEPAGE(13676, L"ibm-13676", L"ibm-13676_P102-2001");
	REGISTER_CODEPAGE(16804, L"ibm-16804", L"ibm-16804_X110-1999");
	REGISTER_CODEPAGE(17221, L"ibm-17221", L"ibm-17221_P100-2001");
	REGISTER_CODEPAGE(17248, L"ibm-17248", L"ibm-17248_X110-1999");
	 REGISTER_CODEPAGE(20000, L"x-chinese-cns", L"x-Chinese_CNS");
	 REGISTER_CODEPAGE(20001, L"x-cp20001");
	 REGISTER_CODEPAGE(20002, L"x-chinese-eten", L"x_Chinese-Eten");
	 REGISTER_CODEPAGE(20003, L"x-cp20003");
	 REGISTER_CODEPAGE(20004, L"x-cp20004");
	 REGISTER_CODEPAGE(20005, L"x-cp20005");
	 REGISTER_CODEPAGE(20105, L"x-IA5");
	 REGISTER_CODEPAGE(20106, L"x-IA5-German");
	 REGISTER_CODEPAGE(20107, L"x-IA5-Swedish");
	 REGISTER_CODEPAGE(20108, L"x-IA5-Norwegian");
	 //REGISTER_CODEPAGE(20261, L"x-cp20261");
	 //REGISTER_CODEPAGE(20269, L"x-cp20269");
	 REGISTER_CODEPAGE(20273, L"IBM273");
	 REGISTER_CODEPAGE(20277, L"IBM277");
	 REGISTER_CODEPAGE(20278, L"IBM278");
	 REGISTER_CODEPAGE(20280, L"IBM280");
	 REGISTER_CODEPAGE(20284, L"IBM284");
	 REGISTER_CODEPAGE(20285, L"IBM285");
	 REGISTER_CODEPAGE(20290, L"IBM290");
	 REGISTER_CODEPAGE(20297, L"IBM297");
	 REGISTER_CODEPAGE(20420, L"IBM420");
	 REGISTER_CODEPAGE(20423, L"IBM423");
	 REGISTER_CODEPAGE(20424, L"IBM424");
	 REGISTER_CODEPAGE(20780, L"ibm-16684", L"ibm-16684_P110-2003", L"ibm-20780");
	 REGISTER_CODEPAGE(20833, L"x-EBCDIC-KoreanExtended");
	 REGISTER_CODEPAGE(20838, L"IBM-Thai");
	 REGISTER_CODEPAGE(20866, L"koi8-r", L"koi", L"koi8r", L"ibm-878", L"Windows-20866", L"csKOI8R", L"ibm-878_P100-1996", L"koi8");
	 REGISTER_CODEPAGE(20871, L"IBM871");
	 REGISTER_CODEPAGE(20880, L"IBM880", L"ibm-880", L"Windows-20880", L"cp880", L"csIBM880", L"EBCDIC-Cyrillic", L"ibm-880_P100-1995");
	 REGISTER_CODEPAGE(20905, L"IBM905", L"ibm-905", L"Windows-20905", L"CP905", L"csIBM905", L"ebcdic-cp-tr", L"ibm-905_P100-1995");
	 REGISTER_CODEPAGE(20924, L"IBM00924");
	 REGISTER_CODEPAGE(20936, L"x-cp20936");
	 REGISTER_CODEPAGE(20949, L"x-cp20949");
	 REGISTER_CODEPAGE(21025, L"cp1025");
	REGISTER_CODEPAGE(21344, L"ibm-21344", L"ibm-21344_P101-2000");
	REGISTER_CODEPAGE(21427, L"ibm-21427", L"ibm-21427_P100-1999");
	 REGISTER_CODEPAGE(21866, L"koi8-u", L"koi8-ru", L"Windows-21866", L"ibm-1168", L"ibm-1168_P100-2002");
	REGISTER_CODEPAGE(25546, L"ibm-25546");
	 REGISTER_CODEPAGE(28591, L"ISO-8859-1");
	 REGISTER_CODEPAGE(28592, L"ISO-8859-2", L"Windows-28592", L"8859_2", L"912", L"cp912", L"csISOLatin2", L"ibm-912", L"ibm-912_P100-1995", L"iso-ir-101", L"ISO_8859-2:1987", L"l2", L"latin2");
	 REGISTER_CODEPAGE(28593, L"ISO-8859-3", L"Windows-28593", L"8859_3", L"913", L"cp913", L"csISOLatin3", L"ibm-913", L"ibm-913_P100-2000", L"iso-ir-109", L"ISO_8859-3:1988", L"l3", L"latin3");
	 REGISTER_CODEPAGE(28594, L"ISO-8859-4", L"Windows-28594", L"8859_4", L"914", L"cp914", L"csISOLatin4", L"ibm-914", L"ibm-914_P100-1995", L"iso-ir-110", L"ISO_8859-4:1988", L"l4", L"latin4");
	 REGISTER_CODEPAGE(28595, L"ISO-8859-5", L"Windows-28595", L"8859_5", L"915", L"cp915", L"csISOLatinCyrillic", L"cyrillic", L"ibm-915", L"ibm-915_P100-1995", L"iso-ir-144", L"ISO_8859-5:1988");
	 REGISTER_CODEPAGE(28596, L"ISO-8859-6", L"Windows-28596", L"1089", L"8859_6", L"arabic", L"cp1089", L"csISOLatinArabic", L"ECMA-114", L"ibm-1089", L"ibm-1089_P100-1995", L"ISO-8859-6-E", L"ISO-8859-6-I", L"iso-ir-127", L"ISO_8859-6:1987", L"x-ISO-8859-6S");
	 REGISTER_CODEPAGE(28597, L"ISO-8859-7", L"Windows-28597", L"csISOLatinGreek", L"ECMA-118", L"ELOT_928", L"greek", L"greek8", L"ibm-9005", L"ibm-9005_X110-2007", L"iso-ir-126", L"ISO_8859-7:1987");
	 REGISTER_CODEPAGE(28598, L"ISO-8859-8", L"Windows-28598", L"8859_8", L"csISOLatinHebrew", L"hebrew", L"ibm-5012", L"ibm-5012_P100-1999", L"ISO-8859-8-E", L"iso-ir-138", L"ISO_8859-8:1988");
	 REGISTER_CODEPAGE(28599, L"ISO-8859-9", L"Windows-28599", L"8859_9", L"920", L"cp920", L"csISOLatin5", L"ibm-920", L"ibm-920_P100-1995", L"iso-ir-148", L"ISO_8859-9:1989", L"l5", L"latin5");
	 REGISTER_CODEPAGE(28603, L"ISO-8859-13", L"Windows-28603", L"8859_13", L"ibm-921", L"ibm-921_P100-1995", L"x-IBM921");
	 REGISTER_CODEPAGE(28605, L"ISO-8859-15", L"Windows-28605", L"8859_15", L"923", L"cp923", L"csisolatin0", L"csisolatin9", L"ibm-923", L"ibm-923_P100-1998", L"iso8859_15_fdis", L"l9", L"Latin-9", L"latin0");
	 REGISTER_CODEPAGE(29001, L"x-Europa");
	REGISTER_CODEPAGE(33058, L"ibm-33058", L"ibm-33058_P100-2000");
	 REGISTER_CODEPAGE(38598, L"iso-8859-8-i");
	 REGISTER_CODEPAGE(50220, L"iso-2022-jp");
	 REGISTER_CODEPAGE(50221, L"csISO2022JP");
	 REGISTER_CODEPAGE(50225, L"iso-2022-kr");
	 REGISTER_CODEPAGE(50227, L"x-cp50227");
	 REGISTER_CODEPAGE(50931, L"x-EBCDIC-JapaneseAndUSCanada");
	 REGISTER_CODEPAGE(51932, L"euc-jp", L"Windows-51932", L"csEUCPkdFmtJapanese", L"Extended_UNIX_Code_Packed_Format_for_Japanese", L"ibm-33722_P12A_P12A-2004_U2", L"X-EUC-JP");
	 REGISTER_CODEPAGE(51936, L"EUC-CN");
	 REGISTER_CODEPAGE(51949, L"EUC-KR", L"Windows-51949", L"5601", L"970", L"cp970", L"csEUCKR", L"ibm-970", L"ibm-970_P110_P110-2006_U2", L"ibm-eucKR", L"x-IBM970");
	 REGISTER_CODEPAGE(52936, L"hz-gb-2312");
	 REGISTER_CODEPAGE(54936, L"gb18030", L"Windows-54936", L"ibm-1392");
	 REGISTER_CODEPAGE(57002, L"x-iscii-de", L"Windows-57002", L"ibm-4902", L"x-ISCII91");
	 REGISTER_CODEPAGE(57003, L"x-iscii-be");
	 REGISTER_CODEPAGE(57004, L"x-iscii-ta", L"Windows-57004");
	 REGISTER_CODEPAGE(57005, L"x-iscii-te", L"Windows-57005");
	 REGISTER_CODEPAGE(57006, L"x-iscii-as", L"Windows-57003", L"Windows-57006");
	 REGISTER_CODEPAGE(57007, L"x-iscii-or", L"Windows-57007");
	 REGISTER_CODEPAGE(57008, L"x-iscii-ka", L"Windows-57008");
	 REGISTER_CODEPAGE(57009, L"x-iscii-ma", L"Windows-57009");
	 REGISTER_CODEPAGE(57010, L"x-iscii-gu", L"Windows-57010");
	 REGISTER_CODEPAGE(57011, L"x-iscii-pa", L"Windows-57011");
	// TODO: utf7 needs BOM detection
	//REGISTER_CODEPAGE(65000, L"UTF-7", L"UTF7", L"Windows-65000");
	REGISTER_CODEPAGE(65025, L"ibm-65025");

#undef REGISTER_CODEPAGE
}

static void LowerWstring(const wchar_t* s, wstring& result)
{
	int len = wcslen(s);
	result.resize(len);
	for (int i=0; i<len; ++i)
	{
		if (s[i]>=L'A' && s[i]<='Z')
			result[i] = s[i]-L'A'+L'a';
		else
			result[i] = s[i];
	}
}

void CEncodings::AddEncoding(IEncoding* encoding)
{
	assert(encoding->GetNameCount() >= 1);
	for (int i=0,e=encoding->GetNameCount(); i<e; ++i)
	{
		wstring name;
		LowerWstring(encoding->GetName(i), name);
		assert(m_NameToEncoding.count(name) == 0);
		m_NameToEncoding[name] = encoding;
	}
	m_Encodings.push_back(encoding);
}

IEncoding* CEncodings::FindEncoding(const wchar_t* name) const
{
	wstring lower_name;
	LowerWstring(name, lower_name);
	std::map<wstring,IEncoding*>::const_iterator it = m_NameToEncoding.find(lower_name);
	if (it == m_NameToEncoding.end())
		return NULL;
	return it->second;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------


// Finds out the encoding of the contents of a text file, and also returns a pointer to
// the first byte of the text data skipping the BOM at the beginning of the file if present.
enum ETextFileEncoding
{
	eTFE_UTF8,			// supported, and the xml declaration encoding attribute must also be UTF-8
	eTFE_UTF16_LE,		// supported, nd the xml declaration encoding attribute must also be UTF-16
	eTFE_UTF16_BE,		// not supported by this prog
	eTFE_UTF32_LE,		// not supported by this prog
	eTFE_UTF32_BE,		// not supported by this prog
	eTFE_Unspecified,	// no BOM specified, will search for the xml declaration and its encoding attribute as if the file had single byte character set
};

ETextFileEncoding DetectFileEncoding(const void* data, int data_size, const void*& first_text_byte)
{
	const unsigned char* p = (const unsigned char*)data;
	if (data_size >= 4)
	{
		if (p[0]==0xFF && p[1]==0xFE && p[2]==0 && p[3]==0)
		{
			first_text_byte = p + 4;
			return eTFE_UTF32_LE;
		}
		if (p[0]==0 && p[1]==0 && p[2]==0xFE && p[3]==0xFF)
		{
			first_text_byte = p + 4;
			return eTFE_UTF32_BE;
		}
	}
	if (data_size >= 3)
	{
		if (p[0]==0xEF && p[1]==0xBB && p[2]==0xBF)
		{
			first_text_byte = p + 3;
			return eTFE_UTF8;
		}
	}
	if (data_size >= 2)
	{
		if (p[0]==0xFF && p[1]==0xFE)
		{
			first_text_byte = p + 2;
			return eTFE_UTF16_LE;
		}
		if (p[0]==0xFE && p[1]==0xFF)
		{
			first_text_byte = p + 2;
			return eTFE_UTF16_BE;
		}
	}
	first_text_byte = p;
	return eTFE_Unspecified;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------


template <typename CharType>
bool IsSpace(CharType c)
{
	return c==' ' || c=='\t' || c=='\r' || c=='\n';
}
template <typename CharType>
const CharType* SkipSpaces(const CharType* p, const CharType* p_end)
{
	for (; p<p_end; ++p)
	{
		if (!IsSpace(*p))
			break;
	}
	return p;
}
template <typename CharType>
bool ParseXmlDeclaration(const CharType* text, int num_chars, SXmlDeclarationAttribs& attributes, const CharType*& xml_data)
{
	const CharType* text_end = text + num_chars;
	text = SkipSpaces(text, text_end);
	if (text_end - text < 5)
		return false;
	// parsing "<?xml"
	if (text[0]!='<' || text[1]!='?' || text[2]!='x' || text[3]!='m' || text[4]!='l')
		return false;
	text += 5;

	// parsing attributes
	while (1)
	{
		text = SkipSpaces(text, text_end);
		if (text >= text_end || text[0]=='?')
			break;
		// parsing name
		wstring name;
		for (const CharType* name_begin=text; text<text_end; ++text)
		{
			if (text[0]=='=' || IsSpace(text[0]))
			{
				name.assign(name_begin, text);
				break;
			}
		}
		text = SkipSpaces(text, text_end);
		if (text >= text_end || text[0]!='=')
			return false;
		text = SkipSpaces(text+1, text_end);
		if (text >= text_end || text[0]!='"')
			return false;
		++text;
		wstring value;
		for (const CharType* value_begin=text; text<text_end; ++text)
		{
			if (text[0] == '"')
			{
				value.assign(value_begin, text);
				break;
			}
		}
		if (text >= text_end || text[0]!='"')
			return false;
		++text;
		attributes.SetAttrib(name, value);
	}

	// parsing "?>"
	text = SkipSpaces(text, text_end);
	if (text_end - text < 2)
		return false;
	if (text[0]!='?' || text[1]!='>')
		return false;
	xml_data = (const CharType*)text + 2;
	return true;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------


bool SXmlDeclarationAttribs::GetAttrib(const wstring& name, wstring& value) const
{
	for (const_iterator it=begin(),eit=end(); it!=eit; ++it)
	{
		if (it->name == name)
		{
			value = it->value;
			return true;
		}
	}
	return false;
}
void SXmlDeclarationAttribs::SetAttrib(const wstring& name, const wstring& value)
{
	for (iterator it=begin(),eit=end(); it!=eit; ++it)
	{
		if (it->name == name)
		{
			it->value = value;
			return;
		}
	}
	push_back(SXmlDeclarationAttrib(name, value));
}

wstring SXmlDeclarationAttribs::GetAsXmlDeclaration() const
{
	wstring s = L"<?xml";
	for (const_iterator it=begin(),eit=end(); it!=eit; ++it)
	{
		s.push_back(L' ');
		s.append(it->name);
		s.append(L"=\"");
		s.append(it->value);
		s.push_back(L'"');
	}
	s.append(L"?>");
	return s;
}


CXmlTextCodec::CXmlTextCodec()
: m_Encoding(NULL)
{
}


static IEncoding* const UTF8_ENCODING = CEncodings::GetInstance().FindEncoding(L"UTF-8");
static IEncoding* const UTF16_ENCODING = CEncodings::GetInstance().FindEncoding(L"UTF-16");

bool CXmlTextCodec::DecodeXmlFileData(const void* data, int data_size)
{
	assert(UTF8_ENCODING);
	assert(UTF16_ENCODING);

	m_ErrorMessage.clear();
	m_XmlDeclarationAttributes.clear();

	const void* first_text_byte;
	ETextFileEncoding text_file_encoding = DetectFileEncoding(data, data_size, first_text_byte);
	switch (text_file_encoding)
	{
	case eTFE_UTF8:			// the encoding attribute of the xml declaration must be "UTF-8"
	case eTFE_Unspecified: // Encoding is specified by the encoding attribute of the xml declaration. We assume single byte character set or utf8.
		{
			const char* xml_decl_and_data = (const char*)first_text_byte;
			int num_chars = data_size - (xml_decl_and_data - (const char*)data);
			const char* xml_body;
			if (!ParseXmlDeclaration(xml_decl_and_data, num_chars, m_XmlDeclarationAttributes, xml_body))
				return Error(L"Error parsing xml declaration!");
			wstring encoding_value;
			if (!m_XmlDeclarationAttributes.GetAttrib(L"encoding", encoding_value))
			{
				if (text_file_encoding == eTFE_Unspecified)
					return Error(L"Xml declaration does not specify an encoding attribute!");
				m_Encoding = UTF8_ENCODING;
			}
			else
			{
				m_Encoding = CEncodings::GetInstance().FindEncoding(encoding_value.c_str());
				if (!m_Encoding)
				{
					m_ErrorMessage = L"Xml declaration specifies an unsupported encoding: ";
					m_ErrorMessage.append(encoding_value);
					return false;
				}

				if (text_file_encoding == eTFE_UTF8)
				{
					if (m_Encoding != UTF8_ENCODING)
					{
						m_ErrorMessage = L"File encoding is UTF-8 but the xml declaration encoding attribute is different: ";
						m_ErrorMessage.append(encoding_value);
						return false;
					}
				}
				else
				{
					if (m_Encoding == UTF16_ENCODING)
						return Error(L"Single by character set xml declaration and UTF-16 xml body???");
				}
			}

			int byte_count = (int)(data_size - (xml_body - (const char*)data));
			int res = m_Encoding->BytesToUTF16(xml_body, byte_count, NULL, 0, &m_ErrorMessage);
			if (res < 0)
				return false;
			m_XmlBody.resize(res);
			if (res)
			{
				if (res != m_Encoding->BytesToUTF16(xml_body, byte_count, &m_XmlBody[0], res, &m_ErrorMessage))
					return false;
			}
		}
		return true;
	case eTFE_UTF16_LE: // the encoding attribute of the xml declaration must be "UTF-16"
		{
			const wchar_t* xml_decl_and_data = (const wchar_t*)first_text_byte;
			int size_bytes = data_size - ((char*)first_text_byte - (char*)data);
			int num_chars = size_bytes / 2;
			const wchar_t* xml_body;
			if (!ParseXmlDeclaration(xml_decl_and_data, num_chars, m_XmlDeclarationAttributes, xml_body))
				return Error(L"Error parsing xml declaration!");
			wstring encoding_value;
			if (m_XmlDeclarationAttributes.GetAttrib(L"encoding", encoding_value))
			{
				m_Encoding = CEncodings::GetInstance().FindEncoding(encoding_value.c_str());
				if (!m_Encoding)
				{
					m_ErrorMessage = L"Xml declaration specifies an unsupported encoding: ";
					m_ErrorMessage.append(encoding_value);
					return false;
				}
				if (m_Encoding != UTF16_ENCODING)
				{
					m_ErrorMessage = L"File encoding is UTF-16 little endian but the xml declaration encoding attribute is different: ";
					m_ErrorMessage.append(encoding_value);
					return false;
				}
			}
			else
			{
				m_Encoding = UTF16_ENCODING;
			}

			m_XmlBody.assign(xml_body, xml_decl_and_data + num_chars);
		}
		return true;
	case eTFE_UTF16_BE: return Error(L"UTF16_BE is unsupported!!!");
	case eTFE_UTF32_LE: return Error(L"UTF32_LE is unsupported!!!");
	case eTFE_UTF32_BE: return Error(L"UTF32_BE is unsupported!!!");
	default: assert(0); return Error(L"Unsupported file encoding!!!");
	}
}

bool CXmlTextCodec::EncodeXmlFileData(const SXmlDeclarationAttribs& _attribs, const wstring& xml_body,
				IEncoding* encoding, ENewLineMode newline_mode, std::vector<char>& data, wstring* error_message)
{
	SXmlDeclarationAttribs attribs = _attribs;
	attribs.SetAttrib(L"encoding", encoding->GetName(0));
	wstring xml_data = attribs.GetAsXmlDeclaration();
	xml_data.append(ToString(newline_mode));
	xml_data.append(xml_body);

	if (int bom_size = encoding->GetBOMSizeBytes())
	{
		const char* bom = encoding->GetBOM();
		data.insert(data.end(), bom, bom+bom_size);
	}

	int encoded_size = encoding->UTF16ToBytes(xml_data.data(), xml_data.size(), NULL, 0, error_message);
	if (encoded_size < 0)
		return false;

	size_t offs = data.size();
	data.resize(offs + (size_t)encoded_size);
	return encoded_size == encoding->UTF16ToBytes(xml_data.data(), xml_data.size(), &data[offs], encoded_size, error_message);
}

bool CXmlTextCodec::Error(const wchar_t* error_mesasge)
{
	m_ErrorMessage = error_mesasge;
	return false;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------


static IEncoding* const ASCII_ENCODING = CEncodings::GetInstance().FindEncoding(L"ASCII");
CXmlCharacterReferenceMap::CXmlCharacterReferenceMap()
{
	SetEncoding(ASCII_ENCODING, true);
}

void CXmlCharacterReferenceMap::SetupStaticRefTypes(bool can_represent_all_unichars)
{
	for (int i=0; i<0x20; ++i)
		m_Map[i] = eCRM_RefDecimal;

	ECharRefMode non_char = can_represent_all_unichars ? eCRM_NoRefNonCharacter : eCRM_RefHexNonCharacter;
	for (int i=0xFDD0; i<=0xFDEF; ++i)
		m_Map[i] = non_char;
	m_Map[0xFFFE] = non_char;
	m_Map[0xFFFF] = non_char;
	for (int i=0xDC00; i<0xE000; ++i)
		m_Map[i] = non_char;
	ECharRefMode surrogate = can_represent_all_unichars ? eCRM_NoRefSurrogate : eCRM_RefSurrogate;
	for (int i=0xD800; i<0xDC00; ++i)
		m_Map[i] = surrogate;
}

void CXmlCharacterReferenceMap::SetEncoding(IEncoding* encoding, bool safe_encoding)
{
	assert(ASCII_ENCODING && UTF8_ENCODING && UTF16_ENCODING);
	if (encoding->GetFlags() & IEncoding::eF_CanRepresentAllUniChars)
	{
		// UTF encodings
		memset(&m_Map, eCRM_NoRef, sizeof(m_Map));
		SetupStaticRefTypes(true);
		return;
	}

	memset(&m_Map[0x20], eCRM_NoRef, 0x80-0x20);
	memset(&m_Map[0x80], eCRM_RefHex, sizeof(m_Map)-0x80);
	if (safe_encoding)
	{
		SetupStaticRefTypes(false);
		return;
	}

	for (int i=0x80; i<0x100; ++i)
	{
		char b = (char)i;
		wchar_t w;
		int res = encoding->BytesToUTF16(&b, 1, &w, 1, NULL);
		if (res > 0)
			m_Map[w] = eCRM_NoRef;
	}
	SetupStaticRefTypes(false);
}
