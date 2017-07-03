/*
 * Copyright(c) Live2D Inc. All rights reserved.
 * 
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at http://live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */


#ifndef LIVE2D_CUBISM_FRAMEWORK_INTERNAL_H
#define LIVE2D_CUBISM_FRAMEWORK_INTERNAL_H


// ----- //
// TYPES //
// ----- //

/// 16-bit hash value.
typedef unsigned short csmHash;


/// JSON token type.
typedef enum csmJsonTokenType
{
  /// Begin of object.
  csmJsonObjectBegin,

  /// End of object.
  csmJsonObjectEnd,


  /// Begin of array.
  csmJsonArrayBegin,

  /// End of array.
  csmJsonArrayEnd,


  /// Key.
  csmJsonName,


  /// String value.
  csmJsonString,

  /// Number value.
  csmJsonNumber,

  /// 'true' value.
  csmJsonTrue,

  /// 'false' value.
  csmJsonFalse,

  /// 'null' value.
  csmJsonNull,


  /// Number of JSON token types.
  // (Make sure this value always is the last value of the enumeration).
  csmJsonTokenTypeCount
}
csmJsonTokenType;

/// JSON token handler when lexing JSONs.
///
/// @param  jsonString  Input JSON string.
/// @param  type        Token type.
/// @param  begin       Begin of token as offset into string (in chars).
/// @param  end         End of token as offset into string (in chars).
/// @param  userData    [Optional] User data.
///
/// @param Non-zero to continue lexing; '0' to stop lexing.
typedef int (*csmJsonTokenHandler)(const char* jsonString,
                                   const csmJsonTokenType type,
                                   const int begin,
                                   const int end,
                                   void* userData);


/// Float target.
typedef enum csmFloatSinkValueType
{
  /// Float targeting model.
  csmModelFloat,

  /// Float targeting model parameter.
  csmParameterFloat,

  /// Float targeting model part opacity.
  csmPartOpacityFloat,


  // Number of float sink value types.
  // (Make sure this value always is the last value of the enumeration).
  csmFloatSinkValueTypeCount
}
csmFloatSinkValueTarget;

/// Single value in a float sink.
typedef struct csmFloatSinkValue
{
  /// Target type.
  char Type;

  /// Non-zero if value has changed.
  char IsDirty;

  /// ID of value.
  csmHash Id;
  
  /// Current value.
  float Value;
}
csmFloatSinkValue;

/// Sink for float values.
typedef struct csmFloatSink
{
  /// Number of values.
  int ValueCount;

  /// Values.
  csmFloatSinkValue* Values;
}
csmFloatSink;


/// Single point making up an animation curve.
typedef struct csmAnimationPoint
{
  /// Timing of point.
  float Time;

  /// Value at time.
  float Value;
}
csmAnimationPoint;

/// Animation segment evaluation function.
///
/// @param  points  Points making up the segment.
/// @param  time    Time to evaluate at.
///
/// @return  Evaluation result.
typedef float (*csmAnimationSegmentEvaluationFunction)(const csmAnimationPoint* points, const float time);

/// Builtin linear animation segment evaluation.
///
/// First 2 points are evaluated.
float csmLinearAnimationSegmentEvaluationFunction(const csmAnimationPoint* points, const float time);

/// Builtin bezier animation segment evaluation.
///
/// First 4 points are evaluated.
float csmBezierAnimationSegmentEvaluationFunction(const csmAnimationPoint* points, const float time);

/// Builtin stepped animation segment evaluation.
///
/// First 2 points are evaluated.
float csmSteppedAnimationSegmentEvaluationFunction(const csmAnimationPoint* points, const float time);

/// Builtin inverse stepped animation segment evaluation.
///
/// First 2 points are evaluated.
float csmInverseSteppedAnimationSegmentEvaluationFunction(const csmAnimationPoint* points, const float time);

/// Single animation curve segment.
typedef struct csmAnimationSegment
{
  /// Allows evaluating segment.
  csmAnimationSegmentEvaluationFunction Evaluate;

  /// Index of first segment point.
  int BasePointIndex;
}
csmAnimationSegment;

/// Animation curve.
typedef struct csmAnimationCurve
{
  /// Float target type.
  short Type;

  /// ID of curve.
  csmHash Id;

  /// Number of segments the curve contains.
  int SegmentCount;

  /// Index of first segment in curve.
  int BaseSegmentIndex;
}
csmAnimationCurve;

/// Animation.
typedef struct csmAnimation
{
  /// Duration in seconds.
  float Duration;

  /// Non-zero if animation should loop.
  short Loop;


  /// Number of curves.
  short CurveCount;

  /// Curves.
  csmAnimationCurve* Curves;

  /// Curve segments.
  csmAnimationSegment* Segments;

  /// Curve points.
  csmAnimationPoint* Points;
}
csmAnimation;


// ---- //
// HASH //
// ---- //

/// Hashes a null-terminated string up to the first 64 characters.
///
/// @param  string  String to hash.
///
/// @return  Hash value.
csmHash csmHashString(const char* string);

/// Hashes part of a string.
///
/// @param  string  String to hash.
/// @param  begin   Inclusive offset into string to start hashing at.
/// @param  end     Exclusive offset into string to stop hashing at.
///
/// @return  Hash value.
csmHash csmHashSubString(const char* string, const int begin, const int end);


// ---- //
// JSON //
// ---- //

/// Lexes a JSON string.
///
/// @param  jsonString  JSON string to lex.
/// @param  onToken     Token handler.
/// @param  userData    [Optional] Data to pass to token handler.
void csmLexJson(const char* jsonString, csmJsonTokenHandler onToken, void* userData);


// ---------- //
// FLOAT SINK //
// ---------- //

/// Finds index of a float value.
///
/// @param  sink  Sink to query.
/// @param  type  Type to match.
/// @param  id    ID to match.
///
/// @return  Valid index if found; '-1' otherwise.
int csmGetIndexofFloatSinkValue(const csmFloatSink* sink, const char type, const csmHash id);


/// Initializes/Resets a float sink.
///
/// @param  sink        Sink to reset.
/// @param  values      Float value data.
/// @param  valueCount  Number of values.
void csmResetFloatSink(csmFloatSink* sink,
                       csmFloatSinkValue* values, const int valueCount);


// --------- //
// ANIMATION //
// --------- //

/// Initializes/Resets an animation.
///
/// @param  animation     Animation to reset.
/// @param  duration      Duration in seconds.
/// @param  loop          Loop flag.
/// @param  curves        Curve data.
/// @param  curveCount    Number of curves.
/// @param  segments      Segment data.
/// @param  points        Point data.
void csmResetAnimation(csmAnimation* animation,
                       float duration,
                       short loop,
                       csmAnimationCurve* curves, const short curveCount,
                       csmAnimationSegment* segments,
                       csmAnimationPoint* points);


#endif
