/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Vector3.h>

namespace MultiplayerSample
{
    class GameplayEffectsNotifications : public AZ::EBusTraits
    {
    public:
        ~GameplayEffectsNotifications() override = default;

        virtual void OnGameplayEffect(AZ::Crc32 effectName, const AZ::Vector3& position) {}
    };

    using GameplayEffectsNotificationBus = AZ::EBus<GameplayEffectsNotifications>;
}
