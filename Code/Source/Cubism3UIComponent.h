#pragma once

#include <IRenderer.h>
#include <Cry_Color.h>
#include <ITexture.h>
#include <VertexFormats.h>
#include <CryThread.h>

#include <LyShine/Bus/UiRenderBus.h>
#include <LyShine/Bus/UiTransformBus.h>
#include <LyShine/Bus/UiTransform2dBus.h>
#include <LyShine/Bus/UiRenderControlBus.h>
#include <LyShine/Bus/UiMaskBus.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/SerializeContext.h>

#include "../../Engine/LmbrCentral/include/LmbrCentral/Rendering/MaterialAsset.h"

#include "Cubism3Assets.h"
#include <Cubism3/Cubism3UIBus.h>

#include "Live2DCubismCore.h"

#ifdef USE_CUBISM3_ANIM_FRAMEWORK
#include "Live2DCubismFramework.h"
#include "Live2DCubismFrameworkInternal.h"
#endif


#ifdef ENABLE_CUBISM3_DEBUG
	#ifdef ENABLE_CUBISM3_DEBUGLOG
		#define CLOG(...) CryLog(__VA_ARGS__)
	#else
		#define CLOG(...)
	#endif
	#ifdef ENABLE_CUBISM3_THREADLOG
		#define TLOG(...) CryLog(__VA_ARGS__)
	#else
		#define TLOG(...)
	#endif
#else
	#define CLOG(...)
	#define TLOG(...)
#endif

namespace Cubism3 {
	class ModelParameter {
	public:
		AZ_RTTI(ModelParameter, "{F804F1A2-F3F5-4489-B326-2906F90FCB0F}");

	public:
		virtual ~ModelParameter() {}
		AZStd::string name;
		int id;
		float min, max;
		float *val;

	public: //editor stuff
		AZ::Edit::ElementData ed;
		void InitEdit();

	public: //RTTI stuff
		static void Reflect(AZ::SerializeContext* serializeContext);
	};

	class ModelParametersGroup {
	public:
		AZ_TYPE_INFO(ModelParametersGroup, "{0C617BC0-697E-4BEA-856B-F56776D7C32B}");

	public:
		AZStd::string m_name;
		AZStd::vector<ModelParameter*>  m_params;
		AZStd::unordered_map<AZStd::string, int> m_idMap;

		ModelParameter* at(unsigned int index) { return m_params.at(index); }
		size_t size() { return this->m_params.size(); }

		void Clear();

		ModelParametersGroup() : m_name("Parameters") {}
		~ModelParametersGroup() { Clear(); }

		// Disallow copying, only moving
		ModelParametersGroup(const ModelParametersGroup& rhs) = delete;
		ModelParametersGroup& operator=(ModelParametersGroup&) = delete;

		ModelParametersGroup(ModelParametersGroup&& rhs) { *this = AZStd::move(rhs); }
		ModelParametersGroup& operator=(ModelParametersGroup&& rhs);

	public:
		static void Reflect(AZ::SerializeContext* serializeContext);
	};

	class ModelPart {
	public:
		AZ_RTTI(ModelPart, "{0E84D0AB-9ECF-4654-BD50-7D16D816C554}");

	public:
		virtual ~ModelPart() {}
		AZStd::string name;
		int id;
		float *val;

	public: //editor stuff
		AZ::Edit::ElementData ed;
		void InitEdit();

	public: //RTTI stuff
		static void Reflect(AZ::SerializeContext* serializeContext);
	};

	class ModelPartsGroup {
	public:
		AZ_TYPE_INFO(ModelPartsGroup, "{59D3B9A9-E175-40C4-896A-AD85C8DE7D4F}");

	public:
		AZStd::string m_name;
		AZStd::vector<ModelPart*>  m_parts;
		AZStd::unordered_map<AZStd::string, int> m_idMap;

		ModelPart* at(unsigned int index) { return m_parts.at(index); }
		size_t size() { return this->m_parts.size(); }

		void Clear();

		ModelPartsGroup() : m_name("Parts") {}
		~ModelPartsGroup() { Clear(); }

		// Disallow copying, only moving
		ModelPartsGroup(const ModelPartsGroup& rhs) = delete;
		ModelPartsGroup& operator=(ModelPartsGroup&) = delete;

		ModelPartsGroup(ModelPartsGroup&& rhs) { *this = AZStd::move(rhs); }
		ModelPartsGroup& operator=(ModelPartsGroup&& rhs);

	public:
		static void Reflect(AZ::SerializeContext* serializeContext);
	};

	class Cubism3UIComponent
		: public AZ::Component
		, public UiRenderBus::Handler
		, public Cubism3UIBus::Handler
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

	private: //dynamic editor listing
		static const AZ::Edit::ElementData* GetEditData(const void* handlerPtr, const void* elementPtr, const AZ::Uuid& elementType);
		const AZ::Edit::ElementData* GetDataElement(const void* element, const AZ::Uuid& typeUuid) const;

		struct ElementInfo {
			AZ::Uuid m_uuid;                    // Type uuid for the class field that should use this edit data.
			AZ::Edit::ElementData m_editData;   // Edit metadata (name, description, attribs, etc).
		};
		AZStd::unordered_map<const void*, ElementInfo> m_dataElements;

	public:
		// UiRenderInterface
		void Render() override;
		// ~UiRenderInterface

	public:
		// Cubism3UIBus
		//load type
		void SetLoadType(LoadType lt);
		LoadType GetLoadType() { return this->lType; }

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

		//threading
		void SetThreading(Cubism3UIInterface::Threading t);
		Cubism3UIInterface::Threading GetThreading();
		void SetMultiThreadLimiter(unsigned int limiter);
		unsigned int GetMultiThreadLimiter() { return this->threadLimiter; }
		// ~Cubism3UIBus
		
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

		#ifdef USE_CUBISM3_ANIM_FRAMEWORK
		void LoadAnimation();
		void FreeAnimation();
		#endif

	private: //on change notifications
		void OnMocFileChange();
		void OnImageFileChange();
		void OnJSONFileChange();
		void OnThreadingChange();
		void OnFillChange();
		void OnLoadTypeChange();

	private: //rendering updating
		void PreRender();
		void PostRender();
		void EnableMasking();
		void DisableMasking();

	private: //misc funcs
		bool IsLoadTypeSingle() { return this->lType == Single; }
		bool IsLoadTypeJSON() { return this->lType == JSON; }

	private:
		csmMoc * moc;
		csmModel * model;
		ITexture * texture; //used by single
		AZStd::vector<ITexture*> textures; //used by json

		AZ::Vector2 modelCanvasSize;
		AZ::Vector2 modelOrigin;
		float modelAspect;
		unsigned int numTextures;

		AZ::Vector2 modelSize;
		bool fill;

		bool modelLoaded;

		LoadType lType;

	private: //animation stuff
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

	private: //asset stuff
		AzFramework::SimpleAssetReference<MocAsset> m_mocPathname;
		AzFramework::SimpleAssetReference<LmbrCentral::TextureAsset> m_imagePathname;
		
		AzFramework::SimpleAssetReference<Cubism3Asset> m_jsonPathname;
		AZStd::vector<AzFramework::SimpleAssetReference<LmbrCentral::TextureAsset>> m_imagesPathname;

		//AZStd::vector<AzFramework::SimpleAssetReference<MotionAsset>> m_animationPathnames;
		//AzFramework::SimpleAssetReference<MotionAsset> *m_AnimationPathname;

	private: //parameter stuff
		ModelParametersGroup params;
		//AZStd::vector<ModelParameter*> parameters;
		//AZStd::unordered_map<AZStd::string, int> parametersMap; //using a map/hash table should be faster in finding indexes by name rather than searching for it sequentially.

	private: //part stuff
		ModelPartsGroup parts;
		/*AZStd::vector<ModelPart*> parts;
		AZStd::unordered_map<AZStd::string, int> partsMap;*/

	private: //drawable stuff
		AZ::Matrix4x4 transform, uvTransform, prevViewport;
		bool transformUpdated;

		UiTransform2dInterface::Anchors prevAnchors;
		UiTransform2dInterface::Offsets prevOffsets;

		typedef struct Drawable {
			AZStd::string name; //csmGetDrawableIds
			int id;
			csmFlags constFlags; //csmGetDrawableConstantFlags
			csmFlags dynFlags; //csmGetDrawableDynamicFlags //constant update?
			int texId; //csmGetDrawableTextureIndices //used when using model3.json
			int drawOrder; //csmGetDrawableDrawOrders
			int renderOrder; //csmGetDrawableRenderOrders
			float opacity; //csmGetDrawableOpacities //color (alpha) //update as needed?
			uint32 packedOpacity;

			int maskCount; //csmGetDrawableMaskCounts
			uint16 * maskIndices; //csmGetDrawableMasks

			AZ::Matrix4x4 *transform, *uvTransform;

			int vertCount;
			SVF_P3F_C4B_T2F * verts; //csmGetDrawableVertexPositions, csmGetDrawableVertexUvs //update only when needed to?

			const csmVector2* rawVerts;
			const csmVector2* rawUVs;

			int indicesCount; //csmGetDrawableIndexCounts
			uint16 * indices; //csmGetDrawableIndices

			bool visible;

			void update(csmModel* model, bool transformUpdate, bool &renderOrderChanged);
		} Drawable;

		AZStd::vector<Drawable*> drawables;

		bool renderOrderChanged;

		#ifdef ENABLE_CUBISM3_DEBUG
		bool wireframe;
		#endif

	private: //threading stuff
		Cubism3UIInterface::Threading m_threading;
		static Cubism3UIInterface::Threading m_threadingOverride;

		class DrawableThreadBase : public CryThread<CryRunnable> {
		public:
			DrawableThreadBase(AZStd::vector<Drawable*> *drawables);
			virtual ~DrawableThreadBase() {}
		public:
			void SetModel(csmModel * model) { this->m_model = model; }
			csmModel * GetModel() { return this->m_model; }
			void SetTransformUpdate(bool update) { this->m_transformUpdate = update; }
			bool GetTransformUpdate() { return this->m_transformUpdate; }
		public:
			virtual bool RenderOrderChanged() { return this->m_renderOrderChanged; }
			virtual void SetRenderOrderChanged(bool changed) { this->m_renderOrderChanged = changed; }
		public:
			virtual void Cancel();
			virtual void Run() = 0;
			virtual void WaitTillReady();
		protected:
			bool m_renderOrderChanged;
		protected:
			AZStd::vector<Drawable*> *m_drawables;
			csmModel * m_model;
			bool m_transformUpdate;
		protected:
			bool m_canceled;
			CryMutex mutex;
		};

		//one thread to rule them all.
		class DrawableSingleThread : public DrawableThreadBase {
		public:
			DrawableSingleThread(AZStd::vector<Drawable*> *drawables) : DrawableThreadBase(drawables) {}
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
			DrawableMultiThread(AZStd::vector<Drawable*> *drawables, unsigned int limiter);
			~DrawableMultiThread();
		public:
			void Run();
		public:
			bool RenderOrderChanged();
		protected:
			Drawable * GetNextDrawable();
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
				CryMutex mutex;
				DrawableMultiThread *m_dmt;
				bool m_canceled;
			};
		private:
			SubThread ** m_threads;
			unsigned int numThreads;
			CryRWLock rwmutex;
			CryMutex dMutex;
			AZStd::vector<Drawable*> *drawables;
			unsigned int nextDrawable;
		};

		DrawableThreadBase *tJob;
		unsigned int threadLimiter;
		CryMutex threadMutex; //used to block when creating or destroying the update thread.

	private: //masking stuff
		bool enableMasking;
		//bool drawMaskVisualBehindChildren;
		//bool drawMaskVisualInFrontOfChildren;
		bool useAlphaTest;
		//bool maskInteraction;
		int priorBaseState;
		//Pass currPass;

	#ifdef ENABLE_CUBISM3_DEBUG
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
		SFunc stencilFunc, stencilCCWFunc;
		bool sTwoSided;

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

		SOp opFail, opZFail, opPass;
		SOp opCCWFail, opCCWZFail, opCCWPass;
	#endif
	};
}