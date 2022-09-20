/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Source/AutoGen/JumpPadComponent.AutoComponent.h>

namespace MultiplayerSample
{
    class JumpPadComponent
        : public JumpPadComponentBase
    {
    public:
        AZ_MULTIPLAYER_COMPONENT(MultiplayerSample::JumpPadComponent, s_jumpPadComponentConcreteUuid, MultiplayerSample::JumpPadComponentBase);

        static void Reflect(AZ::ReflectContext* context);

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        void OnInit() override;
        void OnActivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
    };


    class JumpPadComponentController
        : public JumpPadComponentControllerBase
    {
    public:
        explicit JumpPadComponentController(JumpPadComponent& parent);

        void OnActivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;

    private:
        void OnTriggerEnter(AzPhysics::SimulatedBodyHandle bodyHandle, const AzPhysics::TriggerEvent& triggerEvent);
        AzPhysics::SimulatedBodyEvents::OnTriggerEnter::Handler m_enterTrigger{ [this](
            AzPhysics::SimulatedBodyHandle bodyHandle, const AzPhysics::TriggerEvent& triggerEvent)
        {
            OnTriggerEnter(bodyHandle, triggerEvent);
        } };
    };
}
