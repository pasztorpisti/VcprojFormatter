## Introduction

There is nothing wrong with .vcproj files until you start working in a team and perchance use a source control system with .vcprojs in it. These project files are pure XML files with elements and attributes only, but every time you save them in Visual Studio, there is a chance that some neighboring XML elements are shuffled, making it very difficult to diff your changes against the original file in the repository, and the same is true for merging before submitting/committing your changes.

## A deeper explanation of the problem

The format of .vcproj files is public; you can take a look at its XSD [here](http://msdn.microsoft.com/en-us/library/y4sy8216%28VS.90%29.aspx "XSD for Visual C++ .vcproj files."). There are many sources of the problem I described previously:

1.  According to the XML specification, the attributes of elements are unordered.
2.  The _.vcproj_ XSD uses `<xs:choice/>` declarations that allow a sequence of different sibling elements to appear in random order inside the _.vcproj_.
3.  If an element type (e.g.: `<Configuration/>` nodes in the _.vcproj_) can have more than one instance in a sequence, then Visual Studio may randomly shuffle them when you save the _.vcproj_.

My commandline tool can rearrange the _.vcproj_ XML file according to a much more strict set of rules than those of the Microsoft XSD for _.vcproj_ files. The result is that Visual Studio can still load the _.vcproj_, but XML attributes and elements are much more strictly ordered.

## The commandline parameters of the tool

This is a simple commandline tool and not an add-in because I haven't found the right place to intercept a _.vcproj_ file saving in the add-in SDK. I'm not even sure if it's possible to intercept the _.vcproj_ saving event. (You may give me hints about this to improve this program.) Here are the commandline parameters:

<pre lang="text">Usage: VcprojFormatter.exe [options...] [-] vcproj1 [, vcproj2 [, ...]]
You can use wildcards to specify vcproj files. (Eg.: c:\code\*.vcproj)
The files must have .vcproj extension!

OPTIONS:
-DECIMAL_POINT:[.|,]  Specify a decimal point character that will be used in
                      the value of the version attribute of your vcproj. It
                      can be '.' or ',' depending on your locale settings.
                      This prog keeps the original decimal point in the version
                      number by default but you can force it to be always
                      '.' or ',' using this commandline parameter.
-NEWLINE:newline_mode Specifies the newline format of the output file.
                      Possible newline modes: CR, CRLF, LF, LFCR, AUTO
                      The default is AUTO which keeps the original newlines.
-ENCODING:encoding    Sets the encoding of the output file.
                      The default is AUTO which keeps the original encoding.
-SAFE_ENCODING        Used only with non-UTF encodings. This forces the
                      character codes above 0x7F to be represented as character
                      references (like codes below 0x20) in the xml values.
                      You may need this because the Windows API allows mapping
                      of some characters above 0x7F to UTF-16 even if the
                      specified character is not defined in the codepage. For
                      example: char 0x81 on codepage Windows-1252\. However
                      these non-existing characters should not normally occur
                      in a file with a codepage that doesn't define them.
-LIST_ENCODINGS       Show the list of supported encodings.</pre>

The tool can control the binary representation of newlines and the encoding of the XML. The newlines can be other than CRLF; for example, if your use perforce source control and check out text files to Linux with newlines used on the actual platform. (Actually, I had to use the tool on Linux from script, and ran it using _wine_.) The encoding is another thing that can make your life harder if you have a team of coders of different nationalities with localized Windows. With this tool, you can also control the encoding of the _.vcproj_ file that is uploaded to perforce. Another annoying thing that is locale dependent is the decimal point of the Visual Studio version number in the _.vcproj_ file; it can be either a dot or comma. You can ask the formatter tool to use only one of these.

## Using the tool

1.  Upload a formatted version of the _.vcproj_ file to your source control system.
2.  When a coder modifies and saves a _.vcproj_ file, he/she must run the vcproj formatter tool on it before diffing/merging it with the one in the repository. The perforce source control client has a "Tools" menu where you can easily add this vcproj formatter tool as a file right-click menu.

After formatting the vcproj, Visual Studio usually finds out that the _.vcproj_ file has been modified and prompts you whether to reload the new version or to ignore it. You can safely choose the ignore option.

The tool saves the _.vcproj_ file in a format that is very easy to diff/merge with a simple text-based tool. Another thing that I personally dislike about Visual Studio saved _.vcproj_ files is that elements with only one attribute are still wrapped into three lines. Saving these elements to a single line makes your _.vcproj_ more brief and easy-to-merge.

Here is a sample non-formatted _.vcproj_ file:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<VisualStudioProject
    ProjectType="Visual C++"
    Version="9,00"
    Name="VcprojFormatter"
    ProjectGUID="{A1857D12-A8DB-4615-AD0D-BE2BB3519057}"
    RootNamespace="VcprojFormatter"
    Keyword="Win32Proj"
    TargetFrameworkVersion="196613"
    >
    <Platforms>
        <Platform
            Name="Win32"
        />
    </Platforms>
    <ToolFiles>
    </ToolFiles>
    <Configurations>
        <Configuration
            Name="Debug|Win32"
            OutputDirectory="$(SolutionDir)$(ConfigurationName)"
            IntermediateDirectory="$(ConfigurationName)"
            ConfigurationType="1"
            CharacterSet="1"
            >
            <Tool
                Name="VCPreBuildEventTool"
            />
            <Tool
                Name="VCCustomBuildTool"
            />
            <Tool
                Name="VCXMLDataGeneratorTool"
            />
            <Tool
                Name="VCWebServiceProxyGeneratorTool"
            />
            <Tool
                Name="VCMIDLTool"
            />
            <Tool
                Name="VCCLCompilerTool"
                Optimization="0"
                PreprocessorDefinitions="WIN32;_DEBUG;_CONSOLE"
                MinimalRebuild="true"
                BasicRuntimeChecks="3"
                RuntimeLibrary="3"
                UsePrecompiledHeader="2"
                WarningLevel="3"
                DebugInformationFormat="4"
            />
            <Tool
                Name="VCManagedResourceCompilerTool"
            />
            <Tool
                Name="VCResourceCompilerTool"
            />
            <Tool
                Name="VCPreLinkEventTool"
            />
            <Tool
                Name="VCLinkerTool"
                LinkIncremental="2"
                GenerateDebugInformation="true"
                SubSystem="1"
                TargetMachine="1"
            />
            <Tool
                Name="VCALinkTool"
            />
            <Tool
                Name="VCManifestTool"
            />
            <Tool
                Name="VCXDCMakeTool"
            />
            <Tool
                Name="VCBscMakeTool"
            />
            <Tool
                Name="VCFxCopTool"
            />
            <Tool
                Name="VCAppVerifierTool"
            />
            <Tool
                Name="VCPostBuildEventTool"
            />
        </Configuration>
        <Configuration
            Name="Release|Win32"
            OutputDirectory="$(SolutionDir)$(ConfigurationName)"
            IntermediateDirectory="$(ConfigurationName)"
            ConfigurationType="1"
            CharacterSet="1"
            WholeProgramOptimization="1"
            >
            <Tool
                Name="VCPreBuildEventTool"
            />
            <Tool
                Name="VCCustomBuildTool"
            />
            <Tool
                Name="VCXMLDataGeneratorTool"
            />
            <Tool
                Name="VCWebServiceProxyGeneratorTool"
            />
            <Tool
                Name="VCMIDLTool"
            />
            <Tool
                Name="VCCLCompilerTool"
                Optimization="3"
                EnableIntrinsicFunctions="true"
                FavorSizeOrSpeed="1"
                PreprocessorDefinitions="WIN32;NDEBUG;_CONSOLE"
                StringPooling="true"
                ExceptionHandling="0"
                RuntimeLibrary="0"
                BufferSecurityCheck="false"
                EnableFunctionLevelLinking="true"
                UsePrecompiledHeader="2"
                WarningLevel="3"
                DebugInformationFormat="3"
                CallingConvention="1"
            />
            <Tool
                Name="VCManagedResourceCompilerTool"
            />
            <Tool
                Name="VCResourceCompilerTool"
            />
            <Tool
                Name="VCPreLinkEventTool"
            />
            <Tool
                Name="VCLinkerTool"
                LinkIncremental="1"
                GenerateDebugInformation="true"
                SubSystem="1"
                OptimizeReferences="2"
                EnableCOMDATFolding="2"
                TargetMachine="1"
            />
            <Tool
                Name="VCALinkTool"
            />
            <Tool
                Name="VCManifestTool"
            />
            <Tool
                Name="VCXDCMakeTool"
            />
            <Tool
                Name="VCBscMakeTool"
            />
            <Tool
                Name="VCFxCopTool"
            />
            <Tool
                Name="VCAppVerifierTool"
            />
            <Tool
                Name="VCPostBuildEventTool"
            />
        </Configuration>
    </Configurations>
    <References>
    </References>
    <Files>
        <File
            RelativePath=".\smart.h"
            >
        </File>
        <File
            RelativePath=".\stdafx.cpp"
            >
            <FileConfiguration
                Name="Debug|Win32"
                >
                <Tool
                    Name="VCCLCompilerTool"
                    UsePrecompiledHeader="1"
                />
            </FileConfiguration>
            <FileConfiguration
                Name="Release|Win32"
                >
                <Tool
                    Name="VCCLCompilerTool"
                    UsePrecompiledHeader="1"
                />
            </FileConfiguration>
        </File>
        <File
            RelativePath=".\stdafx.h"
            >
        </File>
        <File
            RelativePath=".\Vcproj.cpp"
            >
        </File>
        <File
            RelativePath=".\Vcproj.h"
            >
        </File>
        <File
            RelativePath=".\VcprojFormatter.cpp"
            >
        </File>
        <File
            RelativePath=".\VcprojParser.cpp"
            >
        </File>
        <File
            RelativePath=".\VcprojParser.h"
            >
        </File>
        <File
            RelativePath=".\XmlEncoding.cpp"
            >
        </File>
        <File
            RelativePath=".\XmlEncoding.h"
            >
        </File>
    </Files>
    <Globals>
    </Globals>
</VisualStudioProject>
```

The same _.vcproj_ after formatting:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<VisualStudioProject
    Name="VcprojFormatter"
    Keyword="Win32Proj"
    ProjectGUID="{A1857D12-A8DB-4615-AD0D-BE2BB3519057}"
    ProjectType="Visual C++"
    RootNamespace="VcprojFormatter"
    TargetFrameworkVersion="196613"
    Version="9,00"
    >
    <Platforms>
        <Platform Name="Win32"/>
    </Platforms>
    <ToolFiles/>
    <Configurations>
        <Configuration
            Name="Debug|Win32"
            CharacterSet="1"
            ConfigurationType="1"
            IntermediateDirectory="$(ConfigurationName)"
            OutputDirectory="$(SolutionDir)$(ConfigurationName)"
            >
            <Tool Name="VCALinkTool"/>
            <Tool Name="VCAppVerifierTool"/>
            <Tool Name="VCBscMakeTool"/>
            <Tool
                Name="VCCLCompilerTool"
                BasicRuntimeChecks="3"
                DebugInformationFormat="4"
                MinimalRebuild="true"
                Optimization="0"
                PreprocessorDefinitions="WIN32;_DEBUG;_CONSOLE"
                RuntimeLibrary="3"
                UsePrecompiledHeader="2"
                WarningLevel="3"
                />
            <Tool Name="VCCustomBuildTool"/>
            <Tool Name="VCFxCopTool"/>
            <Tool
                Name="VCLinkerTool"
                GenerateDebugInformation="true"
                LinkIncremental="2"
                SubSystem="1"
                TargetMachine="1"
                />
            <Tool Name="VCMIDLTool"/>
            <Tool Name="VCManagedResourceCompilerTool"/>
            <Tool Name="VCManifestTool"/>
            <Tool Name="VCPostBuildEventTool"/>
            <Tool Name="VCPreBuildEventTool"/>
            <Tool Name="VCPreLinkEventTool"/>
            <Tool Name="VCResourceCompilerTool"/>
            <Tool Name="VCWebServiceProxyGeneratorTool"/>
            <Tool Name="VCXDCMakeTool"/>
            <Tool Name="VCXMLDataGeneratorTool"/>
        </Configuration>
        <Configuration
            Name="Release|Win32"
            CharacterSet="1"
            ConfigurationType="1"
            IntermediateDirectory="$(ConfigurationName)"
            OutputDirectory="$(SolutionDir)$(ConfigurationName)"
            WholeProgramOptimization="1"
            >
            <Tool Name="VCALinkTool"/>
            <Tool Name="VCAppVerifierTool"/>
            <Tool Name="VCBscMakeTool"/>
            <Tool
                Name="VCCLCompilerTool"
                BufferSecurityCheck="false"
                CallingConvention="1"
                DebugInformationFormat="3"
                EnableFunctionLevelLinking="true"
                EnableIntrinsicFunctions="true"
                ExceptionHandling="0"
                FavorSizeOrSpeed="1"
                Optimization="3"
                PreprocessorDefinitions="WIN32;NDEBUG;_CONSOLE"
                RuntimeLibrary="0"
                StringPooling="true"
                UsePrecompiledHeader="2"
                WarningLevel="3"
                />
            <Tool Name="VCCustomBuildTool"/>
            <Tool Name="VCFxCopTool"/>
            <Tool
                Name="VCLinkerTool"
                EnableCOMDATFolding="2"
                GenerateDebugInformation="true"
                LinkIncremental="1"
                OptimizeReferences="2"
                SubSystem="1"
                TargetMachine="1"
                />
            <Tool Name="VCMIDLTool"/>
            <Tool Name="VCManagedResourceCompilerTool"/>
            <Tool Name="VCManifestTool"/>
            <Tool Name="VCPostBuildEventTool"/>
            <Tool Name="VCPreBuildEventTool"/>
            <Tool Name="VCPreLinkEventTool"/>
            <Tool Name="VCResourceCompilerTool"/>
            <Tool Name="VCWebServiceProxyGeneratorTool"/>
            <Tool Name="VCXDCMakeTool"/>
            <Tool Name="VCXMLDataGeneratorTool"/>
        </Configuration>
    </Configurations>
    <References/>
    <Files>
        <File RelativePath=".\Vcproj.cpp"/>
        <File RelativePath=".\Vcproj.h"/>
        <File RelativePath=".\VcprojFormatter.cpp"/>
        <File RelativePath=".\VcprojParser.cpp"/>
        <File RelativePath=".\VcprojParser.h"/>
        <File RelativePath=".\XmlEncoding.cpp"/>
        <File RelativePath=".\XmlEncoding.h"/>
        <File RelativePath=".\smart.h"/>
        <File RelativePath=".\stdafx.cpp">
            <FileConfiguration Name="Debug|Win32">
                <Tool
                    Name="VCCLCompilerTool"
                    UsePrecompiledHeader="1"
                    />
            </FileConfiguration>
            <FileConfiguration Name="Release|Win32">
                <Tool
                    Name="VCCLCompilerTool"
                    UsePrecompiledHeader="1"
                    />
            </FileConfiguration>
        </File>
        <File RelativePath=".\stdafx.h"/>
    </Files>
    <Globals/>
</VisualStudioProject>
```

## Known issues

*   The Windows `WideCharToMultiByte()` and `MultiByteToWideChar()` functions are used to encode/decode text, and sometimes they do not work as intended, and this depends on your Windows version too. They may encode/decode invalid characters. For example, on my WinXP, the codepage 37 encoding can make garbage from a _.vcproj_ file. Let's try the encoding of your choice before sticking to it!

## History

*   4<sup>th</sup> December, 2010: Initial version.
*   9<sup>th</sup> December, 2010: Better support for encodings.
