/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Console/IConsole.h>
#include <Multiplayer/Session/MatchmakingNotifications.h>

#include <Request/AWSGameLiftRequestBus.h>
#include <Request/AWSGameLiftSessionRequestBus.h>
#include <Request/AWSGameLiftMatchmakingRequestBus.h>


namespace MultiplayerSample
{
    class GameLiftComponent
        : public Multiplayer::SessionAsyncRequestNotificationBus::Handler
        , public Multiplayer::MatchmakingNotificationBus::Handler
        , public Multiplayer::MatchmakingAsyncRequestNotificationBus::Handler
    {
    public:
        GameLiftComponent();
        virtual ~GameLiftComponent();

        void AcceptMatch(const AZ::ConsoleCommandContainer& consoleFunctionParameters);
        AZ_CONSOLEFUNC(GameLiftComponent, AcceptMatch, AZ::ConsoleFunctorFlags::Null, "");

        void ConfigureClient(const AZ::ConsoleCommandContainer& consoleFunctionParameters);
        AZ_CONSOLEFUNC(GameLiftComponent, ConfigureClient, AZ::ConsoleFunctorFlags::Null, "");

        void CreateSession(const AZ::ConsoleCommandContainer& consoleFunctionParameters);
        AZ_CONSOLEFUNC(GameLiftComponent, CreateSession, AZ::ConsoleFunctorFlags::Null, "");

        void CreateSessionOnQueue(const AZ::ConsoleCommandContainer& consoleFunctionParameters);
        AZ_CONSOLEFUNC(GameLiftComponent, CreateSessionOnQueue, AZ::ConsoleFunctorFlags::Null, "");

        void JoinSession(const AZ::ConsoleCommandContainer& consoleFunctionParameters);
        AZ_CONSOLEFUNC(GameLiftComponent, JoinSession, AZ::ConsoleFunctorFlags::Null, "");

        void JoinSessionFromSearch(const AZ::ConsoleCommandContainer& consoleFunctionParameters);
        AZ_CONSOLEFUNC(GameLiftComponent, JoinSessionFromSearch, AZ::ConsoleFunctorFlags::Null, "");

        void SearchSessions(const AZ::ConsoleCommandContainer& consoleFunctionParameters);
        AZ_CONSOLEFUNC(GameLiftComponent, SearchSessions, AZ::ConsoleFunctorFlags::Null, "");

        void LeaveSession(const AZ::ConsoleCommandContainer& consoleFunctionParameters);
        AZ_CONSOLEFUNC(GameLiftComponent, LeaveSession, AZ::ConsoleFunctorFlags::Null, "");

        void StartMatchmaking(const AZ::ConsoleCommandContainer& consoleFunctionParameters);
        AZ_CONSOLEFUNC(GameLiftComponent, StartMatchmaking, AZ::ConsoleFunctorFlags::Null, "");

        void StartMatchmakingWithTicketId(const AZ::ConsoleCommandContainer& consoleFunctionParameters);
        AZ_CONSOLEFUNC(GameLiftComponent, StartMatchmakingWithTicketId, AZ::ConsoleFunctorFlags::Null, "");

        void StopMatchmaking(const AZ::ConsoleCommandContainer& consoleFunctionParameters);
        AZ_CONSOLEFUNC(GameLiftComponent, StopMatchmaking, AZ::ConsoleFunctorFlags::Null, "");

        // SessionAsyncRequestNotificationBus handler implementation
        void OnCreateSessionAsyncComplete(const AZStd::string& createSessionReponse) override;
        void OnSearchSessionsAsyncComplete(const Multiplayer::SearchSessionsResponse& searchSessionsResponse) override;
        void OnJoinSessionAsyncComplete(bool joinSessionsResponse) override;
        void OnLeaveSessionAsyncComplete() override;

        //MatchmakingAsyncRequestNotificationBus handler implementation
        void OnAcceptMatchAsyncComplete() override;
        void OnStartMatchmakingAsyncComplete(const AZStd::string& matchmakingTicketId) override;
        void OnStopMatchmakingAsyncComplete() override;

        //MatchmakingNotificationBus handler implementation
        void OnMatchAcceptance() override {}
        void OnMatchComplete() override {}
        void OnMatchError() override {}
        void OnMatchFailure() override {}

    private:
        AZStd::vector<AZStd::string> m_searchResults;

        AZStd::string m_ticketId;
        AZStd::string m_playerId;
    };
} // namespace MultiplayerSample
