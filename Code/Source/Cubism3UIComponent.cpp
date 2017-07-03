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

//#include <AZCore/JSON/rapidjson.h>

Cubism3UIComponent::Cubism3UIComponent() {
	this->transform = AZ::Matrix4x4::CreateIdentity();
	this->prevTransform = AZ::Matrix4x4::CreateZero();

	this->moc = nullptr;
	this->model = nullptr;
	this->texture = nullptr;
}

Cubism3UIComponent::~Cubism3UIComponent() {
	this->ReleaseObject();
}

void Cubism3UIComponent::Reflect(AZ::ReflectContext* context) {
	AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

	if (serializeContext) {
		AzFramework::SimpleAssetReference<MocAsset>::Register(*serializeContext);

		serializeContext->Class<Cubism3UIComponent, AZ::Component>()
			->Version(1)
			->Field("MocFile", &Cubism3UIComponent::m_mocPathname)
			->Field("ImageFile", &Cubism3UIComponent::m_imagePathname)
		#ifdef USE_CUBISM3_ANIM_FRAMEWORK
			->Field("AnimationPaths", &Cubism3UIComponent::m_animationPathnames)
		#endif
			;

		AZ::EditContext* ec = serializeContext->GetEditContext();
		if (ec) {
			auto editInfo = ec->Class<Cubism3UIComponent>("Cubism3", "A visual component to draw a Cubism3 Model.");

			editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
				->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/CharacterPhysics.png")
				->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/CharacterPhysics.png")
				->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("UI", 0x27ff46b0));

			editInfo->DataElement("Moc", &Cubism3UIComponent::m_mocPathname, "Moc path", "The Moc path. Can be overridden by another component such as an interactable.")
				->Attribute(AZ::Edit::Attributes::ChangeNotify, &Cubism3UIComponent::OnMocFileChange);

			editInfo->DataElement("Image", &Cubism3UIComponent::m_imagePathname, "Image path", "The Image path. Can be overridden by another component such as an interactable.")
				->Attribute(AZ::Edit::Attributes::ChangeNotify, &Cubism3UIComponent::OnImageFileChange);
		}

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
}

void Cubism3UIComponent::Deactivate() {
	UiRenderBus::Handler::BusDisconnect();
}

void Cubism3UIComponent::Render() {
	const char* profileMarker = "UI_CUBISM3";
	gEnv->pRenderer->PushProfileMarker(profileMarker);

	if (this->model) {
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

		EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetTransformToViewport, this->transform);

		bool transformUpdated = false;
		if (this->transform != this->prevTransform) {
			this->prevTransform = this->transform;
			transformUpdated = true;
		}

		IRenderer *renderer = gEnv->pRenderer;

		//move to for loop where drawable->texIndices indicates which texture to use.
		if (this->texture != nullptr) renderer->SetTexture(texture->GetTextureID());
		else renderer->SetTexture(renderer->GetWhiteTextureId());

		for (Drawable * d : this->drawables) {
			d->update(this->model, this->transform, transformUpdated);

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
	if (!this->m_mocPathname.GetAssetPath().empty()) this->LoadMoc();

#ifdef USE_CUBISM3_ANIM_FRAMEWORK
	this->LoadAnimation();
#endif

	if (!this->m_imagePathname.GetAssetPath().empty()) this->LoadTexture();
}

void Cubism3UIComponent::ReleaseObject() {
	this->FreeTexture();

#ifdef USE_CUBISM3_ANIM_FRAMEWORK
	this->FreeAnimation();
#endif

	this->FreeMoc();
}


void Cubism3UIComponent::LoadMoc() {
	if (this->m_mocPathname.GetAssetPath().empty()) return;

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
	this->parameters.shrink_to_fit(); //free up unused memory

	//load drawable data
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
		d->packedOpacity = ColorF(1, 1, 1, d->opacity).pack_argb8888();
		d->maskCount = maskCounts[i];
		d->maskIndices = masks[i];

		d->vertCount = vertCount[i];
		d->data = new SVF_P3F_C4B_T2F[d->vertCount];
		d->rawdata = verts[i];
		d->rawUVs = uvs[i];

		for (int v = 0; v < d->vertCount; v++) {
			d->data[v].xyz = Vec3(0.0f, 0.0f, 0.0f);
			d->data[v].st = Vec2(0.0f, 0.0f);
			d->data[v].color.dcolor = d->packedOpacity;
		}

		d->numIndices = numIndexes[i];
		d->indices = indices[i];

		d->visible = d->dynFlags & csmIsVisible;
	}
	this->drawables.shrink_to_fit(); //free up unused memory

	this->transform = AZ::Matrix4x4::CreateIdentity();
	this->prevTransform = AZ::Matrix4x4::CreateIdentity();
}
void Cubism3UIComponent::FreeMoc() {
	if (this->drawables.size() != 0) {
		for (Drawable * d : this->drawables) delete d->data; //delete the vector data
		this->drawables.clear(); //clear the drawables vector
	}

	if (this->parameters.size() != 0) this->parameters.clear(); //clear the parameters vector

	if (this->model) CryModuleMemalignFree(this->model); //free the model
	if (this->moc) CryModuleMemalignFree(this->moc); //free the moc

	this->model = nullptr;
	this->moc = nullptr;
}

void Cubism3UIComponent::LoadTexture() {
	if (this->m_imagePathname.GetAssetPath().empty()) return;
	//load the texture
	this->texture = gEnv->pSystem->GetIRenderer()->EF_LoadTexture(this->m_imagePathname.GetAssetPath().c_str(), FT_DONT_STREAM);
	this->texture->AddRef(); //Release

	//this->m_modelLoaded = this->m_mocPathname.GetAssetPath().empty() && this->m_imagePathname.GetAssetPath().empty();
}
void Cubism3UIComponent::FreeTexture() {
	SAFE_RELEASE(texture);
}


#ifdef USE_CUBISM3_ANIM_FRAMEWORK
void Cubism3UIComponent::LoadAnimation() {
	//load animations
	if (this->m_animationPathnames.size() != 0) {
		for (AzFramework::SimpleAssetReference<MotionAsset> asset : this->m_animationPathnames) {
			if (asset.GetAssetPath().empty()) continue;

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
		this->animations.shrink_to_fit();
	}

	//load sink
	unsigned int sinkSize = csmGetSizeofFloatSink(this->model);
	void *sinkBuf = malloc(sinkSize); //free
	this->sink = csmInitializeFloatSinkInPlace(this->model, sinkBuf, sinkSize);
}
void Cubism3UIComponent::FreeAnimation() {
	if (this->sink) free(this->sink); //free the float sink

	if (this->animations.size() != 0) {
		for (AnimationLayer* layer : this->animations) free(layer->Animation); //free each animation
		this->animations.clear(); //clear the animations vector
	}

	this->sink = nullptr;
}
#endif

void Cubism3UIComponent::OnMocFileChange() {
	this->FreeMoc();
	if (!this->m_mocPathname.GetAssetPath().empty()) this->LoadMoc();
}

void Cubism3UIComponent::OnImageFileChange() {
	this->FreeTexture();
	if (!this->m_imagePathname.GetAssetPath().empty()) this->LoadTexture();
}

void Cubism3UIComponent::Drawable::update(csmModel* model, AZ::Matrix4x4 transform, bool transformUpdate) {
	this->dynFlags = csmGetDrawableDynamicFlags(model)[this->id];

	if (this->dynFlags & csmVisibilityDidChange) this->visible = this->dynFlags & csmIsVisible;

	if (this->dynFlags & csmOpacityDidChange) {
		this->opacity = csmGetDrawableOpacities(model)[this->id];
		this->packedOpacity = ColorF(1, 1, 1, this->opacity).pack_argb8888();
		for (int i = 0; i < this->vertCount; i++) this->data[i].color.dcolor = this->packedOpacity;
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
			this->data[i].color.dcolor = this->packedOpacity;
		}
	}
}