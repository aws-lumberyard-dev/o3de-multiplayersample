
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/std/containers/vector.h>
#include <MPSGameLift/MPSLatencyComponentInterface.h>

namespace MPSGameLift
{
    class IRegionalLatencyFinder
    {
    public:
        AZ_RTTI(IRegionalLatencyFinder, "{D2171936-1BC5-44B9-BC49-9666A829ED17}");

        virtual ~IRegionalLatencyFinder() = default;

        // Request latency checks for all set regions
        virtual void RequestLatencies() = 0;

        // Returns true if we measured all expected latencies
        virtual bool HasLatencies() const = 0;

        // Gets the measured latency for a given AWS region
        virtual uint32_t GetLatencyForRegion(const AZStd::string& region) const = 0;

        // Return latency for region as a string, in format (region_latency)
        virtual AZStd::string GetLatencyString(const AZStd::string& region) const = 0;

        using LatencyAvailableEvent = AZ::Event<bool>;
        LatencyAvailableEvent m_latenciesAvailable;
    };


    class LatencySystemComponent final
        : public AZ::Component
        , public IRegionalLatencyFinder
    {
    public:
        AZ_COMPONENT(LatencySystemComponent, "{699E7875-5274-4516-88C9-A8D3010B9D3A}");

        /*
        * Reflects component data into the reflection contexts, including the serialization, edit, and behavior contexts.
        */
        static void Reflect(AZ::ReflectContext* context);

        LatencySystemComponent();
        ~LatencySystemComponent() override;

        /* IRegionalLatencyFinder overrides... */
        void RequestLatencies() override;
        bool HasLatencies() const override;
        uint32_t GetLatencyForRegion(const AZStd::string& region) const override;
        AZStd::string GetLatencyString(const AZStd::string & region) const override;
   
    protected:
        void Activate() override;
        void Deactivate() override;
        void SetLatencyForRegion(const AZStd::string& region, uint32_t latency);

    private:
        AZStd::vector<AZStd::string> m_regions = {"us-west-2", "us-east-1"};
        AZStd::unordered_map<AZStd::string, uint32_t> m_latencyMap;
    };
} // namespace MPSGameLift
