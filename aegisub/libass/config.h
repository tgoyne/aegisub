#ifdef _MSC_VER
#define CONFIG_FONTCONFIG 1
#define CONFIG_ICONV 1

#define inline __inline

#define strtoll(p, e, b) _strtoi64(p, e, b)

#define M_PI 3.1415926535897932384626433832795

#pragma warning(disable : 4996)
#endif
