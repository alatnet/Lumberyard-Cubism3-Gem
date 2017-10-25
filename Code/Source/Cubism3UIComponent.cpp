#include "StdAfx.h"

#include <AzCore/Math/Crc.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>

#include <AzCore/std/sort.h>

#include <LyShine/IDraw2d.h>
#include <LyShine/UiSerializeHelpers.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiTransform2dBus.h>
#include <LyShine/IUiRenderer.h>

#include <AZCore/JSON/rapidjson.h>
#include <AZCore/JSON/document.h>

#include "Cubism3UIComponent.h"

namespace Cubism3 {
	const char* cubism3_profileMarker = "UI_CUBISM3";

	Cubism3UIComponent::Cubism3UIComponent() {
		this->m_transform = this->m_prevViewport =  AZ::Matrix4x4::CreateIdentity();
		this->m_transformUpdated = false;
		this->m_uvTransform = AZ::Matrix4x4::CreateIdentity() * AZ::Matrix4x4::CreateScale(AZ::Vector3(1.0f, -1.0f, 1.0f)); //flip the texture on the y vector
		
		this->m_prevAnchors = UiTransform2dInterface::Anchors(0.4f,0.4f,0.4f,0.4f);
		this->m_prevOffsets = UiTransform2dInterface::Offsets(-40,-40,40,40);

		this->m_moc = nullptr;
		this->m_model = nullptr;
		this->m_texture = nullptr;

	#if defined(CUBISM3_ANIMATION_FRAMEWORK) && CUBISM3_ANIMATION_FRAMEWORK == 1
		this->m_hashTable = nullptr;
	#endif

		this->m_modelLoaded = false;

		this->m_threading = NONE;
		this->m_thread = nullptr;

		this->m_threadLimiter = CUBISM3_MULTITHREAD_LIMITER;

		#ifdef ENABLE_CUBISM3_DEBUG
		this->m_wireframe = false;
		#endif

		this->m_modelSize = AZ::Vector2(0.0f, 0.0f);
		this->m_fill = false;

		this->m_lType = Single;

		this->m_opacity = this->m_prevOpacity = 1.0f;

		#ifdef ENABLE_CUBISM3_DEBUG
		//masking stuff
		this->m_enableMasking = true;
		this->m_useAlphaTest = Greater;
		this->m_useColorMask = None;

		//stencil stuff
		this->m_stencilFunc = SFunc::EQUAL;
		this->m_stencilCCWFunc = SFunc::FDISABLE;
		this->m_sTwoSided = false;

		this->m_opFail = SOp::KEEP;
		this->m_opZFail = SOp::KEEP;
		this->m_opPass = SOp::ODISABLE;

		this->m_opCCWFail = SOp::ODISABLE;
		this->m_opCCWZFail = SOp::ODISABLE;
		this->m_opCCWPass = SOp::ODISABLE;
		#endif
	}

	Cubism3UIComponent::~Cubism3UIComponent() {
		this->ReleaseObject();
		for (AZStd::pair<AZStd::string, Cubism3Animation*> a : this->m_animations) delete a.second;
		this->m_animations.clear();
	}

	void Cubism3UIComponent::Init() {
		for (AnimationControl a : this->m_animControls) a.SetUIComponent(this);
		this->LoadObject();
	}

	void Cubism3UIComponent::Activate() {
		UiRenderBus::Handler::BusConnect(m_entity->GetId());
		Cubism3UIBus::Handler::BusConnect(m_entity->GetId());
		Cubism3AnimationBus::Handler::BusConnect(m_entity->GetId());
	}

	void Cubism3UIComponent::Deactivate() {
		UiRenderBus::Handler::BusDisconnect();
		Cubism3UIBus::Handler::BusDisconnect();
		Cubism3AnimationBus::Handler::BusDisconnect();
	}

	void Cubism3UIComponent::Reflect(AZ::ReflectContext* context) {
		AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

		if (serializeContext) {
			ModelParameter::Reflect(serializeContext);
			ModelParametersGroup::Reflect(serializeContext);
			ModelPart::Reflect(serializeContext);
			ModelPartsGroup::Reflect(serializeContext);
			AnimationControl::Reflect(serializeContext);
			Cubism3Animation::Reflect(serializeContext);

			#define QFIELD(x) ->Field(#x,&Cubism3UIComponent::##x##) //quick field
			serializeContext->Class<Cubism3UIComponent, AZ::Component>()
				->Version(1)
				->Field("LoadType", &Cubism3UIComponent::m_lType)
				->Field("MocFile", &Cubism3UIComponent::m_mocPathname)
				->Field("ImageFile", &Cubism3UIComponent::m_imagePathname)
				->Field("JSONFile", &Cubism3UIComponent::m_jsonPathname)
				->Field("Fill", &Cubism3UIComponent::m_fill)
				->Field("Opacity", &Cubism3UIComponent::m_opacity)
				->Field("Params", &Cubism3UIComponent::m_params)
				->Field("Parts", &Cubism3UIComponent::m_parts)
				->Field("AnimationControls", &Cubism3UIComponent::m_animControls)

				#ifdef ENABLE_CUBISM3_DEBUG
				->Field("Masking", &Cubism3UIComponent::m_enableMasking)
				->Field("Masking_Alpha", &Cubism3UIComponent::m_useAlphaTest)
				->Field("Masking_Color", &Cubism3UIComponent::m_useColorMask)
				->Field("Wireframe", &Cubism3UIComponent::m_wireframe)
				->Field("Threading", &Cubism3UIComponent::m_threading)
				QFIELD(m_stencilFunc)
				QFIELD(m_stencilCCWFunc)
				QFIELD(m_sTwoSided)
				QFIELD(m_opFail)
				QFIELD(m_opZFail)
				QFIELD(m_opPass)
				QFIELD(m_opCCWFail)
				QFIELD(m_opCCWZFail)
				QFIELD(m_opCCWPass)
				#endif
				;
			#undef QFIELD

			AZ::EditContext* ec = serializeContext->GetEditContext();
			if (ec) {
				auto editInfo = ec->Class<Cubism3UIComponent>("Cubism3", "A visual component to draw a Cubism3 Model.");

				editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
					->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Ragdoll.png")
					->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/Ragdoll.png")
					->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("UI", 0x27ff46b0))
					->Attribute(AZ::Edit::Attributes::AutoExpand, true);

				editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &Cubism3UIComponent::m_lType, "Load Type", "What type of Cubism3 Model to load.")
					->EnumAttribute(Cubism3UIInterface::LoadType::Single, "Single")
					->EnumAttribute(Cubism3UIInterface::LoadType::JSON, "JSON")
					->Attribute(AZ::Edit::Attributes::ChangeNotify, &Cubism3UIComponent::OnLoadTypeChange)
					->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree);

				editInfo->DataElement(0, &Cubism3UIComponent::m_mocPathname, "Moc path", "The Moc path. Can be overridden by another component such as an interactable.")
					->Attribute(AZ::Edit::Attributes::Visibility, &Cubism3UIComponent::IsLoadTypeSingle)
					->Attribute(AZ::Edit::Attributes::ChangeNotify, &Cubism3UIComponent::OnMocFileChange)
					->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree);

				editInfo->DataElement(0, &Cubism3UIComponent::m_imagePathname, "Image path", "The Image path. Can be overridden by another component such as an interactable.")
					->Attribute(AZ::Edit::Attributes::Visibility, &Cubism3UIComponent::IsLoadTypeSingle)
					->Attribute(AZ::Edit::Attributes::ChangeNotify, &Cubism3UIComponent::OnImageFileChange);

				editInfo->DataElement(0, &Cubism3UIComponent::m_jsonPathname, "JSON path", "The JSON path. Can be overridden by another component such as an interactable.")
					->Attribute(AZ::Edit::Attributes::Visibility, &Cubism3UIComponent::IsLoadTypeJSON)
					->Attribute(AZ::Edit::Attributes::ChangeNotify, &Cubism3UIComponent::OnJSONFileChange)
					->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree);

				editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &Cubism3UIComponent::m_fill, "Fill", "Fill the model to the element's dimentions.")
					->Attribute(AZ::Edit::Attributes::ChangeNotify, &Cubism3UIComponent::OnFillChange);

				editInfo->DataElement(AZ::Edit::UIHandlers::Slider, &Cubism3UIComponent::m_opacity, "Opacity", "The opacity of the model.")
					->Attribute(AZ::Edit::Attributes::Min, 0.0f)
					->Attribute(AZ::Edit::Attributes::Max, 1.0f);

				editInfo->SetDynamicEditDataProvider(&Cubism3UIComponent::GetEditData);

				//parameters group
				{
					editInfo->DataElement(0, &Cubism3UIComponent::m_params, "Parameters", "List of parameters of the model.");
					ModelParameter::ReflectEdit(ec);
					ModelParametersGroup::ReflectEdit(ec);
				}

				//part group
				{
					editInfo->DataElement(0, &Cubism3UIComponent::m_parts, "Parts", "List of parts of the model.");
					ModelPart::ReflectEdit(ec);
					ModelPartsGroup::ReflectEdit(ec);
				}

				//animation group
				{
					editInfo->DataElement(0, &Cubism3UIComponent::m_animControls, "Animations", "List of animations of the model.\nNote! Animations override Parameters and Parts!")
						->Attribute(AZ::Edit::Attributes::ChangeNotify, &Cubism3UIComponent::OnAnimControlsChange);
					AnimationControl::ReflectEdit(ec);
				}

				//debug group
				#ifdef ENABLE_CUBISM3_DEBUG
				{
					editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Debugging")
						->Attribute(AZ::Edit::Attributes::AutoExpand, false);

					editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &Cubism3UIComponent::m_enableMasking, "Enable masking (Debug)", "Enable masking usage of the Cubism3 model.")
						->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree);

					editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &Cubism3UIComponent::m_useAlphaTest, "Use alpha test (Debug)", "The alpha channel in the mask visual's texture to define the mask.")
						->Attribute(AZ::Edit::Attributes::Visibility, &Cubism3UIComponent::m_enableMasking)
						->EnumAttribute(Cubism3UIComponent::AlphaTest::ATDISABLE, "Disabled")
						->EnumAttribute(Cubism3UIComponent::AlphaTest::Greater, "Greater")
						->EnumAttribute(Cubism3UIComponent::AlphaTest::Less, "Less")
						->EnumAttribute(Cubism3UIComponent::AlphaTest::GEqual, "Greater or Equal")
						->EnumAttribute(Cubism3UIComponent::AlphaTest::LEqual, "Less or Equal");

					editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &Cubism3UIComponent::m_useColorMask, "Use color test (Debug)", "The color mask in the mask visual's texture to define the mask.")
						->Attribute(AZ::Edit::Attributes::Visibility, &Cubism3UIComponent::m_enableMasking)
						->EnumAttribute(Cubism3UIComponent::ColorMask::CMDISABLE, "Disabled")
						->EnumAttribute(Cubism3UIComponent::ColorMask::NoR, "No R")
						->EnumAttribute(Cubism3UIComponent::ColorMask::NoG, "No G")
						->EnumAttribute(Cubism3UIComponent::ColorMask::NoB, "No B")
						->EnumAttribute(Cubism3UIComponent::ColorMask::NoA, "No A")
						->EnumAttribute(Cubism3UIComponent::ColorMask::RGB, "RGB")
						->EnumAttribute(Cubism3UIComponent::ColorMask::A, "A")
						->EnumAttribute(Cubism3UIComponent::ColorMask::None, "None");

					editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &Cubism3UIComponent::m_wireframe, "Wireframe (Debug)", "Wireframe Mode");

					editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &Cubism3UIComponent::m_threading, "Threading (Debug)", "Threading Mode")
						->Attribute(AZ::Edit::Attributes::ChangeNotify, &Cubism3UIComponent::OnThreadingChange)
						->EnumAttribute(Cubism3UIInterface::Threading::NONE, "None")
						->EnumAttribute(Cubism3UIInterface::Threading::SINGLE, "Single")
						->EnumAttribute(Cubism3UIInterface::Threading::MULTI, "Multi");

					//Debug Stencil
					#define QENUMSFUNC(x) ->EnumAttribute(Cubism3UIComponent::SFunc::##x##, #x)
					editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &Cubism3UIComponent::m_stencilFunc, "Stencil Func (Debug)", "Stencil Function")
						->EnumAttribute(Cubism3UIComponent::SFunc::FDISABLE, "DISABLE")
						QENUMSFUNC(ALWAYS)
						QENUMSFUNC(NEVER)
						QENUMSFUNC(LESS)
						QENUMSFUNC(LEQUAL)
						QENUMSFUNC(GREATER)
						QENUMSFUNC(GEQUAL)
						QENUMSFUNC(EQUAL)
						QENUMSFUNC(NOTEQUAL)
						QENUMSFUNC(MASK)
						;
					editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &Cubism3UIComponent::m_stencilCCWFunc, "Stencil CCW Func (Debug)", "Stencil CCW Function")
						->EnumAttribute(Cubism3UIComponent::SFunc::FDISABLE, "DISABLE")
						QENUMSFUNC(ALWAYS)
						QENUMSFUNC(NEVER)
						QENUMSFUNC(LESS)
						QENUMSFUNC(LEQUAL)
						QENUMSFUNC(GREATER)
						QENUMSFUNC(GEQUAL)
						QENUMSFUNC(EQUAL)
						QENUMSFUNC(NOTEQUAL)
						QENUMSFUNC(MASK)
						;
				#undef QENUMSFUNC

				#define QENUMSOP(x) ->EnumAttribute(Cubism3UIComponent::SOp::##x##, #x)
					editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &Cubism3UIComponent::m_opFail, "Stencil Fail Op (Debug)", "Stencil Fail Operation")
						->EnumAttribute(Cubism3UIComponent::SOp::ODISABLE, "DISABLE")
						QENUMSOP(KEEP)
						QENUMSOP(REPLACE)
						QENUMSOP(INCR)
						QENUMSOP(DECR)
						QENUMSOP(ZERO)
						QENUMSOP(INCR_WRAP)
						QENUMSOP(DECR_WRAP)
						QENUMSOP(INVERT)
						;
					editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &Cubism3UIComponent::m_opZFail, "Stencil ZFail Op (Debug)", "Stencil ZFail Operation")
						->EnumAttribute(Cubism3UIComponent::SOp::ODISABLE, "DISABLE")
						QENUMSOP(KEEP)
						QENUMSOP(REPLACE)
						QENUMSOP(INCR)
						QENUMSOP(DECR)
						QENUMSOP(ZERO)
						QENUMSOP(INCR_WRAP)
						QENUMSOP(DECR_WRAP)
						QENUMSOP(INVERT)
						;
					editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &Cubism3UIComponent::m_opPass, "Stencil Pass Op (Debug)", "Stencil Pass Operation")
						->EnumAttribute(Cubism3UIComponent::SOp::ODISABLE, "DISABLE")
						QENUMSOP(KEEP)
						QENUMSOP(REPLACE)
						QENUMSOP(INCR)
						QENUMSOP(DECR)
						QENUMSOP(ZERO)
						QENUMSOP(INCR_WRAP)
						QENUMSOP(DECR_WRAP)
						QENUMSOP(INVERT)
						;

					editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &Cubism3UIComponent::m_opCCWFail, "Stencil CCW Fail Op (Debug)", "Stencil CCW Fail Operation")
						->EnumAttribute(Cubism3UIComponent::SOp::ODISABLE, "DISABLE")
						QENUMSOP(KEEP)
						QENUMSOP(REPLACE)
						QENUMSOP(INCR)
						QENUMSOP(DECR)
						QENUMSOP(ZERO)
						QENUMSOP(INCR_WRAP)
						QENUMSOP(DECR_WRAP)
						QENUMSOP(INVERT)
						;
					editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &Cubism3UIComponent::m_opCCWZFail, "Stencil CCW ZFail Op (Debug)", "Stencil CCW ZFail Operation")
						->EnumAttribute(Cubism3UIComponent::SOp::ODISABLE, "DISABLE")
						QENUMSOP(KEEP)
						QENUMSOP(REPLACE)
						QENUMSOP(INCR)
						QENUMSOP(DECR)
						QENUMSOP(ZERO)
						QENUMSOP(INCR_WRAP)
						QENUMSOP(DECR_WRAP)
						QENUMSOP(INVERT)
						;
					editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &Cubism3UIComponent::m_opCCWPass, "Stencil CCW Pass Op (Debug)", "Stencil CCW Pass Operation")
						->EnumAttribute(Cubism3UIComponent::SOp::ODISABLE, "DISABLE")
						QENUMSOP(KEEP)
						QENUMSOP(REPLACE)
						QENUMSOP(INCR)
						QENUMSOP(DECR)
						QENUMSOP(ZERO)
						QENUMSOP(INCR_WRAP)
						QENUMSOP(DECR_WRAP)
						QENUMSOP(INVERT)
						;
					#undef QENUMSOP
				}
				#endif
			}
		}

		AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
		if (behaviorContext) {
			behaviorContext->Class<Cubism3UIInterface::LoadType>("Cubism3LoadType")
				->Attribute(AZ::Script::Attributes::Category, "Cubism3")
				->Enum<Cubism3UIInterface::LoadType::Single>("Single")
				->Enum<Cubism3UIInterface::LoadType::JSON>("JSON");

			behaviorContext->Class<Cubism3UIInterface::Threading>("Cubism3Threading")
				->Attribute(AZ::Script::Attributes::Category, "Cubism3")
				->Enum<Cubism3UIInterface::Threading::NONE>("None")
				->Enum<Cubism3UIInterface::Threading::SINGLE>("Single")
				->Enum<Cubism3UIInterface::Threading::MULTI>("Multi");

			#define EBUS_METHOD(name) ->Event(#name, &Cubism3UIBus::Events::##name##)
			behaviorContext->EBus<Cubism3UIBus>("Cubism3UIBus")
				->Attribute(AZ::Script::Attributes::Category, "Cubism3")
				//load type
				EBUS_METHOD(SetLoadType)
				EBUS_METHOD(GetLoadType)
				//pathnames
				EBUS_METHOD(SetMocPathname)
				EBUS_METHOD(SetTexturePathname)
				EBUS_METHOD(GetMocPathname)
				EBUS_METHOD(GetTexturePathname)
				//JSON pathname
				EBUS_METHOD(SetJSONPathname)
				EBUS_METHOD(GetJSONPathname)
				//parameters
				EBUS_METHOD(GetParameterCount)
				EBUS_METHOD(GetParameterIdByName)
				EBUS_METHOD(GetParameterName)
				//parameters by index
				EBUS_METHOD(GetParameterMaxI)
				EBUS_METHOD(GetParameterMinI)
				EBUS_METHOD(GetParameterValueI)
				EBUS_METHOD(SetParameterValueI)
				//parameters by name
				EBUS_METHOD(GetParameterMaxS)
				EBUS_METHOD(GetParameterMinS)
				EBUS_METHOD(GetParameterValueS)
				EBUS_METHOD(SetParameterValueS)
				//parts
				EBUS_METHOD(GetPartCount)
				EBUS_METHOD(GetPartIdByName)
				EBUS_METHOD(GetPartName)
				//parts by index
				EBUS_METHOD(GetPartOpacityI)
				EBUS_METHOD(SetPartOpacityI)
				//parts by name
				EBUS_METHOD(GetPartOpacityS)
				EBUS_METHOD(SetPartOpacityS)
				//Threading
				EBUS_METHOD(SetThreading)
				EBUS_METHOD(GetThreading)
				EBUS_METHOD(SetMultiThreadLimiter)
				EBUS_METHOD(GetMultiThreadLimiter)
				;
			#undef EBUS_METHOD

			#define EBUS_METHOD(name) ->Event(#name, &Cubism3AnimationBus::Events::##name##)
			behaviorContext->EBus<Cubism3AnimationBus>("Cubism3AnimationBus")
				->Attribute(AZ::Script::Attributes::Category, "Cubism3")
				EBUS_METHOD(AddAnimation)
				EBUS_METHOD(RemoveAnimation)
				EBUS_METHOD(Loaded)
				EBUS_METHOD(Play)
				EBUS_METHOD(Stop)
				EBUS_METHOD(Pause)
				EBUS_METHOD(SetLooping)
				EBUS_METHOD(IsPlaying)
				EBUS_METHOD(IsStopped)
				EBUS_METHOD(IsPaused)
				EBUS_METHOD(IsLooping)
				EBUS_METHOD(Reset)
				EBUS_METHOD(SetWeight)
				EBUS_METHOD(GetWeight)
				;
			#undef EBUS_METHOD
		}
	}

	//dynamic listing stuff
	const AZ::Edit::ElementData* Cubism3UIComponent::GetEditData(const void* handlerPtr, const void* elementPtr, const AZ::Uuid& elementType) {
		const Cubism3UIComponent * owner = reinterpret_cast<const Cubism3UIComponent*>(handlerPtr);
		return owner->GetDataElement(elementPtr, elementType);
	}
	const AZ::Edit::ElementData* Cubism3UIComponent::GetDataElement(const void* element, const AZ::Uuid& typeUuid) const {
		auto it = m_dataElements.find(element);
		if (it != m_dataElements.end()) {
			if (it->second->m_uuid == typeUuid) {
				return &it->second->m_editData;
			}
		}

		return nullptr;
	}

	// UiRenderInterface
	void Cubism3UIComponent::Render() {
		this->m_threadMutex.Lock();
		gEnv->pRenderer->PushProfileMarker(cubism3_profileMarker);

		IRenderer *renderer = gEnv->pRenderer;

		if (this->m_modelLoaded) {
			this->PreRender();

			this->m_renderOrderChanged = false;
			for (Cubism3Drawable * d : this->m_drawables) {
				if (this->m_threading == NONE && !this->m_thread)
					d->update(this->m_model, this->m_transformUpdated, this->m_renderOrderChanged, m_opacity, m_opacity != m_prevOpacity);

				if (d->m_visible) {
					//draw masking drawable
					#ifdef ENABLE_CUBISM3_DEBUG
					if (this->m_enableMasking) {
					#endif
						if (d->m_maskCount > 0) {
							this->EnableMasking();
							for (int i = 0; i < d->m_maskCount; i++) {
								Cubism3Drawable * mask = this->m_drawables[d->m_maskIndices[i]];
								int flags = GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA;
								if (mask->m_constFlags & csmBlendAdditive) flags = GS_BLSRC_SRCALPHA | GS_BLDST_ONE;
								else if (mask->m_constFlags & csmBlendMultiplicative) flags = GS_BLDST_ONE | GS_BLDST_ONEMINUSSRCALPHA;

								if (this->m_lType==Single) renderer->SetTexture(this->m_texture ? this->m_texture->GetTextureID() : renderer->GetWhiteTextureId());
								else renderer->SetTexture((mask->m_texId >= this->m_textures.size()) ? renderer->GetWhiteTextureId() : (this->m_textures[mask->m_texId] ? this->m_textures[mask->m_texId]->GetTextureID() : renderer->GetWhiteTextureId()));

								renderer->SetState(flags | IUiRenderer::Get()->GetBaseState());
								renderer->SetColorOp(eCO_MODULATE, eCO_MODULATE, DEF_TEXARG0, DEF_TEXARG0);
								renderer->DrawDynVB(mask->m_verts, mask->m_indices, mask->m_vertCount, mask->m_indicesCount, prtTriangleList);
							}
							this->DisableMasking();
						}
					#ifdef ENABLE_CUBISM3_DEBUG
					}
					#endif

					//draw drawable
					int flags = GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA;
					if (d->m_constFlags & csmBlendAdditive) flags = GS_BLSRC_SRCALPHA | GS_BLDST_ONE;
					else if (d->m_constFlags & csmBlendMultiplicative) flags = GS_BLDST_ONE | GS_BLDST_ONEMINUSSRCALPHA;

					#ifdef ENABLE_CUBISM3_DEBUG
					if (this->m_wireframe) renderer->PushWireframeMode(R_WIREFRAME_MODE);
					#endif
					(d->m_constFlags & csmIsDoubleSided) ? renderer->SetCullMode(R_CULL_DISABLE) : renderer->SetCullMode(R_CULL_BACK);

					if (this->m_lType == Single) renderer->SetTexture(this->m_texture ? this->m_texture->GetTextureID() : renderer->GetWhiteTextureId());
					else renderer->SetTexture((d->m_texId >= this->m_textures.size()) ? renderer->GetWhiteTextureId() : (this->m_textures[d->m_texId] ? this->m_textures[d->m_texId]->GetTextureID() : renderer->GetWhiteTextureId()));

					//renderer->SetMaterialColor(this->modelR, this->modelG, this->modelB, this->modelA);

					renderer->SetState(flags | IUiRenderer::Get()->GetBaseState());
					renderer->SetColorOp(eCO_MODULATE, eCO_MODULATE, DEF_TEXARG0, DEF_TEXARG0);
					renderer->DrawDynVB(d->m_verts, d->m_indices, d->m_vertCount, d->m_indicesCount, prtTriangleList);

					#ifdef ENABLE_CUBISM3_DEBUG
					if (this->m_wireframe) renderer->PopWireframeMode();
					#endif
				}
			}

			this->PostRender();
		} else {
			//draw a blank space
			UiTransformInterface::RectPoints points;
			EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetCanvasSpacePointsNoScaleRotate, points);

			AZ::Vector2 pivot;
			EBUS_EVENT_ID_RESULT(pivot, GetEntityId(), UiTransformBus, GetPivot);

			AZ::Vector2 textureSize;

			textureSize.SetX(this->m_texture ? this->m_texture->GetWidth() : (100.0f));
			textureSize.SetY(this->m_texture ? this->m_texture->GetHeight() : (100.0f));

			AZ::Vector2 rectSize = points.GetAxisAlignedSize();
			const float scaleFactorX = rectSize.GetX() / textureSize.GetX();
			const float scaleFactorY = rectSize.GetY() / textureSize.GetY();
			const float scaleFactor = AZ::GetMin(scaleFactorX, scaleFactorY);

			AZ::Vector2 scaledTextureSize = textureSize * scaleFactor;
			AZ::Vector2 sizeDiff = scaledTextureSize - rectSize;

			AZ::Vector2 topLeftOffset(sizeDiff.GetX() * pivot.GetX(), sizeDiff.GetY() * pivot.GetY());
			AZ::Vector2 bottomRightOffset(sizeDiff.GetX() * (1.0f - pivot.GetX()), sizeDiff.GetY() * (1.0f - pivot.GetY()));

			points.TopLeft() -= topLeftOffset;
			points.BottomRight() += bottomRightOffset;
			points.TopRight() = AZ::Vector2(points.BottomRight().GetX(), points.TopLeft().GetY());
			points.BottomLeft() = AZ::Vector2(points.TopLeft().GetX(), points.BottomRight().GetY());

			// now apply scale and rotation
			EBUS_EVENT_ID(GetEntityId(), UiTransformBus, RotateAndScalePoints, points);

			// now draw the same as Stretched
			static const AZ::Vector2 uvs[4] = { AZ::Vector2(0, 0), AZ::Vector2(1, 0), AZ::Vector2(1, 1), AZ::Vector2(0, 1) };

			AZ::Color color = AZ::Color(1.0f, 1.0f, 1.0f, 1.0f);

			IDraw2d::VertexPosColUV verts[4];
			for (int i = 0; i < 4; ++i) {
				verts[i].position = points.pt[i];
				verts[i].color = color;
				verts[i].uv = uvs[i];
			}

			Draw2dHelper::GetDraw2d()->DrawQuad(
				this->m_texture ? this->m_texture->GetTextureID() : renderer->GetWhiteTextureId(),
				verts,
				GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA,
				IDraw2d::Rounding::Nearest,
				IUiRenderer::Get()->GetBaseState()
			);
		}

		gEnv->pRenderer->PopProfileMarker(cubism3_profileMarker);
		this->m_threadMutex.Unlock();
	}
	//rendering process
	///for each drawable
	/// for each mask drawable in drawable
	///   draw mask
	/// end
	/// draw drawable
	///end
	// ~UiRenderInterface
	
	// Cubism3UIBus
	//load type
	void Cubism3UIComponent::SetLoadType(LoadType lt) {
		this->m_lType = lt;
		this->OnLoadTypeChange();
	}
	//pathnames
	void Cubism3UIComponent::SetMocPathname(AZStd::string path) {
		this->m_mocPathname.SetAssetPath(path.c_str());
		this->OnMocFileChange();
	}
	void Cubism3UIComponent::SetTexturePathname(AZStd::string path) {
		this->m_imagePathname.SetAssetPath(path.c_str());
		this->OnImageFileChange();
	}

	AZStd::string Cubism3UIComponent::GetMocPathname() {
		return this->m_mocPathname.GetAssetPath();
	}
	AZStd::string Cubism3UIComponent::GetTexturePathname() {
		return this->m_imagePathname.GetAssetPath();
	}

	//json path
	void Cubism3UIComponent::SetJSONPathname(AZStd::string path) {
		this->m_jsonPathname.SetAssetPath(path.c_str());
		this->OnJSONFileChange();

	}
	AZStd::string Cubism3UIComponent::GetJSONPathname() {
		return this->m_jsonPathname.GetAssetPath();
	}

	//parameters
	int Cubism3UIComponent::GetParameterCount() { return this->m_params.size(); }
	int Cubism3UIComponent::GetParameterIdByName(AZStd::string name) { return this->m_params.find(name); }
	AZStd::string Cubism3UIComponent::GetParameterName(int index) {
		if (index < 0 || index >= this->m_params.size()) return "";
		return this->m_params.at(index)->m_name;
	}
	
	//parameters by index
	float Cubism3UIComponent::GetParameterMaxI(int index) {
		if (index < 0 || index >= this->m_params.size()) return -1;
		return this->m_params.at(index)->m_max;
	}
	float Cubism3UIComponent::GetParameterMinI(int index) {
		if (index < 0 || index >= this->m_params.size()) return -1;
		return this->m_params.at(index)->m_min;
	}
	float Cubism3UIComponent::GetParameterValueI(int index) {
		if (index < 0 || index >= this->m_params.size()) return -1;
		return *(this->m_params.at(index)->m_val);
	}
	void Cubism3UIComponent::SetParameterValueI(int index, float value) {
		if (index < 0 || index >= this->m_params.size()) return;
		*(this->m_params.at(index)->m_val) = value;
	}

	//parameters by name
	float Cubism3UIComponent::GetParameterMaxS(AZStd::string name) { return GetParameterMaxI(GetParameterIdByName(name)); }
	float Cubism3UIComponent::GetParameterMinS(AZStd::string name) { return GetParameterMinI(GetParameterIdByName(name)); }
	float Cubism3UIComponent::GetParameterValueS(AZStd::string name) { return GetParameterValueI(GetParameterIdByName(name)); }
	void Cubism3UIComponent::SetParameterValueS(AZStd::string name, float value) { return SetParameterValueI(GetParameterIdByName(name), value); }

	//parts
	int Cubism3UIComponent::GetPartCount() { return this->m_parts.size(); }
	int Cubism3UIComponent::GetPartIdByName(AZStd::string name) { return this->m_parts.find(name); }
	AZStd::string Cubism3UIComponent::GetPartName(int index) {
		if (index < 0 || index >= this->m_parts.size()) return "";
		return this->m_parts.at(index)->m_name;
	}

	//parts by index
	float Cubism3UIComponent::GetPartOpacityI(int index) {
		if (index < 0 || index >= this->m_parts.size()) return -1;
		return *(this->m_parts.at(index)->m_val);
	}
	void Cubism3UIComponent::SetPartOpacityI(int index, float value) {
		if (index < 0 || index >= this->m_parts.size()) return;
		*(this->m_parts.at(index)->m_val) = value;
	}

	//parts by name
	float Cubism3UIComponent::GetPartOpacityS(AZStd::string name) { return GetPartOpacityI(GetPartIdByName(name)); }
	void Cubism3UIComponent::SetPartOpacityS(AZStd::string name, float value) { SetPartOpacityI(GetPartIdByName(name), value); }

	//opacity
	void Cubism3UIComponent::SetOpacity(float opacity) {
		if (opacity < 0.0f) opacity = 0.0f;
		if (opacity > 1.0f) opacity = 1.0f;
		this->m_opacity = opacity;
	}

	//threading
	void Cubism3UIComponent::SetThreading(Cubism3UIInterface::Threading t) {
		if (this->m_modelLoaded) {
			if (Cubism3UIComponent::m_threadingOverride == DISABLED) {
				if (t == DISABLED) t = NONE;
				this->m_threading = t;
			} else
				this->m_threading = Cubism3UIComponent::m_threadingOverride;

			this->m_threadMutex.Lock();
			if (this->m_thread) {
				this->m_thread->Cancel();
				this->m_thread->WaitTillReady();
				this->m_thread->Stop();
				delete this->m_thread;
				this->m_thread = nullptr;
			}

			//depending if we want no threading, single thread, or multithread
			//create a new update thread.
			switch (this->m_threading) {
			case SINGLE:
				this->m_thread = new DrawableSingleThread(&this->m_drawables);
				break;
			case MULTI:
				if (this->m_threadLimiter == 1) this->m_thread = new DrawableSingleThread(&this->m_drawables);
				else this->m_thread = new DrawableMultiThread(&this->m_drawables, this->m_threadLimiter);
				break;
			}

			if (this->m_thread) {
				this->m_thread->SetModel(this->m_model);
				this->m_thread->SetAnimations(&this->m_animations);
				this->m_thread->SetParams(&this->m_params);
				this->m_thread->SetParts(&this->m_parts);
				
			/*#if defined(CUBISM3_ANIMATION_FRAMEWORK) && CUBISM3_ANIMATION_FRAMEWORK == 1
				this->m_thread->SetFloatSink(this->m_sink);
			#endif*/

				this->m_thread->SetTransformUpdate(this->m_transformUpdated);
				this->m_thread->SetDelta(gEnv->pSystem->GetITimer()->GetRealFrameTime());
				this->m_thread->SetOpacity(this->m_opacity);

				this->m_thread->Start(); //start the update thread
			}
			this->m_threadMutex.Unlock();
		}
	}
	Cubism3UIInterface::Threading Cubism3UIComponent::GetThreading() { return this->m_threading; }
	void Cubism3UIComponent::SetMultiThreadLimiter(unsigned int limiter) {
		if (limiter == 0) limiter = 1;
		this->m_threadLimiter = limiter;
		if (this->m_threading == MULTI) this->SetThreading(this->m_threading); //recreate the update thread if we are multithreading.
	}
	// ~Cubism3UIBus

	// Cubism3AnimationBus
	Cubism3Animation * Cubism3UIComponent::FindAnim(AZStd::string name) {
		auto it = this->m_animations.find(name);
		if (it != this->m_animations.end()) return it->second;
		return nullptr;
	}

	#define IFANIM(name) \
	Cubism3Animation * a = this->FindAnim(name); \
	if (a)

	bool Cubism3UIComponent::AddAnimation(AZStd::string path) {
		IFANIM(path) return false; //make sure we do not already have the animation

		a = new Cubism3Animation();
		MotionAssetRef asset;
		asset.SetAssetPath(path.c_str());

		if (m_moc) {
			a->SetParametersAndParts(&this->m_params, &this->m_parts);
			a->SetDrawables(&this->m_drawables);
		#if defined(CUBISM3_ANIMATION_FRAMEWORK) && CUBISM3_ANIMATION_FRAMEWORK == 1
			a->SetHashTable(this->m_hashTable);
			a->SetModel(this->m_model);
		#endif
		}

		a->Load(asset);

		if (a->Loaded()) {
			this->m_animations.insert(AZStd::make_pair(path, a));
			return true;
		}

		delete a;
		return false;
	}
	void Cubism3UIComponent::RemoveAnimation(AZStd::string name) {
		this->m_animations.erase(name);
	}

	bool Cubism3UIComponent::Loaded(AZStd::string name) {
		IFANIM(name) return a->Loaded();
		return false;
	}

	void Cubism3UIComponent::Play(AZStd::string name) { IFANIM(name) a->Play(); }
	void Cubism3UIComponent::Stop(AZStd::string name) { IFANIM(name) a->Stop(); }
	void Cubism3UIComponent::Pause(AZStd::string name) { IFANIM(name) a->Pause(); }

	void Cubism3UIComponent::SetLooping(AZStd::string name, bool loop) { IFANIM(name) a->SetLooping(loop); }

	bool Cubism3UIComponent::IsPlaying(AZStd::string name) {
		IFANIM(name) return a->IsPlaying();
		return false;
	}
	bool Cubism3UIComponent::IsStopped(AZStd::string name) {
		IFANIM(name) return a->IsStopped();
		return false;
	}
	bool Cubism3UIComponent::IsPaused(AZStd::string name) {
		IFANIM(name) return a->IsPaused();
		return false;
	}
	bool Cubism3UIComponent::IsLooping(AZStd::string name) {
		IFANIM(name) return a->IsLooping();
		return false;
	}

	void Cubism3UIComponent::Reset(AZStd::string name) { IFANIM(name) a->Reset(); }

	void Cubism3UIComponent::SetWeight(AZStd::string name, float weight) { IFANIM(name) a->SetWeight(weight); }
	float Cubism3UIComponent::GetWeight(AZStd::string name) {
		IFANIM(name) return a->GetWeight();
		return 0.0f;
	}

	void Cubism3UIComponent::SetFloatBlend(AZStd::string name, Cubism3AnimationFloatBlend floatBlendFunc) { IFANIM(name) a->SetFloatBlend(floatBlendFunc); }
	#undef IFANIM
	// ~Cubism3AnimationBus

	void Cubism3UIComponent::LoadObject() {
		if (this->m_lType == Single) {
			if (!this->m_mocPathname.GetAssetPath().empty()) this->LoadMoc();
			if (!this->m_imagePathname.GetAssetPath().empty()) this->LoadTexture();
		} else {
			if (!this->m_jsonPathname.GetAssetPath().empty()) this->LoadJson();
		}
	}
	void Cubism3UIComponent::ReleaseObject() {
		if (this->m_lType == Single) {
			this->FreeTexture();
			this->FreeMoc();
		} else {
			this->FreeJson();
		}
	}

	void Cubism3UIComponent::LoadMoc() {
		if (this->m_mocPathname.GetAssetPath().empty()) return;

		//read moc file
		AZ::IO::HandleType fileHandler;
		gEnv->pFileIO->Open(this->m_mocPathname.GetAssetPath().c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary, fileHandler);

		AZ::u64 mocSize;
		gEnv->pFileIO->Size(fileHandler, mocSize);

		if (fileHandler != AZ::IO::InvalidHandle && mocSize > 0) {
			//void *mocBuf = CryModuleMemalign(mocSize, csmAlignofMoc); //CryModuleMemalignFree
			void *mocBuf = azmalloc(mocSize, csmAlignofMoc);
			gEnv->pFileIO->Read(fileHandler, mocBuf, mocSize);

			gEnv->pFileIO->Close(fileHandler);

			this->m_moc = csmReviveMocInPlace(mocBuf, (unsigned int)mocSize);

			if (this->m_moc) {
				//load model
				unsigned int modelSize = csmGetSizeofModel(this->m_moc);
				//void * modelBuf = CryModuleMemalign(modelSize, csmAlignofModel); //CryModuleMemalignFree
				void * modelBuf = azmalloc(modelSize, csmAlignofModel);

				this->m_model = csmInitializeModelInPlace(this->m_moc, modelBuf, modelSize);

				if (this->m_model) {
				#if defined(CUBISM3_ANIMATION_FRAMEWORK) && CUBISM3_ANIMATION_FRAMEWORK == 1
					unsigned int hashTableSize = csmGetSizeofModelHashTable(this->m_model);
					void * hashBuf = azmalloc(hashTableSize);
					this->m_hashTable = csmInitializeModelHashTableInPlace(this->m_model, hashBuf, hashTableSize);
				#endif

					//get canvas info
					csmVector2 canvasSize;
					csmVector2 modelOrigin;
					csmReadCanvasInfo(this->m_model, &canvasSize, &modelOrigin, &this->m_modelAspect);
					this->m_modelCanvasSize = AZ::Vector2(canvasSize.X, canvasSize.Y);
					this->m_modelOrigin = AZ::Vector2(modelOrigin.X, modelOrigin.Y);

					this->m_numTextures = 0;

					/*CryLog("Origin: %f, %f", this->m_modelOrigin.GetX(), this->m_modelOrigin.GetY());
					CryLog("Canvas Size: %f, %f", this->m_modelCanvasSize.GetX(), this->m_modelCanvasSize.GetY());
					CryLog("Origin Scaled: %f, %f", this->m_modelOrigin.GetX()/this->m_modelCanvasSize.GetX(), this->m_modelOrigin.GetX()/this->m_modelCanvasSize.GetY());*/

					//get the parameters of the model
					const char** paramNames = csmGetParameterIds(this->m_model);
					if (this->m_params.size() == 0) { //if it's a brand new component
						for (int i = 0; i < csmGetParameterCount(this->m_model); i++) {
							ModelParameter * p = new ModelParameter;
							p->m_id = i;
							p->m_name = AZStd::string(paramNames[i]);
							p->m_min = csmGetParameterMinimumValues(this->m_model)[i];
							p->m_max = csmGetParameterMaximumValues(this->m_model)[i];
							p->m_val = &csmGetParameterValues(this->m_model)[i];
							p->m_animVal = *p->m_val;
							p->m_animDirty = false;

							this->m_params.m_params.push_back(p);
							this->m_params.m_idMap[p->m_name] = p->m_id;

							//editor data
							p->InitEdit();
							this->m_dataElements.insert(AZStd::make_pair(p->m_val, &p->m_ei));
						}
						this->m_params.m_params.shrink_to_fit(); //free up unused memory
					} else { //if we are loading a component
						for (int i = 0; i < this->m_params.size(); i++) {
							ModelParameter * p = this->m_params.at(i);
							this->m_params.m_idMap[p->m_name] = p->m_id;
							p->m_min = csmGetParameterMinimumValues(this->m_model)[p->m_id];
							p->m_max = csmGetParameterMaximumValues(this->m_model)[p->m_id];
							float savedVal = *p->m_val;
							p->m_val = &csmGetParameterValues(this->m_model)[i];
							*p->m_val = savedVal;
							p->m_animVal = savedVal;
							p->m_animDirty = false;

							//editor data
							p->InitEdit();
							this->m_dataElements.insert(AZStd::make_pair(p->m_val, &p->m_ei));
						}
						this->m_params.m_params.shrink_to_fit(); //free up unused memory
					}

					//get the parts of the model
					const char** partsNames = csmGetPartIds(this->m_model);
					if (this->m_parts.size() == 0) { //if it's a brand new component
						for (int i = 0; i < csmGetPartCount(this->m_model); i++) {
							ModelPart * p = new ModelPart;
							p->m_id = i;
							p->m_name = AZStd::string(partsNames[i]);
							p->m_val = &csmGetPartOpacities(this->m_model)[i];
							p->m_animVal = *p->m_val;
							p->m_animDirty = false;

							this->m_parts.m_parts.push_back(p);
							this->m_parts.m_idMap[p->m_name] = p->m_id;

							//editor data
							p->InitEdit();
							this->m_dataElements.insert(AZStd::make_pair(p->m_val, &p->m_ei));
						}
						this->m_parts.m_parts.shrink_to_fit(); //free up unused memory
					} else { //if we are loading a component
						for (int i = 0; i < this->m_parts.size(); i++) {
							ModelPart * p = this->m_parts.at(i);
							this->m_parts.m_idMap[p->m_name] = p->m_id;
							float savedVal = *p->m_val;
							p->m_val = &csmGetPartOpacities(this->m_model)[i];
							*p->m_val = savedVal;
							p->m_animVal = savedVal;
							p->m_animDirty = false;

							//editor data
							p->InitEdit();
							this->m_dataElements.insert(AZStd::make_pair(p->m_val, &p->m_ei));
						}
						this->m_parts.m_parts.shrink_to_fit(); //free up unused memory
					}

					//load drawable data
					const char** drawableNames = csmGetDrawableIds(this->m_model);
					const csmFlags* constFlags = csmGetDrawableConstantFlags(this->m_model);
					const csmFlags* dynFlags = csmGetDrawableDynamicFlags(this->m_model);
					const int* texIndices = csmGetDrawableTextureIndices(this->m_model);
					const float* opacities = csmGetDrawableOpacities(this->m_model);
					const int* maskCounts = csmGetDrawableMaskCounts(this->m_model);
					const int** masks = csmGetDrawableMasks(this->m_model);
					const int* vertCount = csmGetDrawableVertexCounts(this->m_model);
					const csmVector2** verts = csmGetDrawableVertexPositions(this->m_model);
					const csmVector2** uvs = csmGetDrawableVertexUvs(this->m_model);
					const int* numIndexes = csmGetDrawableIndexCounts(this->m_model);
					const unsigned short** indices = csmGetDrawableIndices(this->m_model);
					unsigned int drawCount = csmGetDrawableCount(this->m_model);
					const int * drawOrder = csmGetDrawableDrawOrders(this->m_model);
					const int * renderOrder = csmGetDrawableRenderOrders(this->m_model);

					float minX = 0.0f, minY = 0.0f, maxX = 0.0f, maxY = 0.0f;

					for (int i = 0; i < drawCount; ++i) {
						Cubism3Drawable * d = new Cubism3Drawable;
						d->m_name = drawableNames[i];
						d->m_id = i;
						d->m_constFlags = constFlags[i];
						d->m_dynFlags = dynFlags[i];
						d->m_texId = texIndices[i];
						if (d->m_texId > this->m_numTextures) this->m_numTextures = d->m_texId;
						d->m_drawOrder = drawOrder[i];
						d->m_renderOrder = renderOrder[i];
						d->m_opacity = opacities[i];
						d->m_packedOpacity = ColorF(1.0f, 1.0f, 1.0f, d->m_opacity).pack_argb8888();
						d->m_maskCount = maskCounts[i];

						d->m_maskIndices = new uint16[d->m_maskCount];
						for (int in = 0; in < d->m_maskCount; in++) d->m_maskIndices[in] = masks[i][in];

						d->m_transform = &this->m_transform;
						d->m_uvTransform = &this->m_uvTransform;

						d->m_vertCount = vertCount[i];
						d->m_verts = new SVF_P3F_C4B_T2F[d->m_vertCount];
						d->m_rawVerts = verts[i];
						d->m_rawUVs = uvs[i];

						for (int v = 0; v < d->m_vertCount; v++) {
							d->m_verts[v].xyz = Vec3(0.0f, 0.0f, 0.0f);
							d->m_verts[v].st = Vec2(0.0f, 0.0f);
							d->m_verts[v].color.dcolor = d->m_packedOpacity;

							if (d->m_rawVerts[v].X < minX) minX = d->m_rawVerts[v].X;
							if (d->m_rawVerts[v].Y < minY) minY = d->m_rawVerts[v].Y;

							if (d->m_rawVerts[v].X > maxX) maxX = d->m_rawVerts[v].X;
							if (d->m_rawVerts[v].Y > maxY) maxY = d->m_rawVerts[v].Y;
						}

						d->m_indicesCount = numIndexes[i];
						d->m_indices = new uint16[d->m_indicesCount];

						for (int in = 0; in < d->m_indicesCount; in++) d->m_indices[in] = indices[i][in];

						d->m_visible = d->m_dynFlags & csmIsVisible;

						this->m_drawables.push_back(d);
					}
					this->m_drawables.shrink_to_fit(); //free up unused memory

					this->m_numTextures++;

					this->m_modelSize.SetX(abs(minX) + abs(maxX));
					this->m_modelSize.SetY(abs(minY) + abs(maxY));

					//sort the drawables by render order
					AZStd::sort(
						this->m_drawables.begin(),
						this->m_drawables.end(),
						[](Cubism3Drawable * a, Cubism3Drawable * b) -> bool {
							return a->m_renderOrder < b->m_renderOrder;
						}
					);

					this->SetThreading(this->m_threading);

					for (AnimationControl a : this->m_animControls) {
						if (!a.IsLoaded()) {
							a.OnAssetChange();
						}
					}

					for (AZStd::pair<AZStd::string, Cubism3Animation*> a : this->m_animations) {
						a.second->SetParametersAndParts(&this->m_params, &this->m_parts);
						a.second->SetDrawables(&this->m_drawables);

					#if defined(CUBISM3_ANIMATION_FRAMEWORK) && CUBISM3_ANIMATION_FRAMEWORK == 1
						a.second->SetHashTable(this->m_hashTable);
						a.second->SetModel(this->m_model);
					#endif
					}

					this->m_modelLoaded = true;
				} else {
					azfree(this->m_model);
					azfree(this->m_moc);
					this->m_model = nullptr;
					this->m_moc = nullptr;
					this->m_modelLoaded = false;
					CRY_ASSERT_MESSAGE(false, "Could not initialize model data.");
				}
			} else {
				azfree(this->m_moc);
				this->m_moc = nullptr;
				this->m_modelLoaded = false;
				CRY_ASSERT_MESSAGE(false, "Could not initialize moc data.");
			}
		} else {
			gEnv->pFileIO->Close(fileHandler);
			this->m_modelLoaded = false;
			CRY_ASSERT_MESSAGE(false, "Could not open Moc file.");
		}
		this->m_prevViewport = AZ::Matrix4x4::CreateIdentity();
	}
	void Cubism3UIComponent::FreeMoc() {
		this->m_modelLoaded = false;

		for (AZStd::pair<AZStd::string, Cubism3Animation*> a : this->m_animations) {
			a.second->SetParametersAndParts(nullptr, nullptr);
			a.second->SetDrawables(nullptr);
		#if defined(CUBISM3_ANIMATION_FRAMEWORK) && CUBISM3_ANIMATION_FRAMEWORK == 1
			a.second->SetHashTable(nullptr);
			a.second->SetModel(nullptr);
		#endif
		}

		//delete the drawable update thread
		this->m_threadMutex.Lock();
		if (this->m_thread) {
			this->m_thread->Cancel();
			this->m_thread->WaitTillReady();
			this->m_thread->Stop();
			delete this->m_thread;
			this->m_thread = nullptr;
		}
		this->m_threadMutex.Unlock();

		if (this->m_drawables.size() != 0) {
			for (Cubism3Drawable * d : this->m_drawables) {
				delete d->m_verts; //delete the vector data
				delete d->m_indices; //delete the indices data
				delete d->m_maskIndices; //delete the mask indices
			}
			this->m_drawables.clear(); //clear the drawables vector
		}

		this->m_params.Clear();
		this->m_parts.Clear();
		this->m_dataElements.clear(); //clear all editor data

		//if (this->m_model) CryModuleMemalignFree(this->m_model); //free the model
		//if (this->m_moc) CryModuleMemalignFree(this->m_moc); //free the moc

		if (this->m_model) {
			azfree(this->m_model); //free the model
			this->m_model = nullptr;
		}
		if (this->m_moc) {
			azfree(this->m_moc); //free the moc
			this->m_moc = nullptr;
		}

	#if defined(CUBISM3_ANIMATION_FRAMEWORK) && CUBISM3_ANIMATION_FRAMEWORK == 1
		if (this->m_hashTable) {
			azfree(this->m_hashTable);
			this->m_hashTable = nullptr;
		}
	#endif

		this->m_model = nullptr;
		this->m_moc = nullptr;
	}

	void Cubism3UIComponent::LoadTexture() {
		if (m_lType == Single) {
			if (this->m_imagePathname.GetAssetPath().empty()) return;
			//load the texture
			this->m_texture = gEnv->pSystem->GetIRenderer()->EF_LoadTexture(this->m_imagePathname.GetAssetPath().c_str(), FT_DONT_STREAM);
			this->m_texture->AddRef(); //Release
		} else {
			//check for missing path names
			for (AzFramework::SimpleAssetReference<LmbrCentral::TextureAsset> a : m_imagesPathname) if (a.GetAssetPath().empty()) return;

			for (AzFramework::SimpleAssetReference<LmbrCentral::TextureAsset> a : m_imagesPathname) {
				ITexture * t = gEnv->pSystem->GetIRenderer()->EF_LoadTexture(a.GetAssetPath().c_str(), FT_DONT_STREAM);
				t->AddRef();
				this->m_textures.push_back(t);
			}
		}
	}
	void Cubism3UIComponent::FreeTexture() {
		SAFE_RELEASE(m_texture);
		if (this->m_textures.size() > 0) {
			for (ITexture* t : this->m_textures) {
				SAFE_RELEASE(t);
			}
			this->m_textures.clear();
			this->m_textures.shrink_to_fit();
		}
	}

	//private string functions for usage in load json
	bool endsWith(const AZStd::string& s, const AZStd::string& suffix) {
		return s.size() >= suffix.size() &&
			s.substr(s.size() - suffix.size()) == suffix;
	}
	AZStd::vector<AZStd::string> split(const AZStd::string& s, const AZStd::string& delimiter, const bool& removeEmptyEntries = false) {
		AZStd::vector<AZStd::string> tokens;

		for (size_t start = 0, end; start < s.length(); start = end + delimiter.length()) {
			size_t position = s.find(delimiter, start);
			end = position != AZStd::string::npos ? position : s.length();

			AZStd::string token = s.substr(start, end - start);
			if (!removeEmptyEntries || !token.empty()) {
				tokens.push_back(token);
			}
		}

		if (!removeEmptyEntries &&
			(s.empty() || endsWith(s, delimiter))) {
			tokens.push_back("");
		}

		return tokens;
	}

	void Cubism3UIComponent::LoadJson() {
		if (this->m_jsonPathname.GetAssetPath().empty()) return;

		AZStd::string basePath = PathUtil::GetPath(this->m_jsonPathname.GetAssetPath().c_str());
		bool error = false;

		//load the json file
		AZ::IO::HandleType fileHandler;
		gEnv->pFileIO->Open(this->m_jsonPathname.GetAssetPath().c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeText, fileHandler);

		AZ::u64 size;
		gEnv->pFileIO->Size(fileHandler, size);

		char *jsonBuff = (char *)azmalloc(size+1);
		gEnv->pFileIO->Read(fileHandler, jsonBuff, size);

		gEnv->pFileIO->Close(fileHandler);

		jsonBuff[size] = '\0';
		
		//parse the json file
		rapidjson::Document d;
		d.Parse(jsonBuff);
		azfree(jsonBuff);

		//make sure the json file is valid
		if (!d.IsObject()) {
			CRY_ASSERT_MESSAGE(false, "JSON file not valid.");
			error = true;
		}

		//version check
		if (!error) {
			if (d.HasMember("Version")) {
				if (d["Version"].GetInt() != 3) {
					CRY_ASSERT_MESSAGE(false, "incorrect model3.json version");
					error = true;
				}
			} else {
				CRY_ASSERT_MESSAGE(false, "model3.json does not have a Version definition");
				error = true;
			}
		}

		//file sanity check
		if (!error) {
			if (!d.HasMember("FileReferences")) {
				CRY_ASSERT_MESSAGE(false, "model3.json does not have a FileReferences definition");
				error = true;
			}
			if (!d["FileReferences"].HasMember("Moc")) {
				CRY_ASSERT_MESSAGE(false, "model3.json does not have a Moc definition");
				error = true;
			}
			if (!d["FileReferences"].HasMember("Textures")) {
				CRY_ASSERT_MESSAGE(false, "model3.json does not have a Textures definition");
				error = true;
			}
		}

		if (!error) {
			//load the moc
			AZStd::string MocFileName = d["FileReferences"]["Moc"].GetString();
			AZStd::string MocPath = basePath + MocFileName;
			const char * sanitizedPath = PathUtil::ToNativePath(MocPath.c_str());

			CLOG("[Cubism3] Moc Path: %s", sanitizedPath);

			//verify moc exists
			if (gEnv->pFileIO->Exists(sanitizedPath)) {
				this->m_mocPathname.SetAssetPath(sanitizedPath);
				this->LoadMoc();
			} else {
				CRY_ASSERT_MESSAGE(false, "Moc file listed in JSON does not exist.");
				error = true;
			}

			if (!error) {
				//check to make sure we have the right number of textures
				unsigned int textureListSize = d["FileReferences"]["Textures"].Size();

				if (textureListSize != this->m_numTextures) {
					CRY_ASSERT_MESSAGE(false, "number of textures listed in model3.json does not match moc's number of textures");
					error = true;
				}

				if (!error) {
					AZStd::vector<AZStd::string> assetFilter = split(LmbrCentral::TextureAsset::GetFileFilter(), "; ", true);

					//verify textures exist
					for (int i = 0; i < textureListSize; i++) { //for each texture listing
						string textureFN = d["FileReferences"]["Textures"][i].GetString();
						PathUtil::RemoveExtension(textureFN);
						AZStd::string textureFileName = textureFN.c_str();
						
						bool found = false;
						for (AZStd::string ext : assetFilter) { //for each asset extension
							AZStd::string texturePath = basePath + textureFileName + ext.substr(1);
							const char * sanitizedPath = PathUtil::ToNativePath(texturePath.c_str());

							CLOG("[Cubism3] Texture Path: %s", sanitizedPath);

							if (gEnv->pFileIO->Exists(sanitizedPath)) {
								found = true;
								break;
							}
						}

						if (!found) {
							AZStd::string errorstr = "Texture listed in JSON does not exist: " + textureFileName;
							CRY_ASSERT_MESSAGE(false, errorstr.c_str());
							error = true;
						}
					}

					//load the textures
					if (!error) {
						for (int i = 0; i < textureListSize; i++) {
							AZStd::string textureFileName = d["FileReferences"]["Textures"][i].GetString();
							AZStd::string texturePath = basePath + textureFileName;
							const char * sanitizedPath = PathUtil::ToNativePath(texturePath.c_str());

							AzFramework::SimpleAssetReference<LmbrCentral::TextureAsset> asset;
							asset.SetAssetPath(sanitizedPath);

							this->m_imagesPathname.push_back(asset);
						}
						this->m_imagesPathname.shrink_to_fit();
						this->LoadTexture();
					}
				}
			}
		}

		if (error) this->FreeJson();
	}
	void Cubism3UIComponent::FreeJson() {
		this->m_mocPathname.SetAssetPath("");
		this->m_imagesPathname.clear();
		this->m_imagesPathname.shrink_to_fit();
		this->m_imagePathname.SetAssetPath("");
		this->FreeMoc();
		this->FreeTexture();
	}

	void Cubism3UIComponent::OnMocFileChange() {
		this->FreeMoc();
		if (!this->m_mocPathname.GetAssetPath().empty()) this->LoadMoc();
		this->m_prevViewport = AZ::Matrix4x4::CreateIdentity();
	}
	void Cubism3UIComponent::OnImageFileChange() {
		this->FreeTexture();
		if (!this->m_imagePathname.GetAssetPath().empty()) this->LoadTexture();
	}
	void Cubism3UIComponent::OnJSONFileChange() {
		this->FreeJson();
		this->LoadJson();
		this->m_prevViewport = AZ::Matrix4x4::CreateIdentity();
	}
	void Cubism3UIComponent::OnThreadingChange() {
		this->SetThreading(this->m_threading);
	}
	void Cubism3UIComponent::OnFillChange() {
		this->m_prevViewport = AZ::Matrix4x4::CreateIdentity();
	}
	void Cubism3UIComponent::OnLoadTypeChange() {
		this->m_mocPathname.SetAssetPath("");
		this->m_imagePathname.SetAssetPath("");
		this->m_jsonPathname.SetAssetPath("");
		this->m_imagesPathname.clear();
		this->m_imagesPathname.shrink_to_fit();

		if (m_lType == Single) {
			OnMocFileChange();
			OnImageFileChange();
		} else if (m_lType == JSON) {
			OnJSONFileChange();
		} else {
			CRY_ASSERT_MESSAGE(false, "unhandled load type");
		}
	}
	void Cubism3UIComponent::OnAnimControlsChange() {
		this->m_animControls.at(this->m_animControls.size() - 1).SetUIComponent(this);
	}

	void Cubism3UIComponent::PreRender() {
		//threading
		//if we are threading the drawable updates
		if (this->m_threading != NONE && this->m_thread) this->m_thread->WaitTillReady(); //wait until the update thread is ready.

		if (this->m_threading == NONE && !this->m_thread) { //if we are not threading
			//update animation
			//CLOG("FrameTime: %f", gEnv->pSystem->GetITimer()->GetFrameTime());
			//CLOG("RealFrameTime: %f", gEnv->pSystem->GetITimer()->GetRealFrameTime());
			float frametime = gEnv->pSystem->GetITimer()->GetRealFrameTime();
			for (AZStd::pair<AZStd::string, Cubism3Animation*> a : this->m_animations) a.second->Tick(frametime);

		#if !defined(CUBISM3_ANIMATION_FRAMEWORK) || CUBISM3_ANIMATION_FRAMEWORK == 0
			//sync animation
			this->m_params.SyncAnimations();
			this->m_parts.SyncAnimations();
		#endif

			//update the model
			csmUpdateModel(this->m_model);
		}

		///have this also update in update thread if threading?
		UiTransform2dInterface::Anchors anchors;
		EBUS_EVENT_ID_RESULT(anchors, GetEntityId(), UiTransform2dBus, GetAnchors);
		UiTransform2dInterface::Offsets offsets;
		EBUS_EVENT_ID_RESULT(offsets, GetEntityId(), UiTransform2dBus, GetOffsets);
		AZ::Matrix4x4 viewport;
		EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetTransformToViewport, viewport);

		//update transform as nessessary.
		this->m_transformUpdated = false;
		if (this->m_prevAnchors != anchors || this->m_prevOffsets != offsets || this->m_prevViewport != viewport) {
			this->m_prevViewport = viewport;
			this->m_prevAnchors = anchors;
			this->m_prevOffsets = offsets;
			this->m_transformUpdated = true;

			UiTransformInterface::Rect rect;
			EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetCanvasSpaceRectNoScaleRotate, rect);

			UiTransformInterface::RectPoints points;
			EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetCanvasSpacePointsNoScaleRotate, points);

			this->m_transform = viewport *
				AZ::Matrix4x4::CreateTranslation( //translate the object
					AZ::Vector3(
						points.pt[UiTransformInterface::RectPoints::Corner_TopLeft].GetX() + (rect.GetWidth() / 2),
						points.pt[UiTransformInterface::RectPoints::Corner_TopLeft].GetY() + (rect.GetHeight() / 2),
						0.0f
					)
				);

			float scaleX = 1.0f;
			float scaleY = 1.0f;

			if (this->m_fill) {
				scaleX = rect.GetWidth() / this->m_modelSize.GetX();
				scaleY = rect.GetHeight() / this->m_modelSize.GetY();
			} else {
				float modelSizeAspect = this->m_modelSize.GetX() / this->m_modelSize.GetY();

				scaleX = (rect.GetHeight() * modelSizeAspect) / this->m_modelSize.GetX();
				scaleY = rect.GetHeight() / this->m_modelSize.GetY();
			}

			//scale the object
			this->m_transform.MultiplyByScale(AZ::Vector3(scaleX, -scaleY, 1.0f));
		}
	}
	void Cubism3UIComponent::PostRender() {
		//threading
		if (this->m_threading != NONE && this->m_thread) { //if update is threaded
			this->m_thread->SetTransformUpdate(this->m_transformUpdated); //notify that the transform has updated or not
			this->m_thread->SetDelta(gEnv->pSystem->GetITimer()->GetRealFrameTime());
			this->m_thread->SetOpacity(this->m_opacity);
			this->m_thread->Notify(); //wake up the update thread.
		} else {
			csmResetDrawableDynamicFlags(this->m_model);
			if (this->m_renderOrderChanged)
				AZStd::sort(
					this->m_drawables.begin(),
					this->m_drawables.end(),
					[](Cubism3Drawable * a, Cubism3Drawable * b) -> bool {
						return a->m_renderOrder < b->m_renderOrder;
					}
				);
			if (this->m_opacity != this->m_prevOpacity) this->m_prevOpacity = this->m_opacity;
		}
	}

	void Cubism3UIComponent::EnableMasking() {
		this->m_priorBaseState = IUiRenderer::Get()->GetBaseState();

		#ifndef ENABLE_CUBISM3_DEBUG
		int alphaTest = GS_ALPHATEST_GREATER;
		#else
		int alphaTest = 0;
		if (this->m_useAlphaTest != ATDISABLE) alphaTest = m_useAlphaTest;
		#endif

		#ifndef ENABLE_CUBISM3_DEBUG
		int colorMask = GS_COLMASK_NONE;
		#else
		int colorMask = 0;
		if (this->m_useColorMask != CMDISABLE) colorMask = m_useColorMask;
		#endif


		#ifdef ENABLE_CUBISM3_DEBUG
		if (this->m_enableMasking) {
		#endif
			// set up for stencil write
			const uint32 stencilRef = IUiRenderer::Get()->GetStencilRef();
			const uint32 stencilMask = 0xFF;
			const uint32 stencilWriteMask = 0xFF;
			
			#ifndef ENABLE_CUBISM3_DEBUG
			const int32 stencilState = STENC_FUNC(FSS_STENCFUNC_EQUAL) | STENCOP_FAIL(FSS_STENCOP_KEEP) | STENCOP_ZFAIL(FSS_STENCOP_KEEP);
			#else
			int32 stencilState = 0;

			if (m_stencilFunc != SFunc::FDISABLE) stencilState |= STENC_FUNC(m_stencilFunc);
			if (m_stencilCCWFunc != SFunc::FDISABLE) stencilState |= STENC_CCW_FUNC(m_stencilCCWFunc);

			if (m_opFail != SOp::ODISABLE) stencilState |= STENCOP_FAIL(m_opFail);
			if (m_opZFail != SOp::ODISABLE) stencilState |= STENCOP_ZFAIL(m_opZFail);
			if (m_opPass != SOp::ODISABLE) stencilState |= STENCOP_PASS(m_opPass);

			if (m_opCCWFail != SOp::ODISABLE) stencilState |= STENCOP_CCW_FAIL(m_opCCWFail);
			if (m_opCCWZFail != SOp::ODISABLE) stencilState |= STENCOP_CCW_ZFAIL(m_opCCWZFail);
			if (m_opCCWPass != SOp::ODISABLE) stencilState |= STENCOP_CCW_PASS(m_opCCWPass);
			if (m_sTwoSided) stencilState |= FSS_STENCIL_TWOSIDED;
			#endif

			gEnv->pRenderer->SetStencilState(stencilState, stencilRef, stencilMask, stencilWriteMask);

			IUiRenderer::Get()->SetBaseState(this->m_priorBaseState | GS_STENCIL | alphaTest | colorMask);

		#ifdef ENABLE_CUBISM3_DEBUG
		} else {
			if (colorMask || alphaTest) {
				IUiRenderer::Get()->SetBaseState(this->m_priorBaseState | colorMask | alphaTest);
			}
		}
		#endif
	}
	void Cubism3UIComponent::DisableMasking() {
		#ifdef ENABLE_CUBISM3_DEBUG
		if (this->m_enableMasking) {
		#endif
			// turn off stencil write and turn on stencil test
			const uint32 stencilRef = IUiRenderer::Get()->GetStencilRef();
			const uint32 stencilMask = 0xFF;
			const uint32 stencilWriteMask = 0x00;
			
			#ifndef ENABLE_CUBISM3_DEBUG
			const int32 stencilState = STENC_FUNC(FSS_STENCFUNC_EQUAL) | STENCOP_FAIL(FSS_STENCOP_KEEP) | STENCOP_ZFAIL(FSS_STENCOP_KEEP) | STENCOP_PASS(FSS_STENCOP_KEEP);
			#else
			int32 stencilState = 0;

			if (m_stencilFunc != SFunc::FDISABLE) stencilState |= STENC_FUNC(m_stencilFunc);
			if (m_stencilCCWFunc != SFunc::FDISABLE) stencilState |= STENC_CCW_FUNC(m_stencilCCWFunc);

			if (m_opFail != SOp::ODISABLE) stencilState |= STENCOP_FAIL(m_opFail);
			if (m_opZFail != SOp::ODISABLE) stencilState |= STENCOP_ZFAIL(m_opZFail);
			if (m_opPass != SOp::ODISABLE) stencilState |= STENCOP_PASS(m_opPass);

			if (m_opCCWFail != SOp::ODISABLE) stencilState |= STENCOP_CCW_FAIL(m_opCCWFail);
			if (m_opCCWZFail != SOp::ODISABLE) stencilState |= STENCOP_CCW_ZFAIL(m_opCCWZFail);
			if (m_opCCWPass != SOp::ODISABLE) stencilState |= STENCOP_CCW_PASS(m_opCCWPass);
			if (m_sTwoSided) stencilState |= FSS_STENCIL_TWOSIDED;
			#endif

			gEnv->pRenderer->SetStencilState(stencilState, stencilRef, stencilMask, stencilWriteMask);

			IUiRenderer::Get()->SetBaseState(this->m_priorBaseState);
		#ifdef ENABLE_CUBISM3_DEBUG
		} else {
			IUiRenderer::Get()->SetBaseState(this->m_priorBaseState);
		}
		#endif
	}
}