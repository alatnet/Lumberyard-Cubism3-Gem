#include "StdAfx.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include "Cubism3EditorData.h"
#include "Cubism3\Cubism3UIBus.h"

#include "Cubism3UIComponent.h"

namespace Cubism3 {
	//----------------------------------------------------------------------------------
	//Model Parameters Stuff
	void ModelParameter::InitEdit() {
		this->ei.m_uuid = AZ::SerializeTypeInfo<float>::GetUuid();
		this->ei.m_editData.m_elementId = AZ::Edit::UIHandlers::Slider;
		this->ei.m_editData.m_name = this->name.c_str();
		this->ei.m_editData.m_description = this->name.c_str();
		this->ei.m_editData.m_attributes.push_back(AZ::Edit::AttributePair(AZ::Edit::Attributes::Max, aznew AZ::Edit::AttributeData<float>(this->max))); //parameter min
		this->ei.m_editData.m_attributes.push_back(AZ::Edit::AttributePair(AZ::Edit::Attributes::Min, aznew AZ::Edit::AttributeData<float>(this->min))); //parameter max
	}

	void ModelParameter::Reflect(AZ::SerializeContext* serializeContext) {
		serializeContext->Class<ModelParameter>()
			->Version(1)
			->Field("id", &ModelParameter::id)
			->Field("name", &ModelParameter::name)
			->Field("value", &ModelParameter::val);
	}

	void ModelParameter::SyncAnimation() {
		*this->val = animVal;
		this->animDirty = false;
	}

	void ModelParameter::ReflectEdit(AZ::EditContext* ec) {
		ec->Class<ModelParameter>("Parameter", "A Model Parameter")
			->ClassElement(AZ::Edit::ClassElements::EditorData, "Parameter Attribute.")
				->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
			->DataElement(AZ::Edit::UIHandlers::Slider, &ModelParameter::val, "val", "A float");
	}

	void ModelParametersGroup::Clear() {
		if (this->m_params.size() != 0) {
			this->m_params.clear(); //clear the parameters vector
			this->m_idMap.clear(); //clear the parameters id map
		}
	}

	void ModelParametersGroup::Reflect(AZ::SerializeContext* serializeContext) {
		serializeContext->Class<ModelParametersGroup>()
			->Version(1)
			->Field("Name", &ModelParametersGroup::m_name)
			->Field("Params", &ModelParametersGroup::m_params);
	}

	int ModelParametersGroup::find(AZStd::string name) {
		auto it = this->m_idMap.find(name); //should be faster to find an id by name rather than searching for it sequentially
		if (it != this->m_idMap.end()) return it->second;
		return -1;
	}

	void ModelParametersGroup::ReflectEdit(AZ::EditContext* ec) {
		ec->Class<ModelParametersGroup>("A Models parameter group", "This is a model group")
			->ClassElement(AZ::Edit::ClassElements::EditorData, "ModelParametersGroup's class attributes.")
				->Attribute(AZ::Edit::Attributes::NameLabelOverride, &ModelParametersGroup::m_name)
				->Attribute(AZ::Edit::Attributes::AutoExpand, false)
			->DataElement(0, &ModelParametersGroup::m_params, "m_params", "Parameters in this property group")
				->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
	}
	//----------------------------------------------------------------------------------

	//----------------------------------------------------------------------------------
	//Model Parts Stuff
	void ModelPart::InitEdit() {
		this->ei.m_uuid = AZ::SerializeTypeInfo<float>::GetUuid();
		this->ei.m_editData.m_elementId = AZ::Edit::UIHandlers::Slider;
		this->ei.m_editData.m_name = this->name.c_str();
		this->ei.m_editData.m_description = this->name.c_str();
		this->ei.m_editData.m_attributes.push_back(AZ::Edit::AttributePair(AZ::Edit::Attributes::Max, aznew AZ::Edit::AttributeData<float>(1.0f))); //opacity min
		this->ei.m_editData.m_attributes.push_back(AZ::Edit::AttributePair(AZ::Edit::Attributes::Min, aznew AZ::Edit::AttributeData<float>(0.0f))); //opacity max
	}

	void ModelPart::Reflect(AZ::SerializeContext* serializeContext) {
		serializeContext->Class<ModelPart>()
			->Version(1)
			->Field("id", &ModelPart::id)
			->Field("name", &ModelPart::name)
			->Field("value", &ModelPart::val);
	}

	void ModelPart::SyncAnimation() {
		*this->val = animVal;
		this->animDirty = false;
	}

	void ModelPart::ReflectEdit(AZ::EditContext* ec) {
		ec->Class<ModelPart>("Part", "A Part Opacity")
			->ClassElement(AZ::Edit::ClassElements::EditorData, "Part Opacity.")
				->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
			->DataElement(AZ::Edit::UIHandlers::Slider, &ModelPart::val, "val", "A float");
	}

	void ModelPartsGroup::Clear() {
		if (this->m_parts.size() != 0) {
			this->m_parts.clear(); //clear the parameters vector
			this->m_idMap.clear(); //clear the parameters id map
		}
	}

	void ModelPartsGroup::Reflect(AZ::SerializeContext* serializeContext) {
		serializeContext->Class<ModelPartsGroup>()
			->Version(1)
			->Field("Name", &ModelPartsGroup::m_name)
			->Field("Parts", &ModelPartsGroup::m_parts);
	}

	int ModelPartsGroup::find(AZStd::string name) {
		auto it = this->m_idMap.find(name); //should be faster to find an id by name rather than searching for it sequentially
		if (it != this->m_idMap.end()) return it->second;
		return -1;
	}

	void ModelPartsGroup::ReflectEdit(AZ::EditContext* ec) {
		ec->Class<ModelPartsGroup>("A Parts opacity group", "This is a model group")
			->ClassElement(AZ::Edit::ClassElements::EditorData, "ModelPartsGroup's class attributes.")
				->Attribute(AZ::Edit::Attributes::NameLabelOverride, &ModelPartsGroup::m_name)
				->Attribute(AZ::Edit::Attributes::AutoExpand, false)
			->DataElement(0, &ModelPartsGroup::m_parts, "m_parts", "Parts in this property group")
				->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
	}
	//----------------------------------------------------------------------------------
	AnimationControl::AnimationControl() {
		this->loop = true;
		this->weight = 1.0f;
		this->assetPath = "";
		this->loaded = false;

		this->m_component = nullptr;

		this->pblank = this->sblank = this->rblank = false;
	}

	void AnimationControl::PlayPause() {
		/*if (this->entId.IsValid() && this->loaded) {
			bool isPlaying = false;
			EBUS_EVENT_ID_RESULT(isPlaying, this->entId, Cubism3AnimationBus, IsPlaying, this->assetPath);

			if (isPlaying) EBUS_EVENT_ID(this->entId, Cubism3AnimationBus, Pause, this->assetPath);
			else EBUS_EVENT_ID(this->entId, Cubism3AnimationBus, Play, this->assetPath);
		}*/

		if (this->m_component) {
			if (this->m_component->IsPlaying(this->assetPath)) this->m_component->Pause(this->assetPath);
			else this->m_component->Play(this->assetPath);
		}
	}

	void AnimationControl::Stop() {
		/*if (this->entId.IsValid() && this->loaded)
			EBUS_EVENT_ID(this->entId, Cubism3AnimationBus, Stop, this->assetPath);*/

		if (this->m_component) this->m_component->Stop(this->assetPath);
	}

	void AnimationControl::Reset() {
		/*if (this->entId.IsValid() && this->loaded)
			EBUS_EVENT_ID(this->entId, Cubism3AnimationBus, Reset, this->assetPath);*/

		if (this->m_component) this->m_component->Reset(this->assetPath);
	}

	void AnimationControl::LoopCN() {
		/*if (this->entId.IsValid() && this->loaded)
			EBUS_EVENT_ID(this->entId, Cubism3AnimationBus, SetLooping, this->assetPath, this->loop);*/

		if (this->m_component) this->m_component->SetLooping(this->assetPath, this->loop);
	}

	void AnimationControl::WeightCN() {
		/*if (this->entId.IsValid() && this->loaded)
			EBUS_EVENT_ID(this->entId, Cubism3AnimationBus, SetWeight, this->assetPath, this->weight);*/

		if (this->m_component) this->m_component->SetWeight(this->assetPath, this->weight);
	}

	void AnimationControl::AssetCN() {
		/*if (this->entId.IsValid()) {
			if (!this->asset.GetAssetPath().empty()) {
				this->assetPath = this->asset.GetAssetPath();
				EBUS_EVENT_ID_RESULT(this->loaded, this->entId, Cubism3AnimationBus, AddAnimation, this->assetPath);
				EBUS_EVENT_ID(this->entId, Cubism3AnimationBus, SetLooping, this->assetPath, this->loop);
				EBUS_EVENT_ID(this->entId, Cubism3AnimationBus, SetWeight, this->assetPath, this->weight);

				switch (this->blending) {
				case 0:
					if (this->entId.IsValid() && this->loaded)
						EBUS_EVENT_ID(this->entId, Cubism3AnimationBus, SetFloatBlend, this->assetPath, Cubism3::FloatBlend::Default);
					break;
				case 1:
					if (this->entId.IsValid() && this->loaded)
						EBUS_EVENT_ID(this->entId, Cubism3AnimationBus, SetFloatBlend, this->assetPath, Cubism3::FloatBlend::Additive);
					break;
				}

				if (this->loaded)
					CLOG("[Cubism3] Animation Asset Loaded. - %s", this->assetPath.c_str());
				else
					CLOG("[Cubism3] Animation Asset did not load. - %s", this->assetPath.c_str());
			} else {
				EBUS_EVENT_ID(this->entId, Cubism3AnimationBus, RemoveAnimation, this->assetPath);
				this->assetPath = "";
				this->loaded = false;
			}
		}*/

		if (this->m_component) {
			if (!this->asset.GetAssetPath().empty()) {
				this->assetPath = this->asset.GetAssetPath();

				this->loaded = this->m_component->AddAnimation(this->assetPath);
				this->m_component->SetLooping(this->assetPath, this->loop);
				this->m_component->SetWeight(this->assetPath, this->weight);
				switch (this->blending) {
				case 0:
					this->m_component->SetFloatBlend(this->assetPath, Cubism3::FloatBlend::Default);
					break;
				case 1:
					this->m_component->SetFloatBlend(this->assetPath, Cubism3::FloatBlend::Additive);
					break;
				}

				if (this->loaded)
					CLOG("[Cubism3] Animation Asset Loaded. - %s", this->assetPath.c_str());
				else
					CLOG("[Cubism3] Animation Asset did not load. - %s", this->assetPath.c_str());
			} else {
				this->m_component->RemoveAnimation(this->assetPath);
				this->assetPath = "";
				this->loaded = false;
			}
		}
	}

	void AnimationControl::BlendingCN() {
		/*switch (this->blending) {
		case 0:
			if (this->entId.IsValid() && this->loaded)
				EBUS_EVENT_ID(this->entId, Cubism3AnimationBus, SetFloatBlend, this->assetPath, Cubism3::FloatBlend::Default);
			break;
		case 1:
			if (this->entId.IsValid() && this->loaded)
				EBUS_EVENT_ID(this->entId, Cubism3AnimationBus, SetFloatBlend, this->assetPath, Cubism3::FloatBlend::Additive);
			break;
		}*/
		
		switch (this->blending) {
		case 0:
			this->m_component->SetFloatBlend(this->assetPath, Cubism3::FloatBlend::Default);
			break;
		case 1:
			this->m_component->SetFloatBlend(this->assetPath, Cubism3::FloatBlend::Additive);
			break;
		}
	}

	void AnimationControl::Reflect(AZ::SerializeContext* serializeContext) {
		serializeContext->Class<AnimationControl>()
			->Version(1)
			->Field("asset", &AnimationControl::asset)
			->Field("loop", &AnimationControl::loop)
			->Field("weight", &AnimationControl::weight)
			->Field("blending", &AnimationControl::blending)
			->Field("pblank", &AnimationControl::pblank)
			->Field("sblank", &AnimationControl::sblank)
			->Field("rblank", &AnimationControl::rblank)
			;
	}

	void AnimationControl::ReflectEdit(AZ::EditContext* ec) {
		ec->Class<AnimationControl>("Animation Asset", "An animation asset.")
			->ClassElement(AZ::Edit::ClassElements::EditorData, "Animation Attribute.")
			->DataElement(0, &AnimationControl::asset, "Asset", "The animation asset.")
				->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimationControl::AssetCN)
			->DataElement(AZ::Edit::UIHandlers::Button, &AnimationControl::pblank, "Play/Pause", "Play/Pause the animation.")
				->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimationControl::PlayPause)
			->DataElement(AZ::Edit::UIHandlers::Button, &AnimationControl::sblank, "Stop", "Stop the animation.")
				->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimationControl::Stop)
			->DataElement(AZ::Edit::UIHandlers::Button, &AnimationControl::rblank, "Reset", "Reset the animation.")
				->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimationControl::Reset)
			->DataElement(0, &AnimationControl::loop, "Loop", "Set the looping of the animation.")
				->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimationControl::LoopCN)
			->DataElement(0, &AnimationControl::weight, "Weight", "Set the weight of the animation.")
				->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimationControl::WeightCN)
			->DataElement(AZ::Edit::UIHandlers::ComboBox, &AnimationControl::blending, "Blending", "Set the blending of the animation.")
				->EnumAttribute(AnimationControl::BlendFunc::Default, "Default")
				->EnumAttribute(AnimationControl::BlendFunc::Additive, "Additive")
				->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimationControl::BlendingCN)
			;
	}
	//----------------------------------------------------------------------------------

}