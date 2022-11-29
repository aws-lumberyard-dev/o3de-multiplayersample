/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/Components/Multiplayer/GameplayEffectsComponent.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace MultiplayerSample
{
    void GameplayEffectsComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<GameplayEffectsComponent, GameplayEffectsComponentBase>()
                ->Version(1);
        }
        GameplayEffectsComponentBase::Reflect(context);
    }

    void GameplayEffectsComponent::OnInit()
    {
    }

    void GameplayEffectsComponent::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
    }

    void GameplayEffectsComponent::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
    }

    GameplayEffectsComponentController::GameplayEffectsComponentController(GameplayEffectsComponent& parent)
        : GameplayEffectsComponentControllerBase(parent)
    {
    }

    void GameplayEffectsComponentController::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
    }

    void GameplayEffectsComponentController::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
    }
}
