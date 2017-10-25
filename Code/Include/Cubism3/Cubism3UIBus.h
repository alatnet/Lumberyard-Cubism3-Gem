#pragma once

namespace Cubism3 {
	#ifndef CUBISM3_MULTITHREAD_LIMITER
		#define CUBISM3_MULTITHREAD_LIMITER 4
	#endif

	class Cubism3UIInterface
		: public AZ::ComponentBus {
	public:
		virtual ~Cubism3UIInterface() {}
	public: //load type
		enum LoadType {
			Single, //Moc/Texture(s)
			JSON //model3.json
		};
		virtual void SetLoadType(LoadType lt) = 0;
		virtual LoadType GetLoadType() = 0;

	public: //pathnames (Moc/Texture)
		virtual void SetMocPathname(AZStd::string path) = 0;
		virtual void SetTexturePathname(AZStd::string path) = 0;
		virtual AZStd::string GetMocPathname() = 0;
		virtual AZStd::string GetTexturePathname() = 0;

	public: //JSON pathname
		virtual void SetJSONPathname(AZStd::string path) = 0;
		virtual AZStd::string GetJSONPathname() = 0;
		
	public: //parameters
		virtual int GetParameterCount() = 0;
		virtual int GetParameterIdByName(AZStd::string name) = 0;
		virtual AZStd::string GetParameterName(int index) = 0;

	public: //parameters by index
		virtual float GetParameterMaxI(int index) = 0;
		virtual float GetParameterMinI(int index) = 0;
		virtual float GetParameterValueI(int index) = 0;
		virtual void SetParameterValueI(int index, float value) = 0;

	public: //parameters by name
		virtual float GetParameterMaxS(AZStd::string name) = 0;
		virtual float GetParameterMinS(AZStd::string name) = 0;
		virtual float GetParameterValueS(AZStd::string name) = 0;
		virtual void SetParameterValueS(AZStd::string name, float value) = 0;

	public: //parts
		virtual int GetPartCount() = 0;
		virtual int GetPartIdByName(AZStd::string name) = 0;
		virtual  AZStd::string GetPartName(int index) = 0;

	public: //parts by index
		virtual float GetPartOpacityI(int index) = 0;
		virtual void SetPartOpacityI(int index, float value) = 0;

	public: //parts by name
		virtual float GetPartOpacityS(AZStd::string name) = 0;
		virtual void SetPartOpacityS(AZStd::string name, float value) = 0;

	public: //opacity
		virtual float GetOpacity() = 0;
		virtual void SetOpacity(float opacity) = 0;

	public: //threading
		enum Threading {
			NONE,
			SINGLE,
			MULTI,
			DISABLED //used internally defaults to NONE
		};
		virtual void SetThreading(Threading t) = 0;
		virtual Threading GetThreading() = 0;
		virtual void SetMultiThreadLimiter(unsigned int limiter) = 0;
		virtual unsigned int GetMultiThreadLimiter() = 0;
	public:
		static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
	};
	using Cubism3UIBus = AZ::EBus<Cubism3UIInterface>;
	

	typedef float(*Cubism3AnimationFloatBlend)(float base, float value, float weight);

	class Cubism3AnimationInterface
		: public AZ::ComponentBus {
	public:
		virtual bool AddAnimation(AZStd::string path) = 0; //animation path is also it's id/name.
		virtual void RemoveAnimation(AZStd::string name) = 0;
	public:
		virtual bool Loaded(AZStd::string name) = 0;

	public:
		virtual void Play(AZStd::string name) = 0;
		virtual void Stop(AZStd::string name) = 0;
		virtual void Pause(AZStd::string name) = 0;
		virtual void SetLooping(AZStd::string name, bool loop) = 0;

	public:
		virtual bool IsPlaying(AZStd::string name) = 0;
		virtual bool IsStopped(AZStd::string name) = 0;
		virtual bool IsPaused(AZStd::string name) = 0;
		virtual bool IsLooping(AZStd::string name) = 0;

	public:
		virtual void Reset(AZStd::string name) = 0;

	public:
		virtual void SetWeight(AZStd::string name, float weight) = 0;
		virtual float GetWeight(AZStd::string name) = 0;

	public:
		virtual void SetFloatBlend(AZStd::string name, Cubism3AnimationFloatBlend floatBlendFunc) = 0;

	public:
		static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
	};

	using Cubism3AnimationBus = AZ::EBus<Cubism3AnimationInterface>;
} // namespace Cubism3

namespace AZ {
	AZ_TYPE_INFO_SPECIALIZE(Cubism3::Cubism3UIInterface::Threading, "{6F52376F-69C4-4337-9ED3-956FDD6BFCA9}");
	AZ_TYPE_INFO_SPECIALIZE(Cubism3::Cubism3UIInterface::LoadType, "{822167BA-5A15-4F30-95B5-9DBAFBC9FE57}");
}