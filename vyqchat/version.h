/*
 * This file is generated by 'make version.h'.
 * Any changes you make here will be lost, please
 * modify the Makefile instead and run 'make version'.
 */

#ifndef __VERSION_H
#define __VERSION_H

#define __MAJOR_VERSION   0
#define __MINOR_VERSION   9
#define __RELEASE_NUM     3
#define __BUILD_NUM       1

#define VERSION PLUGIN_MAKE_VERSION(__MAJOR_VERSION, __MINOR_VERSION, __RELEASE_NUM, __BUILD_NUM)

#define __PLUGINVERSION_STRING      __MAJOR_VERSION,__MINOR_VERSION,__RELEASE_NUM,__BUILD_NUM
#define __PLUGINVERSION_STRING_DOTS	__MAJOR_VERSION.__MINOR_VERSION.__RELEASE_NUM.__BUILD_NUM
#define __STRINGIFY_(x) #x
#define __STRINGIFY(x) __STRINGIFY_(x)
#define __VERSION_STRING            __STRINGIFY(__PLUGINVERSION_STRING_DOTS)

#define __DESC                  "This plugin lets you control Miranda from the command line."
#define __AUTHOR                "Saulius Menkevicius"
#define __AUTHOREMAIL           "bob@nulis.lt"
#define __COPYRIGHT             "� 2005 Saulius Menkevicius"
#define __AUTHORWEB             "http://www.miranda-im.org/"

#define __PLUGIN_DISPLAY_NAME   "vypresschat"

#endif /* #ifndef __VERSION_H */
