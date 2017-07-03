#include "StdAfx.h"

#include "Cubism3UIComponent.h"

#include <AzCore/Math/Crc.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>

#include <IRenderer.h>

#include <LyShine/IDraw2d.h>
#include <LyShine/UiSerializeHelpers.h>
#include <LyShine/Bus/UiElementBus.h>
//#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiTransformBus.h>
#include <LyShine/Bus/UiTransform2dBus.h>
//#include <LyShine/ISprite.h>
#include <LyShine/IUiRenderer.h>

Cubism3UIComponent::Cubism3UIComponent() {
	this->m_modelLoaded = false;
	this->transform = AZ::Matrix4x4::CreateIdentity();
}

Cubism3UIComponent::~Cubism3UIComponent() {
	if (this->m_modelLoaded) this->ReleaseObject();
}

void Cubism3UIComponent::Reflect(AZ::ReflectContext* context) {
	AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

	if (serializeContext) {
		serializeContext->Class<Cubism3UIComponent, AZ::Component>()
			->Version(1, nullptr)
			->Field("MocFile", &Cubism3UIComponent::m_mocPathname)
			->Field("ImageFile", &Cubism3UIComponent::m_imagePathname)
			#ifdef USE_CUBISM3_ANIM_FRAMEWORK
			->Field("AnimationPaths", &Cubism3UIComponent::m_animationPathnames)
			#endif
			;

		AZ::EditContext* ec = serializeContext->GetEditContext();
		/*if (ec) {
			auto editInfo = ec->Class<UiImageComponent>("Image", "A visual component to draw a rectangle with an optional sprite/texture");

			editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
				->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiImage.png")
				->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiImage.png")
				->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("UI", 0x27ff46b0));

			editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiImageComponent::m_spriteType, "SpriteType", "The sprite type.")
				->EnumAttribute(UiImageInterface::SpriteType::SpriteAsset, "Sprite/Texture asset")
				->EnumAttribute(UiImageInterface::SpriteType::RenderTarget, "Render target")
				->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiImageComponent::OnSpriteTypeChange)
				->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshEntireTree", 0xefbc823c));
			editInfo->DataElement("Sprite", &UiImageComponent::m_spritePathname, "Sprite path", "The sprite path. Can be overridden by another component such as an interactable.")
				->Attribute(AZ::Edit::Attributes::Visibility, &UiImageComponent::IsSpriteTypeAsset)
				->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiImageComponent::OnSpritePathnameChange);
			editInfo->DataElement(0, &UiImageComponent::m_renderTargetName, "Render target name", "The name of the render target associated with the sprite.")
				->Attribute(AZ::Edit::Attributes::Visibility, &UiImageComponent::IsSpriteTypeRenderTarget)
				->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiImageComponent::OnSpriteRenderTargetNameChange);
			editInfo->DataElement(AZ::Edit::UIHandlers::Color, &UiImageComponent::m_color, "Color", "The color tint for the image. Can be overridden by another component such as an interactable.")
				->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiImageComponent::OnColorChange);
			editInfo->DataElement(AZ::Edit::UIHandlers::Slider, &UiImageComponent::m_alpha, "Alpha", "The transparency. Can be overridden by another component such as an interactable.")
				->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiImageComponent::OnColorChange)
				->Attribute(AZ::Edit::Attributes::Min, 0.0f)
				->Attribute(AZ::Edit::Attributes::Max, 1.0f);
			editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiImageComponent::m_imageType, "ImageType", "The image type. Affects how the texture/sprite is mapped to the image rectangle.")
				->EnumAttribute(UiImageInterface::ImageType::Stretched, "Stretched")
				->EnumAttribute(UiImageInterface::ImageType::Sliced, "Sliced")
				->EnumAttribute(UiImageInterface::ImageType::Fixed, "Fixed")
				->EnumAttribute(UiImageInterface::ImageType::Tiled, "Tiled")
				->EnumAttribute(UiImageInterface::ImageType::StretchedToFit, "Stretched To Fit")
				->EnumAttribute(UiImageInterface::ImageType::StretchedToFill, "Stretched To Fill");
			editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiImageComponent::m_blendMode, "BlendMode", "The blend mode used to draw the image")
				->EnumAttribute(LyShine::BlendMode::Normal, "Normal")
				->EnumAttribute(LyShine::BlendMode::Add, "Add")
				->EnumAttribute(LyShine::BlendMode::Screen, "Screen")
				->EnumAttribute(LyShine::BlendMode::Darken, "Darken")
				->EnumAttribute(LyShine::BlendMode::Lighten, "Lighten");
			}
		}*/

		AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
		/*if (behaviorContext) {
			behaviorContext->Enum<(int)UiImageInterface::ImageType::Stretched>("eUiImageType_Stretched")
				->Enum<(int)UiImageInterface::ImageType::Sliced>("eUiImageType_Sliced")
				->Enum<(int)UiImageInterface::ImageType::Fixed>("eUiImageType_Fixed")
				->Enum<(int)UiImageInterface::ImageType::Tiled>("eUiImageType_Tiled")
				->Enum<(int)UiImageInterface::ImageType::StretchedToFit>("eUiImageType_StretchedToFit")
				->Enum<(int)UiImageInterface::ImageType::StretchedToFill>("eUiImageType_StretchedToFill")
				->Enum<(int)UiImageInterface::SpriteType::SpriteAsset>("eUiSpriteType_SpriteAsset")
				->Enum<(int)UiImageInterface::SpriteType::RenderTarget>("eUiSpriteType_RenderTarget");

			behaviorContext->EBus<UiImageBus>("UiImageBus")
				->Event("GetColor", &UiImageBus::Events::GetColor)
				->Event("SetColor", &UiImageBus::Events::SetColor)
				->Event("GetSpritePathname", &UiImageBus::Events::GetSpritePathname)
				->Event("SetSpritePathname", &UiImageBus::Events::SetSpritePathname)
				->Event("GetRenderTargetName", &UiImageBus::Events::GetRenderTargetName)
				->Event("SetRenderTargetName", &UiImageBus::Events::SetRenderTargetName)
				->Event("GetSpriteType", &UiImageBus::Events::GetSpriteType)
				->Event("SetSpriteType", &UiImageBus::Events::SetSpriteType)
				->Event("GetImageType", &UiImageBus::Events::GetImageType)
				->Event("SetImageType", &UiImageBus::Events::SetImageType);
		}*/
	}
}

void Cubism3UIComponent::Init() {
	this->LoadObject();
}

void Cubism3UIComponent::Activate() {
	UiRenderBus::Handler::BusConnect(m_entity->GetId());
	//AZ::TickBus::Handler::BusConnect();
}

void Cubism3UIComponent::Deactivate() {
	UiRenderBus::Handler::BusDisconnect();
	//AZ::TickBus::Handler::BusDisconnect();
}

//void Cubism3UIComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time) {
	//csmTickAnimationState(nullptr, deltaTime);
//}

void Cubism3UIComponent::Render() {
	const char* profileMarker = "UI_CUBISM3";
	gEnv->pRenderer->PushProfileMarker(profileMarker);

	if (this->m_modelLoaded) {
		#ifdef USE_CUBISM3_ANIM_FRAMEWORK
		if (this->animations.size() != 0) {
			for (AnimationLayer* layer : this->animations) {
				if (layer->enabled) {
					csmTickAnimationState(&layer->State, gEnv->pTimer->GetFrameTime());
					csmEvaluateAnimation(
						layer->Animation,
						&layer->State,
						layer->Blend,
						layer->Weight,
						this->sink
					);
				}
			}

			csmFlushFloatSink(this->sink, this->model, 0, 0);
		}
		#endif

		csmUpdateModel(this->model);

		AZ::Matrix4x4 localtransform;
		EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetTransformToViewport, localtransform);

		bool transformUpdated = false;
		if (this->transform != localtransform) {
			this->transform = localtransform;
			transformUpdated = true;
		}

		IRenderer *renderer = gEnv->pRenderer;
		renderer->SetTexture(texture->GetTextureID());

		for (Drawable * d : this->drawables) {
			d->update(this->model, localtransform, transformUpdated);

			if (d->visible) {
				int flags = GS_BLSRC_ONE | GS_BLSRC_ONEMINUSSRCALPHA;

				if (d->constFlags & csmBlendAdditive) {
					flags = GS_BLSRC_SRCALPHA | GS_BLDST_ONE;
				} else if (d->constFlags & csmBlendMultiplicative) {
					flags = GS_BLDST_ONE | GS_BLDST_ONEMINUSSRCALPHA;
				}

				renderer->SetState(flags | IUiRenderer::Get()->GetBaseState());
				renderer->SetColorOp(eCO_MODULATE, eCO_MODULATE, DEF_TEXARG0, DEF_TEXARG0);
				renderer->DrawDynVB(d->data, (uint16 *)d->indices, d->vertCount, d->numIndices, prtTriangleList);
			}
		}
	} else {
		//draw a blank space
	}

	gEnv->pRenderer->PopProfileMarker(profileMarker);
}

void Cubism3UIComponent::LoadObject() {
	//read moc file
	AZ::IO::HandleType fileHandler;
	gEnv->pFileIO->Open(this->m_mocPathname.GetAssetPath().c_str(), AZ::IO::OpenMode::ModeBinary, fileHandler);

	AZ::u64 mocSize;
	gEnv->pFileIO->Size(fileHandler, mocSize);

	void *mocBuf = CryModuleMemalign(mocSize, csmAlignofMoc); //CryModuleMemalignFree
	gEnv->pFileIO->Read(fileHandler, mocBuf, mocSize);

	gEnv->pFileIO->Close(fileHandler);

	this->moc = csmReviveMocInPlace(mocBuf, (unsigned int)mocSize);

	//load model
	unsigned int modelSize = csmGetSizeofModel(this->moc);
	void * modelBuf = CryModuleMemalign(modelSize, csmAlignofModel); //CryModuleMemalignFree

	this->model = csmInitializeModelInPlace(this->moc, modelBuf, modelSize);

	//load animations
	#ifdef USE_CUBISM3_ANIM_FRAMEWORK
	if (this->m_animationPathnames.size() != 0) {
		for (AzFramework::SimpleAssetReference<MotionAsset> asset : this->m_animationPathnames) {
			AZ::IO::HandleType animFileHandler;
			gEnv->pFileIO->Open(asset.GetAssetPath().c_str(), AZ::IO::OpenMode::ModeBinary, animFileHandler);

			AZ::u64 motionJsonSize;
			gEnv->pFileIO->Size(animFileHandler, motionJsonSize);

			char * motionJson = (char *)malloc(motionJsonSize);
			gEnv->pFileIO->Read(animFileHandler, motionJson, motionJsonSize);

			gEnv->pFileIO->Close(animFileHandler);

			AnimationLayer* layer = new AnimationLayer; //delete
			layer->Blend = csmOverrideFloatBlendFunction;
			layer->Weight = 1.0f;
			layer->enabled = false;
			csmResetAnimationState(&layer->State);

			unsigned int animSize = csmGetDeserializedSizeofAnimation(motionJson);

			void * animbuff = malloc(animSize); //free
			layer->Animation = csmDeserializeAnimationInPlace(motionJson, animbuff, animSize);

			free(motionJson);

			this->animations.push_back(layer);
		}
	}

	//load sink
	unsigned int sinkSize = csmGetSizeofFloatSink(this->model);
	void *sinkBuf = malloc(sinkSize); //free
	this->sink = csmInitializeFloatSinkInPlace(this->model, sinkBuf, sinkSize);
	#endif

	//get the parameters of the model
	const char** paramNames = csmGetParameterIds(this->model);

	for (int i = 0; i < csmGetParameterCount(this->model); i++) {
		Parameter * p = new Parameter;
		p->id = i;
		p->name = AZStd::string(paramNames[i]);
		p->min = csmGetParameterMinimumValues(this->model)[i];
		p->max = csmGetParameterMaximumValues(this->model)[i];
		p->val = &csmGetParameterValues(this->model)[i];

		this->parameters.push_back(p);
	}

	//load the texture
	this->texture = gEnv->pSystem->GetIRenderer()->EF_LoadTexture(this->m_imagePathname.GetAssetPath().c_str(), FT_DONT_STREAM);
	this->texture->AddRef(); //Release

	//load vector data
	const char** drawableNames = csmGetDrawableIds(this->model);
	const csmFlags* constFlags = csmGetDrawableConstantFlags(this->model);
	const csmFlags* dynFlags = csmGetDrawableDynamicFlags(this->model);
	const int* texIndices = csmGetDrawableTextureIndices(this->model);
	const int* drawOrder = csmGetDrawableDrawOrders(this->model);
	const int* renderOrder = csmGetDrawableRenderOrders(this->model);
	const float* opacities = csmGetDrawableOpacities(this->model);
	const int* maskCounts = csmGetDrawableMaskCounts(this->model);
	const int** masks = csmGetDrawableMasks(this->model);
	const int* vertCount = csmGetDrawableVertexCounts(this->model);
	const csmVector2** verts = csmGetDrawableVertexPositions(this->model);
	const csmVector2** uvs = csmGetDrawableVertexUvs(this->model);
	const int* numIndexes = csmGetDrawableIndexCounts(this->model);
	const unsigned short** indices = csmGetDrawableIndices(this->model);

	for (int i = 0; i < csmGetDrawableCount(this->model); i++) {
		Drawable * d = new Drawable;
		d->name = drawableNames[i];
		d->id = i;
		d->constFlags = constFlags[i];
		d->dynFlags = dynFlags[i];
		d->texIndices = texIndices[i];
		d->drawOrder = drawOrder[i];
		d->renderOrder = renderOrder[i];
		d->opacity = opacities[i];
		d->maskCount = maskCounts[i];
		d->maskIndices = masks[i];
		
		d->vertCount = vertCount[i];
		d->data = new SVF_P3F_C4B_T2F[d->vertCount];
		d->rawdata = verts[i];
		
		for (int v = 0; v < d->vertCount; v++) {
			d->data[v].xyz = Vec3(0.0f,0.0f,0.0f);
			d->data[v].st = Vec2(0.0f,0.0f);
			d->data[v].color.dcolor = ColorF(1,1,1,d->opacity).pack_argb8888();
		}

		d->numIndices = numIndexes[i];
		d->indices = indices[i];

		d->visible = d->dynFlags & csmIsVisible;
	}

	this->m_modelLoaded = true;
}

void Cubism3UIComponent::ReleaseObject() {
	this->m_modelLoaded = false;

	this->texture->Release(); //free the texture

	for (Drawable * d : this->drawables) delete d->data; //delete the vector data
	this->drawables.clear(); //clear the drawables vector

	#ifdef USE_CUBISM3_ANIM_FRAMEWORK
	free(this->sink); //free the float sink
	for (AnimationLayer* layer : this->animations) free(layer->Animation); //free each animation
	this->animations.clear(); //clear the animations vector
	#endif

	this->parameters.clear(); //clear the parameters vector
	
	CryModuleMemalignFree(this->model); //free the model
	CryModuleMemalignFree(this->moc); //free the moc
}

void Cubism3UIComponent::Drawable::update(csmModel* model, AZ::Matrix4x4 transform, bool transformUpdate) {
	this->dynFlags = csmGetDrawableDynamicFlags(model)[this->id];

	if (this->dynFlags & csmVisibilityDidChange) this->visible = this->dynFlags & csmIsVisible;

	if (this->dynFlags & csmOpacityDidChange) {
		this->opacity = csmGetDrawableOpacities(model)[this->id];

		for (int i = 0; i < this->vertCount; i++) this->data[i].color.dcolor = ColorF(1, 1, 1, this->opacity).pack_argb8888();
	}
	
	if (this->dynFlags & csmDrawOrderDidChange) this->drawOrder = csmGetDrawableDrawOrders(model)[this->id];
	if (this->dynFlags & csmRenderOrderDidChange) this->renderOrder = csmGetDrawableRenderOrders(model)[this->id];
	
	if (this->dynFlags & csmVertexPositionsDidChange) {
		const int vertCount = csmGetDrawableVertexCounts(model)[this->id];

		//recreate buffer if needed
		if (vertCount != this->vertCount) {
			delete this->data;
			this->vertCount = vertCount;
			this->data = new SVF_P3F_C4B_T2F[this->vertCount];
		}

		this->rawdata = csmGetDrawableVertexPositions(model)[this->id];
		this->rawUVs = csmGetDrawableVertexUvs(model)[this->id];

		this->numIndices = csmGetDrawableIndexCounts(model)[this->id];
		this->indices = csmGetDrawableIndices(model)[this->id];

		transformUpdate = true; //make sure that we convert the data to lumberyard compatable data
	}

	//update data only when transform has changed.
	if (transformUpdate) {
		for (int i = 0; i < this->vertCount; i++) {
			AZ::Vector3 vec(rawdata[i].X, rawdata[i].Y, 1.0f);
			vec = transform * vec;

			this->data[i].xyz = Vec3(vec.GetX(), vec.GetY(), vec.GetZ());
			this->data[i].st = Vec2(this->rawUVs[i].X, this->rawUVs[i].Y);
			this->data[i].color.dcolor = ColorF(1, 1, 1, this->opacity).pack_argb8888();
		}
	}
}