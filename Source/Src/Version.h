#define Version_Major       2
#define Version_Minor       05      // Two Digits
#define Version_Beta        01      // Two Digits (00 = Not beta)
#define MyPlugName          "TWriteAVI\0"

#define MyVersion_Copyright   "(c) 2005, tritical, squid_80 and mikeytown2\0"
#define MyVersion_Implementor "(c) 2018, Stephen Jones AKA StainlessS\0"

// #x  Encloses the argument x in quotes.
// #@x Encloses the argument x in single quotes.
// ##  Concatenates tokens used as arguments to form other tokens.
#define _STR(x) #x
#define STR(x) _STR(x)

#define MyVersion_Number        Version_Major,Version_Minor,Version_Beta,0

#if(Version_Beta > 0)
    #define MyVersion_String    STR(Version_Major) "." STR(Version_Minor) ".Beta" STR(Version_Beta)
#else
    #define MyVersion_String    STR(Version_Major) "." STR(Version_Minor) "." STR(Version_Beta) "." STR(0)
#endif

// Below MUST BE set for avs v2.58 and avs+ x64, Configuration Properties/Resources/General/PreProcessor Definitions
// Avs  v2.58, Add definition 'AVISYNTH_PLUGIN_25' 
// Avs+ v2.60 x64, Add definition '_WIN64' 
#ifdef AVISYNTH_PLUGIN_25
    #define MyComments "Windows XP Rules OK\0"
    #define MyOriginalDllName MyPlugName "_25.dll\0"
    #define MyDescription  "Avisynth v2.58 32 bit CPP Plugin\0"
#else
    #ifdef _WIN64
        #define MyComments "Windows XP Rules OK\0"
        #define MyOriginalDllName MyPlugName "_x64.dll\0"
        #define MyDescription "Avisynth+ v2.60 64 bit CPP Plugin\0"
    #else
        #define MyComments "Windows XP Rules OK\0"
        #define MyOriginalDllName MyPlugName "_x86.dll\0"
        #define MyDescription "Avisynth+ v2.60 32 bit CPP Plugin\0"
    #endif
#endif
