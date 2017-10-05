#pragma once

#include "Cubism3Debug.h"
#include "Cubism3Assets.h"
//#include "Cubism3EditorData.h"
#include "Include\Cubism3\Cubism3UIBus.h"
#include "Cubism3Drawable.h"


#if defined(CUBISM3_ANIMATION_FRAMEWORK) && CUBISM3_ANIMATION_FRAMEWORK == 1
#include <Live2DCubismFramework.h>
#endif

namespace Cubism3 {
	class ModelParametersGroup;
	class ModelPartsGroup;

	namespace FloatBlend {
		float Default(float base, float value, float weight);
		float Additive(float base, float value, float weight);
	}

	class Cubism3Animation {
	public:
		AZ_RTTI(Cubism3Animation, "{C80B4CDB-8FB4-4E10-B098-6B88213B9F4A}");

	public:
		Cubism3Animation();
		~Cubism3Animation();

	public:
		void Load(MotionAssetRef asset);

	public:
		bool Loaded() { return this->m_loaded; }

	public:
		void SetParametersAndParts(ModelParametersGroup * paramGroup, ModelPartsGroup * partsGroup);
		void SetDrawables(AZStd::vector<Cubism3Drawable*> *drawables);

	public:
		void SetFloatBlend(Cubism3AnimationFloatBlend floatBlendFunc) { this->m_floatBlendFunc = floatBlendFunc; }
		void SetWeight(float weight) { this->m_weight = weight; }
		float GetWeight() { return this->m_weight; }

	public:
		bool IsPlaying();
		bool IsStopped();
		bool IsPaused();
		bool IsLooping() { return this->m_meta.m_loop; }

	public:
		void Play();
		void Stop();
		void Pause();
		void SetLooping(bool loop);

	public:
		void Tick(float delta);
		void Reset();

	private:
	#if !defined(CUBISM3_ANIMATION_FRAMEWORK) || CUBISM3_ANIMATION_FRAMEWORK == 0
		void UpdateCurves();
	#endif

	#if defined(CUBISM3_ANIMATION_FRAMEWORK) && CUBISM3_ANIMATION_FRAMEWORK == 1
	public:
		virtual void SetHashTable(csmModelHashTable* hashTable) { this->m_hashTable = hashTable; }
		virtual void SetModel(csmModel* model) { this->m_model = model; }
	private:
		csmModelHashTable* m_hashTable;
		csmModel* m_model;
		csmAnimation* m_anim;
		csmAnimationState m_animState;
	#endif

	private:
		bool m_loaded;

	#if !defined(CUBISM3_ANIMATION_FRAMEWORK) || CUBISM3_ANIMATION_FRAMEWORK == 0
		ModelParametersGroup * m_paramGroup;
		ModelPartsGroup * m_partsGroup;
		AZStd::vector<Cubism3Drawable*> *m_drawables;
	#endif

		Cubism3AnimationFloatBlend m_floatBlendFunc;


	#if !defined(CUBISM3_ANIMATION_FRAMEWORK) || CUBISM3_ANIMATION_FRAMEWORK == 0
		float m_time;
	#endif
		float m_weight;
		bool m_playedOnce;

		bool m_playing;

	private: //meta data
		struct Meta {
			bool m_loop;
		#if !defined(CUBISM3_ANIMATION_FRAMEWORK) || CUBISM3_ANIMATION_FRAMEWORK == 0
			float m_duration; //end time
			float m_fps;
			unsigned int m_curveCount;
			unsigned int m_totalSegCount;
			unsigned int m_totalPointCount;
		#endif
		};

		Meta m_meta;

	#if !defined(CUBISM3_ANIMATION_FRAMEWORK) || CUBISM3_ANIMATION_FRAMEWORK == 0
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
			AZStd::pair<float, float> m_data[2];
			float calculate(float time);
			SegmentType getType() { return LINEAR; }
		};

		class Bezier : public Calc {
		public:
			AZStd::pair<float, float> m_data[4];
			float calculate(float time);
			SegmentType getType() { return BEZIER; }
		};

		class Stepped : public Calc {
		public:
			SegmentType m_type;
			AZStd::pair<float, float> m_data;
			float calculate(float time) { return this->m_data.second; }
			SegmentType getType() { return m_type; }
		};

		enum CurveTarget {
			Model,
			Parameter,
			Part
		};

		struct Curve {
			CurveTarget m_target;
			int m_id; //id from m_group - -1 for model opacity
			AZStd::string m_idStr;
			AZStd::vector<AZStd::pair<float, Calc *>> m_segments; //keyframe -> calc //reverse lookup, if time > keyframe then calc(time)
		};

		AZStd::vector<Curve *> m_curves;
	#endif

	public: //RTTI stuff
		static void Reflect(AZ::SerializeContext* serializeContext);
	};
}