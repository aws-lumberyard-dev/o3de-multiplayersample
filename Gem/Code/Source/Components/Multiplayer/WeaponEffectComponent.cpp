/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <LmbrCentral/Audio/AudioTriggerComponentBus.h>
#include <PopcornFX/PopcornFXBus.h>
#include <Source/Components/Multiplayer/WeaponEffectComponent.h>

namespace MultiplayerSample
{
    void WeaponEffectComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<WeaponEffectComponent, WeaponEffectComponentBase>()
                ->Version(1);
        }
        WeaponEffectComponentBase::Reflect(context);
    }

    void WeaponEffectComponent::OnInit()
    {
    }

    void WeaponEffectComponent::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
    }

    void WeaponEffectComponent::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
    }

    void WeaponEffectComponent::HandleRPC_ConfirmedHit([[maybe_unused]] AzNetworking::IConnection* invokingConnection,
        [[maybe_unused]] const AZ::Vector3& hitPosition)
    {
        // Don't play the effect on the originating player as the weapon effect was predicated there already.
        if (IsNetEntityRoleAutonomous() == false)
        {
            // Note: ideally, the animation needs to be played to the frame of shooting before grabbing the bone position.
            const AZ::Vector3 start = GetNetworkWeaponsComponent()->GetCurrentShotStartPosition();
            PlayParticleEffect(start, hitPosition);
        }
    }

    void WeaponEffectComponent::PlayParticleEffect(const AZ::Vector3& start, const AZ::Vector3& end)
    {
        if (PopcornFX::PopcornFXEmitterComponentRequests* particle =
            PopcornFX::PopcornFXEmitterComponentRequestBus::FindFirstHandler(GetEntityId()))
        {
            AZ::s32 attributeId = particle->GetAttributeId("Start");
            if (attributeId >= 0)
            {
                particle->SetAttributeAsFloat3(attributeId, start);
            }

            attributeId = particle->GetAttributeId("End");
            if (attributeId >= 0)
            {
                particle->SetAttributeAsFloat3(attributeId, end);
            }

            particle->Restart(true);
        }

        LmbrCentral::AudioTriggerComponentRequestBus::Event(GetEntityId(), &LmbrCentral::AudioTriggerComponentRequestBus::Events::Play);
    }


    WeaponEffectComponentController::WeaponEffectComponentController(WeaponEffectComponent& parent)
        : WeaponEffectComponentControllerBase(parent)
    {
    }

    void WeaponEffectComponentController::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        if (IsNetEntityRoleAutonomous() || IsNetEntityRoleAuthority())
        {
            GetParent().GetNetworkWeaponsComponent()->AddOnWeaponActivateEventHandler(m_weaponActivateHandler);
            GetParent().GetNetworkWeaponsComponent()->AddOnWeaponPredictHitEventHandler(m_weaponPredictHandler);
            GetParent().GetNetworkWeaponsComponent()->AddOnWeaponConfirmHitEventHandler(m_weaponConfirmHandler);
        }
    }

    void WeaponEffectComponentController::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        m_weaponActivateHandler.Disconnect();
        m_weaponPredictHandler.Disconnect();
        m_weaponConfirmHandler.Disconnect();
    }

    void WeaponEffectComponentController::OnWeaponActivate(const WeaponActivationInfo& info)
    {
        // This is the locally predicated effect on the player that initiated the weapon.
        const AZ::Vector3& start = info.m_activateEvent.m_initialTransform.GetTranslation();
        const AZ::Vector3& end = info.m_activateEvent.m_targetPosition;
        GetParent().PlayParticleEffect(start, end);
    }

    void WeaponEffectComponentController::OnWeaponPredictHit([[maybe_unused]] const WeaponHitInfo& info)
    {
        /*
         * At the moment, the particle effect plays both the trace and the hit effect in one go when predicted on the originated player.
         * If the particle effects were separated, one could spawn an entity to play the hit effect only.
         * Then this call would play the predicted hit effect, while @OnWeaponActivate would only play the trace line effect.
         */
    }

    void WeaponEffectComponentController::OnWeaponConfirmHit([[maybe_unused]] const WeaponHitInfo& info)
    {
        if (IsNetEntityRoleAuthority())
        {
            if (info.m_hitEvent.m_hitEntities.empty() == false)
            {
                // Send the confirmed hit to all players.
                const AZ::Vector3& hitPosition = info.m_hitEvent.m_hitEntities.front().m_hitPosition;
                RPC_ConfirmedHit(hitPosition);

                const AZ::Vector3 start = GetParent().GetNetworkWeaponsComponent()->GetCurrentShotStartPosition();
                // Play the effect on client-server.
                // Note: dedicated server might not have particle effects enabled. (PopcornFX doesn't.)
                GetParent().PlayParticleEffect(start, hitPosition);
            }
        }
    }
}
