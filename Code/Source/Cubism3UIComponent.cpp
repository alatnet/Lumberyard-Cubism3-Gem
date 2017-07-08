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

namespace Cubism3 {
	#ifdef CUBISM3_THREADING
		Cubism3UIInterface::Threading Cubism3UIComponent::m_threadingOverride = CUBISM3_THREADING;
	#else
		Cubism3UIInterface::Threading Cubism3UIComponent::m_threadingOverride = Cubism3UIInterface::Threading::DISABLED;
	#endif

	Cubism3UIComponent::Cubism3UIComponent() {
		this->transform = AZ::Matrix4x4::CreateIdentity();
		this->prevTransform = AZ::Matrix4x4::CreateZero();

		this->moc = nullptr;
		this->model = nullptr;
		this->texture = nullptr;

		this->modelLoaded = false;

		//this->allverts = nullptr;

		//this->rType = rtSequential;

		this->m_threading = NONE;
		this->tJob = nullptr;

		this->threadLimiter = CUBISM3_MULTITHREAD_LIMITER;

		this->wireframe = false;
	}

	Cubism3UIComponent::~Cubism3UIComponent() {
		this->ReleaseObject();
	}

	void Cubism3UIComponent::Reflect(AZ::ReflectContext* context) {
		AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

		if (serializeContext) {
			serializeContext->Class<Cubism3UIComponent, AZ::Component>()
				->Version(1)
				->Field("MocFile", &Cubism3UIComponent::m_mocPathname)
				->Field("ImageFile", &Cubism3UIComponent::m_imagePathname)
				->Field("Wireframe", &Cubism3UIComponent::wireframe)
				//->Field("RenderType", &Cubism3UIComponent::rType)
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

				editInfo->DataElement(0, &Cubism3UIComponent::m_mocPathname, "Moc path", "The Moc path. Can be overridden by another component such as an interactable.")
					->Attribute(AZ::Edit::Attributes::ChangeNotify, &Cubism3UIComponent::OnMocFileChange);

				editInfo->DataElement(0, &Cubism3UIComponent::m_imagePathname, "Image path", "The Image path. Can be overridden by another component such as an interactable.")
					->Attribute(AZ::Edit::Attributes::ChangeNotify, &Cubism3UIComponent::OnImageFileChange);

				/*editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &Cubism3UIComponent::rType, "Render Type", "How to render the Cubism3 Model.\nSequential - One drawable after the other.\nDraw - Draw based on draw order.\nRender - Draw based on render order.")
					->EnumAttribute(Cubism3UIInterface::RenderType::rtSequential, "Sequential")
					->EnumAttribute(Cubism3UIInterface::RenderType::rtDraw, "Draw")
					->EnumAttribute(Cubism3UIInterface::RenderType::rtRender, "Render");*/

				editInfo->DataElement(0, &Cubism3UIComponent::wireframe, "Wireframe (Debug)", "Wireframe Mode");

			}

			AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
			if (behaviorContext) {
				#define EBUS_METHOD(name) ->Event(#name, &Cubism3UIBus::Events::##name##)
				behaviorContext->Class<Cubism3UIComponent>("Cubisim3UI")
					/*->Enum<Cubism3UIInterface::RenderType::rtSequential>("rtSequential")
					->Enum<Cubism3UIInterface::RenderType::rtDraw>("rtDraw")
					->Enum<Cubism3UIInterface::RenderType::rtRender>("rtRender")*/
					->Enum<Cubism3UIInterface::Threading::NONE>("tNone")
					->Enum<Cubism3UIInterface::Threading::SINGLE>("tSingle")
					->Enum<Cubism3UIInterface::Threading::MULTI>("tMulti")
					;

				behaviorContext->EBus<Cubism3UIBus>("Cubism3UIBus")
					//pathnames
					EBUS_METHOD(SetMocPathname)
					EBUS_METHOD(SetTexturePathname)
					EBUS_METHOD(GetMocPathname)
					EBUS_METHOD(GetTexturePathname)
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
					//render types
					/*EBUS_METHOD(SetRenderType)
					EBUS_METHOD(GetRenderType)*/
					//Threading
					EBUS_METHOD(SetThreading)
					EBUS_METHOD(GetThreading)
					EBUS_METHOD(SetMultiThreadLimiter)
					EBUS_METHOD(GetMultiThreadLimiter)
					;
				#undef EBUS_METHOD
				/*	
				behaviorContext->Enum<(int)UiImageInterface::ImageType::Stretched>("eUiImageType_Stretched")
					->Enum<(int)UiImageInterface::ImageType::Sliced>("eUiImageType_Sliced")
					->Enum<(int)UiImageInterface::ImageType::Fixed>("eUiImageType_Fixed")
					->Enum<(int)UiImageInterface::ImageType::Tiled>("eUiImageType_Tiled")
					->Enum<(int)UiImageInterface::ImageType::StretchedToFit>("eUiImageType_StretchedToFit")
					->Enum<(int)UiImageInterface::ImageType::StretchedToFill>("eUiImageType_StretchedToFill")
					->Enum<(int)UiImageInterface::SpriteType::SpriteAsset>("eUiSpriteType_SpriteAsset")
					->Enum<(int)UiImageInterface::SpriteType::RenderTarget>("eUiSpriteType_RenderTarget");
					*/
			}
		}
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

	void Cubism3UIComponent::Render() {
		this->threadMutex.Lock();
		const char* profileMarker = "UI_CUBISM3";
		gEnv->pRenderer->PushProfileMarker(profileMarker);

		IRenderer *renderer = gEnv->pRenderer;

		if (this->modelLoaded) {
			//threading
			if (this->m_threading != NONE && this->tJob) { //if we are threading the drawable updates
				this->tJob->WaitTillReady(); //wait until the update thread is ready.
				//update rendering orders if needed
				if (this->tJob->DrawOrderChanged()) this->drawOrder = csmGetDrawableDrawOrders(this->model);
				if (this->tJob->RenderOrderChanged()) this->renderOrder = csmGetDrawableRenderOrders(this->model);
			}

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

			if (this->m_threading == NONE && !this->tJob)
				csmUpdateModel(this->model);

			EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetTransformToViewport, this->transform);
			/*EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetTransformFromViewport, this->transform);
			EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetTransformToCanvasSpace, this->transform);
			EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetTransformFromCanvasSpace, this->transform);
			EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetLocalTransform, this->transform);*/

			bool transformUpdated = false;
			if (this->transform != this->prevTransform) {
				this->prevTransform = this->transform;
				transformUpdated = true;
			}

			bool drawOrderChanged = false, renderOrderChanged = false;

			/*switch (this->rType) {
			case rtSequential:*/
				/*for (Drawable * d : this->drawables) {
					if (this->m_threading == NONE && !this->tJob)
						d->update(this->model, this->transform, transformUpdated, drawOrderChanged, renderOrderChanged);

					if (d->visible) {
						int flags = GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA;

						if (d->constFlags & csmBlendAdditive) {
							flags = GS_BLSRC_SRCALPHA | GS_BLDST_ONE;
						} else if (d->constFlags & csmBlendMultiplicative) {
							flags = GS_BLDST_ONE | GS_BLDST_ONEMINUSSRCALPHA;
						}

						if (this->texture != nullptr) renderer->SetTexture(texture->GetTextureID());
						else renderer->SetTexture(renderer->GetWhiteTextureId());
						renderer->SetState(flags | IUiRenderer::Get()->GetBaseState() | GS_NODEPTHTEST);
						renderer->SetColorOp(eCO_MODULATE, eCO_MODULATE, DEF_TEXARG0, DEF_TEXARG0);
						renderer->DrawDynVB(d->verts, d->indices, d->vertCount, d->indicesCount, prtTriangleList);
					}
				}*/
				/*break;
			case rtDraw:*/
				/*for (int i = 0; i < this->drawCount; i++) {
					Drawable * d = this->drawables[this->drawOrder[i]];
					if (this->m_threading == NONE && !this->tJob)
						d->update(this->model, this->transform, transformUpdated, drawOrderChanged, renderOrderChanged);

					if (d->visible) {
						int flags = GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA;

						if (d->constFlags & csmBlendAdditive) {
							flags = GS_BLSRC_SRCALPHA | GS_BLDST_ONE;
						} else if (d->constFlags & csmBlendMultiplicative) {
							flags = GS_BLDST_ONE | GS_BLDST_ONEMINUSSRCALPHA;
						}

						if (this->texture != nullptr) renderer->SetTexture(texture->GetTextureID() | GS_NODEPTHTEST);
						else renderer->SetTexture(renderer->GetWhiteTextureId());
						renderer->SetState(flags | IUiRenderer::Get()->GetBaseState());
						renderer->SetColorOp(eCO_MODULATE, eCO_MODULATE, DEF_TEXARG0, DEF_TEXARG0);
						renderer->DrawDynVB(d->verts, d->indices, d->vertCount, d->indicesCount, prtTriangleList);
					}
				}*/
				/*break;
			case rtRender:*/
				for (int i = 0; i < this->drawCount; i++) {
					Drawable * d = this->drawables[this->renderOrder[i]];
					if (this->m_threading == NONE && !this->tJob)
						d->update(this->model, this->transform, transformUpdated, drawOrderChanged, renderOrderChanged);

					if (d->visible) {
						int flags = GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA;

						if (d->constFlags & csmBlendAdditive) {
							flags = GS_BLSRC_SRCALPHA | GS_BLDST_ONE;
						} else if (d->constFlags & csmBlendMultiplicative) {
							flags = GS_BLDST_ONE | GS_BLDST_ONEMINUSSRCALPHA;
						}

						if (this->wireframe) renderer->PushWireframeMode(R_WIREFRAME_MODE);
						(d->constFlags & csmIsDoubleSided) ? renderer->SetCullMode(R_CULL_DISABLE) : renderer->SetCullMode(R_CULL_BACK);
						renderer->SetTexture(this->texture ? this->texture->GetTextureID() : renderer->GetWhiteTextureId());
						renderer->SetState(flags | IUiRenderer::Get()->GetBaseState() | GS_NODEPTHTEST);
						renderer->SetColorOp(eCO_MODULATE, eCO_MODULATE, DEF_TEXARG0, DEF_TEXARG0);
						renderer->DrawDynVB(d->verts, d->indices, d->vertCount, d->indicesCount, prtTriangleList);
						if (this->wireframe) renderer->PopWireframeMode();
					}
				}
			/*	break;
			}*/

			//threading
			if (this->m_threading != NONE && this->tJob) { //if update is threaded
				this->tJob->SetTransformUpdate(transformUpdated); //notify that the transform has updated or not //TRANSFORM
				this->tJob->Notify(); //wake up the update thread.
			} else {
				if (drawOrderChanged) this->drawOrder = csmGetDrawableDrawOrders(this->model);
				if (renderOrderChanged) this->renderOrder = csmGetDrawableRenderOrders(this->model);
			}
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

		gEnv->pRenderer->PopProfileMarker(profileMarker);
		this->threadMutex.Unlock();
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
		gEnv->pFileIO->Open(this->m_mocPathname.GetAssetPath().c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary, fileHandler);

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
			this->parametersMap[p->name] = p->id;
		}
		this->parameters.shrink_to_fit(); //free up unused memory

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

		this->drawCount = csmGetDrawableCount(this->model);
		this->drawOrder = csmGetDrawableDrawOrders(this->model);
		this->renderOrder = csmGetDrawableRenderOrders(this->model);
		
		for (int i = 0; i < this->drawCount; i++) {
			Drawable * d = new Drawable;
			d->name = drawableNames[i];
			d->id = i;
			d->constFlags = constFlags[i];
			d->dynFlags = dynFlags[i];
			d->texIndices = texIndices[i];
			d->drawOrder = this->drawOrder[i];
			d->renderOrder = this->renderOrder[i];
			d->opacity = opacities[i];
			d->packedOpacity = ColorF(1.0f, 1.0f, 1.0f, d->opacity).pack_argb8888();
			d->maskCount = maskCounts[i];
			d->maskIndices = masks[i];

			d->vertCount = vertCount[i];
			d->verts = new SVF_P3F_C4B_T2F[d->vertCount];
			d->rawVerts = verts[i];
			d->rawUVs = uvs[i];

			for (int v = 0; v < d->vertCount; v++) {
				d->verts[v].xyz = Vec3(0.0f, 0.0f, 0.0f);
				d->verts[v].st = Vec2(0.0f, 0.0f);
				d->verts[v].color.dcolor = d->packedOpacity;
			}
			
			d->indicesCount = numIndexes[i];
			d->indices = new uint16[d->indicesCount];

			for (int in = 0; in < d->indicesCount; in++) {
				d->indices[in] = indices[i][in];
			}

			d->visible = d->dynFlags & csmIsVisible;

			this->drawables.push_back(d);
		}
		this->drawables.shrink_to_fit(); //free up unused memory

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
			this->tJob = new DrawableSingleThread(this->drawables, this->transform);
			this->tJob->SetModel(this->model);
			break;
		case MULTI:
			this->tJob = new DrawableMultiThread(this->drawables, this->transform, this->threadLimiter);
			this->tJob->SetModel(this->model);
			break;
		}

		if (this->tJob) this->tJob->Start(); //start the update thread
		this->threadMutex.Unlock();
		this->modelLoaded = true;
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
			}
			this->drawables.clear(); //clear the drawables vector
		}

		if (this->parameters.size() != 0) {
			this->parameters.clear(); //clear the parameters vector
			this->parametersMap.clear(); //clear the parameters vector
		}

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

	void Cubism3UIComponent::Drawable::update(csmModel* model, AZ::Matrix4x4 transform, bool transformUpdate, bool &drawOrderChanged, bool &renderOrderChanged) {
		this->dynFlags = csmGetDrawableDynamicFlags(model)[this->id];

		if (this->dynFlags & csmVisibilityDidChange) this->visible = this->dynFlags & csmIsVisible;

		if (this->dynFlags & csmOpacityDidChange) {
			this->opacity = csmGetDrawableOpacities(model)[this->id];
			this->packedOpacity = ColorF(1.0f, 1.0f, 1.0f, this->opacity).pack_argb8888();
			for (int i = 0; i < this->vertCount; i++) this->verts[i].color.dcolor = this->packedOpacity;
		}

		if (this->dynFlags & csmDrawOrderDidChange) {
			this->drawOrder = csmGetDrawableDrawOrders(model)[this->id];
			drawOrderChanged = true;
		}
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
				vec = transform * vec;

				this->verts[i].xyz = Vec3(vec.GetX(), vec.GetY(), vec.GetZ());
				this->verts[i].st = Vec2(this->rawUVs[i].X, this->rawUVs[i].Y);
				this->verts[i].color.dcolor = this->packedOpacity;
			}
		}
	}

	// Cubism3UIBus
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

	//parameters
	int Cubism3UIComponent::GetParameterCount() { return this->parameters.size(); }
	int Cubism3UIComponent::GetParameterIdByName(AZStd::string name) {
		auto it = this->parametersMap.find(name); //should be faster to find an id by name rather than searching for it sequentially
		if (it != this->parametersMap.end()) return it->second;
		return -1;

		/*int ret = -1;

		for (int i = 0; i < this->parameters.size(); i++) {
			if (this->parameters.at(i)->name == name) {
				ret = i;
				break;
			}
		}

		return ret;*/
	}
	AZStd::string Cubism3UIComponent::GetParameterName(int index) {
		if (index < 0 || index >= this->parameters.size()) return "";
		return this->parameters.at(index)->name;
	}
	
	//parameters by index
	float Cubism3UIComponent::GetParameterMaxI(int index) {
		if (index < 0 || index >= this->parameters.size()) return -1;
		return this->parameters.at(index)->max;
	}
	float Cubism3UIComponent::GetParameterMinI(int index) {
		if (index < 0 || index >= this->parameters.size()) return -1;
		return this->parameters.at(index)->min;
	}
	float Cubism3UIComponent::GetParameterValueI(int index) {
		if (index < 0 || index >= this->parameters.size()) return -1;
		return *(this->parameters.at(index)->val);
	}
	void Cubism3UIComponent::SetParameterValueI(int index, float value) {
		if (index < 0 || index >= this->parameters.size()) return;
		*(this->parameters.at(index)->val) = value;
	}

	//parameters by name
	float Cubism3UIComponent::GetParameterMaxS(AZStd::string name) { return GetParameterMaxI(GetParameterIdByName(name)); }
	float Cubism3UIComponent::GetParameterMinS(AZStd::string name) { return GetParameterMinI(GetParameterIdByName(name)); }
	float Cubism3UIComponent::GetParameterValueS(AZStd::string name) { return GetParameterValueI(GetParameterIdByName(name)); }
	void Cubism3UIComponent::SetParameterValueS(AZStd::string name, float value) { return SetParameterValueI(GetParameterIdByName(name), value); }

	//rendertype
	/*void Cubism3UIComponent::SetRenderType(Cubism3UIInterface::RenderType rt) { this->rType = rt; }
	Cubism3UIInterface::RenderType Cubism3UIComponent::GetRenderType() { return this->rType; }*/

	//threading
	void Cubism3UIComponent::SetThreading(Cubism3UIInterface::Threading t) {
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
			this->tJob = new DrawableSingleThread(this->drawables, this->transform);
			this->tJob->SetModel(this->model);
			break;
		case MULTI:
			this->tJob = new DrawableMultiThread(this->drawables, this->transform, this->threadLimiter);
			this->tJob->SetModel(this->model);
			break;
		}

		if (this->tJob) this->tJob->Start(); //start the update thread
		this->threadMutex.Unlock();
	}
	Cubism3UIInterface::Threading Cubism3UIComponent::GetThreading() { return this->m_threading; }
	void Cubism3UIComponent::SetMultiThreadLimiter(unsigned int limiter) {
		if (limiter == 0) limiter = 1;
		this->threadLimiter = limiter;
		this->SetThreading(this->m_threading); //recreate the update thread.
	}
	// ~Cubism3UIBus

	//Threading stuff
	///base thread
	Cubism3UIComponent::DrawableThreadBase::DrawableThreadBase(AZStd::vector<Drawable*> &drawables, AZ::Matrix4x4 &transform) {
		this->m_drawables = &drawables;
		this->m_transform = &transform;
		this->m_drawOrderChanged = false;
		this->m_renderOrderChanged = false;
		this->m_canceled = false;
	}

	void Cubism3UIComponent::DrawableThreadBase::Cancel() {
		this->WaitTillReady();
		this->m_canceled = true;
		this->Notify();
	}

	void Cubism3UIComponent::DrawableThreadBase::WaitTillReady() {
		this->mutex.Lock();
		this->mutex.Unlock();
	}

	///single thread
	void Cubism3UIComponent::DrawableSingleThread::Run() {
		while (!this->m_canceled) {
			this->Wait();
			this->mutex.Lock();
			if (this->m_canceled) break;
			this->m_drawOrderChanged = this->m_renderOrderChanged = false;
			csmUpdateModel(this->m_model);
			for (Drawable* d : *m_drawables) d->update(this->m_model, *this->m_transform, this->m_transformUpdate, this->m_drawOrderChanged, this->m_renderOrderChanged);
			this->mutex.Unlock();
		}
		this->mutex.Unlock();
	}

	///multithread
	//each drawable gets a thread.
	/*
	Cubism3UIComponent::DrawableMultiThread::DrawableMultiThread(AZStd::vector<Drawable*> &drawables, AZ::Matrix4x4 &transform, unsigned int limiter = CUBISM3_MULTITHREAD_LIMITER) : DrawableThreadBase(drawables, transform) {
		for (Drawable * d : drawables) {
			SubThread * t = new SubThread(d, this);
			t->Start();
			this->m_threads.push_back(t);
		}
		this->m_threads.shrink_to_fit();

		this->semaphore = new CrySemaphore(limiter); //limiter for subthread execution
	}
	Cubism3UIComponent::DrawableMultiThread::~DrawableMultiThread() {
		for (SubThread* t : this->m_threads) {
			t->Cancel();
			t->WaitTillDone();
			t->WaitForThread();
		}
		this->m_threads.clear();
		
		this->WaitTillReady();
		this->Cancel();
	}
	
	void Cubism3UIComponent::DrawableMultiThread::Run() {
		while (!this->m_canceled) {
			this->Wait();
			this->mutex.Lock();
			if (this->m_canceled) break;

			this->rwmutex.WLock();
			this->m_drawOrderChanged = this->m_renderOrderChanged = false;
			this->rwmutex.WUnlock();

			csmUpdateModel(this->m_model);

			for (SubThread * t : this->m_threads) t->Notify();
			for (SubThread * t : this->m_threads) t->WaitTillReady(); //make sure every thread is done before unblocking.
			this->mutex.Unlock();
		}
		this->mutex.Unlock();
	}

	bool Cubism3UIComponent::DrawableMultiThread::DrawOrderChanged() {
		bool ret = false;
		this->rwmutex.RLock();
		ret = this->m_drawOrderChanged;
		this->rwmutex.RUnlock();
		return ret;
	}
	bool Cubism3UIComponent::DrawableMultiThread::RenderOrderChanged() {
		bool ret = false;
		this->rwmutex.RLock();
		ret = this->m_renderOrderChanged;
		this->rwmutex.RUnlock();
		return ret;
	}

	///multithread - sub thread
	void Cubism3UIComponent::DrawableMultiThread::SubThread::Run() {
		while (!this->m_canceled) {
			this->Wait();
			this->mutex.Lock();
			this->m_dmt->semaphore->Acquire();

			if (this->m_canceled) break;
			bool drawOrderChanged = false, renderOrderChanged = false;
			this->m_d->update(m_dmt->GetModel(),*m_dmt->GetTransform(), m_dmt->GetTransformUpdate(), drawOrderChanged, renderOrderChanged);

			m_dmt->rwmutex.WLock();
			if (drawOrderChanged) this->m_dmt->SetDrawOrderChanged(true);
			if (renderOrderChanged) this->m_dmt->SetRenderOrderChanged(true);
			m_dmt->rwmutex.WUnlock();
			this->m_dmt->semaphore->Release();
			this->mutex.Unlock();
		}
		this->mutex.Unlock();
		this->m_dmt->semaphore->Release();
	}
	void Cubism3UIComponent::DrawableMultiThread::SubThread::Cancel() {
		this->WaitTillDone();
		this->m_canceled = true;
		this->Notify();
	}

	void Cubism3UIComponent::DrawableMultiThread::SubThread::WaitTillDone() {
		this->mutex.Lock();
		this->mutex.Unlock();
	}
	*/

	///multithread alt
	//limited number of threads, each thread updates a drawable.
	Cubism3UIComponent::DrawableMultiThread::DrawableMultiThread(AZStd::vector<Drawable*> &drawables, AZ::Matrix4x4 &transform, unsigned int limiter) : DrawableThreadBase(drawables, transform) {
		this->drawables = &drawables;
		m_threads = new SubThread*[limiter];
		this->numThreads = limiter;

		for (int i = 0; i < limiter; i++) {
			m_threads[i] = new SubThread(this);
			m_threads[i]->Start();
		}
	}
	Cubism3UIComponent::DrawableMultiThread::~DrawableMultiThread() {
		for (int i = 0; i < numThreads; i++) {
			this->m_threads[i]->Cancel();
			this->m_threads[i]->WaitTillReady();
			this->m_threads[i]->WaitForThread();
			delete this->m_threads[i];
		}
		delete this->m_threads;

		this->Cancel();
		this->WaitTillReady();
	}

	void Cubism3UIComponent::DrawableMultiThread::Run() {
		while (!this->m_canceled) {
			this->Wait();
			this->mutex.Lock();
			if (this->m_canceled) break;

			this->rwmutex.WLock();
			this->m_drawOrderChanged = this->m_renderOrderChanged = false;
			this->rwmutex.WUnlock();

			this->dMutex.Lock();
			this->nextDrawable = 0;
			this->dMutex.Unlock();

			csmUpdateModel(this->m_model);

			for (int i = 0; i < numThreads; i++) this->m_threads[i]->Notify(); //wake up worker threads
			for (int i = 0; i < numThreads; i++) this->m_threads[i]->WaitTillReady(); //wait until the worker threads are done
			this->mutex.Unlock();
		}
		this->mutex.Unlock();
	}

	bool Cubism3UIComponent::DrawableMultiThread::DrawOrderChanged() {
		bool ret = false;
		this->rwmutex.RLock();
		ret = this->m_drawOrderChanged;
		this->rwmutex.RUnlock();
		return ret;
	}
	bool Cubism3UIComponent::DrawableMultiThread::RenderOrderChanged() {
		bool ret = false;
		this->rwmutex.RLock();
		ret = this->m_renderOrderChanged;
		this->rwmutex.RUnlock();
		return ret;
	}

	Cubism3UIComponent::Drawable * Cubism3UIComponent::DrawableMultiThread::GetNextDrawable() {
		if (!this->m_canceled) {
			if (this->nextDrawable >= this->drawables->size()) return nullptr;
			Drawable * ret = this->drawables->at(this->nextDrawable);
			this->nextDrawable++;
			return ret;
		}
		return nullptr;
	}

	///multithread alt - sub thread
	void Cubism3UIComponent::DrawableMultiThread::SubThread::Cancel() {
		this->WaitTillReady();
		this->m_canceled = true;
		this->Notify();
	}
	void Cubism3UIComponent::DrawableMultiThread::SubThread::Run(){
		Drawable * d = nullptr;
		while (!this->m_canceled) {
			this->Wait();
			this->mutex.Lock();
			if (this->m_canceled) break;

			d = nullptr;

			//get first drawable to update
			this->m_dmt->dMutex.Lock();
			d = this->m_dmt->GetNextDrawable();
			this->m_dmt->dMutex.Unlock();
			while(d){
				if (this->m_canceled) break;
				bool drawOrderChanged = false, renderOrderChanged = false;

				//update the drawable
				d->update(m_dmt->GetModel(), *m_dmt->GetTransform(), m_dmt->GetTransformUpdate(), drawOrderChanged, renderOrderChanged);

				m_dmt->rwmutex.WLock();
				if (drawOrderChanged) this->m_dmt->SetDrawOrderChanged(true);
				if (renderOrderChanged) this->m_dmt->SetRenderOrderChanged(true);
				m_dmt->rwmutex.WUnlock();

				//get next drawable to update
				this->m_dmt->dMutex.Lock();
				d = this->m_dmt->GetNextDrawable();
				this->m_dmt->dMutex.Unlock();
			}

			if (this->m_canceled) break;
			this->mutex.Unlock();
		}
		this->mutex.Unlock();
	}

	void Cubism3UIComponent::DrawableMultiThread::SubThread::WaitTillReady() {
		this->mutex.Lock();
		this->mutex.Unlock();
	}
}