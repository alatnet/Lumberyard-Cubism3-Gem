#pragma once

#include <IRenderer.h>
#include <ITexture.h>
#include <CryThread.h>

#include <LyShine/Bus/UiRenderBus.h>
#include <LyShine/Bus/UiTransformBus.h>
#include <LyShine/Bus/UiTransform2dBus.h>
#include <LyShine/Bus/UiRenderControlBus.h>
#include <LyShine/Bus/UiMaskBus.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/SerializeContext.h>

//#include "../../Engine/LmbrCentral/include/LmbrCentral/Rendering/MaterialAsset.h"
#include <../../LmbrCentral/Code/include/LmbrCentral/Rendering/MaterialAsset.h>

#include "Cubism3Assets.h"
#include "Cubism3EditorData.h"
#include "Cubism3Debug.h"
#include "Cubism3Animation.h"
#include "Cubism3Drawable.h"
#include "Cubism3Bus.h"
#include <Cubism3/Cubism3UIBus.h>

#include "Live2DCubismCore.h"

#if defined(CUBISM3_ANIMATION_FRAMEWORK) && CUBISM3_ANIMATION_FRAMEWORK == 1
#include <Live2DCubismFramework.h>
#endif

namespace Cubism3 {
	class Cubism3UIComponent
		: public AZ::Component
		, public UiRenderBus::Handler
		, public Cubism3UIBus::Handler
		, public Cubism3AnimationBus::Handler
	{

	public:
		AZ_COMPONENT(Cubism3UIComponent, "{B132DFB2-D204-4394-9C90-3F3A0BD6A70A}", AZ::Component)

	public:
		Cubism3UIComponent();
		~Cubism3UIComponent() override;

	private:
		//! Copying not allowed
		Cubism3UIComponent(const Cubism3UIComponent&);
		Cubism3UIComponent& operator=(const Cubism3UIComponent&);

	protected: // member functions
		// AZ::Component
		void Init() override;
		void Activate() override;
		void Deactivate() override;
		// ~AZ::Component

	public:  // static member functions
		static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided) {
			provided.push_back(AZ_CRC("UiCubism3Service", 0x02be01c3));
		}
		static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible) {
			incompatible.push_back(AZ_CRC("UiCubism3Service", 0x02be01c3));
			incompatible.push_back(AZ_CRC("UiVisualService", 0xa864fdf8));
		}
		static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required) {
			required.push_back(AZ_CRC("UiElementService", 0x3dca7ad4));
			required.push_back(AZ_CRC("UiTransformService", 0x3a838e34));
		}

		static void Reflect(AZ::ReflectContext* context);

	private: //dynamic editor listing
		static const AZ::Edit::ElementData* GetEditData(const void* handlerPtr, const void* elementPtr, const AZ::Uuid& elementType);
		const AZ::Edit::ElementData* GetDataElement(const void* element, const AZ::Uuid& typeUuid) const;
		
		AZStd::unordered_map<const void*, ElementInfo*> m_dataElements;

	public:
		// UiRenderInterface
		void Render() override;
		// ~UiRenderInterface

	public:
		// Cubism3UIBus
		//load type
		void SetLoadType(LoadType lt);
		LoadType GetLoadType() { return this->m_lType; }

		//pathnames
		void SetMocPathname(AZStd::string path);
		void SetTexturePathname(AZStd::string path);
		AZStd::string GetMocPathname();
		AZStd::string GetTexturePathname();

		//json path
		void SetJSONPathname(AZStd::string path);
		AZStd::string GetJSONPathname();
		
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

		//parts
		int GetPartCount();
		int GetPartIdByName(AZStd::string name);
		AZStd::string GetPartName(int index);

		//parts by index
		float GetPartOpacityI(int index);
		void SetPartOpacityI(int index, float value);

		//parts by name
		float GetPartOpacityS(AZStd::string name);
		void SetPartOpacityS(AZStd::string name, float value);

	public: //opacity
		virtual float GetOpacity() { return this->m_opacity; }
		virtual void SetOpacity(float opacity);

		//threading
		void SetThreading(Cubism3UIInterface::Threading t);
		Cubism3UIInterface::Threading GetThreading();
		void SetMultiThreadLimiter(unsigned int limiter);
		unsigned int GetMultiThreadLimiter() { return this->m_threadLimiter; }
		// ~Cubism3UIBus

	public:
		// Cubism3AnimationBus
		bool AddAnimation(AZStd::string path);
		void RemoveAnimation(AZStd::string name);
		bool Loaded(AZStd::string name);
		void Play(AZStd::string name);
		void Stop(AZStd::string name);
		void Pause(AZStd::string name);
		void SetLooping(AZStd::string name, bool loop);
		bool IsPlaying(AZStd::string name);
		bool IsStopped(AZStd::string name);
		bool IsPaused(AZStd::string name);
		bool IsLooping(AZStd::string name);
		void Reset(AZStd::string name);
		void SetWeight(AZStd::string name, float weight);
		float GetWeight(AZStd::string name);
		void SetFloatBlend(AZStd::string name, Cubism3AnimationFloatBlend floatBlendFunc);
		// ~Cubism3AnimationBus
	private:
		Cubism3Animation * FindAnim(AZStd::string name);
		
	private:
		void LoadObject();
		void ReleaseObject();

	private:
		void LoadMoc();
		void FreeMoc();
		void LoadTexture();
		void FreeTexture();
		void LoadJson();
		void FreeJson();

	private: //on change notifications
		void OnMocFileChange();
		void OnImageFileChange();
		void OnJSONFileChange();
		void OnThreadingChange();
		void OnFillChange();
		void OnLoadTypeChange();
		void OnAnimControlsChange();

	private: //rendering updating
		void PreRender();
		void PostRender();
		void EnableMasking();
		void DisableMasking();

	private: //misc funcs
		bool IsLoadTypeSingle() { return this->m_lType == Single; }
		bool IsLoadTypeJSON() { return this->m_lType == JSON; }

	private:
		csmMoc * m_moc;
		csmModel * m_model;
		ITexture * m_texture; //used by single
		AZStd::vector<ITexture*> m_textures; //used by json

		AZ::Vector2 m_modelCanvasSize;
		AZ::Vector2 m_modelOrigin;
		float m_modelAspect;
		unsigned int m_numTextures;

		AZ::Vector2 m_modelSize;
		bool m_fill;

		bool m_modelLoaded;

		LoadType m_lType;

		float m_opacity, m_prevOpacity;

	private: //asset stuff
		MocAssetRef m_mocPathname;
		AzFramework::SimpleAssetReference<LmbrCentral::TextureAsset> m_imagePathname;
		
		Cubism3AssetRef m_jsonPathname;
		AZStd::vector<AzFramework::SimpleAssetReference<LmbrCentral::TextureAsset>> m_imagesPathname;

	private: //animations stuff
		AZStd::unordered_map<AZStd::string, Cubism3Animation*> m_animations;
		AZStd::vector<AnimationControl> m_animControls;

	#if defined(CUBISM3_ANIMATION_FRAMEWORK) && CUBISM3_ANIMATION_FRAMEWORK == 1
	private:
		csmModelHashTable* m_hashTable;
	#endif

	private: //parameter stuff
		ModelParametersGroup m_params;

	private: //part stuff
		ModelPartsGroup m_parts;

	private: //drawable stuff
		AZ::Matrix4x4 m_transform, m_uvTransform, m_prevViewport;
		bool m_transformUpdated;

		UiTransform2dInterface::Anchors m_prevAnchors;
		UiTransform2dInterface::Offsets m_prevOffsets;

	private:
		AZStd::vector<Cubism3Drawable*> m_drawables;

		bool m_renderOrderChanged;

		#ifdef ENABLE_CUBISM3_DEBUG
		bool m_wireframe;
		#endif

	private: //threading stuff
		Cubism3UIInterface::Threading m_threading;
		static Cubism3UIInterface::Threading m_threadingOverride;

		class DrawableThreadBase : public CryThread<CryRunnable> {
		public:
			DrawableThreadBase(AZStd::vector<Cubism3Drawable*> *drawables);
			virtual ~DrawableThreadBase() { this->Cancel(); }
		public:
			void SetModel(csmModel * model) { this->m_model = model; }
			csmModel * GetModel() { return this->m_model; }
			void SetTransformUpdate(bool update) { this->m_transformUpdate = update; }
			bool GetTransformUpdate() { return this->m_transformUpdate; }
		public: //animations stuff
			void SetDelta(float delta) { this->m_delta = delta; }
			float GetDelta() { return this->m_delta; }
			void SetAnimations(AZStd::unordered_map<AZStd::string, Cubism3Animation*> *animations) { this->m_animations = animations; }
			void SetParams(ModelParametersGroup * params) { this->m_params = params; }
			void SetParts(ModelPartsGroup * parts) { this->m_parts = parts; }
		public:
			void SetOpacity(float opacity) { this->m_opacity = opacity; }
		public:
			virtual bool RenderOrderChanged() { return this->m_renderOrderChanged; }
			virtual void SetRenderOrderChanged(bool changed) { this->m_renderOrderChanged = changed; }
		public:
			virtual void Cancel();
			virtual void Run() = 0;
			virtual void WaitTillReady();
		/*#if defined(CUBISM3_ANIMATION_FRAMEWORK) && CUBISM3_ANIMATION_FRAMEWORK == 1
		public:
			virtual void SetHashTable(csmModelHashTable* hashTable) { this->m_hashTable = hashTable; }
		protected:
			csmModelHashTable* m_hashTable;
		#endif*/
		protected:
			bool m_renderOrderChanged;
		protected:
			AZStd::vector<Cubism3Drawable*> *m_drawables;
			csmModel * m_model;
			bool m_transformUpdate;
		protected: //animations stuff
			float m_delta;
			AZStd::unordered_map<AZStd::string, Cubism3Animation*> *m_animations;
			ModelParametersGroup * m_params;
			ModelPartsGroup * m_parts;
			float m_opacity, m_prevOpacity;
		protected:
			bool m_canceled;
		};

		//one thread to rule them all.
		class DrawableSingleThread : public DrawableThreadBase {
		public:
			DrawableSingleThread(AZStd::vector<Cubism3Drawable*> *drawables) : DrawableThreadBase(drawables) {}
		public:
			void Run();
		};

		//each drawable gets a thread.
		/*class DrawableMultiThread : public DrawableThreadBase {
		public:
			DrawableMultiThread(AZStd::vector<Drawable*> &drawables, AZ::Matrix4x4 &transform, unsigned int limiter);
			~DrawableMultiThread();
		public:
			void Run();
		public:
			bool DrawOrderChanged();
			bool RenderOrderChanged();
		private:
			class SubThread : public CryThread<CryRunnable> {
			public:
				SubThread(Drawable* drawable, DrawableMultiThread * dmt) : m_d(drawable), m_dmt(dmt), m_canceled(false) {}
			public:
				void Cancel();
				void Run();
			public:
				void WaitTillReady();
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
		};*/

		//producer-consumer multi-thread
		//limited number of threads, each thread updates a drawable.
		class DrawableMultiThread : public DrawableThreadBase {
		public:
			DrawableMultiThread(AZStd::vector<Cubism3Drawable*> *drawables, unsigned int limiter);
			~DrawableMultiThread();
		public:
			void Run();
		public:
			bool RenderOrderChanged();
		protected:
			Cubism3Drawable * GetNextDrawable();
			float GetOpacity() { return this->m_opacity; }
			bool GetOpacityChanged() { return this->m_opacity != this->m_prevOpacity; }
		private:
			class SubThread : public CryThread<CryRunnable> {
			public:
				SubThread(DrawableMultiThread * dmt) : m_dmt(dmt), m_canceled(false) {}
			public:
				void Cancel();
				void Run();
			public:
				void WaitTillReady();
			private:
				DrawableMultiThread *m_dmt;
				bool m_canceled;
			};
		protected:
			//bool animations;
		private:
			SubThread ** m_threads;
			unsigned int m_numThreads;
			CryRWLock m_rwmutex;
			CryMutex m_dMutex;
			unsigned int m_nextDrawable;
		};

		DrawableThreadBase *m_thread;
		unsigned int m_threadLimiter;
		CryMutex m_threadMutex; //used to block when creating or destroying the update thread.

	private: //masking stuff
		int m_priorBaseState;

	#ifdef ENABLE_CUBISM3_DEBUG
	private: //masking stuff
		bool m_enableMasking;

		enum AlphaTest {
			ATDISABLE = -1,
			Greater = GS_ALPHATEST_GREATER,
			Less = GS_ALPHATEST_LESS,
			GEqual = GS_ALPHATEST_GEQUAL,
			LEqual = GS_ALPHATEST_LEQUAL
		};

		AlphaTest m_useAlphaTest;

		enum ColorMask {
			CMDISABLE = -1,
			NoR = GS_NOCOLMASK_R,
			NoG = GS_NOCOLMASK_G,
			NoB = GS_NOCOLMASK_B,
			NoA = GS_NOCOLMASK_A,
			RGB = GS_COLMASK_RGB,
			A = GS_COLMASK_A,
			None = GS_COLMASK_NONE
		};

		ColorMask m_useColorMask;

	private: //stencil stuff
		enum SFunc {
			FDISABLE = -1,
			ALWAYS = FSS_STENCFUNC_ALWAYS,
			NEVER = FSS_STENCFUNC_NEVER,
			LESS = FSS_STENCFUNC_LESS,
			LEQUAL = FSS_STENCFUNC_LEQUAL,
			GREATER = FSS_STENCFUNC_GREATER,
			GEQUAL = FSS_STENCFUNC_GEQUAL,
			EQUAL = FSS_STENCFUNC_EQUAL,
			NOTEQUAL = FSS_STENCFUNC_NOTEQUAL,
			MASK = FSS_STENCFUNC_MASK
		};
		SFunc m_stencilFunc, m_stencilCCWFunc;
		bool m_sTwoSided;

		enum SOp {
			ODISABLE = -1,
			KEEP = FSS_STENCOP_KEEP,
			REPLACE = FSS_STENCOP_REPLACE,
			INCR = FSS_STENCOP_INCR,
			DECR = FSS_STENCOP_DECR,
			ZERO = FSS_STENCOP_ZERO,
			INCR_WRAP = FSS_STENCOP_INCR_WRAP,
			DECR_WRAP = FSS_STENCOP_DECR_WRAP,
			INVERT = FSS_STENCOP_INVERT
		};

		SOp m_opFail, m_opZFail, m_opPass;
		SOp m_opCCWFail, m_opCCWZFail, m_opCCWPass;
	#endif
	};
}