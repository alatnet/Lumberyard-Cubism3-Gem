#pragma once

#include <LyShine/Bus/UiRenderBus.h>

#include <Cubism3/Cubism3UIBus.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Vector3.h>

#include "../../Engine/LmbrCentral/include/LmbrCentral/Rendering/MaterialAsset.h"

#include "Live2DCubismCore.h"

#ifdef USE_CUBISM3_ANIM_FRAMEWORK
#include "Live2DCubismFramework.h"
#include "Live2DCubismFrameworkInternal.h"
#endif

#include <ITexture.h>
#include <VertexFormats.h>

#include <AZCore/RTTI/TypeInfo.h>

#include <CryThread.h>

namespace Cubism3 {
	class Cubism3UIComponent
		: public AZ::Component
		, public UiRenderBus::Handler
		, public Cubism3UIBus::Handler {

	public:
		AZ_COMPONENT(Cubism3UIComponent, "{B132DFB2-D204-4394-9C90-3F3A0BD6A70A}", AZ::Component)

	public:
		Cubism3UIComponent();
		~Cubism3UIComponent() override;

	public:
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

	protected:
		// Cubism3UIBus
		//pathnames
		void SetMocPathname(AZStd::string path);
		void SetTexturePathname(AZStd::string path);
		AZStd::string GetMocPathname();
		AZStd::string GetTexturePathname();
		
		//parameters
		int GetParameterCount();
		int GetParameterIdByName(AZStd::string name);
		AZStd::string GetParameterName(int index);
		
		//parameters by index
		float GetParameterMaxI(int index);
		float GetParameterMinI(int index);
		float GetParameterValueI(int index);
		void SetParameterValueI(int index, float value);
		
		//parameters by name
		float GetParameterMaxS(AZStd::string name);
		float GetParameterMinS(AZStd::string name);
		float GetParameterValueS(AZStd::string name);
		void SetParameterValueS(AZStd::string name, float value);

		//rendertype
		void SetRenderType(Cubism3UIInterface::RenderType rt);
		Cubism3UIInterface::RenderType GetRenderType();

		//threading
		void SetThreading(Cubism3UIInterface::Threading t);
		Cubism3UIInterface::Threading GetThreading();
		void SetMultiThreadLimiter(unsigned int limiter);
		unsigned int GetMultiThreadLimiter() { return this->threadLimiter; }
		// ~Cubism3UIBus

	private:
		class Cubism3Asset {
		public:
			AZ_TYPE_INFO(Cubism3Asset, "{A679F1C0-60A1-48FB-8107-A68195D76CF2}");
			static const char* GetFileFilter() {
				return "*.model3.json";
			}
		};

		/*class MotionAsset {
		public:
			AZ_TYPE_INFO(MotionAsset, "{DC1BA430-5D5E-4A09-BA5F-1FB05180C6A1}");
			static const char* GetFileFilter() {
				return "*.motion3.json";
			}
		};*/

		class MocAsset {
		public:
			AZ_TYPE_INFO(MocAsset, "{7DB33C8B-8498-404C-A301-B0269AE60388}");
			static const char* GetFileFilter() {
				return "*.moc3";
			}
		};

		AzFramework::SimpleAssetReference<MocAsset> m_mocPathname;
		AzFramework::SimpleAssetReference<LmbrCentral::TextureAsset> m_imagePathname;

		//AzFramework::SimpleAssetReference<Cubism3Asset> m_jsonPathname;

		//AZStd::vector<AzFramework::SimpleAssetReference<MotionAsset>> m_animationPathnames;
		//AzFramework::SimpleAssetReference<MotionAsset> *m_AnimationPathname;

	private:
		void LoadObject();
		void ReleaseObject();

	private:
		void OnMocFileChange();
		void OnImageFileChange();

	private:
		void LoadMoc();
		void FreeMoc();
		void LoadTexture();
		void FreeTexture();

		#ifdef USE_CUBISM3_ANIM_FRAMEWORK
		void LoadAnimation();
		void FreeAnimation();
		#endif

	private:
		//! Copying not allowed
		Cubism3UIComponent(const Cubism3UIComponent&);
		Cubism3UIComponent& operator=(const Cubism3UIComponent&);

	private:
		csmMoc *moc;
		//void * mocBuf;
		csmModel * model;
		//void * modelBuf;
		ITexture * texture;

		//AZStd::vector<ITexture*> textures;

		//animation stuff
		#ifdef USE_CUBISM3_ANIM_FRAMEWORK
		csmFloatSink* sink;
		//void* sinkBuf;

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
		#endif

	private: //parameter stuff
		typedef struct Parameter {
			AZStd::string name;
			int id;
			float min, max;
			float *val;
		} Parameter;

		AZStd::vector<Parameter*> parameters;
		AZStd::unordered_map<AZStd::string, int> parametersMap; //using a map/hash table should be faster in finding indexes by name rather than searching for it sequentially.

	private: //drawable stuff
		AZ::Matrix4x4 prevTransform, transform;

		typedef struct Drawable {
			AZStd::string name; //csmGetDrawableIds
			int id;
			csmFlags constFlags; //csmGetDrawableConstantFlags
			csmFlags dynFlags; //csmGetDrawableDynamicFlags //constant update?
			int texIndices; //csmGetDrawableTextureIndices //used when using model3.json
			int drawOrder; //csmGetDrawableDrawOrders
			int renderOrder; //csmGetDrawableRenderOrders
			float opacity; //csmGetDrawableOpacities //color (alpha) //update as needed?
			uint32 packedOpacity;

			//UiMaskBus usage
			int maskCount; //csmGetDrawableMaskCounts
			const int *maskIndices; //csmGetDrawableMasks

			int vertCount;
			SVF_P3F_C4B_T2F * verts; //csmGetDrawableVertexPositions, csmGetDrawableVertexUvs //update only when needed to?

			const csmVector2* rawVerts;
			const csmVector2* rawUVs;

			int indicesCount; //csmGetDrawableIndexCounts
			const unsigned short * indices; //csmGetDrawableIndices

			bool visible;

			void update(csmModel* model, AZ::Matrix4x4 transform, bool transformUpdate, bool &drawOrderChanged, bool &renderOrderChanged);
		} Drawable;

		AZStd::vector<Drawable*> drawables;

	private: //rendering order stuff
		int drawCount;
		const int* drawOrder;
		const int* renderOrder;

		Cubism3UIInterface::RenderType rType;

	private: //threading stuff
		Cubism3UIInterface::Threading m_threading;
		static Cubism3UIInterface::Threading m_threadingOverride;

		class DrawableThreadBase : public CryThread<CryRunnable> {
		public:
			DrawableThreadBase(AZStd::vector<Drawable*> &drawables, AZ::Matrix4x4 &transform);
			virtual ~DrawableThreadBase() {}
		public:
			void SetModel(csmModel * model) { this->m_model = model; }
			csmModel * GetModel() { return this->m_model; }
			void SetTransformUpdate(bool update) { this->m_transformUpdate = update; }
			bool GetTransformUpdate() { return this->m_transformUpdate; }
			AZ::Matrix4x4 * GetTransform() { return this->m_transform; }
		public:
			virtual bool DrawOrderChanged() { return this->m_drawOrderChanged; }
			virtual bool RenderOrderChanged() { return this->m_renderOrderChanged; }

			virtual void SetDrawOrderChanged(bool changed) { this->m_drawOrderChanged = changed; }
			virtual void SetRenderOrderChanged(bool changed) { this->m_renderOrderChanged = changed; }
		public:
			virtual void Cancel();
			virtual void Run() = 0;
			virtual void WaitTillReady();
		protected:
			bool m_drawOrderChanged, m_renderOrderChanged;
		protected:
			AZStd::vector<Drawable*> *m_drawables;
			csmModel * m_model;
			AZ::Matrix4x4 *m_transform;
			bool m_transformUpdate;
		protected:
			bool m_canceled;
			CryMutex mutex;
		};

		class DrawableSingleThread : public DrawableThreadBase {
		public:
			DrawableSingleThread(AZStd::vector<Drawable*> &drawables, AZ::Matrix4x4 &transform) : DrawableThreadBase(drawables, transform) {}
		public:
			void Run();
		};

		class DrawableMultiThread : public DrawableThreadBase {
		public:
			DrawableMultiThread(AZStd::vector<Drawable*> &drawables, AZ::Matrix4x4 &transform, unsigned int limiter);
			~DrawableMultiThread();
		public:
			void Run();
		public:
			bool DrawOrderChanged();
			bool RenderOrderChanged();
		public:
		private:
			class SubThread : public CryThread<CryRunnable> {
			public:
				SubThread(Drawable* drawable, DrawableMultiThread * dmt) : m_d(drawable), m_dmt(dmt), m_canceled(false) {}
			public:
				void Cancel();
				void Run();
			public:
				void WaitTillDone();
			private:
				Drawable * m_d;
				DrawableMultiThread *m_dmt;
				bool m_canceled;
				CryMutex mutex;
			};
		private:
			AZStd::vector<SubThread *> m_threads;
			CryRWLock rwmutex;
			CrySemaphore * semaphore;
		};

		DrawableThreadBase *tJob;
		unsigned int threadLimiter;
		CryMutex threadMutex; //used to block when creating or destroying the update thread.
	};
}