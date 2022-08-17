/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PlayerIdentityBus.h>
#include <Source/Components/Multiplayer/PlayerIdentityComponent.h>

namespace MultiplayerSample
{
    void PlayerIdentityComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<PlayerIdentityComponent, PlayerIdentityComponentBase>()
                ->Version(1);
        }
        PlayerIdentityComponentBase::Reflect(context);
    }

    void PlayerIdentityComponent::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
    }

    void PlayerIdentityComponent::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
    }

    void PlayerIdentityComponent::AssignPlayerName(const PlayerNameString& newPlayerName)
    {
        if (IsNetEntityRoleServer())
        {
            RPC_AssignPlayerName(newPlayerName);
        }
        else
        {
            static_cast<PlayerIdentityComponentController*>(GetController())->SetPlayerName(newPlayerName);
        }
    }


    PlayerIdentityComponentController::PlayerIdentityComponentController(PlayerIdentityComponent& parent)
        : PlayerIdentityComponentControllerBase(parent)
    {
    }

    void PlayerIdentityComponentController::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        if (IsAuthority())
        {
            PlayerIdentityNotificationBus::Broadcast(&PlayerIdentityNotificationBus::Events::OnPlayerActivated, GetNetEntityId());
        }
    }

    void PlayerIdentityComponentController::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        if (IsAuthority())
        {
            PlayerIdentityNotificationBus::Broadcast(&PlayerIdentityNotificationBus::Events::OnPlayerDeactivated, GetNetEntityId());
        }
    }

    void PlayerIdentityComponentController::HandleRPC_AssignPlayerName([[maybe_unused]] AzNetworking::IConnection* invokingConnection,
        const PlayerNameString& newPlayerName)
    {
        SetPlayerName(newPlayerName);
    }

    void PlayerIdentityComponentController::HandleRPC_ResetPlayerState([[maybe_unused]] AzNetworking::IConnection* invokingConnection)
    {
        GetNetworkHealthComponentController()->SetHealth(GetNetworkHealthComponentController()->GetMaxHealth());
        GetPlayerCoinCollectorComponentController()->SetCoinsCollected(0);
        PlayerIdentityComponentControllerBase::HandleRPC_ResetPlayerState(invokingConnection);
    }
}
