
#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace Cubism3 {

	#ifndef CUBISM3_MULTITHREAD_LIMITER
		#define CUBISM3_MULTITHREAD_LIMITER 10
	#endif

	class Cubism3UIInterface
		: public AZ::ComponentBus {
	public:
		virtual ~Cubism3UIInterface() {}
	public: //load type
		/*enum LoadType {
			Single, //Moc/Texture(s)
			JSON //model3.json
		};
		virtual void SetLoadType(LoadType lt) = 0;
		virtual LoadType GetLoadType() = 0;*/

	public: //pathnames (Moc/Texture)
		virtual void SetMocPathname(AZStd::string path) = 0;
		virtual void SetTexturePathname(AZStd::string path) = 0;
		virtual AZStd::string GetMocPathname() = 0;
		virtual AZStd::string GetTexturePathname() = 0;

	public: //JSON pathname
		/*virtual void SetJSONPathname(AZStd::string path) = 0;
		virtual AZStd::string GetJSONPathname() = 0;*/
		
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
	public:
		static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
	public: //render types
		enum RenderType {
			rtSequential, //sequential order
			rtDraw, //draw order
			rtRender //render order
		};
		virtual void SetRenderType(RenderType rt) = 0;
		virtual RenderType GetRenderType() = 0;

	//#ifdef CUBISM3_ENABLE_THREADING
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
	//#endif
	};
	using Cubism3UIBus = AZ::EBus<Cubism3UIInterface>;
} // namespace Cubism3