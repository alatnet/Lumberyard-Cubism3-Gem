
#pragma once

#include <AzCore/EBus/EBus.h>

namespace Cubism3
{
    class Cubism3Requests
        : public AZ::EBusTraits
    {

    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        // Public functions
    };
    using Cubism3RequestBus = AZ::EBus<Cubism3Requests>;
} // namespace Cubism3
