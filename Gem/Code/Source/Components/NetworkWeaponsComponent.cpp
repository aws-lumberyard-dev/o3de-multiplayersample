/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/Components/NetworkWeaponsComponent.h>
#include <Source/Components/NetworkAnimationComponent.h>
#include <Source/Components/SimplePlayerCameraComponent.h>
#include <Source/Weapons/BaseWeapon.h>
#include <AzCore/Component/TransformBus.h>
#include <DebugDraw/DebugDrawBus.h>

namespace MultiplayerSample
{
    AZ_CVAR(bool, cl_WeaponsDrawDebug, true, nullptr, AZ::ConsoleFunctorFlags::Null, "If enabled, weapons will debug draw various important events");

    void NetworkWeaponsComponent::NetworkWeaponsComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<NetworkWeaponsComponent, NetworkWeaponsComponentBase>()
                ->Version(1);
        }
        NetworkWeaponsComponentBase::Reflect(context);
    }

    NetworkWeaponsComponent::NetworkWeaponsComponent()
        : NetworkWeaponsComponentBase()
        , m_activationCountHandler([this](int32_t index, uint8_t value) { OnUpdateActivationCounts(index, value); })
    {
        ;
    }

    void NetworkWeaponsComponent::OnInit()
    {
        AZStd::uninitialized_fill_n(m_fireBoneJointIds.data(), MaxWeaponsPerComponent, InvalidBoneId);
    }

    void NetworkWeaponsComponent::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        for (uint32_t weaponIndex = 0; weaponIndex < MaxWeaponsPerComponent; ++weaponIndex)
        {
            const ConstructParams constructParams
            {
                GetEntityHandle(),
                aznumeric_cast<WeaponIndex>(weaponIndex),
                GetWeaponParams(weaponIndex),
                *this
            };

            m_weapons[weaponIndex] = AZStd::move(CreateWeapon(constructParams));
        }

        if (IsNetEntityRoleClient())
        {
            ActivationCountsAddEvent(m_activationCountHandler);
        }

        if (m_debugDraw == nullptr)
        {
            m_debugDraw = DebugDraw::DebugDrawRequestBus::FindFirstHandler();
        }
    }

    void NetworkWeaponsComponent::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        ;
    }

    void NetworkWeaponsComponent::HandleSendConfirmHit([[maybe_unused]] AzNetworking::IConnection* invokingConnection, const WeaponIndex& weaponIndex, const HitEvent& hitEvent)
    {
        if (GetWeapon(weaponIndex) == nullptr)
        {
            AZLOG_ERROR("Got confirmed hit for null weapon index");
            return;
        }

        WeaponHitInfo weaponHitInfo(*GetWeapon(weaponIndex), hitEvent);
        OnWeaponConfirmHit(weaponHitInfo);
    }

    void NetworkWeaponsComponent::ActivateWeaponWithParams(WeaponIndex weaponIndex, WeaponState& weaponState, const FireParams& fireParams, bool validateActivations)
    {
        const uint32_t weaponIndexInt = aznumeric_cast<uint32_t>(weaponIndex);

        // Temp hack for weapon firing due to late ebus binding in 1.14
        if (m_fireBoneJointIds[weaponIndexInt] == InvalidBoneId)
        {
            const char* fireBoneName = GetFireBoneNames(weaponIndexInt).c_str();
            m_fireBoneJointIds[weaponIndexInt] = GetNetworkAnimationComponent()->GetBoneIdByName(fireBoneName);
        }

        AZ::Transform fireBoneTransform;
        if (!GetNetworkAnimationComponent()->GetJointTransformById(m_fireBoneJointIds[weaponIndexInt], fireBoneTransform))
        {
            AZLOG_WARN("Failed to get transform for fire bone %s, joint Id %u", GetFireBoneNames(weaponIndexInt).c_str(), m_fireBoneJointIds[weaponIndexInt]);
        }

        const AZ::Vector3    position = fireBoneTransform.GetTranslation();
        const AZ::Quaternion orientation = AZ::Quaternion::CreateShortestArc(AZ::Vector3::CreateAxisX(), (fireParams.m_targetPosition - position).GetNormalized());
        const AZ::Transform  transform = AZ::Transform::CreateFromQuaternionAndTranslation(orientation, position);
        ActivateEvent activateEvent{ transform, fireParams.m_targetPosition, GetNetEntityId(), Multiplayer::InvalidNetEntityId };

        IWeapon* weapon = GetWeapon(weaponIndex);
        weapon->Activate(weaponState, GetEntityHandle(), activateEvent, validateActivations);
    }

    IWeapon* NetworkWeaponsComponent::GetWeapon(WeaponIndex weaponIndex) const
    {
        return m_weapons[aznumeric_cast<uint32_t>(weaponIndex)].get();
    }

    void NetworkWeaponsComponent::OnWeaponActivate([[maybe_unused]] const WeaponActivationInfo& activationInfo)
    {
        // If we're replaying inputs then early out
        if (GetNetBindComponent()->IsReprocessingInput())
        {
            return;
        }

        if (cl_WeaponsDrawDebug && m_debugDraw)
        {
            m_debugDraw->DrawSphereAtLocation
            (
                activationInfo.m_activateEvent.m_initialTransform.GetTranslation(),
                0.25f, 
                AZ::Colors::GreenYellow,
                10.0f
            );

            m_debugDraw->DrawSphereAtLocation
            (
                activationInfo.m_activateEvent.m_targetPosition,
                0.25f,
                AZ::Colors::Crimson,
                10.0f
            );
        }

        AZLOG
        (
            NET_TraceWeapons,
            "Weapon activated with target position %f x %f x %f",
            activationInfo.m_activateEvent.m_targetPosition.GetX(),
            activationInfo.m_activateEvent.m_targetPosition.GetY(),
            activationInfo.m_activateEvent.m_targetPosition.GetZ()
        );

    }

    void NetworkWeaponsComponent::OnWeaponPredictHit(const WeaponHitInfo& hitInfo)
    {
        // If we're replaying inputs then early out
        if (GetNetBindComponent()->IsReprocessingInput())
        {
            return;
        }

        for (uint32_t i = 0; i < hitInfo.m_hitEvent.m_hitEntities.size(); ++i)
        {
            const HitEntity& hitEntity = hitInfo.m_hitEvent.m_hitEntities[i];

            if (cl_WeaponsDrawDebug && m_debugDraw)
            {
                m_debugDraw->DrawSphereAtLocation
                (
                    hitEntity.m_hitPosition,
                    1.0f,
                    AZ::Colors::OrangeRed,
                    10.0f
                );
            }

            AZLOG
            (
                NET_TraceWeapons,
                "Predicted hit on entity %u at position %f x %f x %f",
                hitEntity.m_hitNetEntityId,
                hitEntity.m_hitPosition.GetX(),
                hitEntity.m_hitPosition.GetY(),
                hitEntity.m_hitPosition.GetZ()
            );
        }
    }

    void NetworkWeaponsComponent::OnWeaponConfirmHit(const WeaponHitInfo& hitInfo)
    {
        // If we're replaying inputs then early out
        if (GetNetBindComponent()->IsReprocessingInput())
        {
            return;
        }

        // If we're a simulated weapon, or if the weapon is not predictive, then issue material hit effects since the predicted callback above will not get triggered
        [[maybe_unused]] bool shouldIssueMaterialEffects = !HasController() || !hitInfo.m_weapon.GetParams().m_locallyPredicted;

        for (uint32_t i = 0; i < hitInfo.m_hitEvent.m_hitEntities.size(); ++i)
        {
            const HitEntity& hitEntity = hitInfo.m_hitEvent.m_hitEntities[i];

            if (cl_WeaponsDrawDebug && m_debugDraw)
            {
                m_debugDraw->DrawSphereAtLocation
                (
                    hitEntity.m_hitPosition,
                    1.0f,
                    AZ::Colors::Red,
                    10.0f
                );
            }

            AZLOG
            (
                NET_TraceWeapons,
                "Confirmed hit on entity %u at position %f x %f x %f",
                hitEntity.m_hitNetEntityId,
                hitEntity.m_hitPosition.GetX(),
                hitEntity.m_hitPosition.GetY(),
                hitEntity.m_hitPosition.GetZ()
            );
        }
    }

    void NetworkWeaponsComponent::OnUpdateActivationCounts(int32_t index, uint8_t value)
    {
        IWeapon* weapon = GetWeapon(aznumeric_cast<WeaponIndex>(index));

        if (weapon == nullptr)
        {
            return;
        }

        if (HasController() && weapon->GetParams().m_locallyPredicted)
        {
            // If this is a predicted weapon, exit out because autonomous weapons predict activations
            return;
        }

        AZLOG(NET_TraceWeapons, "Client activation event for weapon index %u", index);

        WeaponState& weaponState = m_simulatedWeaponStates[index];
        const FireParams& fireParams = GetActivationParams(index);
        weapon->SetFireParams(fireParams);

        AZLOG
        (
            NET_TraceWeapons,
            "UpdateActivationCounts fire params target %f x %f x %f",
            fireParams.m_targetPosition.GetX(),
            fireParams.m_targetPosition.GetY(),
            fireParams.m_targetPosition.GetZ()
        );

        while (weaponState.m_activationCount != value)
        {
            const bool validateActivations = false;
            ActivateWeaponWithParams(aznumeric_cast<WeaponIndex>(index), weaponState, fireParams, validateActivations);
        }
    }


    NetworkWeaponsComponentController::NetworkWeaponsComponentController(NetworkWeaponsComponent& parent)
        : NetworkWeaponsComponentControllerBase(parent)
    {
        ;
    }

    void NetworkWeaponsComponentController::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        if (IsAutonomous())
        {
            StartingPointInput::InputEventNotificationBus::MultiHandler::BusConnect(DrawEventId);
            StartingPointInput::InputEventNotificationBus::MultiHandler::BusConnect(FirePrimaryEventId);
            StartingPointInput::InputEventNotificationBus::MultiHandler::BusConnect(FireSecondaryEventId);
        }
    }

    void NetworkWeaponsComponentController::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        if (IsAutonomous())
        {
            StartingPointInput::InputEventNotificationBus::MultiHandler::BusDisconnect(DrawEventId);
            StartingPointInput::InputEventNotificationBus::MultiHandler::BusDisconnect(FirePrimaryEventId);
            StartingPointInput::InputEventNotificationBus::MultiHandler::BusDisconnect(FireSecondaryEventId);
        }
    }

    void NetworkWeaponsComponentController::CreateInput(Multiplayer::NetworkInput& input, [[maybe_unused]] float deltaTime)
    {
        // Inputs for your own component always exist
        NetworkWeaponsComponentNetworkInput* weaponInput = input.FindComponentInput<NetworkWeaponsComponentNetworkInput>();

        weaponInput->m_draw = m_weaponDrawn;
        weaponInput->m_firing = m_weaponFiring;
    }

    void NetworkWeaponsComponentController::ProcessInput(Multiplayer::NetworkInput& input, [[maybe_unused]] float deltaTime)
    {
        NetworkWeaponsComponentNetworkInput* weaponInput = input.FindComponentInput<NetworkWeaponsComponentNetworkInput>();
        GetNetworkAnimationComponentController()->ModifyActiveAnimStates().SetBit(aznumeric_cast<uint32_t>(CharacterAnimState::Aiming), weaponInput->m_draw);

        for (AZStd::size_t weaponIndex = 0; weaponIndex < MaxWeaponsPerComponent; ++weaponIndex)
        {
            const CharacterAnimState animState = CharacterAnimState::Shooting;
            GetNetworkAnimationComponentController()->ModifyActiveAnimStates().SetBit(aznumeric_cast<uint32_t>(animState), weaponInput->m_firing.GetBit(static_cast<uint32_t>(weaponIndex)));
        }

        const AZ::Transform worldTm = GetParent().GetEntity()->GetTransform()->GetWorldTM();

        for (uint32_t weaponIndexInt = 0; weaponIndexInt < MaxWeaponsPerComponent; ++weaponIndexInt)
        {
            if (weaponInput->m_firing.GetBit(weaponIndexInt))
            {
                const AZ::Vector3& aimAngles = GetSimplePlayerCameraComponentController()->GetAimAngles();
                const AZ::Quaternion aimRotation = AZ::Quaternion::CreateRotationZ(aimAngles.GetZ()) * AZ::Quaternion::CreateRotationX(aimAngles.GetX());
                // TODO: This should probably be a physx raycast out to some maxDistance
                const AZ::Vector3 fwd = AZ::Vector3::CreateAxisY();
                const AZ::Vector3 aimTarget = worldTm.GetTranslation() + aimRotation.TransformVector(fwd * 5.0f);
                FireParams fireParams{ aimTarget, Multiplayer::InvalidNetEntityId };
                TryStartFire(aznumeric_cast<WeaponIndex>(weaponIndexInt), fireParams);
            }
        }

        UpdateWeaponFiring(deltaTime);
    }

    void NetworkWeaponsComponentController::UpdateWeaponFiring([[maybe_unused]] float deltaTime)
    {
        for (uint32_t weaponIndexInt = 0; weaponIndexInt < MaxWeaponsPerComponent; ++weaponIndexInt)
        {
            IWeapon* weapon = GetParent().GetWeapon(aznumeric_cast<WeaponIndex>(weaponIndexInt));

            if ((weapon == nullptr) || !weapon->GetParams().m_locallyPredicted)
            {
                continue;
            }

            WeaponState& weaponState = ModifyWeaponStates(weaponIndexInt);
            if ((weaponState.m_status == WeaponStatus::Firing) && (weaponState.m_cooldownTime <= 0.0f))
            {
                AZLOG(NET_TraceWeapons, "Weapon predicted activation event for weapon index %u", weaponIndexInt);

                const bool validateActivations = true;
                const FireParams& fireParams = weapon->GetFireParams();
                GetParent().ActivateWeaponWithParams(aznumeric_cast<WeaponIndex>(weaponIndexInt), weaponState, fireParams, validateActivations);

                if (IsAuthority())
                {
                    AZLOG
                    (
                        NET_TraceWeapons,
                        "UpdateActivationCounts fire params target %f x %f x %f",
                        fireParams.m_targetPosition.GetX(),
                        fireParams.m_targetPosition.GetY(),
                        fireParams.m_targetPosition.GetZ()
                    );

                    SetActivationParams(weaponIndexInt, fireParams);
                    SetActivationCounts(weaponIndexInt, weaponState.m_activationCount);
                }
            }
            weapon->UpdateWeaponState(weaponState, deltaTime);
        }
    }

    bool NetworkWeaponsComponentController::TryStartFire(WeaponIndex weaponIndex, const FireParams& fireParams)
    {
        const uint32_t weaponIndexInt = aznumeric_cast<uint32_t>(weaponIndex);
        AZLOG(NET_TraceWeapons, "Weapon start fire on %u", weaponIndexInt);

        IWeapon* weapon = GetParent().GetWeapon(weaponIndex);
        if (weapon == nullptr)
        {
            return false;
        }

        WeaponState& weaponState = ModifyWeaponStates(weaponIndexInt);
        if (weapon->TryStartFire(weaponState, fireParams))
        {
            const uint32_t animBit = static_cast<uint32_t>(weapon->GetParams().m_animFlag);
            if (!GetNetworkAnimationComponentController()->GetActiveAnimStates().GetBit(animBit))
            {
                GetNetworkAnimationComponentController()->ModifyActiveAnimStates().SetBit(animBit, true);
            }
            return true;
        }

        return false;
    }

    void NetworkWeaponsComponentController::OnPressed([[maybe_unused]] float value)
    {
        const StartingPointInput::InputEventNotificationId* inputId = StartingPointInput::InputEventNotificationBus::GetCurrentBusId();

        if (inputId == nullptr)
        {
            return;
        }
        else if (*inputId == DrawEventId)
        {
            m_weaponDrawn = !m_weaponDrawn;
        }
        else if (*inputId == FirePrimaryEventId)
        {
            m_weaponFiring.SetBit(aznumeric_cast<uint32_t>(PrimaryWeaponIndex), true);
        }
        else if (*inputId == FireSecondaryEventId)
        {
            m_weaponFiring.SetBit(aznumeric_cast<uint32_t>(SecondaryWeaponIndex), true);
        }
    }

    void NetworkWeaponsComponentController::OnReleased([[maybe_unused]] float value)
    {
        const StartingPointInput::InputEventNotificationId* inputId = StartingPointInput::InputEventNotificationBus::GetCurrentBusId();

        if (inputId == nullptr)
        {
            return;
        }
        else if (*inputId == FirePrimaryEventId)
        {
            m_weaponFiring.SetBit(aznumeric_cast<uint32_t>(PrimaryWeaponIndex), false);
        }
        else if (*inputId == FireSecondaryEventId)
        {
            m_weaponFiring.SetBit(aznumeric_cast<uint32_t>(SecondaryWeaponIndex), false);
        }
    }

    void NetworkWeaponsComponentController::OnHeld([[maybe_unused]] float value)
    {
        ;
    }
}
