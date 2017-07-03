/*
 * Copyright(c) Live2D Inc. All rights reserved.
 * 
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at http://live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */


#ifndef LIVE2D_CUBISM_FRAMEWORK_H
#define LIVE2D_CUBISM_FRAMEWORK_H


// -------- //
// REQUIRES //
// -------- //

// Cubism model.
typedef struct csmModel csmModel;


// ----- //
// TYPES //
// ----- //


/// Opaque Cubism float sink.
typedef struct csmFloatSink csmFloatSink;

/// Model float type.
typedef enum csmModelFloatType
{
  /// Model opacity float.
  csmModelOpacity,


  /// Number of model float types.
  // (Make sure this value always is the last value of the enumeration).
  csmModelFloatTypeCount
}
csmModelFloatType;

/// Handles floats targeting model.
///
/// @param  sender    Event source.
/// @param  type      Model float type.
/// @param  value     Float value.
/// @param  userData  [Optional] User data.
typedef void csmModelFloatHandler(const csmFloatSink* sender, const csmModelFloatType type, const float value, void* userData);

/// Float blend function.
///
/// @param  base    Current value.
/// @param  value   Value to blend in.
/// @param  weight  Blend weight to use.
///
/// @return Blend result.
typedef float (*csmFloatBlendFunction)(float base, float value, float weight);

/// Builtin override float blend function.
float csmOverrideFloatBlendFunction(float base, float value, float weight);

/// Builtin additive float blend function.
float csmAdditiveFloatBlendFunction(float base, float value, float weight);


/// Opaque Cubism animation.
typedef struct csmAnimation csmAnimation;

/// Play state of an animation.
typedef struct csmAnimationState
{
  float Time;
}
csmAnimationState;


// ---------------- //
// MODEL EXTENSIONS //
// ---------------- //

/// Queries whether a model uses clipping masks.
///
/// @param  model  Model to query.
///
/// @return  Non-zero if model uses clipping masks; '0'otherwise.
int csmDoesModelUseMasks(const csmModel* model);


// -------------------- //
// PARAMETER EXTENSIONS //
// -------------------- //

/// Finds a parameter by its ID.
///
/// @param  model  Model to query.
/// @param  id     ID to look for.
///
/// @return  Valid index if ID found; '-1' otherwise.
int csmGetIndexofParameter(const csmModel* model, const char* id);


// --------------- //
// PART EXTENSIONS //
// --------------- //

/// Finds a part by its ID.
///
/// @param  model  Model to query.
/// @param  id     ID to look for.
///
/// @return  Valid index if ID found; '-1' otherwise.
int csmGetIndexofPart(const csmModel* model, const char* id);


// ---------- //
// FLOAT SINK //
// ---------- //

/// Gets the necessary size for an float sink in bytes.
///
/// @param  model  Model to create sink for.
///
/// @return  Number of bytes necessary.
unsigned int csmGetSizeofFloatSink(const csmModel* model);


/// Initializes a float sink.
///
/// @param  model    Model to initialize sink for.
/// @param  address  Address to place sink at.
/// @param  size     Size of memory block for instance (in bytes).
///
/// @return  Valid pointer on success; '0' otherwise.
csmFloatSink* csmInitializeFloatSinkInPlace(csmModel* model, void* address, const unsigned int size);


/// Applies any dirty float values.
///
/// @param  sink          Sink to flush.
/// @param  model         Model to apply values to.
/// @param  onModelFloat  [Optional] Model float handler.
/// @param  userData      [Optional] Data to pass to model float handler.
void csmFlushFloatSink(csmFloatSink *sink, csmModel *model, csmModelFloatHandler onModelFloat, void* userData);


// --------- //
// ANIMATION //
// --------- //

/// Gets the deserialized size of a serialized animation in bytes.
///
/// @param  motionJson  Serialized animation to query for.
///
/// @return  Number of bytes necessary.
unsigned int csmGetDeserializedSizeofAnimation(const char* motionJsonString);


/// Deserializes an animotion.
///
/// @param[in]  motionJson  Serialized animation.
/// @param[in]  address     Address to place deserialized animation at.
/// @param[in]  size        Size of passed memory block (in bytes).
///
/// @return  Valid pointer on success; '0' otherwise.
csmAnimation *csmDeserializeAnimationInPlace(const char *motionJson, void* address, const unsigned int size);


/// Evaluates an animation filling a sink with the results.
///
/// @param  animation  Animation to evaluate.
/// @param  state      Animation state.
/// @param  blend      Blend function to use for filling sink.
/// @param  weight     Blend weight factor.
/// @param  sink       Sink to fill with results.
void csmEvaluateAnimation(const csmAnimation *animation,
                          const csmAnimationState *state,
                          const csmFloatBlendFunction blend,
                          const float weight,
                          csmFloatSink *sink);


// --------------- //
// ANIMATION STATE //
// --------------- //

/// Initializes/Resets an animation state.
///
/// @param  state  Animation state to reset.
void csmResetAnimationState(csmAnimationState* state);

/// Ticks an animation state.
///
/// @param  state      State to tick.
/// @param  deltaTime  Time passed since last tick.
void csmTickAnimationState(csmAnimationState* state, const float deltaTime);


#endif
