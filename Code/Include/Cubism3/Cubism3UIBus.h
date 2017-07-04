
#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace Cubism3 {
	class Cubism3UIInterface
		: public AZ::ComponentBus {
	public:
		virtual ~Cubism3UIInterface() {}
	public: //pathnames
		virtual void SetMocPathname(AZStd::string path) = 0;
		virtual void SetTexturePathname(AZStd::string path) = 0;
		virtual AZStd::string GetMocPathname() = 0;
		virtual AZStd::string GetTexturePathname() = 0;

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
			rtSequential,
			rtDraw,
			rtRender
		};
		virtual void SetRenderType(RenderType rt) = 0;
		virtual RenderType GetRenderType() = 0;
	};
	using Cubism3UIBus = AZ::EBus<Cubism3UIInterface>;
} // namespace Cubism3