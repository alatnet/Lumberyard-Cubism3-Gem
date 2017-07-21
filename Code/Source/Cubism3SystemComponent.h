
#pragma once

#include <AzCore/Component/Component.h>

#include <LyShine/Bus/UiSystemBus.h>

#include "Live2DCubismCore.h"

#include "Cubism3Assets.h"

namespace Cubism3
{
    class Cubism3SystemComponent
        : public AZ::Component
		, protected UiSystemBus::Handler
    {
    public:
        AZ_COMPONENT(Cubism3SystemComponent, "{83111C9B-F11C-4429-81FF-B95D36893258}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

		static void SetCubism3ComponentDescriptors(const AZStd::list<AZ::ComponentDescriptor*>* descriptors);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // Cubism3RequestBus interface implementation

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // UiSystemBus interface implementation
		void InitializeSystem() override;
		void RegisterComponentTypeForMenuOrdering(const AZ::Uuid& typeUuid) override;
		const AZStd::vector<AZ::Uuid>* GetComponentTypesForMenuOrdering() override;
		const AZStd::list<AZ::ComponentDescriptor*>* GetLyShineComponentDescriptors();
		////////////////////////////////////////////////////////////////////////

	protected:
		// The components types registers in order to cotrol their order in the add component
		// menu and the properties pane - may go away soon
		AZStd::vector<AZ::Uuid> m_componentTypes;

		// We only store this in order to generate metrics on LyShine specific components
		static const AZStd::list<AZ::ComponentDescriptor*>* m_componentDescriptors;
	private:
		static void LogMessage(const char * message);
    };
}
