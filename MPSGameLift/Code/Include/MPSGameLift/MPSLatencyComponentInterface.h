#pragma once

#include <AzCore/EBus/Event.h>
#include <AzCore/Component/ComponentBus.h>

namespace MPSGameLift
{
    class MPSLatencyComponentRequests
        : public AZ::ComponentBus
    {
    public:
        AZ_RTTI(MPSGameLift::MPSLatencyComponentRequests, "{B257E80A-FCEF-4245-B5DD-0E475AB01959}");

        // Put your public request methods here.
        virtual void RequestLatencies() = 0;
        virtual bool HasLatencies() const = 0;
        virtual uint32_t GetLatencyForRegion(const AZStd::string& region) const = 0;

        // Put notification events here. Examples:
        // void RegisterEvent(AZ::EventHandler<...> notifyHandler);
        // AZ::Event<...> m_notifyEvent1;

        using LatencyAvailableEvent = AZ::Event<bool>;
        LatencyAvailableEvent m_latenciesAvailable;
    };
    using MPSLatencyComponentRequestBus = AZ::EBus<MPSLatencyComponentRequests>;

    struct MatchmakingResults
    {
        AZStd::string address;
        uint32_t port = 0;
        bool found = false;
    };

    class MPSMatchmakingComponentRequests
        : public AZ::ComponentBus
    {
    public:
        AZ_RTTI(MPSGameLift::MPSMatchmakingComponentRequests, "{371687E5-9626-4201-91E3-0FD1F79CB8B6}");

        virtual bool RequestMatch(const AZStd::string& latencies) = 0;
        virtual AZStd::string GetTicketId() const = 0;
        virtual bool HasMatch(const AZStd::string& ticketId) = 0;
    };
    using MPSMatchmakingComponentRequestBus = AZ::EBus<MPSMatchmakingComponentRequests>;

} // namespace MPSGameLift
