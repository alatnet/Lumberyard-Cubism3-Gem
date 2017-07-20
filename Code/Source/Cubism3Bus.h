#pragma once

#include "Live2DCubismCore.h"

#include <AzCore/Component/ComponentBus.h>

namespace Cubism3 {
    class Cubism3Requests
        : public AZ::ComponentBus {
	public:
		virtual csmModel* GetModel() = 0;

    public:
		static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
    };
    using Cubism3Bus = AZ::EBus<Cubism3Requests>;
} // namespace Cubism3
