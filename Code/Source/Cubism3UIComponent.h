#pragma once

#include <LyShine/Bus/UiRenderBus.h>

#include <Cubism3/Cubism3UIBus.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Component/TickBus.h>

#include "../../Engine/LmbrCentral/include/LmbrCentral/Rendering/MaterialAsset.h"

class Cubism3UIComponent
	: public AZ::Component
	, public UiRenderBus::Handler
	//, public AZ::TickBus::Handler
{
public:
	AZ_COMPONENT(Cubism3UIComponent, "{B132DFB2-D204-4394-9C90-3F3A0BD6A70A}", AZ::Component);

	Cubism3UIComponent();
	~Cubism3UIComponent() override;

	// UiRenderInterface
	void Render() override;
	// ~UiRenderInterface

public:  // static member functions
	static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided) {
		provided.push_back(AZ_CRC("UiCubism3Service"));
	}

	static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible) {
		incompatible.push_back(AZ_CRC("UiCubism3Service"));
		incompatible.push_back(AZ_CRC("UiVisualService", 0xa864fdf8));
	}

	static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required) {
		required.push_back(AZ_CRC("UiElementService", 0x3dca7ad4));
		required.push_back(AZ_CRC("UiTransformService", 0x3a838e34));
	}

	static void Reflect(AZ::ReflectContext* context);

protected: // member functions
		// AZ::Component
		void Init() override;
		void Activate() override;
		void Deactivate() override;
		// ~AZ::Component

		// AZ::TickBus
		//void OnTick(float deltaTime, AZ::ScriptTimePoint time);
		// ~AZ::TickBus

private:
	class MocAsset {
	public:
		AZ_TYPE_INFO(MocAsset, "{7DB33C8B-8498-404C-A301-B0269AE60388}")
		static const char* GetFileFilter() {
			return "*.moc3";
		}
	};

	/*class Cubism3Asset {
	public:
		AZ_TYPE_INFO(Cubism3Asset, "")
			static const char* GetFileFilter() {
			return "*.model3.json";
		}
	};*/

	class MotionAsset {
	public:
		AZ_TYPE_INFO(MotionAsset, "")
		static const char* GetFileFilter() {
			return "*.motion3.json";
		}
	};

	AzFramework::SimpleAssetReference<MocAsset> m_mocPathname;
	AzFramework::SimpleAssetReference<LmbrCentral::TextureAsset> m_imagePathname;
	
	AZStd::vector<AzFramework::SimpleAssetReference<MotionAsset>> m_animationPathnames;

	//AzFramework::SimpleAssetReference<MotionAsset> *m_AnimationPathname;

private:
	void LoadObject();
	void ReleaseObject();

private:
	bool m_modelLoaded;

	csmMoc *moc;
	//void * mocBuf;
	csmModel * model;
	//void * modelBuf;
	csmFloatSink* sink;
	//void* sinkBuf;
	ITexture * texture;

	//animation stuff
	/*struct animStruct {
		csmAnimation* anim;
		//void* buff;
	};*/

	typedef struct AnimationLayer {
		/// Animation.
		csmAnimation *Animation;

		/// Animation state.
		csmAnimationState State;

		/// Blend function.
		csmFloatBlendFunction Blend;

		/// Blend weight.
		float Weight;
		
		///is the animation enabled
		bool enabled;
	} AnimationLayer;

	AZStd::vector<AnimationLayer*> animations;

	//parameter stuff
	typedef struct Parameter {
		AZStd::string name;
		int id;
		float min, max;
		float *val;
	} Parameter;

	AZStd::vector<Parameter*> parameters;

	//drawable stuff
	AZ::Matrix4x4 transform;

	typedef struct Drawable {
		AZStd::string name; //csmGetDrawableIds
		int id;
		csmFlags constFlags; //csmGetDrawableConstantFlags
		csmFlags dynFlags; //csmGetDrawableDynamicFlags //constant update?
		int texIndices; //csmGetDrawableTextureIndices //ignore?
		int drawOrder; //csmGetDrawableDrawOrders //?
		int renderOrder; //csmGetDrawableRenderOrders //?
		float opacity; //csmGetDrawableOpacities //color (alpha) //update as needed?
		int maskCount; //csmGetDrawableMaskCounts //ignore?
		const int *maskIndices; //csmGetDrawableMasks //ignore?

		int vertCount;
		SVF_P3F_C4B_T2F * data; //csmGetDrawableVertexPositions, csmGetDrawableVertexUvs //update only when needed to?

		const csmVector2* rawdata;
		const csmVector2* rawUVs;

		int numIndices; //csmGetDrawableIndexCounts
		const unsigned short * indices; //csmGetDrawableIndices

		bool visible;
		
		void update(csmModel* model, AZ::Matrix4x4 transform, bool transformUpdate);
	} Drawable;

	AZStd::vector<Drawable*> drawables;
};