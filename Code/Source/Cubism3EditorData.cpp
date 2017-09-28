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
		this->m_ei.m_uuid = AZ::SerializeTypeInfo<float>::GetUuid();
		this->m_ei.m_editData.m_elementId = AZ::Edit::UIHandlers::Slider;
		this->m_ei.m_editData.m_name = this->m_name.c_str();
		this->m_ei.m_editData.m_description = this->m_name.c_str();
		this->m_ei.m_editData.m_attributes.push_back(AZ::Edit::AttributePair(AZ::Edit::Attributes::Max, aznew AZ::Edit::AttributeData<float>(this->m_max))); //parameter min
		this->m_ei.m_editData.m_attributes.push_back(AZ::Edit::AttributePair(AZ::Edit::Attributes::Min, aznew AZ::Edit::AttributeData<float>(this->m_min))); //parameter max
	}

	void ModelParameter::Reflect(AZ::SerializeContext* serializeContext) {
		serializeContext->Class<ModelParameter>()
			->Version(1)
			->Field("id", &ModelParameter::m_id)
			->Field("name", &ModelParameter::m_name)
			->Field("value", &ModelParameter::m_val);
	}

	void ModelParameter::SyncAnimation() {
		*this->m_val = m_animVal;
		this->m_animDirty = false;
	}

	void ModelParameter::ReflectEdit(AZ::EditContext* ec) {
		ec->Class<ModelParameter>("Parameter", "A Model Parameter")
			->ClassElement(AZ::Edit::ClassElements::EditorData, "Parameter Attribute.")
				->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
			->DataElement(AZ::Edit::UIHandlers::Slider, &ModelParameter::m_val, "val", "A float");
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
		this->m_ei.m_uuid = AZ::SerializeTypeInfo<float>::GetUuid();
		this->m_ei.m_editData.m_elementId = AZ::Edit::UIHandlers::Slider;
		this->m_ei.m_editData.m_name = this->m_name.c_str();
		this->m_ei.m_editData.m_description = this->m_name.c_str();
		this->m_ei.m_editData.m_attributes.push_back(AZ::Edit::AttributePair(AZ::Edit::Attributes::Max, aznew AZ::Edit::AttributeData<float>(1.0f))); //opacity max
		this->m_ei.m_editData.m_attributes.push_back(AZ::Edit::AttributePair(AZ::Edit::Attributes::Min, aznew AZ::Edit::AttributeData<float>(0.0f))); //opacity min
	}

	void ModelPart::Reflect(AZ::SerializeContext* serializeContext) {
		serializeContext->Class<ModelPart>()
			->Version(1)
			->Field("id", &ModelPart::m_id)
			->Field("name", &ModelPart::m_name)
			->Field("value", &ModelPart::m_val);
	}

	void ModelPart::SyncAnimation() {
		*this->m_val = m_animVal;
		this->m_animDirty = false;
	}

	void ModelPart::ReflectEdit(AZ::EditContext* ec) {
		ec->Class<ModelPart>("Part", "A Part Opacity")
			->ClassElement(AZ::Edit::ClassElements::EditorData, "Part Opacity.")
				->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
			->DataElement(AZ::Edit::UIHandlers::Slider, &ModelPart::m_val, "val", "A float");
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
		this->m_assetPath = "";
		this->m_loop = true;
		this->m_weight = 1.0f;
		this->m_blending = AnimationControl::BlendFunc::Default;
		this->m_loaded = false;

		this->m_component = nullptr;

		this->m_pblank = this->m_sblank = this->m_rblank = false;
	}

	void AnimationControl::OnPlayPause() {
		/*if (this->entId.IsValid() && this->loaded) {
			bool isPlaying = false;
			EBUS_EVENT_ID_RESULT(isPlaying, this->entId, Cubism3AnimationBus, IsPlaying, this->assetPath);

			if (isPlaying) EBUS_EVENT_ID(this->entId, Cubism3AnimationBus, Pause, this->assetPath);
			else EBUS_EVENT_ID(this->entId, Cubism3AnimationBus, Play, this->assetPath);
		}*/

		if (this->m_component) {
			if (this->m_component->IsPlaying(this->m_assetPath)) this->m_component->Pause(this->m_assetPath);
			else this->m_component->Play(this->m_assetPath);
		}
	}
	void AnimationControl::OnStop() {
		/*if (this->entId.IsValid() && this->loaded)
			EBUS_EVENT_ID(this->entId, Cubism3AnimationBus, Stop, this->assetPath);*/

		if (this->m_component) this->m_component->Stop(this->m_assetPath);
	}
	void AnimationControl::OnReset() {
		/*if (this->entId.IsValid() && this->loaded)
			EBUS_EVENT_ID(this->entId, Cubism3AnimationBus, Reset, this->assetPath);*/

		if (this->m_component) this->m_component->Reset(this->m_assetPath);
	}
	void AnimationControl::OnLoopChange() {
		/*if (this->entId.IsValid() && this->loaded)
			EBUS_EVENT_ID(this->entId, Cubism3AnimationBus, SetLooping, this->assetPath, this->loop);*/

		if (this->m_component) this->m_component->SetLooping(this->m_assetPath, this->m_loop);
	}
	void AnimationControl::OnWeightChange() {
		/*if (this->entId.IsValid() && this->loaded)
			EBUS_EVENT_ID(this->entId, Cubism3AnimationBus, SetWeight, this->assetPath, this->weight);*/

		if (this->m_component) this->m_component->SetWeight(this->m_assetPath, this->m_weight);
	}
	void AnimationControl::OnAssetChange() {
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
			if (!this->m_asset.GetAssetPath().empty()) {
				this->m_assetPath = this->m_asset.GetAssetPath();

				this->m_loaded = this->m_component->AddAnimation(this->m_assetPath);
				this->m_component->SetLooping(this->m_assetPath, this->m_loop);
				this->m_component->SetWeight(this->m_assetPath, this->m_weight);
				switch (this->m_blending) {
				case 0:
					this->m_component->SetFloatBlend(this->m_assetPath, Cubism3::FloatBlend::Default);
					break;
				case 1:
					this->m_component->SetFloatBlend(this->m_assetPath, Cubism3::FloatBlend::Additive);
					break;
				}

				if (this->m_loaded)
					CLOG("[Cubism3] Animation Asset Loaded. - %s", this->m_assetPath.c_str());
				else
					CLOG("[Cubism3] Animation Asset did not load. - %s", this->m_assetPath.c_str());
			} else {
				this->m_component->RemoveAnimation(this->m_assetPath);
				this->m_assetPath = "";
				this->m_loaded = false;
			}
		}
	}
	void AnimationControl::OnBlendingChange() {
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
		
		switch (this->m_blending) {
		case 0:
			this->m_component->SetFloatBlend(this->m_assetPath, Cubism3::FloatBlend::Default);
			break;
		case 1:
			this->m_component->SetFloatBlend(this->m_assetPath, Cubism3::FloatBlend::Additive);
			break;
		}
	}

	void AnimationControl::Reflect(AZ::SerializeContext* serializeContext) {
		serializeContext->Class<AnimationControl>()
			->Version(1)
			->Field("Asset", &AnimationControl::m_asset)
			->Field("Loop", &AnimationControl::m_loop)
			->Field("Weight", &AnimationControl::m_weight)
			->Field("Blending", &AnimationControl::m_blending)
			->Field("pblank", &AnimationControl::m_pblank)
			->Field("sblank", &AnimationControl::m_sblank)
			->Field("rblank", &AnimationControl::m_rblank)
			;
	}

	void AnimationControl::ReflectEdit(AZ::EditContext* ec) {
		ec->Class<AnimationControl>("Animation Asset", "An animation asset.")
			->ClassElement(AZ::Edit::ClassElements::EditorData, "Animation Attribute.")
			->DataElement(0, &AnimationControl::m_asset, "Asset", "The animation asset.")
				->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimationControl::OnAssetChange)
			->DataElement(AZ::Edit::UIHandlers::Button, &AnimationControl::m_pblank, "Play/Pause", "Play/Pause the animation.")
				->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimationControl::OnPlayPause)
			->DataElement(AZ::Edit::UIHandlers::Button, &AnimationControl::m_sblank, "Stop", "Stop the animation.")
				->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimationControl::OnStop)
			->DataElement(AZ::Edit::UIHandlers::Button, &AnimationControl::m_rblank, "Reset", "Reset the animation.")
				->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimationControl::OnReset)
			->DataElement(0, &AnimationControl::m_loop, "Loop", "Set the looping of the animation.")
				->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimationControl::OnLoopChange)
			->DataElement(0, &AnimationControl::m_weight, "Weight", "Set the weight of the animation.")
				->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimationControl::OnWeightChange)
			->DataElement(AZ::Edit::UIHandlers::ComboBox, &AnimationControl::m_blending, "Blending", "Set the blending of the animation.")
				->EnumAttribute(AnimationControl::BlendFunc::Default, "Default")
				->EnumAttribute(AnimationControl::BlendFunc::Additive, "Additive")
				->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimationControl::OnBlendingChange)
			;
	}
	//----------------------------------------------------------------------------------

}