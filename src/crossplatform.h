#ifndef MKXP_CROSSPLATFORM_H
#define MKXP_CROSSPLATFORM_H

#ifdef _MSC_VER
//#define _CRT_SECURE_NO_DEPRECATE //should be defined in binding-mri's preprocessor for vsnprintf, fopen

#include <direct.h>

#ifndef snprintf
#define snprintf _snprintf
#endif
#ifndef chdir
#define chdir _chdir
#endif
#endif

#endif //MKXP_CROSSPLATFORM_H
