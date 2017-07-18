#include "StdAfx.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include "Cubism3EditorData.h"

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

	void ModelParametersGroup::Clear() {
		if (this->m_params.size() != 0) {
			this->m_params.clear(); //clear the parameters vector
			this->m_idMap.clear(); //clear the parameters id map
		}
	}

	void ModelParametersGroup::Reflect(AZ::SerializeContext* serializeContext) {
		serializeContext->Class<ModelParametersGroup>()
			->Field("Name", &ModelParametersGroup::m_name)
			->Field("Params", &ModelParametersGroup::m_params);
	}

	int ModelParametersGroup::find(AZStd::string name) {
		auto it = this->m_idMap.find(name); //should be faster to find an id by name rather than searching for it sequentially
		if (it != this->m_idMap.end()) return it->second;
		return -1;
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

	void ModelPartsGroup::Clear() {
		if (this->m_parts.size() != 0) {
			this->m_parts.clear(); //clear the parameters vector
			this->m_idMap.clear(); //clear the parameters id map
		}
	}

	void ModelPartsGroup::Reflect(AZ::SerializeContext* serializeContext) {
		serializeContext->Class<ModelPartsGroup>()
			->Field("Name", &ModelPartsGroup::m_name)
			->Field("Parts", &ModelPartsGroup::m_parts);
	}

	int ModelPartsGroup::find(AZStd::string name) {
		auto it = this->m_idMap.find(name); //should be faster to find an id by name rather than searching for it sequentially
		if (it != this->m_idMap.end()) return it->second;
		return -1;
	}
	//----------------------------------------------------------------------------------
}