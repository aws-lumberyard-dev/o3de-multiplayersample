/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <Source/Components/Multiplayer/MatchPlayerCoinsComponent.h>
#include <Components/Multiplayer/PlayerIdentityComponent.h>

#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiTextBus.h>

#include <Source/Components/UI/UiMatchPlayerCoinCountsComponent.h>
#include <Multiplayer\Components\NetworkDebugPlayerIdComponent.h>

namespace MultiplayerSample
{
    const StartingPointInput::InputEventNotificationId ShowPlayerCoinCountsEventId("show_player_coin_counts");

    void UiMatchPlayerCoinCountsComponent::Activate()
    {
        StartingPointInput::InputEventNotificationBus::MultiHandler::BusConnect(ShowPlayerCoinCountsEventId);
    }

    void UiMatchPlayerCoinCountsComponent::Deactivate()
    {
        StartingPointInput::InputEventNotificationBus::MultiHandler::BusDisconnect();
    }

    void UiMatchPlayerCoinCountsComponent::UpdatePlayerScoreUI()
    {
        AZStd::vector<PlayerCoinState> coins = AZ::Interface<MatchPlayerCoinsComponent>::Get()->GetPlayerCoinCounts();
        AZStd::size_t elementIndex = 0;
        for (const PlayerCoinState& state : coins)
        {
            if (elementIndex >= m_playerRowElement.size())
            {
                break;
            }

            if (state.m_playerId != Multiplayer::InvalidNetEntityId)
            {
                AZStd::vector<AZ::EntityId> children;
                UiElementBus::EventResult(children, m_playerRowElement[elementIndex], &UiElementBus::Events::GetChildEntityIds);
                if (children.size() >= 2)
                {
                    const PlayerNameString name = GetPlayerName(state.m_playerId);
                    UiTextBus::Event(children[0], &UiTextBus::Events::SetText, AZStd::string::format("%s", name.c_str()));
                    UiTextBus::Event(children[1], &UiTextBus::Events::SetText, AZStd::string::format("%d", state.m_coins));
                }
                elementIndex++;
            }
        }

        // Clear out the unused fields.
        for (; elementIndex < m_playerRowElement.size(); ++elementIndex)
        {
            AZStd::vector<AZ::EntityId> children;
            UiElementBus::EventResult(children, m_playerRowElement[elementIndex], &UiElementBus::Events::GetChildEntityIds);
            if (children.size() >= 3)
            {
                UiTextBus::Event(children[0], &UiTextBus::Events::SetText, ""); // name
                UiTextBus::Event(children[1], &UiTextBus::Events::SetText, ""); // score
                UiTextBus::Event(children[2], &UiTextBus::Events::SetText, ""); // rank
            }
        }
    }

    void UiMatchPlayerCoinCountsComponent::OnPressed([[maybe_unused]] float value)
    {
        const StartingPointInput::InputEventNotificationId* inputId = StartingPointInput::InputEventNotificationBus::GetCurrentBusId();

        if (inputId == nullptr)
        {
            return;
        }

        if (*inputId == ShowPlayerCoinCountsEventId)
        {
            UiElementBus::Event(m_rootElementId, &UiElementBus::Events::SetIsEnabled, true);
            AZ::Interface<MatchPlayerCoinsComponent>::Get()->CoinsPerPlayerAddEvent(m_onPlayerScoreChanged);
            UpdatePlayerScoreUI();
        }
    }

    void UiMatchPlayerCoinCountsComponent::OnReleased([[maybe_unused]] float value)
    {
        const StartingPointInput::InputEventNotificationId* inputId = StartingPointInput::InputEventNotificationBus::GetCurrentBusId();

        if (inputId == nullptr)
        {
            return;
        }

        if (*inputId == ShowPlayerCoinCountsEventId)
        {
            m_onPlayerScoreChanged.Disconnect();
            UiElementBus::Event(m_rootElementId, &UiElementBus::Events::SetIsEnabled, false);
        }
    }

    PlayerNameString UiMatchPlayerCoinCountsComponent::GetPlayerName(Multiplayer::NetEntityId playerEntity)
    {
        const auto playerHandle = Multiplayer::GetNetworkEntityManager()->GetEntity(playerEntity);
        if (playerHandle.Exists())
        {
            if (const PlayerIdentityComponent* identity = playerHandle.GetEntity()->FindComponent<PlayerIdentityComponent>())
            {
                PlayerNameString playerName = identity->GetPlayerName();
                if (playerName.empty())
                {
                    if(const auto playerDebugId= playerHandle.GetEntity()->FindComponent<Multiplayer::NetworkDebugPlayerIdComponent>())
                    {
                        AZStd::string playerIdText = AZStd::string::format("<debug id: %i>", static_cast<uint32_t>(playerDebugId->GetPlayerId()));
                        return playerIdText.c_str();
                    }
                    return "<player_identity_empty>";
                }

                return playerName;
            }
        }

        return "<player_handle_doesn't_exist>";
    }

    void UiMatchPlayerCoinCountsComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<UiMatchPlayerCoinCountsComponent, AZ::Component>()
                ->Version(1)
                ->Field("Root Element", &UiMatchPlayerCoinCountsComponent::m_rootElementId)
                ->Field("Stat Rows for Players", &UiMatchPlayerCoinCountsComponent::m_playerRowElement)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<UiMatchPlayerCoinCountsComponent>("UiMatchPlayerCoinCountsComponent",
                    "Shows list of coins collected by each player in the match.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("CanvasUI"))

                    ->DataElement(AZ::Edit::UIHandlers::Default, &UiMatchPlayerCoinCountsComponent::m_rootElementId,
                        "Root Element",
                        "Top level element that contains all other elements for showing coins collected by players")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &UiMatchPlayerCoinCountsComponent::m_playerRowElement,
                        "Stat Rows for Players",
                        "List of rows that contains 2 text elements, one for player id and one for coin count")
                    ;
            }
        }
    }
} // namespace MultiplayerSample
