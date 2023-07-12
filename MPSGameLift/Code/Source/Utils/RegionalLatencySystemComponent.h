
#pragma once

#include <MPSGameLift/IRegionalLatencyFinder.h>

#include <AzCore/Component/Component.h>
#include <AzCore/std/containers/vector.h>

namespace MPSGameLift
{
    class RegionalLatencySystemComponent final
        : public AZ::Component
        , public IRegionalLatencyFinder
    {
    public:
        static constexpr char RegionalEndpointUrlFormat[] = "https://dynamodb.%s.amazonaws.com";
        static constexpr const char* Regions[] = { "us-west-2", "us-east-1", "ru-central-1", "me-south-1" };


        AZ_COMPONENT(RegionalLatencySystemComponent, "{699E7875-5274-4516-88C9-A8D3010B9D3A}");

        /*
        * Reflects component data into the reflection contexts, including the serialization, edit, and behavior contexts.
        */
        static void Reflect(AZ::ReflectContext* context);

        RegionalLatencySystemComponent();
        ~RegionalLatencySystemComponent() override;

        /* IRegionalLatencyFinder overrides... */
        void RequestLatencies() override;
        bool HasLatencies() const override;
        AZStd::chrono::milliseconds GetLatencyForRegion(const AZStd::string& region) const override;
   
    protected:
        void Activate() override;
        void Deactivate() override;

    private:
        AZStd::unordered_map<AZStd::string, AZStd::chrono::milliseconds> m_latencyMap;
    };
} // namespace MPSGameLift
