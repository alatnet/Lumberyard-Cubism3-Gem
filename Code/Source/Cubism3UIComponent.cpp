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
		this->transform = this->prevViewport =  AZ::Matrix4x4::CreateIdentity();
		this->transformUpdated = false;
		this->uvTransform = AZ::Matrix4x4::CreateIdentity() * AZ::Matrix4x4::CreateScale(AZ::Vector3(1.0f, -1.0f, 1.0f)); //flip the texture on the y vector
		
		this->prevAnchors = UiTransform2dInterface::Anchors(0.4f,0.4f,0.4f,0.4f);
		this->prevOffsets = UiTransform2dInterface::Offsets(-40,-40,40,40);

		this->moc = nullptr;
		this->model = nullptr;
		this->texture = nullptr;

		this->modelLoaded = false;

		this->m_threading = NONE;
		this->tJob = nullptr;

		this->threadLimiter = CUBISM3_MULTITHREAD_LIMITER;

		#ifdef ENABLE_CUBISM3_DEBUG
		this->wireframe = false;
		#endif

		this->modelSize = AZ::Vector2(0.0f, 0.0f);
		this->fill = false;

		this->enableMasking = true;
		//this->drawMaskVisualBehindChildren = false;
		//this->drawMaskVisualInFrontOfChildren = false;
		this->useAlphaTest = false;

		this->lType = Single;

		#ifdef ENABLE_CUBISM3_DEBUG
		//stencil stuff
		this->stencilFunc = SFunc::EQUAL;
		this->stencilCCWFunc = SFunc::FDISABLE;
		this->sTwoSided = false;

		this->opFail = SOp::KEEP;
		this->opZFail = SOp::KEEP;
		this->opPass = SOp::ODISABLE;

		this->opCCWFail = SOp::ODISABLE;
		this->opCCWZFail = SOp::ODISABLE;
		this->opCCWPass = SOp::ODISABLE;
		#endif
	}

	Cubism3UIComponent::~Cubism3UIComponent() {
		this->ReleaseObject();
	}

	void Cubism3UIComponent::Init() {
		this->LoadObject();
	}

	void Cubism3UIComponent::Activate() {
		UiRenderBus::Handler::BusConnect(m_entity->GetId());
		Cubism3UIBus::Handler::BusConnect(m_entity->GetId());
	}

	void Cubism3UIComponent::Deactivate() {
		UiRenderBus::Handler::BusDisconnect();
		Cubism3UIBus::Handler::BusDisconnect();
	}

	void Cubism3UIComponent::Reflect(AZ::ReflectContext* context) {
		AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

		if (serializeContext) {
			ModelParameter::Reflect(serializeContext);
			ModelParametersGroup::Reflect(serializeContext);
			ModelPart::Reflect(serializeContext);
			ModelPartsGroup::Reflect(serializeContext);

			#define QFIELD(x) ->Field(#x,&Cubism3UIComponent::##x##) //quick field
			serializeContext->Class<Cubism3UIComponent, AZ::Component>()
				->Version(1)
				->Field("LoadType", &Cubism3UIComponent::lType)
				->Field("MocFile", &Cubism3UIComponent::m_mocPathname)
				->Field("ImageFile", &Cubism3UIComponent::m_imagePathname)
				->Field("JSONFile", &Cubism3UIComponent::m_jsonPathname)
				->Field("Fill", &Cubism3UIComponent::fill)
				->Field("Masking", &Cubism3UIComponent::enableMasking)
				->Field("Masking_Alpha", &Cubism3UIComponent::useAlphaTest)
				->Field("Params", &Cubism3UIComponent::params)
				->Field("Parts", &Cubism3UIComponent::parts)

				//->Field("testMotion", &Cubism3UIComponent::m_testMotion)

				#ifdef ENABLE_CUBISM3_DEBUG
				->Field("Wireframe", &Cubism3UIComponent::wireframe)
				->Field("Threading", &Cubism3UIComponent::m_threading)
				QFIELD(stencilFunc)
				QFIELD(stencilCCWFunc)
				QFIELD(sTwoSided)
				QFIELD(opFail)
				QFIELD(opZFail)
				QFIELD(opPass)
				QFIELD(opCCWFail)
				QFIELD(opCCWZFail)
				QFIELD(opCCWPass)
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

				editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &Cubism3UIComponent::lType, "Load Type", "What type of Cubism3 Model to load.")
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

				editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &Cubism3UIComponent::fill, "Fill", "Fill the model to the element's dimentions")
					->Attribute(AZ::Edit::Attributes::ChangeNotify, &Cubism3UIComponent::OnFillChange);

				editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &Cubism3UIComponent::enableMasking, "Enable masking",
					"Enable masking usage of the Cubism3 model.");

				editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &Cubism3UIComponent::useAlphaTest, "Use alpha test",
					"Check this box to use the alpha channel in the mask visual's texture to define the mask.");

				/*editInfo->DataElement(0, &Cubism3UIComponent::m_testMotion, "Test Motion Asset", "")
					->Attribute(AZ::Edit::Attributes::ChangeNotify, &Cubism3UIComponent::testMotionCN);*/

				editInfo->SetDynamicEditDataProvider(&Cubism3UIComponent::GetEditData);

				//parameters group
				{
					editInfo->DataElement(0, &Cubism3UIComponent::params, "Parameters", "List of parameters of the model.");

					ec->Class<ModelParametersGroup>("A Models parameter group", "This is a model group")
						->ClassElement(AZ::Edit::ClassElements::EditorData, "ModelParametersGroup's class attributes.")
						->Attribute(AZ::Edit::Attributes::NameLabelOverride, &ModelParametersGroup::m_name)
						->Attribute(AZ::Edit::Attributes::AutoExpand, false)
						->DataElement(0, &ModelParametersGroup::m_params, "m_params", "Parameters in this property group")
						->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);

					ec->Class<ModelParameter>("Parameter", "A Model Parameter")
						->ClassElement(AZ::Edit::ClassElements::EditorData, "Parameter Attribute.")
						->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
						->DataElement(AZ::Edit::UIHandlers::Slider, &ModelParameter::val, "val", "A float");
				}

				//part group
				{
					editInfo->DataElement(0, &Cubism3UIComponent::parts, "Parameters", "List of parts of the model.");

					ec->Class<ModelPartsGroup>("A Parts opacity group", "This is a model group")
						->ClassElement(AZ::Edit::ClassElements::EditorData, "ModelPartsGroup's class attributes.")
						->Attribute(AZ::Edit::Attributes::NameLabelOverride, &ModelPartsGroup::m_name)
						->Attribute(AZ::Edit::Attributes::AutoExpand, false)
						->DataElement(0, &ModelPartsGroup::m_parts, "m_parts", "Parts in this property group")
						->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);

					ec->Class<ModelPart>("Part", "A Part Opacity")
						->ClassElement(AZ::Edit::ClassElements::EditorData, "Part Opacity.")
						->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
						->DataElement(AZ::Edit::UIHandlers::Slider, &ModelPart::val, "val", "A float");
				}
				
				//debug group
				#ifdef ENABLE_CUBISM3_DEBUG
				{
					editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Debugging")
						->Attribute(AZ::Edit::Attributes::AutoExpand, false);

					editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &Cubism3UIComponent::wireframe, "Wireframe (Debug)", "Wireframe Mode");

					editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &Cubism3UIComponent::m_threading, "Threading (Debug)", "Threading Mode")
						->Attribute(AZ::Edit::Attributes::ChangeNotify, &Cubism3UIComponent::OnThreadingChange)
						->EnumAttribute(Cubism3UIInterface::Threading::NONE, "None")
						->EnumAttribute(Cubism3UIInterface::Threading::SINGLE, "Single")
						->EnumAttribute(Cubism3UIInterface::Threading::MULTI, "Multi");

					//Debug Stencil
					#define QENUMSFUNC(x) ->EnumAttribute(Cubism3UIComponent::SFunc::##x##, #x)
					editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &Cubism3UIComponent::stencilFunc, "Stencil Func (Debug)", "Stencil Function")
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
					editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &Cubism3UIComponent::stencilCCWFunc, "Stencil CCW Func (Debug)", "Stencil CCW Function")
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
					editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &Cubism3UIComponent::opFail, "Stencil Fail Op (Debug)", "Stencil Fail Operation")
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
					editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &Cubism3UIComponent::opZFail, "Stencil ZFail Op (Debug)", "Stencil ZFail Operation")
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
					editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &Cubism3UIComponent::opPass, "Stencil Pass Op (Debug)", "Stencil Pass Operation")
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

					editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &Cubism3UIComponent::opCCWFail, "Stencil CCW Fail Op (Debug)", "Stencil CCW Fail Operation")
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
					editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &Cubism3UIComponent::opCCWZFail, "Stencil CCW ZFail Op (Debug)", "Stencil CCW ZFail Operation")
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
					editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &Cubism3UIComponent::opCCWPass, "Stencil CCW Pass Op (Debug)", "Stencil CCW Pass Operation")
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
			behaviorContext->Class<Cubism3UIComponent>("Cubisim3UI")
				->Enum<Cubism3UIInterface::LoadType::Single>("ltSingle")
				->Enum<Cubism3UIInterface::LoadType::JSON>("ltJSON")
				->Enum<Cubism3UIInterface::Threading::NONE>("tNone")
				->Enum<Cubism3UIInterface::Threading::SINGLE>("tSingle")
				->Enum<Cubism3UIInterface::Threading::MULTI>("tMulti")
				;

			#define EBUS_METHOD(name) ->Event(#name, &Cubism3UIBus::Events::##name##)
			behaviorContext->EBus<Cubism3UIBus>("Cubism3UIBus")
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
		this->threadMutex.Lock();
		gEnv->pRenderer->PushProfileMarker(cubism3_profileMarker);

		IRenderer *renderer = gEnv->pRenderer;

		if (this->modelLoaded) {
			this->PreRender();

			this->renderOrderChanged = false;
			for (Drawable * d : this->drawables) {
				if (this->m_threading == NONE && !this->tJob)
					d->update(this->model, this->transformUpdated, this->renderOrderChanged);

				if (d->visible) {
					//draw masking drawable
					if (this->enableMasking) {
						if (d->maskCount > 0) {
							this->EnableMasking();
							for (int i = 0; i < d->maskCount; i++) {
								Drawable * mask = this->drawables[d->maskIndices[i]];
								int flags = GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA;
								if (mask->constFlags & csmBlendAdditive) flags = GS_BLSRC_SRCALPHA | GS_BLDST_ONE;
								else if (mask->constFlags & csmBlendMultiplicative) flags = GS_BLDST_ONE | GS_BLDST_ONEMINUSSRCALPHA;

								if (this->lType==Single) renderer->SetTexture(this->texture ? this->texture->GetTextureID() : renderer->GetWhiteTextureId());
								else renderer->SetTexture((mask->texId >= this->textures.size()) ? renderer->GetWhiteTextureId() : (this->textures[mask->texId] ? this->textures[mask->texId]->GetTextureID() : renderer->GetWhiteTextureId()));

								renderer->SetState(flags | IUiRenderer::Get()->GetBaseState());
								renderer->SetColorOp(eCO_MODULATE, eCO_MODULATE, DEF_TEXARG0, DEF_TEXARG0);
								renderer->DrawDynVB(mask->verts, mask->indices, mask->vertCount, mask->indicesCount, prtTriangleList);
							}
							this->DisableMasking();
						}
					}

					//draw drawable
					int flags = GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA;
					if (d->constFlags & csmBlendAdditive) flags = GS_BLSRC_SRCALPHA | GS_BLDST_ONE;
					else if (d->constFlags & csmBlendMultiplicative) flags = GS_BLDST_ONE | GS_BLDST_ONEMINUSSRCALPHA;

					#ifdef ENABLE_CUBISM3_DEBUG
					if (this->wireframe) renderer->PushWireframeMode(R_WIREFRAME_MODE);
					#endif
					(d->constFlags & csmIsDoubleSided) ? renderer->SetCullMode(R_CULL_DISABLE) : renderer->SetCullMode(R_CULL_BACK);

					if (this->lType == Single) renderer->SetTexture(this->texture ? this->texture->GetTextureID() : renderer->GetWhiteTextureId());
					else renderer->SetTexture((d->texId >= this->textures.size()) ? renderer->GetWhiteTextureId() : (this->textures[d->texId] ? this->textures[d->texId]->GetTextureID() : renderer->GetWhiteTextureId()));

					renderer->SetState(flags | IUiRenderer::Get()->GetBaseState());
					renderer->SetColorOp(eCO_MODULATE, eCO_MODULATE, DEF_TEXARG0, DEF_TEXARG0);
					renderer->DrawDynVB(d->verts, d->indices, d->vertCount, d->indicesCount, prtTriangleList);

					#ifdef ENABLE_CUBISM3_DEBUG
					if (this->wireframe) renderer->PopWireframeMode();
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

			textureSize.SetX(this->texture ? this->texture->GetWidth() : (100.0f));
			textureSize.SetY(this->texture ? this->texture->GetHeight() : (100.0f));

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
				this->texture ? this->texture->GetTextureID() : renderer->GetWhiteTextureId(),
				verts,
				GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA,
				IDraw2d::Rounding::Nearest,
				IUiRenderer::Get()->GetBaseState()
			);
		}

		gEnv->pRenderer->PopProfileMarker(cubism3_profileMarker);
		this->threadMutex.Unlock();
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
		this->lType = lt;
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
	int Cubism3UIComponent::GetParameterCount() { return this->params.size(); }
	int Cubism3UIComponent::GetParameterIdByName(AZStd::string name) { return this->params.find(name); }
	AZStd::string Cubism3UIComponent::GetParameterName(int index) {
		if (index < 0 || index >= this->params.size()) return "";
		return this->params.at(index)->name;
	}
	
	//parameters by index
	float Cubism3UIComponent::GetParameterMaxI(int index) {
		if (index < 0 || index >= this->params.size()) return -1;
		return this->params.at(index)->max;
	}
	float Cubism3UIComponent::GetParameterMinI(int index) {
		if (index < 0 || index >= this->params.size()) return -1;
		return this->params.at(index)->min;
	}
	float Cubism3UIComponent::GetParameterValueI(int index) {
		if (index < 0 || index >= this->params.size()) return -1;
		return *(this->params.at(index)->val);
	}
	void Cubism3UIComponent::SetParameterValueI(int index, float value) {
		if (index < 0 || index >= this->params.size()) return;
		*(this->params.at(index)->val) = value;
	}

	//parameters by name
	float Cubism3UIComponent::GetParameterMaxS(AZStd::string name) { return GetParameterMaxI(GetParameterIdByName(name)); }
	float Cubism3UIComponent::GetParameterMinS(AZStd::string name) { return GetParameterMinI(GetParameterIdByName(name)); }
	float Cubism3UIComponent::GetParameterValueS(AZStd::string name) { return GetParameterValueI(GetParameterIdByName(name)); }
	void Cubism3UIComponent::SetParameterValueS(AZStd::string name, float value) { return SetParameterValueI(GetParameterIdByName(name), value); }

	//parts
	int Cubism3UIComponent::GetPartCount() { return this->parts.size(); }
	int Cubism3UIComponent::GetPartIdByName(AZStd::string name) { return this->parts.find(name); }
	AZStd::string Cubism3UIComponent::GetPartName(int index) {
		if (index < 0 || index >= this->parts.size()) return "";
		return this->parts.at(index)->name;
	}

	//parts by index
	float Cubism3UIComponent::GetPartOpacityI(int index) {
		if (index < 0 || index >= this->parts.size()) return -1;
		return *(this->parts.at(index)->val);
	}
	void Cubism3UIComponent::SetPartOpacityI(int index, float value) {
		if (index < 0 || index >= this->parts.size()) return;
		*(this->parts.at(index)->val) = value;
	}

	//parts by name
	float Cubism3UIComponent::GetPartOpacityS(AZStd::string name) { return GetPartOpacityI(GetPartIdByName(name)); }
	void Cubism3UIComponent::SetPartOpacityS(AZStd::string name, float value) { SetPartOpacityI(GetPartIdByName(name), value); }

	//threading
	void Cubism3UIComponent::SetThreading(Cubism3UIInterface::Threading t) {
		if (this->modelLoaded) {
			if (Cubism3UIComponent::m_threadingOverride == DISABLED) {
				if (t == DISABLED) t = NONE;
				this->m_threading = t;
			} else
				this->m_threading = Cubism3UIComponent::m_threadingOverride;

			this->threadMutex.Lock();
			if (this->tJob) {
				this->tJob->Cancel();
				this->tJob->WaitTillReady();
				delete this->tJob;
				this->tJob = nullptr;
			}

			//depending if we want no threading, single thread, or multithread
			//create a new update thread.
			switch (this->m_threading) {
			case SINGLE:
				this->tJob = new DrawableSingleThread(&this->drawables);
				this->tJob->SetModel(this->model);
				break;
			case MULTI:
				this->tJob = new DrawableMultiThread(&this->drawables, this->threadLimiter);
				this->tJob->SetModel(this->model);
				break;
			}

			if (this->tJob) this->tJob->Start(); //start the update thread
			this->threadMutex.Unlock();
		}
	}
	Cubism3UIInterface::Threading Cubism3UIComponent::GetThreading() { return this->m_threading; }
	void Cubism3UIComponent::SetMultiThreadLimiter(unsigned int limiter) {
		if (limiter == 0) limiter = 1;
		this->threadLimiter = limiter;
		if (this->m_threading == MULTI) this->SetThreading(this->m_threading); //recreate the update thread if we are multithreading.
	}
	// ~Cubism3UIBus

	void Cubism3UIComponent::LoadObject() {
		if (this->lType == Single) {
			if (!this->m_mocPathname.GetAssetPath().empty()) this->LoadMoc();
			if (!this->m_imagePathname.GetAssetPath().empty()) this->LoadTexture();
		} else {
			if (!this->m_jsonPathname.GetAssetPath().empty()) this->LoadJson();
		}

		#ifdef USE_CUBISM3_ANIM_FRAMEWORK
		this->LoadAnimation();
		#endif
	}
	void Cubism3UIComponent::ReleaseObject() {
		#ifdef USE_CUBISM3_ANIM_FRAMEWORK
		this->FreeAnimation();
		#endif

		if (this->lType == Single) {
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
			void *mocBuf = CryModuleMemalign(mocSize, csmAlignofMoc); //CryModuleMemalignFree
			gEnv->pFileIO->Read(fileHandler, mocBuf, mocSize);

			gEnv->pFileIO->Close(fileHandler);

			this->moc = csmReviveMocInPlace(mocBuf, (unsigned int)mocSize);

			if (this->moc) {
				//load model
				unsigned int modelSize = csmGetSizeofModel(this->moc);
				void * modelBuf = CryModuleMemalign(modelSize, csmAlignofModel); //CryModuleMemalignFree

				this->model = csmInitializeModelInPlace(this->moc, modelBuf, modelSize);

				if (this->model) {
					//get canvas info
					csmVector2 canvasSize;
					csmVector2 modelOrigin;
					csmReadCanvasInfo(this->model, &canvasSize, &modelOrigin, &this->modelAspect);
					this->modelCanvasSize = AZ::Vector2(canvasSize.X, canvasSize.Y);
					this->modelOrigin = AZ::Vector2(modelOrigin.X, modelOrigin.Y);

					this->numTextures = 0;

					/*CryLog("Origin: %f, %f", this->modelOrigin.GetX(), this->modelOrigin.GetY());
					CryLog("Canvas Size: %f, %f", this->modelCanvasSize.GetX(), this->modelCanvasSize.GetY());
					CryLog("Origin Scaled: %f, %f", this->modelOrigin.GetX()/this->modelCanvasSize.GetX(), this->modelOrigin.GetX()/this->modelCanvasSize.GetY());*/

					//get the parameters of the model
					const char** paramNames = csmGetParameterIds(this->model);

					for (int i = 0; i < csmGetParameterCount(this->model); i++) {
						ModelParameter * p = new ModelParameter;
						p->id = i;
						p->name = AZStd::string(paramNames[i]);
						p->min = csmGetParameterMinimumValues(this->model)[i];
						p->max = csmGetParameterMaximumValues(this->model)[i];
						p->val = &csmGetParameterValues(this->model)[i];

						this->params.m_params.push_back(p);
						this->params.m_idMap[p->name] = p->id;

						//editor data
						p->InitEdit();
						//ElementInfo ei;
						//ei.m_editData = p->ed;
						//ei.m_uuid = AZ::SerializeTypeInfo<float>::GetUuid();
						this->m_dataElements.insert(AZStd::make_pair(p->val, &p->ei));
					}
					this->params.m_params.shrink_to_fit(); //free up unused memory

					//get the parts of the model
					const char** partsNames = csmGetPartIds(this->model);
					for (int i = 0; i < csmGetPartCount(this->model); i++) {
						ModelPart * p = new ModelPart;
						p->id = i;
						p->name = AZStd::string(partsNames[i]);
						p->val = &csmGetPartOpacities(this->model)[i];

						this->parts.m_parts.push_back(p);
						this->parts.m_idMap[p->name] = p->id;

						//editor data
						p->InitEdit();
						//ElementInfo ei;
						//ei.m_editData = p->ed;
						//ei.m_uuid = AZ::SerializeTypeInfo<float>::GetUuid();
						this->m_dataElements.insert(AZStd::make_pair(p->val, &p->ei));

					}
					this->parts.m_parts.shrink_to_fit();

					//load drawable data
					const char** drawableNames = csmGetDrawableIds(this->model);
					const csmFlags* constFlags = csmGetDrawableConstantFlags(this->model);
					const csmFlags* dynFlags = csmGetDrawableDynamicFlags(this->model);
					const int* texIndices = csmGetDrawableTextureIndices(this->model);
					const float* opacities = csmGetDrawableOpacities(this->model);
					const int* maskCounts = csmGetDrawableMaskCounts(this->model);
					const int** masks = csmGetDrawableMasks(this->model);
					const int* vertCount = csmGetDrawableVertexCounts(this->model);
					const csmVector2** verts = csmGetDrawableVertexPositions(this->model);
					const csmVector2** uvs = csmGetDrawableVertexUvs(this->model);
					const int* numIndexes = csmGetDrawableIndexCounts(this->model);
					const unsigned short** indices = csmGetDrawableIndices(this->model);
					unsigned int drawCount = csmGetDrawableCount(this->model);
					const int * drawOrder = csmGetDrawableDrawOrders(this->model);
					const int * renderOrder = csmGetDrawableRenderOrders(this->model);

					float minX = 0.0f, minY = 0.0f, maxX = 0.0f, maxY = 0.0f;

					for (int i = 0; i < drawCount; ++i) {
						Drawable * d = new Drawable;
						d->name = drawableNames[i];
						d->id = i;
						d->constFlags = constFlags[i];
						d->dynFlags = dynFlags[i];
						d->texId = texIndices[i];
						if (d->texId > this->numTextures) this->numTextures = d->texId;
						d->drawOrder = drawOrder[i];
						d->renderOrder = renderOrder[i];
						d->opacity = opacities[i];
						d->packedOpacity = ColorF(1.0f, 1.0f, 1.0f, d->opacity).pack_argb8888();
						d->maskCount = maskCounts[i];

						d->maskIndices = new uint16[d->maskCount];
						for (int in = 0; in < d->maskCount; in++) d->maskIndices[in] = masks[i][in];

						d->transform = &this->transform;
						d->uvTransform = &this->uvTransform;

						d->vertCount = vertCount[i];
						d->verts = new SVF_P3F_C4B_T2F[d->vertCount];
						d->rawVerts = verts[i];
						d->rawUVs = uvs[i];

						for (int v = 0; v < d->vertCount; v++) {
							d->verts[v].xyz = Vec3(0.0f, 0.0f, 0.0f);
							d->verts[v].st = Vec2(0.0f, 0.0f);
							d->verts[v].color.dcolor = d->packedOpacity;

							if (d->rawVerts[v].X < minX) minX = d->rawVerts[v].X;
							if (d->rawVerts[v].Y < minY) minY = d->rawVerts[v].Y;

							if (d->rawVerts[v].X > maxX) maxX = d->rawVerts[v].X;
							if (d->rawVerts[v].Y > maxY) maxY = d->rawVerts[v].Y;
						}

						d->indicesCount = numIndexes[i];
						d->indices = new uint16[d->indicesCount];

						for (int in = 0; in < d->indicesCount; in++) d->indices[in] = indices[i][in];

						d->visible = d->dynFlags & csmIsVisible;

						this->drawables.push_back(d);
					}
					this->drawables.shrink_to_fit(); //free up unused memory

					this->numTextures++;

					this->modelSize.SetX(abs(minX) + abs(maxX));
					this->modelSize.SetY(abs(minY) + abs(maxY));

					//sort the drawables by render order
					AZStd::sort(
						this->drawables.begin(),
						this->drawables.end(),
						[](Drawable * a, Drawable * b) -> bool {
							return a->renderOrder < b->renderOrder;
						}
					);

					this->threadMutex.Lock();
					//threading
					//create new update thread
					if (this->tJob) {
						this->tJob->Cancel();
						this->tJob->WaitTillReady();
						delete this->tJob;
					}

					switch (this->m_threading) {
					case NONE:
						this->tJob = nullptr;
						break;
					case SINGLE:
						this->tJob = new DrawableSingleThread(&this->drawables);
						this->tJob->SetModel(this->model);
						break;
					case MULTI:
						this->tJob = new DrawableMultiThread(&this->drawables, this->threadLimiter);
						this->tJob->SetModel(this->model);
						break;
					}

					if (this->tJob) this->tJob->Start(); //start the update thread
					this->threadMutex.Unlock();

					this->modelLoaded = true;
				} else {
					CryModuleMemalignFree(this->model);
					CryModuleMemalignFree(this->moc);
					this->model = nullptr;
					this->moc = nullptr;
					this->modelLoaded = false;
					CRY_ASSERT_MESSAGE(false, "Could not initialize model data.");
				}
			} else {
				CryModuleMemalignFree(this->moc);
				this->moc = nullptr;
				this->modelLoaded = false;
				CRY_ASSERT_MESSAGE(false, "Could not initialize moc data.");
			}
		} else {
			gEnv->pFileIO->Close(fileHandler);
			this->modelLoaded = false;
			CRY_ASSERT_MESSAGE(false, "Could not open Moc file.");
		}
		this->prevViewport = AZ::Matrix4x4::CreateIdentity();
	}
	void Cubism3UIComponent::FreeMoc() {
		this->modelLoaded = false;

		//delete the drawable update thread
		this->threadMutex.Lock();
		if (this->tJob) {
			this->tJob->Cancel();
			this->tJob->WaitTillReady();
			delete this->tJob;
			this->tJob = nullptr;
		}
		this->threadMutex.Unlock();

		if (this->drawables.size() != 0) {
			for (Drawable * d : this->drawables) {
				delete d->verts; //delete the vector data
				delete d->indices; //delete the indices data
				delete d->maskIndices; //delete the mask indices
			}
			this->drawables.clear(); //clear the drawables vector
		}

		this->params.Clear();
		this->parts.Clear();
		this->m_dataElements.clear(); //clear all editor data

		if (this->model) CryModuleMemalignFree(this->model); //free the model
		if (this->moc) CryModuleMemalignFree(this->moc); //free the moc

		this->model = nullptr;
		this->moc = nullptr;
	}

	void Cubism3UIComponent::LoadTexture() {
		if (lType == Single) {
			if (this->m_imagePathname.GetAssetPath().empty()) return;
			//load the texture
			this->texture = gEnv->pSystem->GetIRenderer()->EF_LoadTexture(this->m_imagePathname.GetAssetPath().c_str(), FT_DONT_STREAM);
			this->texture->AddRef(); //Release
		} else {
			//check for missing path names
			for (AzFramework::SimpleAssetReference<LmbrCentral::TextureAsset> a : m_imagesPathname) if (a.GetAssetPath().empty()) return;

			for (AzFramework::SimpleAssetReference<LmbrCentral::TextureAsset> a : m_imagesPathname) {
				ITexture * t = gEnv->pSystem->GetIRenderer()->EF_LoadTexture(a.GetAssetPath().c_str(), FT_DONT_STREAM);
				t->AddRef();
				this->textures.push_back(t);
			}
		}
	}
	void Cubism3UIComponent::FreeTexture() {
		SAFE_RELEASE(texture);
		if (this->textures.size() > 0) {
			for (ITexture* t : this->textures) {
				SAFE_RELEASE(t);
			}
			this->textures.clear();
			this->textures.shrink_to_fit();
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

		char *jsonBuff = (char *)malloc(size+1);
		gEnv->pFileIO->Read(fileHandler, jsonBuff, size);

		gEnv->pFileIO->Close(fileHandler);

		jsonBuff[size] = '\0';
		
		//parse the json file
		rapidjson::Document d;
		d.Parse(jsonBuff);
		free(jsonBuff);

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

				if (textureListSize != this->numTextures) {
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
		this->prevViewport = AZ::Matrix4x4::CreateIdentity();
	}
	void Cubism3UIComponent::OnImageFileChange() {
		this->FreeTexture();
		if (!this->m_imagePathname.GetAssetPath().empty()) this->LoadTexture();
	}
	void Cubism3UIComponent::OnJSONFileChange() {
		this->FreeJson();
		this->LoadJson();
		this->prevViewport = AZ::Matrix4x4::CreateIdentity();
	}
	void Cubism3UIComponent::OnThreadingChange() {
		this->SetThreading(this->m_threading);
	}
	void Cubism3UIComponent::OnFillChange() {
		this->prevViewport = AZ::Matrix4x4::CreateIdentity();
	}
	void Cubism3UIComponent::OnLoadTypeChange() {
		this->m_mocPathname.SetAssetPath("");
		this->m_imagePathname.SetAssetPath("");
		this->m_jsonPathname.SetAssetPath("");
		this->m_imagesPathname.clear();
		this->m_imagesPathname.shrink_to_fit();

		if (lType == Single) {
			OnMocFileChange();
			OnImageFileChange();
		} else if (lType == JSON) {
			OnJSONFileChange();
		} else {
			CRY_ASSERT_MESSAGE(false, "unhandled load type");
		}
	}

	void Cubism3UIComponent::PreRender() {
		//threading
		//if we are threading the drawable updates
		if (this->m_threading != NONE && this->tJob) this->tJob->WaitTillReady(); //wait until the update thread is ready.
		
		if (this->m_threading == NONE && !this->tJob)
			csmUpdateModel(this->model);

		///have this also update in update thread if threading?
		UiTransform2dInterface::Anchors anchors;
		EBUS_EVENT_ID_RESULT(anchors, GetEntityId(), UiTransform2dBus, GetAnchors);
		UiTransform2dInterface::Offsets offsets;
		EBUS_EVENT_ID_RESULT(offsets, GetEntityId(), UiTransform2dBus, GetOffsets);
		AZ::Matrix4x4 viewport;
		EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetTransformToViewport, viewport);

		//update transform as nessessary.
		this->transformUpdated = false;
		if (this->prevAnchors != anchors || this->prevOffsets != offsets || this->prevViewport != viewport) {
			this->prevViewport = viewport;
			this->prevAnchors = anchors;
			this->prevOffsets = offsets;
			this->transformUpdated = true;

			UiTransformInterface::Rect rect;
			EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetCanvasSpaceRectNoScaleRotate, rect);

			UiTransformInterface::RectPoints points;
			EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetCanvasSpacePointsNoScaleRotate, points);

			this->transform = viewport *
				AZ::Matrix4x4::CreateTranslation( //translate the object
					AZ::Vector3(
						points.pt[UiTransformInterface::RectPoints::Corner_TopLeft].GetX() + (rect.GetWidth() / 2),
						points.pt[UiTransformInterface::RectPoints::Corner_TopLeft].GetY() + (rect.GetHeight() / 2),
						0.0f
					)
				);

			float scaleX = 1.0f;
			float scaleY = 1.0f;

			if (this->fill) {
				scaleX = rect.GetWidth() / this->modelSize.GetX();
				scaleY = rect.GetHeight() / this->modelSize.GetY();
			} else {
				float modelSizeAspect = this->modelSize.GetX() / this->modelSize.GetY();

				scaleX = (rect.GetHeight() * modelSizeAspect) / this->modelSize.GetX();
				scaleY = rect.GetHeight() / this->modelSize.GetY();
			}

			//scale the object
			this->transform.MultiplyByScale(AZ::Vector3(scaleX, -scaleY, 1.0f));
		}
	}
	void Cubism3UIComponent::PostRender() {
		//threading
		if (this->m_threading != NONE && this->tJob) { //if update is threaded
			this->tJob->SetTransformUpdate(this->transformUpdated); //notify that the transform has updated or not //TRANSFORM
			this->tJob->Notify(); //wake up the update thread.
		} else {
			csmResetDrawableDynamicFlags(this->model);
			if (this->renderOrderChanged)
				AZStd::sort(
					this->drawables.begin(),
					this->drawables.end(),
					[](Drawable * a, Drawable * b) -> bool {
						return a->renderOrder < b->renderOrder;
					}
				);
		}
	}

	void Cubism3UIComponent::EnableMasking() {
		this->priorBaseState = IUiRenderer::Get()->GetBaseState();

		int alphaTest = 0;
		if (this->useAlphaTest) alphaTest = GS_ALPHATEST_GREATER;

		int colorMask = GS_COLMASK_NONE;

		if (this->enableMasking) {
			// set up for stencil write
			const uint32 stencilRef = IUiRenderer::Get()->GetStencilRef();
			const uint32 stencilMask = 0xFF;
			const uint32 stencilWriteMask = 0xFF;
			
			#ifndef ENABLE_CUBISM3_DEBUG
			const int32 stencilState = STENC_FUNC(FSS_STENCFUNC_EQUAL) | STENCOP_FAIL(FSS_STENCOP_KEEP) | STENCOP_ZFAIL(FSS_STENCOP_KEEP);
			#else
			int32 stencilState = 0;

			if (stencilFunc != SFunc::FDISABLE) stencilState |= STENC_FUNC(stencilFunc);
			if (stencilCCWFunc != SFunc::FDISABLE) stencilState |= STENC_CCW_FUNC(stencilCCWFunc);

			if (opFail != SOp::ODISABLE) stencilState |= STENCOP_FAIL(opFail);
			if (opZFail != SOp::ODISABLE) stencilState |= STENCOP_ZFAIL(opZFail);
			if (opPass != SOp::ODISABLE) stencilState |= STENCOP_PASS(opPass);

			if (opCCWFail != SOp::ODISABLE) stencilState |= STENCOP_CCW_FAIL(opCCWFail);
			if (opCCWZFail != SOp::ODISABLE) stencilState |= STENCOP_CCW_ZFAIL(opCCWZFail);
			if (opCCWPass != SOp::ODISABLE) stencilState |= STENCOP_CCW_PASS(opCCWPass);
			if (sTwoSided) stencilState |= FSS_STENCIL_TWOSIDED;
			#endif

			gEnv->pRenderer->SetStencilState(stencilState, stencilRef, stencilMask, stencilWriteMask);

			IUiRenderer::Get()->SetBaseState(this->priorBaseState | GS_STENCIL | alphaTest | colorMask);
		} else {
			if (colorMask || alphaTest) {
				IUiRenderer::Get()->SetBaseState(this->priorBaseState | colorMask | alphaTest);
			}
		}
	}
	void Cubism3UIComponent::DisableMasking() {
		if (this->enableMasking) {
			// turn off stencil write and turn on stencil test
			const uint32 stencilRef = IUiRenderer::Get()->GetStencilRef();
			const uint32 stencilMask = 0xFF;
			const uint32 stencilWriteMask = 0x00;
			
			#ifndef ENABLE_CUBISM3_DEBUG
			const int32 stencilState = STENC_FUNC(FSS_STENCFUNC_EQUAL) | STENCOP_FAIL(FSS_STENCOP_KEEP) | STENCOP_ZFAIL(FSS_STENCOP_KEEP) | STENCOP_PASS(FSS_STENCOP_KEEP);
			#else
			int32 stencilState = 0;

			if (stencilFunc != SFunc::FDISABLE) stencilState |= STENC_FUNC(stencilFunc);
			if (stencilCCWFunc != SFunc::FDISABLE) stencilState |= STENC_CCW_FUNC(stencilCCWFunc);

			if (opFail != SOp::ODISABLE) stencilState |= STENCOP_FAIL(opFail);
			if (opZFail != SOp::ODISABLE) stencilState |= STENCOP_ZFAIL(opZFail);
			if (opPass != SOp::ODISABLE) stencilState |= STENCOP_PASS(opPass);

			if (opCCWFail != SOp::ODISABLE) stencilState |= STENCOP_CCW_FAIL(opCCWFail);
			if (opCCWZFail != SOp::ODISABLE) stencilState |= STENCOP_CCW_ZFAIL(opCCWZFail);
			if (opCCWPass != SOp::ODISABLE) stencilState |= STENCOP_CCW_PASS(opCCWPass);
			if (sTwoSided) stencilState |= FSS_STENCIL_TWOSIDED;
			#endif

			gEnv->pRenderer->SetStencilState(stencilState, stencilRef, stencilMask, stencilWriteMask);

			IUiRenderer::Get()->SetBaseState(this->priorBaseState);
		} else {
			IUiRenderer::Get()->SetBaseState(this->priorBaseState);
		}
	}

	void Cubism3UIComponent::Drawable::update(csmModel* model, bool transformUpdate, bool &renderOrderChanged) {
		this->dynFlags = csmGetDrawableDynamicFlags(model)[this->id];

		if (this->dynFlags & csmVisibilityDidChange) this->visible = this->dynFlags & csmIsVisible;

		if (this->dynFlags & csmOpacityDidChange) {
			this->opacity = csmGetDrawableOpacities(model)[this->id];
			this->packedOpacity = ColorF(1.0f, 1.0f, 1.0f, this->opacity).pack_argb8888();
			for (int i = 0; i < this->vertCount; i++) this->verts[i].color.dcolor = this->packedOpacity;
		}

		if (this->dynFlags & csmDrawOrderDidChange) this->drawOrder = csmGetDrawableDrawOrders(model)[this->id];
		if (this->dynFlags & csmRenderOrderDidChange) {
			this->renderOrder = csmGetDrawableRenderOrders(model)[this->id];
			renderOrderChanged = true;
		}

		if (this->dynFlags & csmVertexPositionsDidChange) {
			//vertexes
			const int vertCount = csmGetDrawableVertexCounts(model)[this->id];

			//recreate buffer if needed
			if (vertCount != this->vertCount) {
				delete this->verts;
				this->vertCount = vertCount;
				this->verts = new SVF_P3F_C4B_T2F[this->vertCount];
			}

			this->rawVerts = csmGetDrawableVertexPositions(model)[this->id];
			this->rawUVs = csmGetDrawableVertexUvs(model)[this->id];

			//indicies
			const int icount = csmGetDrawableIndexCounts(model)[this->id];

			//recreate indices if needed
			if (this->indicesCount != icount) {
				delete this->indices;
				this->indicesCount = icount;
				this->indices = new uint16[this->indicesCount];
			}

			const unsigned short * in = csmGetDrawableIndices(model)[this->id];
			for (int i = 0; i < this->indicesCount; i++) this->indices[i] = in[i];

			transformUpdate = true; //make sure that we convert the data to lumberyard compatable data
		}

		//update data only when transform has changed.
		if (transformUpdate) {
			for (int i = 0; i < this->vertCount; i++) {
				AZ::Vector3 vec(rawVerts[i].X, rawVerts[i].Y, 1.0f);
				vec = *this->transform * vec;

				AZ::Vector3 uvTrans = AZ::Vector3(this->rawUVs[i].X, this->rawUVs[i].Y, 0.0f) * *this->uvTransform;

				this->verts[i].xyz = Vec3(vec.GetX(), vec.GetY(), vec.GetZ());
				this->verts[i].st = Vec2(uvTrans.GetX(), uvTrans.GetY());
				this->verts[i].color.dcolor = this->packedOpacity;
			}
		}
	}

	/*void Cubism3UIComponent::testMotionCN() {
		Cubism3Animation animationTest(this->m_testMotion);
	}*/
}