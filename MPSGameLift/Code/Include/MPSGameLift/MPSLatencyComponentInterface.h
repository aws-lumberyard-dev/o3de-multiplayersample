#pragma once

#include <AzCore/EBus/Event.h>
#include <AzCore/Component/ComponentBus.h>

namespace MPSGameLift
{
    // Result data structure for a check on a matchmaking request
    struct MatchmakingResults
    {
        AZStd::string address;
        uint32_t port = 0;
        AZStd::string playerId;
        bool found = false;
    };

    // Supports matchmaking request calls to a serverless backend
    class MPSMatchmakingComponentRequests
        : public AZ::ComponentBus
    {
    public:
        AZ_RTTI(MPSGameLift::MPSMatchmakingComponentRequests, "{371687E5-9626-4201-91E3-0FD1F79CB8B6}");

        // Request a match for the player, providing player latencies for defined regions
        virtual bool RequestMatch(const AZStd::string& latencies) = 0;

        // Gets the current ticket id if any
        virtual AZStd::string GetTicketId() const = 0;

        // Checks if a match has been made
        // TODO: Needs to return the matchmaking results
        virtual bool HasMatch(const AZStd::string& ticketId) = 0;
    };
    using MPSMatchmakingComponentRequestBus = AZ::EBus<MPSMatchmakingComponentRequests>;

} // namespace MPSGameLift
