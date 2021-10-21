/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Uuid.h>
#include <Multiplayer/Session/SessionConfig.h>

#include "GameLiftComponent.h"
#include <AWSGameLiftPlayer.h>
#include <Request/AWSGameLiftAcceptMatchRequest.h>
#include <Request/AWSGameLiftCreateSessionOnQueueRequest.h>
#include <Request/AWSGameLiftCreateSessionRequest.h>
#include <Request/AWSGameLiftJoinSessionRequest.h>
#include <Request/AWSGameLiftSearchSessionsRequest.h>
#include <Request/AWSGameLiftStartMatchmakingRequest.h>
#include <Request/AWSGameLiftStopMatchmakingRequest.h>
#include <ResourceMapping/AWSResourceMappingBus.h>

namespace MultiplayerSample
{
    GameLiftComponent::GameLiftComponent()
    {
        m_playerId = AZ::Uuid().Create().ToString<AZStd::string>();

        Multiplayer::SessionAsyncRequestNotificationBus::Handler::BusConnect();
        Multiplayer::MatchmakingAsyncRequestNotificationBus::Handler::BusConnect();
        Multiplayer::MatchmakingNotificationBus::Handler::BusConnect();
    }

    GameLiftComponent::~GameLiftComponent()
    {
        m_searchResults.clear();
        Multiplayer::MatchmakingNotificationBus::Handler::BusDisconnect();
        Multiplayer::MatchmakingAsyncRequestNotificationBus::Handler::BusDisconnect();
        Multiplayer::SessionAsyncRequestNotificationBus::Handler::BusDisconnect();
    }

    void GameLiftComponent::ConfigureClient(const AZ::ConsoleCommandContainer& consoleFunctionParameters)
    {
        if (consoleFunctionParameters.size() == 1)
        {
            AWSGameLift::AWSGameLiftRequestBus::Broadcast(
                &AWSGameLift::AWSGameLiftRequestBus::Events::ConfigureGameLiftClient, consoleFunctionParameters[0]);
        }
        else
        {
            AWSGameLift::AWSGameLiftRequestBus::Broadcast(
                &AWSGameLift::AWSGameLiftRequestBus::Events::ConfigureGameLiftClient, "");
        }
    }

    void GameLiftComponent::AcceptMatch(const AZ::ConsoleCommandContainer& consoleFunctionParameters)
    {
        AWSGameLift::AWSGameLiftAcceptMatchRequest request;
        if (consoleFunctionParameters.size() == 0 || consoleFunctionParameters.size() > 2)
        {
            AZ_Error("GameLiftComponent", false, "acceptmatch <true/false> <ticket-id>");
            return;
        }

        request.m_acceptMatch = consoleFunctionParameters[0] == "true";
        if (consoleFunctionParameters.size() == 2)
        {
            request.m_ticketId = consoleFunctionParameters[1];
        }
        else
        {
            request.m_ticketId = m_ticketId;
        }

        request.m_playerIds = { m_playerId };

        AWSGameLift::AWSGameLiftMatchmakingAsyncRequestBus::Broadcast(
            &AWSGameLift::AWSGameLiftMatchmakingAsyncRequestBus::Events::AcceptMatchAsync, request);
    }

    void GameLiftComponent::CreateSession(const AZ::ConsoleCommandContainer& consoleFunctionParameters)
    {
        AZStd::string fleetId = "";
        if (consoleFunctionParameters.size() == 3)
        {
            fleetId = consoleFunctionParameters[2];
        }
        else if (consoleFunctionParameters.size() == 2)
        {
            AWSCore::AWSResourceMappingRequestBus::BroadcastResult(
                fleetId, &AWSCore::AWSResourceMappingRequestBus::Events::GetResourceNameId, "MultiplayerSampleFleetId");
            if (fleetId.empty())
            {
                AZ_Error("GameLiftComponent", false, 
                    "Failed to get fleet id from resource mapping file");
                return;
            }
        }
        else
        {
            AZ_Error("GameLiftComponent", false, 
                "createsession <session-id> <max-player> <fleet-id> or createsession <session-id> <max-player>");
            return;
        }

        AWSGameLift::AWSGameLiftCreateSessionRequest request;
        request.m_idempotencyToken = consoleFunctionParameters[0];
        request.m_maxPlayer = AZStd::stoi(AZStd::string(consoleFunctionParameters[1]));
        request.m_fleetId = fleetId;
        AWSGameLift::AWSGameLiftSessionAsyncRequestBus::Broadcast(
            &AWSGameLift::AWSGameLiftSessionAsyncRequestBus::Events::CreateSessionAsync, request);
    }

    void GameLiftComponent::CreateSessionOnQueue(const AZ::ConsoleCommandContainer& consoleFunctionParameters)
    {
        if (consoleFunctionParameters.size() != 3)
        {
            AZ_Error("GameLiftComponent", false, "createsessiononqueue <queue-name> <placement-id> <max-player>");
            return;
        }

        AWSGameLift::AWSGameLiftCreateSessionOnQueueRequest request;
        request.m_queueName = consoleFunctionParameters[0];
        request.m_placementId = consoleFunctionParameters[1];
        request.m_maxPlayer = AZStd::stoi(AZStd::string(consoleFunctionParameters[2]));
        AWSGameLift::AWSGameLiftSessionAsyncRequestBus::Broadcast(
            &AWSGameLift::AWSGameLiftSessionAsyncRequestBus::Events::CreateSessionAsync, request);
    }

    void GameLiftComponent::JoinSession(const AZ::ConsoleCommandContainer& consoleFunctionParameters)
    {
        if (consoleFunctionParameters.size() != 2)
        {
            AZ_Error("GameLiftComponent", false, "joinsession <session-id> <player-id>");
            return;
        }

        AWSGameLift::AWSGameLiftJoinSessionRequest request;
        request.m_sessionId = consoleFunctionParameters[0];
        request.m_playerId = consoleFunctionParameters[1];
        AWSGameLift::AWSGameLiftSessionAsyncRequestBus::Broadcast(
            &AWSGameLift::AWSGameLiftSessionAsyncRequestBus::Events::JoinSessionAsync, request);
    }

    void GameLiftComponent::JoinSessionFromSearch(const AZ::ConsoleCommandContainer& consoleFunctionParameters)
    {
        if (consoleFunctionParameters.size() != 2)
        {
            AZ_Error("GameLiftComponent", false, "joinsessionfromsearch <search-index> <player-id>");
            return;
        }

        int searchIndex = AZStd::stoi(AZStd::string(consoleFunctionParameters[0]));
        if (searchIndex < 0 || searchIndex >= m_searchResults.size())
        {
            AZ_Error("GameLiftComponent", false, "Invalid search result index");
            return;
        }

        AWSGameLift::AWSGameLiftJoinSessionRequest request;
        request.m_sessionId = m_searchResults[searchIndex];
        request.m_playerId = consoleFunctionParameters[1];
        AWSGameLift::AWSGameLiftSessionAsyncRequestBus::Broadcast(
            &AWSGameLift::AWSGameLiftSessionAsyncRequestBus::Events::JoinSessionAsync, request);
    }

    void GameLiftComponent::SearchSessions(const AZ::ConsoleCommandContainer& consoleFunctionParameters)
    {
        AZStd::string fleetId = "";
        if (consoleFunctionParameters.size() == 1)
        {
            fleetId = consoleFunctionParameters[0];
        }
        else if (consoleFunctionParameters.size() == 0)
        {
            AWSCore::AWSResourceMappingRequestBus::BroadcastResult(
                fleetId, &AWSCore::AWSResourceMappingRequestBus::Events::GetResourceNameId, "MultiplayerSampleFleetId");
            if (fleetId.empty())
            {
                AZ_Error("GameLiftComponent", false,
                    "Failed to get fleet id from resource mapping file");
                return;
            }
        }
        else
        {
            AZ_Error("GameLiftComponent", false,
                "searchsessions <fleet-id> or searchsessions");
            return;
        }

        AWSGameLift::AWSGameLiftSearchSessionsRequest request;
        request.m_fleetId = fleetId;
        request.m_maxResult = 0;
        AWSGameLift::AWSGameLiftSessionAsyncRequestBus::Broadcast(
            &AWSGameLift::AWSGameLiftSessionAsyncRequestBus::Events::SearchSessionsAsync, request);
    }

    void GameLiftComponent::LeaveSession(const AZ::ConsoleCommandContainer& consoleFunctionParameters)
    {
        AZ_UNUSED(consoleFunctionParameters);
        AWSGameLift::AWSGameLiftSessionAsyncRequestBus::Broadcast(
            &AWSGameLift::AWSGameLiftSessionAsyncRequestBus::Events::LeaveSessionAsync);
    }

    void GameLiftComponent::StartMatchmaking(const AZ::ConsoleCommandContainer& consoleFunctionParameters)
    {
        AWSGameLift::AWSGameLiftStartMatchmakingRequest request;
        if (consoleFunctionParameters.size() > 1)
        {
            AZ_Error("GameLiftComponent", false, "startmatchmaking <configuration-name>");
            return;
        }

        if (consoleFunctionParameters.size() == 1)
        {
            request.m_configurationName = consoleFunctionParameters[0];
        }
        else
        {
            AZStd::string configurationName;
            AWSCore::AWSResourceMappingRequestBus::BroadcastResult(
                configurationName, &AWSCore::AWSResourceMappingRequestBus::Events::GetResourceNameId, "MultiplayerSampleConfigurationName");
            request.m_configurationName = configurationName;
        }

        AWSGameLift::AWSGameLiftPlayer player;
        player.m_playerAttributes["skill"] = "{\"N\": 23}";
        player.m_playerAttributes["gameMode"] = "{\"S\": \"deathmatch\"}";
        player.m_playerId = m_playerId;
        request.m_players = { player };

        AWSGameLift::AWSGameLiftMatchmakingAsyncRequestBus::Broadcast(
            &AWSGameLift::AWSGameLiftMatchmakingAsyncRequestBus::Events::StartMatchmakingAsync, request);
    }

    void GameLiftComponent::StartMatchmakingWithTicketId(const AZ::ConsoleCommandContainer& consoleFunctionParameters)
    {
        AWSGameLift::AWSGameLiftStartMatchmakingRequest request;
        if (consoleFunctionParameters.size() == 0 || consoleFunctionParameters.size() > 2)
        {
            AZ_Error("GameLiftComponent", false, "startmatchmaking <ticket-id> <configuration-name>");
            return;
        }
        request.m_ticketId = consoleFunctionParameters[0];

        if (consoleFunctionParameters.size() == 2)
        {
            request.m_configurationName = consoleFunctionParameters[1];
        }
        else
        {
            AZStd::string configurationName;
            AWSCore::AWSResourceMappingRequestBus::BroadcastResult(
                configurationName, &AWSCore::AWSResourceMappingRequestBus::Events::GetResourceNameId, "MultiplayerSampleConfigurationName");
            request.m_configurationName = configurationName;
        }

        AWSGameLift::AWSGameLiftPlayer player;
        player.m_playerAttributes["skill"] = "{\"N\": 23}";
        player.m_playerAttributes["gameMode"] = "{\"S\": \"deathmatch\"}";
        player.m_playerId = m_playerId;
        request.m_players = { player };

        AWSGameLift::AWSGameLiftMatchmakingAsyncRequestBus::Broadcast(
            &AWSGameLift::AWSGameLiftMatchmakingAsyncRequestBus::Events::StartMatchmakingAsync, request);
    }

    void GameLiftComponent::StopMatchmaking(const AZ::ConsoleCommandContainer& consoleFunctionParameters)
    {
        AWSGameLift::AWSGameLiftStopMatchmakingRequest request;
        if (consoleFunctionParameters.size() == 1)
        {
            request.m_ticketId = consoleFunctionParameters[0];
        }
        else if (consoleFunctionParameters.size() == 0 && !m_ticketId.empty())
        {
            request.m_ticketId = m_ticketId;
        }
        else
        {
            AZ_Error("GameLiftComponent", false, "stopmatchmaking <ticket-id>");
            return;
        }

        AWSGameLift::AWSGameLiftMatchmakingAsyncRequestBus::Broadcast(
            &AWSGameLift::AWSGameLiftMatchmakingAsyncRequestBus::Events::StopMatchmakingAsync, request);
    }

    void GameLiftComponent::OnCreateSessionAsyncComplete(const AZStd::string& createSessionReponse)
    {
        AZ_UNUSED(createSessionReponse);
        AZ_TracePrintf("MultiplayerSample", "CreateSessionAsync complete with result: %s", createSessionReponse.c_str());
    }

    void GameLiftComponent::OnSearchSessionsAsyncComplete(const Multiplayer::SearchSessionsResponse& searchSessionsResponse)
    {
        m_searchResults.clear();
        AZStd::string sessionIds = "";
        for (auto& sessionConfig: searchSessionsResponse.m_sessionConfigs)
        {
            sessionIds += AZStd::string::format("SessionId=%s, ", sessionConfig.m_sessionId.c_str());
            m_searchResults.push_back(sessionConfig.m_sessionId);
        }
        if (!sessionIds.empty())
        {
            sessionIds = sessionIds.substr(0, sessionIds.size() - 2);
        }

        AZ_TracePrintf("MultiplayerSample", "SearchSessionsAsync complete with result: %s", 
            AZStd::string::format("[%s]", sessionIds.c_str()).c_str());
    }

    void GameLiftComponent::OnJoinSessionAsyncComplete(bool joinSessionsResponse)
    {
        if (joinSessionsResponse)
        {
            AZ_TracePrintf("MultiplayerSample", "JoinSessionAsync complete with result: success");
        }
        else
        {
            AZ_TracePrintf("MultiplayerSample", "JoinSessionAsync complete with result: fail");
        }
    }

    void GameLiftComponent::OnLeaveSessionAsyncComplete()
    {
        AZ_TracePrintf("MultiplayerSample", "LeaveSessionAsync complete");
    }

    void GameLiftComponent::OnAcceptMatchAsyncComplete()
    {
        AZ_TracePrintf("MultiplayerSample", "AcceptMatchAsync complete");
    }

    void GameLiftComponent::OnStartMatchmakingAsyncComplete(const AZStd::string& matchmakingTicketId)
    {
        m_ticketId = matchmakingTicketId;
        AZ_TracePrintf("MultiplayerSample", "StartMatchmakingAsync complete with ticket ID: %s", matchmakingTicketId.c_str());

        AWSGameLift::AWSGameLiftMatchmakingEventRequestBus::Broadcast(
            &AWSGameLift::AWSGameLiftMatchmakingEventRequestBus::Events::StartPolling, m_ticketId, m_playerId);
    }

    void GameLiftComponent::OnStopMatchmakingAsyncComplete()
    {
        m_ticketId = "";
        AZ_TracePrintf("MultiplayerSample", "StopMatchmakingAsync complete");

        AWSGameLift::AWSGameLiftMatchmakingEventRequestBus::Broadcast(
            &AWSGameLift::AWSGameLiftMatchmakingEventRequestBus::Events::StopPolling);
    }
} // namespace MultiplayerSample
