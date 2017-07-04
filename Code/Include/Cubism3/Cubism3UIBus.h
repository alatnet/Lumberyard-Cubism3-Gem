
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
		virtual float GetParameterMax(int index) = 0;
		virtual float GetParameterMin(int index) = 0;
		virtual float GetParameterValue(int index) = 0;
		virtual void SetParameterValue(int index, float value) = 0;
	public:
		static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
		// Public functions
	};
	using Cubism3UIBus = AZ::EBus<Cubism3UIInterface>;
} // namespace Cubism3