
#pragma once

#include <platform.h> // Many CryCommon files require that this is included first.

#include <algorithm>
#include <Cry_Math.h>
#include <ISystem.h>


//because /WX is fucking with us...
//quick and easy way to disable and enable a warning
#ifdef WX_DISABLE
	#ifdef _MSC_VER
		#define WX_DISABLE_(x) __pragma(warning(disable : x));
		#define WX_ENABLE_(x) __pragma(warning(default : x));
	#else
		#define WX_DISABLE_(x)
		#define WX_ENABLE_(x)
	#endif
#else
	#define WX_DISABLE_(x)
	#define WX_ENABLE_(x)
#endif