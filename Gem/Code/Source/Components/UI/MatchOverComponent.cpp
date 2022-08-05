/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/Components/UI/MatchOverComponent.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiTextBus.h>

namespace MultiplayerSample
{
    void MatchOverComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MatchOverComponent, AZ::Component>()
                ->Version(1)
                ->Field("GameOverElementId", &MatchOverComponent::m_gameOverElementId)
                ->Field("HudElementId", &MatchOverComponent::m_hudElementId)
                ->Field("ResultsElementId", &MatchOverComponent::m_resultsElementId)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<MatchOverComponent>("MatchOverComponent", "Handles game over display behavior")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Multiplayer Sample UI")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &MatchOverComponent::m_gameOverElementId, "Game Over element ID", "The element ID of the Game Over UI element to display at Match end.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &MatchOverComponent::m_hudElementId, "HUD element ID", "The element ID of the UI element for the HUD")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &MatchOverComponent::m_resultsElementId, "Results element ID", "The ID of the UI element for the match results")
                    ;
            }
        }
    }

    void MatchOverComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("NetworkMatchComponent"));
        required.push_back(AZ_CRC("UiCanvasRefService"));
    }

    void MatchOverComponent::Activate()
    {
        UiCanvasRefBus::EventResult(m_uiCanvasId, GetEntityId(), &UiCanvasRefBus::Events::GetCanvas);
        if (!m_uiCanvasId.IsValid())
        {
            UiCanvasAssetRefNotificationBus::Handler::BusConnect(GetEntityId());
        }

        if (NetworkMatchComponent* netMatchComponent = GetEntity()->FindComponent<NetworkMatchComponent>())
        {
            m_roundNumberHandler = AZ::EventHandler<uint16_t>([this](uint16_t value) 
                { UpdateRound(value); });
            netMatchComponent->RoundNumberAddEvent(m_roundNumberHandler);

            m_roundTimerHandler = AZ::EventHandler<RoundTimeSec>([this](RoundTimeSec value) 
                { DetermineIfMatchEnded(value); });
            netMatchComponent->RoundTimeAddEvent(m_roundTimerHandler);

            m_matchResultsHandler = AZ::EventHandler<MatchResults>([this](MatchResults value)
                { UpdateResults(value); });
            netMatchComponent->ResultsAddEvent(m_matchResultsHandler);
        }
    }

    void MatchOverComponent::Deactivate()
    {
        UiCanvasAssetRefNotificationBus::Handler::BusDisconnect();
    }

    void MatchOverComponent::OnCanvasLoadedIntoEntity(AZ::EntityId uiCanvasEntity)
    {
        m_uiCanvasId = uiCanvasEntity;
    }

    void MatchOverComponent::UpdateRound(uint16_t round)
    {
        m_currentRound = round;
    }

    void MatchOverComponent::UpdateResults(MatchResults results)
    {
        m_matchResults = results;
        AddResultsToDisplay(results);
    }

    void MatchOverComponent::DetermineIfMatchEnded(RoundTimeSec roundTime)
    {
        
        const NetworkMatchComponent* netMatchComponent = GetEntity()->FindComponent<NetworkMatchComponent>();

        if (m_currentRound >= netMatchComponent->GetTotalRounds())
        {
            auto const currentTime = aznumeric_cast<float>(roundTime);
            AZ_TracePrintf("MatchOverComponent", "Final round. Awaiting match end in %f\n", currentTime);
            if (currentTime <= 0)
            {
                OnMatchEnd();
            }
        }
    }

    void MatchOverComponent::AddResultsToDisplay(MatchResults results)
    {
        if (m_uiCanvasId.IsValid())
        {
            AZ::Entity* winnerTextElement;
            UiCanvasBus::EventResult(winnerTextElement, m_uiCanvasId, &UiCanvasBus::Events::FindElementById, m_resultsElementId);
            if (winnerTextElement)
            {
                auto finalStandings = BuildPlayerSummary(results.m_playerStates);
                UiTextBus::Event(winnerTextElement->GetId(), &UiTextBus::Events::SetText,
                    AZStd::string::format("WINNER:  %s\n\n%s", results.m_winningPlayerName.c_str(), finalStandings.c_str()));
            }
        }
    }

    AZStd::string MatchOverComponent::BuildPlayerSummary(AZStd::vector<PlayerState> playerStates)
    {
        AZStd::string resultTable = "---------- Final Standings ----------\n\n";
        for (PlayerState result : playerStates)
        {
            resultTable.append(
                AZStd::string::format("%s :  score  %i, shields %i\n",
                result.m_playerName.c_str(), result.m_score, result.m_remainingSheild));
        }
        return resultTable;
    }

    void MatchOverComponent::OnMatchEnd()
    {
        AZ::Entity* hudEntity = nullptr;
        UiCanvasBus::EventResult(hudEntity, m_uiCanvasId, &UiCanvasBus::Events::FindElementById, m_hudElementId);
        if (hudEntity)
        {
            UiElementBus::Event(hudEntity->GetId(), &UiElementBus::Events::SetIsEnabled, false);
        }
        
        AZ::Entity* gameOverElement = nullptr;
        UiCanvasBus::EventResult(gameOverElement, m_uiCanvasId, &UiCanvasBus::Events::FindElementById, m_gameOverElementId);
        if (gameOverElement)
        {
            UiElementBus::Event(gameOverElement->GetId(), &UiElementBus::Events::SetIsEnabled, true);
        }
    }
} // namespace MultiplayerSample
