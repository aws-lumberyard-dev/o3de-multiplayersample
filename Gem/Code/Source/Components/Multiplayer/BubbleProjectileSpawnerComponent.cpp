/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/Components/Multiplayer/BubbleProjectileSpawnerComponent.h>

namespace MultiplayerSample
{
    BubbleProjectileSpawnerComponentController::BubbleProjectileSpawnerComponentController(BubbleProjectileSpawnerComponent& parent)
        : BubbleProjectileSpawnerComponentControllerBase(parent)
    {
    }

    void BubbleProjectileSpawnerComponentController::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        BubbleProjectileSpawnerRequestBus::Handler::BusConnect();
    }

    void BubbleProjectileSpawnerComponentController::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        BubbleProjectileSpawnerRequestBus::Handler::BusDisconnect();
    }

    void BubbleProjectileSpawnerComponentController::LaunchBubbleProjectile([[maybe_unused]] const AZ::Transform& from,
        [[maybe_unused]] AZ::EntityId byPlayer)
    {
    }
}
