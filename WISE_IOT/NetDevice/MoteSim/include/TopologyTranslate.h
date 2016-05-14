#ifndef __TOPOLOGY_TRANSLATE_API_H__
#define __TOPOLOGY_TRANSLATE_API_H__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once

#ifndef SNCALL
#define SNCALL __stdcall
#endif

#ifndef EXPORT
#define EXPORT
#endif

#else
#ifndef SNCALL
#define SNCALL
#endif

#ifndef EXPORT
#define EXPORT
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct 
{
	char *list;
	int listlen;
	char *mesh;
	int meshlen;
}Topology;

EXPORT void SNCALL SN_Translate(char *iface, Topology *mac, Topology *index);

#ifdef __cplusplus
}
#endif

#endif // __TOPOLOGY_TRANSLATE_API_H__