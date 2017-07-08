
#include "StdAfx.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include "Cubism3SystemComponent.h"
#include "Cubism3UIComponent.h"

namespace Cubism3
{
	const AZStd::list<AZ::ComponentDescriptor*>* Cubism3SystemComponent::m_componentDescriptors = nullptr;

    void Cubism3SystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
			AzFramework::SimpleAssetReference<MocAsset>::Register(*serialize);
			AzFramework::SimpleAssetReference<Cubism3Asset>::Register(*serialize);

            serialize->Class<Cubism3SystemComponent, AZ::Component>()
                ->Version(0)
                ->SerializerForEmptyClass();

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<Cubism3SystemComponent>("Cubism3", "Provides Cubism3 support to LyShine.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "UI")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void Cubism3SystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("Cubism3Service"));
    }

    void Cubism3SystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("Cubism3Service"));
    }

    void Cubism3SystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void Cubism3SystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC("LyShineService", 0xae98ab29));
		dependent.push_back(AZ_CRC("AssetDatabaseService", 0x3abf5601));
		dependent.push_back(AZ_CRC("AssetCatalogService", 0xc68ffc57));
    }

    void Cubism3SystemComponent::Init()
    {
		CryLog("[Cubism3] Cubism3 System Component initializing.");
		CryLog("[Cubism3] Cubism3 Version: %i", csmGetVersion());
		csmSetLogFunction([](const char* message) -> void { CryLog("[Cubism3] %s", message); });
		CryLog("[Cubism3] Cubism3 System Component ready.");
    }

    void Cubism3SystemComponent::Activate()
    {
        Cubism3RequestBus::Handler::BusConnect();

		RegisterComponentTypeForMenuOrdering(Cubism3UIComponent::RTTI_Type());
    }

    void Cubism3SystemComponent::Deactivate()
    {
        Cubism3RequestBus::Handler::BusDisconnect();
    }

	////////////////////////////////////////////////////////////////////////////////////////////////////
	void Cubism3SystemComponent::InitializeSystem() {
		// Not sure if this is still required
		//gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(&g_system_event_listener_ui);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	void Cubism3SystemComponent::RegisterComponentTypeForMenuOrdering(const AZ::Uuid& typeUuid) {
		m_componentTypes.push_back(typeUuid);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	const AZStd::vector<AZ::Uuid>* Cubism3SystemComponent::GetComponentTypesForMenuOrdering() {
		return &m_componentTypes;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	const AZStd::list<AZ::ComponentDescriptor*>* Cubism3SystemComponent::GetLyShineComponentDescriptors() {
		return m_componentDescriptors;
	}
	
	////////////////////////////////////////////////////////////////////////////////////////////////////
	void Cubism3SystemComponent::SetCubism3ComponentDescriptors(const AZStd::list<AZ::ComponentDescriptor*>* descriptors) {
		m_componentDescriptors = descriptors;
	}
}
