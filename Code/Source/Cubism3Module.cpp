
#include "StdAfx.h"
#include <platform_impl.h>

#include "Cubism3SystemComponent.h"
#include "Cubism3UIComponent.h"

#include <IGem.h>

namespace Cubism3
{
    class Cubism3Module
        : public CryHooksModule
    {
    public:
        AZ_RTTI(Cubism3Module, "{71583B80-4CDE-480F-8E97-E753F35F22B5}", CryHooksModule);

        Cubism3Module()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                Cubism3SystemComponent::CreateDescriptor(),
				Cubism3UIComponent::CreateDescriptor()
            });

			Cubism3SystemComponent::SetCubism3ComponentDescriptors(&m_descriptors);
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<Cubism3SystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Cubism3_0523ea9b22264a73a494263c165760e6, Cubism3::Cubism3Module)
