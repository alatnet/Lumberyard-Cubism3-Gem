#pragma once

#include "Cubism3Debug.h"
#include "Cubism3Assets.h"
#include "Cubism3EditorData.h"

namespace Cubism3 {
	typedef float(*Cubism3AnimationFloatBlend)(float base, float value, float weight);

	namespace FloatBlend {
		float Default(float base, float value, float weight);
		float Additive(float base, float value, float weight);
	}

	class Cubism3Animation {
	public:
		Cubism3Animation(AzFramework::SimpleAssetReference<Cubism3::MotionAsset> asset);
		~Cubism3Animation();

	public:
		bool Loaded() { return this->m_loaded; }

	public:
		void SetParametersAndParts(ModelParametersGroup * paramGroup, ModelPartsGroup * partsGroup);

	public:
		void SetFloatBlend(Cubism3AnimationFloatBlend floatBlendFunc) { m_floatBlendFunc = floatBlendFunc; }

	private:
		bool m_loaded;
		ModelParametersGroup * m_paramGroup;
		ModelPartsGroup * m_partsGroup;

		Cubism3AnimationFloatBlend m_floatBlendFunc;

	private: //meta data
		struct Meta {
			float duration; //end time
			float fps;
			bool loop;
			unsigned int curveCount;
			unsigned int totalSegCount;
			unsigned int totalPointCount;
		};

		Meta m_meta;

	private: //Curve stuff
		enum SegmentType {
			LINEAR = 0,
			BEZIER,
			STEPPED,
			ISTEPPED
		};

		class Calc {
		public:
			virtual float calculate(float time) = 0;
			virtual SegmentType getType() = 0;
		};

		class Linear : public Calc {
		public:
			AZStd::pair<float, float> data[2];
			float calculate(float time);
			SegmentType getType() { return LINEAR; }
		};

		class Bezier : public Calc {
		public:
			AZStd::pair<float, float> data[4];
			float calculate(float time);
			SegmentType getType() { return BEZIER; }
		};

		class Stepped : public Calc {
		public:
			SegmentType type;
			AZStd::pair<float, float> data;
			float calculate(float time) { return this->data.second; }
			SegmentType getType() { return type; }
		};

		enum CurveTarget {
			Model,
			Parameter,
			Part
		};

		struct Curve {
			CurveTarget target;
			int id; //id from m_group - -1 for model opacity
			AZStd::string idStr;
			AZStd::vector<AZStd::pair<float, Calc *>> segments; //keyframe -> calc //reverse lookup, if time > keyframe then calc(time)
		};

		AZStd::vector<Curve *> m_curves;
	};
}