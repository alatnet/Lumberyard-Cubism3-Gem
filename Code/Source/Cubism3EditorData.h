#pragma once

#include <CryThread.h>
#include <AzCore/Serialization/SerializeContext.h>
#include "Cubism3Animation.h"

namespace Cubism3 {
	class ModelAnimation {
	public: //animation stuff
		float m_animVal;
		bool m_animDirty;
		CryMutex m_animMutex;
	public:
		virtual void SyncAnimation() = 0;
	};

	//----------------------------------------------------------------------------
	class ModelParameter : public ModelAnimation {
	public:
		AZ_RTTI(ModelParameter, "{F804F1A2-F3F5-4489-B326-2906F90FCB0F}");

	public:
		ModelParameter() : m_name(""), m_id(0), m_min(0.0f), m_max(0.0f), m_val(0.0f) {}
	public:
		virtual ~ModelParameter() {}
		AZStd::string m_name;
		int m_id;
		float m_min, m_max;
		float *m_pVal;
		float m_val;
		
	public:
		void SyncAnimation();

	public:
		void SyncEditorVals();

	public: //RTTI stuff
		static void Reflect(AZ::SerializeContext* serializeContext);
		static void ReflectEdit(AZ::EditContext* ec);
	};

	class ModelParametersGroup {
	public:
		AZ_TYPE_INFO(ModelParametersGroup, "{0C617BC0-697E-4BEA-856B-F56776D7C32B}");

	public:
		//AZStd::string m_name;
		AZStd::vector<ModelParameter*> m_params;
		AZStd::unordered_map<AZStd::string, int> m_idMap; //using a map/hash table should be faster in finding indexes by name rather than searching for it sequentially.

		ModelParameter* at(unsigned int index) { return m_params.at(index); }
		size_t size() { return this->m_params.size(); }
		int find(AZStd::string name);

		void Clear();
		
	public:
		void SyncAnimations() { for (ModelParameter * p : this->m_params) if (p->m_animDirty) p->SyncAnimation(); }

	public:
		ModelParametersGroup() /*: m_name("Parameters")*/ {}
		~ModelParametersGroup() { Clear(); }

	public:
		// Disallow copying, only moving
		ModelParametersGroup(const ModelParametersGroup& rhs) = delete;
		ModelParametersGroup& operator=(ModelParametersGroup&) = delete;

		ModelParametersGroup(ModelParametersGroup&& rhs) { *this = AZStd::move(rhs); }
		ModelParametersGroup& operator=(ModelParametersGroup&& rhs);

	public:
		static void Reflect(AZ::SerializeContext* serializeContext);
		static void ReflectEdit(AZ::EditContext* ec);
	};
	//----------------------------------------------------------------------------
	class ModelPart : public ModelAnimation {
	public:
		AZ_RTTI(ModelPart, "{0E84D0AB-9ECF-4654-BD50-7D16D816C554}");

	public:
		ModelPart() : m_name(""), m_id(0), m_val(0.0f) {}
	public:
		virtual ~ModelPart() {}
		AZStd::string m_name;
		int m_id;
		float *m_pVal;
		float m_val;

	public:
		void SyncAnimation();

	public:
		void SyncEditorVals();

	public: //RTTI stuff
		static void Reflect(AZ::SerializeContext* serializeContext);
		static void ReflectEdit(AZ::EditContext* ec);
	};

	class ModelPartsGroup {
	public:
		AZ_TYPE_INFO(ModelPartsGroup, "{59D3B9A9-E175-40C4-896A-AD85C8DE7D4F}");

	public:
		//AZStd::string m_name;
		AZStd::vector<ModelPart*> m_parts;
		AZStd::unordered_map<AZStd::string, int> m_idMap; //using a map/hash table should be faster in finding indexes by name rather than searching for it sequentially.

		ModelPart* at(unsigned int index) { return m_parts.at(index); }
		size_t size() { return this->m_parts.size(); }
		int find(AZStd::string name);

		void Clear();

	public:
		void SyncAnimations() { for (ModelPart * p : this->m_parts) if (p->m_animDirty) p->SyncAnimation(); }

	public:
		ModelPartsGroup() /*: m_name("Parts")*/ {}
		~ModelPartsGroup() { Clear(); }

	public:
		// Disallow copying, only moving
		ModelPartsGroup(const ModelPartsGroup& rhs) = delete;
		ModelPartsGroup& operator=(ModelPartsGroup&) = delete;

		ModelPartsGroup(ModelPartsGroup&& rhs) { *this = AZStd::move(rhs); }
		ModelPartsGroup& operator=(ModelPartsGroup&& rhs);

	public:
		static void Reflect(AZ::SerializeContext* serializeContext);
		static void ReflectEdit(AZ::EditContext* ec);
	};

	//----------------------------------------------------------------------------
	class Cubism3UIComponent;

	class AnimationControl {
	public:
		AZ_RTTI(AnimationControl, "{E61C4A2A-7E83-44A6-86E0-0E603FEC9FB4}");

	public:
		AnimationControl();

	private:
		AzFramework::SimpleAssetReference<MotionAsset> m_asset;
		AZStd::string m_assetPath;
		Cubism3UIComponent * m_component;

	public:
		void OnPlayPause();
		void OnStop();
		void OnReset();
		void OnLoopChange();
		void OnWeightChange();
		void OnAssetChange();
		void OnBlendingChange();

	public:
		void SetUIComponent(Cubism3UIComponent * component) { this->m_component = component; }
		bool IsLoaded() { return this->m_loaded; }

	private:
		bool m_loop;
		float m_weight;
		int m_blending;
		bool m_pblank;
		bool m_sblank;
		bool m_rblank;

		bool m_loaded;

	private:
		enum BlendFunc {
			Default,
			Additive
		};

	public: //RTTI stuff
		static void Reflect(AZ::SerializeContext* serializeContext);
		static void ReflectEdit(AZ::EditContext* ec);
	};
}