/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Source/AutoGen/GameplayEffectsComponent.AutoComponent.h>

namespace MultiplayerSample
{
    class GameplayEffectsComponent
        : public GameplayEffectsComponentBase
    {
    public:
        AZ_MULTIPLAYER_COMPONENT(MultiplayerSample::GameplayEffectsComponent, s_gameplayEffectsComponentConcreteUuid, MultiplayerSample::GameplayEffectsComponentBase);

        static void Reflect(AZ::ReflectContext* context);

        void OnInit() override;
        void OnActivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;        
    };

    class GameplayEffectsComponentController
        : public GameplayEffectsComponentControllerBase
    {
    public:
        explicit GameplayEffectsComponentController(GameplayEffectsComponent& parent);

        void OnActivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;        
    };
}
