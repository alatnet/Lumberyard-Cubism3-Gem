#include "StdAfx.h"

//Giant source file for the cubism3 framework.
//Using this so that I dont have to edit the source code to have it compile.

WX_DISABLE_(4101)
WX_DISABLE_(4244)
WX_DISABLE_(4700)
#include "Framework\Animation.c"
#include "Framework\AnimationSegmentEvaluationFunction.c"
#include "Framework\AnimationState.c"
#include "Framework\FloatBlendFunction.c"
#include "Framework\Json.c"
#include "Framework\Local.h"
#include "Framework\ModelExtensions.c"
#include "Framework\MotionJson.c"

//causes compile errors...
#ifdef CUBISM3_ENABLE_PHYSICS
#include "Framework\Physics.c"
#include "Framework\PhysicsJson.c"
#include "Framework\PhysicsMath.c"
#endif

#include "Framework\String.c"
WX_ENABLE_(4700)
WX_ENABLE_(4244)
WX_ENABLE_(4101)