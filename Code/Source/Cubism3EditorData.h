#pragma once

#include <CryThread.h>
#include <AzCore/Serialization/SerializeContext.h>
#include "Cubism3Animation.h"

namespace Cubism3 {
	struct ElementInfo {
		AZ::Uuid m_uuid;                    // Type uuid for the class field that should use this edit data.
		AZ::Edit::ElementData m_editData;   // Edit metadata (name, description, attribs, etc).
	};
	class ModelAnimation {
	public: //animation stuff
		float animVal;
		bool animDirty;
		CryMutex animMutex;
	public:
		virtual void SyncAnimation() = 0;
	};

	//----------------------------------------------------------------------------
	class ModelParameter : public ModelAnimation {
	public:
		AZ_RTTI(ModelParameter, "{F804F1A2-F3F5-4489-B326-2906F90FCB0F}");

	public:
		virtual ~ModelParameter() {}
		AZStd::string name;
		int id;
		float min, max;
		float *val;

	/*public: //animation stuff
		float animVal;
		bool animDirty;
		CryMutex animMutex;*/
		
	public:
		void SyncAnimation();

	public: //editor stuff
		//AZ::Edit::ElementData ed;
		ElementInfo ei;
		void InitEdit();

	public: //RTTI stuff
		static void Reflect(AZ::SerializeContext* serializeContext);
		static void ReflectEdit(AZ::EditContext* ec);
	};

	class ModelParametersGroup {
	public:
		AZ_TYPE_INFO(ModelParametersGroup, "{0C617BC0-697E-4BEA-856B-F56776D7C32B}");

	public:
		AZStd::string m_name;
		AZStd::vector<ModelParameter*>  m_params;
		AZStd::unordered_map<AZStd::string, int> m_idMap; //using a map/hash table should be faster in finding indexes by name rather than searching for it sequentially.

		ModelParameter* at(unsigned int index) { return m_params.at(index); }
		size_t size() { return this->m_params.size(); }
		int find(AZStd::string name);

		void Clear();
		
	public:
		void SyncAnimations() { for (ModelParameter * p : this->m_params) if (p->animDirty) p->SyncAnimation(); }

	public:
		ModelParametersGroup() : m_name("Parameters") {}
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
		virtual ~ModelPart() {}
		AZStd::string name;
		int id;
		float *val;
		/*float animVal;
		bool animDirty;
		CryMutex animMutex;*/

	public:
		void SyncAnimation();

	public: //editor stuff
		//AZ::Edit::ElementData ed;
		ElementInfo ei;
		void InitEdit();

	public: //RTTI stuff
		static void Reflect(AZ::SerializeContext* serializeContext);
		static void ReflectEdit(AZ::EditContext* ec);
	};

	class ModelPartsGroup {
	public:
		AZ_TYPE_INFO(ModelPartsGroup, "{59D3B9A9-E175-40C4-896A-AD85C8DE7D4F}");

	public:
		AZStd::string m_name;
		AZStd::vector<ModelPart*>  m_parts;
		AZStd::unordered_map<AZStd::string, int> m_idMap; //using a map/hash table should be faster in finding indexes by name rather than searching for it sequentially.

		ModelPart* at(unsigned int index) { return m_parts.at(index); }
		size_t size() { return this->m_parts.size(); }
		int find(AZStd::string name);

		void Clear();

	public:
		void SyncAnimations() { for (ModelPart * p : this->m_parts) if (p->animDirty) p->SyncAnimation(); }

	public:
		ModelPartsGroup() : m_name("Parts") {}
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

	public:
		AzFramework::SimpleAssetReference<MotionAsset> asset;
		AZStd::string assetPath;
		//AZ::EntityId entId;
		Cubism3UIComponent * m_component;

	public:
		void PlayPause();
		void Stop();
		void Reset();
		void LoopCN();
		void WeightCN();
		void AssetCN();
		void BlendingCN();

	public:
		//void SetEntityID(AZ::EntityId id) { this->entId = id; }
		void SetUIComponent(Cubism3UIComponent * component) { this->m_component = component; }
		bool IsLoaded() { return this->loaded; }

	public:
		bool loop;
		float weight;
		int blending;
		bool pblank;
		bool sblank;
		bool rblank;

		bool loaded;

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