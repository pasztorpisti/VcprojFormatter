#include "stdafx.h"
#include "XmlEncoding.h"
#include "Vcproj.h"


bool g_SafeEncoding = false;
wchar_t g_DecimalPoint = 0;
ENewLineMode g_NewLineMode = eNLM_Auto;
// NULL means AUTO encoding
IEncoding* g_XmlEncoding = NULL;


bool ProcessError(const wchar_t* fmtstr, ...)
{
	va_list args;
	va_start(args, fmtstr);
	Log(fmtstr, args);
	va_end(args);
	return false;
}


static const wchar_t VCPROJ_EXT[] = L".vcproj";

// Returns false on error.
bool ProcessFile(const wchar_t* filepath)
{
	int ext_len = wcslen(VCPROJ_EXT);
	int len = wcslen(filepath);
	if (len<=ext_len || _wcsicmp(filepath+(len-ext_len), VCPROJ_EXT))
		return ProcessError(L"%s: skipping, because the file extension is not \"%s\"!", filepath, VCPROJ_EXT);

	LogNoNewline(L"Formatting %s... ", filepath);

	// skipping readonly files
	DWORD file_attrib = GetFileAttributes(filepath);
	if (file_attrib == INVALID_FILE_ATTRIBUTES)
		return ProcessError(L"%s", LastErrorToString(GetLastError()).c_str());
	if (file_attrib & FILE_ATTRIBUTE_READONLY)
		return ProcessError(L"Skipping readonly file!");

	CVcprojFile vcproj_file;
	if (!vcproj_file.LoadVcprojFile(filepath, g_NewLineMode))
		return ProcessError(L"Error loading file! %s", vcproj_file.GetErrorMessage().c_str());

	vcproj_file.GetRoot()->SortSubTree();
	if (g_DecimalPoint)
		vcproj_file.SetDecimalPoint(g_DecimalPoint);

	vcproj_file.SetNewLineMode(g_NewLineMode);
	vcproj_file.SetEncoding(g_XmlEncoding);

	wstring temp_path = filepath;
	temp_path += L"_$temp$";
	if (!vcproj_file.SaveVcprojFile(temp_path.c_str(), file_attrib, g_SafeEncoding))
		return ProcessError(L"Error saving temp file: %s %s", temp_path.c_str(), vcproj_file.GetErrorMessage().c_str());

	if (!DeleteFile(filepath))
	{
		DeleteFile(temp_path.c_str());
		return ProcessError(L"Error deleting file! %s", LastErrorToString(GetLastError()).c_str());
	}

	if (!MoveFile(temp_path.c_str(), filepath))
		return ProcessError(L"Error moving \"%s\" to \"%s\"! %s", temp_path.c_str(), filepath, LastErrorToString(GetLastError()).c_str());

	Log(L"OK");
	return true;
}

// Returns the number of errors.
int ProcessFilePattern(const wchar_t* pattern)
{
	wstring dir = DirName(pattern);
	WIN32_FIND_DATA find_data;
	SFindHandle hFind = FindFirstFile(pattern, &find_data);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		WinError(L"%s", pattern);
		return 1;
	}

	int error_count = 0;
	do 
	{
		wstring path = dir + find_data.cFileName;
		if (!ProcessFile(path.c_str()))
			++error_count;
	}
	while (FindNextFile(hFind, &find_data));

	DWORD last_error = GetLastError();
	hFind.Close();
	if (last_error != ERROR_NO_MORE_FILES)
	{
		++error_count;
		Error(L"%s: %s", pattern, LastErrorToString(last_error).c_str());
	}

	return error_count;
}

void PrintHelp()
{
	wchar_t module_filename[MAX_PATH];
	const wchar_t* fname = L"VcprojFormatter.exe";
	if (GetModuleFileName(NULL, module_filename, sizeof(module_filename)/sizeof(module_filename[0])))
	{
		const wchar_t* f = max(wcsrchr(module_filename, L'\\'), wcsrchr(module_filename, L'/'));
		if (f)
			fname = f+1;
	}

	wstring newline_modes;
	for (int i=0; i<eNLM_Last; ++i)
	{
		if (!newline_modes.empty())
			newline_modes.append(L", ");
		newline_modes.append(GetName((ENewLineMode)i));
	}

	Log(
		L"Usage: %s [options...] [-] vcproj1 [, vcproj2 [, ...]]\n"
		L"You can use wildcards to specify vcproj files. (Eg.: c:\\code\\*.vcproj)\n"
		L"The files must have .vcproj extension!\n"
		L"\n"
		L"OPTIONS:\n"
		L"-DECIMAL_POINT:[.|,]  Specify a decimal point character that will be used in\n"
		L"                      the value of the version attribute of your vcproj. It\n"
		L"                      can be '.' or ',' depending on your locale settings.\n"
		L"                      This prog keeps the original decimal point in the version\n"
		L"                      number by default but you can force it to be always\n"
		L"                      '.' or ',' using this commandline parameter.\n"
		L"-NEWLINE:newline_mode Specifies the newline format of the output file.\n"
		L"                      Possible newline modes: %s\n"
		L"                      The default is AUTO which keeps the original newlines.\n"
		L"-ENCODING:encoding    Sets the encoding of the output file.\n"
		L"                      The default is AUTO which keeps the original encoding.\n"
		L"-SAFE_ENCODING        Used only with non-UTF encodings. This forces the\n"
		L"                      character codes above 0x7F to be represented as character\n"
		L"                      references (like codes below 0x20) in the xml values.\n"
		L"                      You may need this because the Windows API allows mapping\n"
		L"                      of some characters above 0x7F to UTF-16 even if the\n"
		L"                      specified character is not defined in the codepage. For\n"
		L"                      example: char 0x81 on codepage Windows-1252. However\n"
		L"                      these non-existing characters should not normally occur\n"
		L"                      in a file with a codepage that doesn't define them.\n"
		L"-LIST_ENCODINGS       Show the list of supported encodings.\n"
		, fname, newline_modes.c_str()
		);
}

void LogEncoding(IEncoding* encoding)
{
	wstring s = L" - ";
	s.append(encoding->GetName(0));
	int name_count = encoding->GetNameCount();
	if (name_count > 1)
	{
		s.append(L" (");
		wstring::size_type base_len = s.size();
		bool need_comma = false;
		for (int i=1; i<name_count; ++i)
		{
			const wchar_t* name = encoding->GetName(i);
			int name_len = wcslen(name);
			if (s.size() + (need_comma?2:0) + name_len > 79)
			{
				need_comma = false;
				s.push_back(L',');
				Log(L"%s", s.c_str());
				s = wstring(base_len, L' ');
			}
			if (!need_comma)
				need_comma = true;
			else
				s.append(L", ");
			s.append(name);
		}
		s.append(L")");
	}
	Log(L"%s", s.c_str());
}

void ListEncodings()
{
	const std::vector<IEncoding*>& encodings = CEncodings::GetInstance().GetAllEncodings();
	// encodings that are supported by the windows
	std::vector<IEncoding*> avail;
	// encodings that aren't supported by the windows
	std::vector<IEncoding*> unavail;

	for (std::vector<IEncoding*>::const_iterator it=encodings.begin(),eit=encodings.end(); it!=eit; ++it)
	{
		IEncoding* e = *it;
		if (e->IsAvailable())
			avail.push_back(e);
		else
			unavail.push_back(e);
	}

	Log(L"Available encodings:");
	for (std::vector<IEncoding*>::const_iterator it=avail.begin(),eit=avail.end(); it!=eit; ++it)
		LogEncoding(*it);
	Log(L"");
	Log(L"Encodings supported by this program but are unsupported by your windows:");
	for (std::vector<IEncoding*>::const_iterator it=unavail.begin(),eit=unavail.end(); it!=eit; ++it)
		LogEncoding(*it);
}

int __cdecl wmain(int argc, wchar_t* argv[])
{
	int argi;
	for (argi=1; argi<argc; ++argi)
	{
		const wchar_t* p = argv[argi];
		if (p[0] != L'-')
			break;
		if (p[1] == 0)
		{
			++argi;
			break;
		}

		static const wchar_t PARAM_SAFE_ENCODING[] = L"SAFE_ENCODING";
		static const wchar_t PARAM_DECIMAL_POINT[] = L"DECIMAL_POINT:";
		static const wchar_t PARAM_ENCODING[] = L"ENCODING:";
		static const wchar_t PARAM_NEWLINE[] = L"NEWLINE:";
		static const wchar_t PARAM_LIST_ENCODINGS[] = L"LIST_ENCODINGS";

		if (0 == _wcsicmp(p+1, PARAM_SAFE_ENCODING))
		{
			g_SafeEncoding = true;
		}
		else if (0 == _wcsnicmp(p+1, PARAM_DECIMAL_POINT, wcslen(PARAM_DECIMAL_POINT)))
		{
			const wchar_t* dp = p + 1 + wcslen(PARAM_DECIMAL_POINT);
			if (dp[1] || (dp[0]!=L'.' && dp[0]!=L','))
			{
				Error(L"Invalid decimal point: '%s' It must be either '.' or ','!!!", dp);
				return 1;
			}
			g_DecimalPoint = dp[0];
		}
		else if (0 == _wcsnicmp(p+1, PARAM_ENCODING, wcslen(PARAM_ENCODING)))
		{
			const wchar_t* pencoding = p + 1 + wcslen(PARAM_ENCODING);
			if (0 == _wcsicmp(pencoding, L"AUTO"))
			{
				g_XmlEncoding = NULL;
			}
			else
			{
				g_XmlEncoding = CEncodings::GetInstance().FindEncoding(pencoding);
				if (!g_XmlEncoding)
				{
					Error(L"Encoding \"%s\" isn't supported by this program.", pencoding);
					return 1;
				}
				if (!g_XmlEncoding->IsAvailable())
				{
					Error(L"Encoding \"%s\" isn't supported by your Windows.", pencoding);
					return 1;
				}
			}
		}
		else if (0 == _wcsnicmp(p+1, PARAM_NEWLINE, wcslen(PARAM_NEWLINE)))
		{
			const wchar_t* pnewline = p + 1 + wcslen(PARAM_NEWLINE);
			g_NewLineMode = NewLineModeFromName(pnewline);
			if (g_NewLineMode == eNLM_Last)
			{
				Error(L"Invalid newline mode: %s", pnewline);
				return 1;
			}
		}
		else if (0 == _wcsicmp(p+1, PARAM_LIST_ENCODINGS))
		{
			ListEncodings();
			return 0;
		}
		else
		{
			Error(L"Unknown parameter: %s", p);
			return 1;
		}
	}

	if (argi >= argc)
	{
		PrintHelp();
		return 1;
	}

	int error_count = 0;
	for (; argi<argc; ++argi)
		error_count += ProcessFilePattern(argv[argi]);

	if (error_count)
		Error(L"Number of errors: %d", error_count);

	return error_count ? 1 : 0;
}
