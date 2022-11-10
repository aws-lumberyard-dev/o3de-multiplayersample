/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <Source/Components/NetworkAiComponent.h>
#include <Source/Components/NetworkSimplePlayerCameraComponent.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Components/CameraBus.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <Multiplayer/IMultiplayer.h>
#include <Multiplayer/MultiplayerPerformanceStats.h>

namespace MultiplayerSample
{
    enum { CAMERA_GROUP = 777 };
    enum { CAMERA_YAW_STAT = 7777 };

    AZ_CVAR(AZ::Vector3, cl_cameraOffset, AZ::Vector3(0.5f, 0.f, 1.5f), nullptr, AZ::ConsoleFunctorFlags::Null, "Offset to use for the player camera");

    NetworkSimplePlayerCameraComponentController::NetworkSimplePlayerCameraComponentController(NetworkSimplePlayerCameraComponent& parent)
        : NetworkSimplePlayerCameraComponentControllerBase(parent)
    {
        DECLARE_PERFORMANCE_STAT_GROUP(CAMERA_GROUP, "CameraStatGroup"); 
        DECLARE_PERFORMANCE_STAT(CAMERA_GROUP, CAMERA_YAW_STAT, "CameraYawStat");
    }

    void NetworkSimplePlayerCameraComponentController::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {

        m_physicsSceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        if (m_physicsSceneInterface)
        {
            m_physicsSceneHandle = m_physicsSceneInterface->GetSceneHandle(AzPhysics::DefaultPhysicsSceneName);
        }

        // Synchronize aim angles with initial transform
        AZ::Vector3& aimAngles = ModifyAimAngles();
        aimAngles.SetZ(GetEntity()->GetTransform()->GetLocalRotation().GetZ());
        SetSyncAimImmediate(true);

        if (IsNetEntityRoleAutonomous())
        {
            m_aiEnabled = FindComponent<NetworkAiComponent>()->GetEnabled();
            if (!m_aiEnabled)
            {
                AZ::EntityId activeCameraId;
                Camera::CameraSystemRequestBus::BroadcastResult(activeCameraId, &Camera::CameraSystemRequestBus::Events::GetActiveCamera);
                m_activeCameraEntity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(activeCameraId);
            }
        }

        AZ::TickBus::Handler::BusConnect();
    }

    void NetworkSimplePlayerCameraComponentController::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        if(AZ::TickBus::Handler::BusIsConnected())
        { 
            AZ::TickBus::Handler::BusDisconnect();
        }
    }

    float NetworkSimplePlayerCameraComponentController::GetCameraYaw() const
    {
        return GetAimAngles().GetZ();
    }

    float NetworkSimplePlayerCameraComponentController::GetCameraPitch() const
    {
        return GetAimAngles().GetX();
    }

    float NetworkSimplePlayerCameraComponentController::GetCameraRoll() const
    {
        return GetAimAngles().GetY();
    }

    float NetworkSimplePlayerCameraComponentController::GetCameraYawPrevious() const
    {
        return GetAimAnglesPrevious().GetZ();
    }

    float NetworkSimplePlayerCameraComponentController::GetCameraPitchPrevious() const
    {
        return GetAimAnglesPrevious().GetX();
    }

    float NetworkSimplePlayerCameraComponentController::GetCameraRollPrevious() const
    {
        return GetAimAnglesPrevious().GetY();
    }

    void NetworkSimplePlayerCameraComponentController::SetPredictedAimAngles(const AZ::Vector3& predictedAimAngles)
    {
        m_predictedAimAngles = predictedAimAngles;
    }

    void NetworkSimplePlayerCameraComponentController::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (m_activeCameraEntity != nullptr && m_activeCameraEntity->GetState() == AZ::Entity::State::Active)
        {
            // exclude roll from any rotation calculations
#ifdef USE_BLENDED_AIM_ANGLES
            const AZ::Quaternion targetYaw = AZ::Quaternion::CreateRotationZ(GetCameraYaw());
            const AZ::Quaternion targetRotation = targetYaw * AZ::Quaternion::CreateRotationX(GetCameraPitch());
            const float blendFactor = Multiplayer::GetMultiplayer()->GetCurrentBlendFactor();
            
            AZ::Quaternion aimRotation = targetRotation;
            if (!GetSyncAimImmediate() && !AZ::IsClose(blendFactor, 1.0f))
            {
                const AZ::Quaternion prevRotation = AZ::Quaternion::CreateRotationZ(GetCameraYawPrevious()) * AZ::Quaternion::CreateRotationX(GetCameraPitchPrevious());
                if (!prevRotation.IsClose(targetRotation))
                {
                    aimRotation = prevRotation.Slerp(targetRotation, blendFactor).GetNormalized();
                }
            }

            SET_PERFORMANCE_STAT(CAMERA_YAW_STAT, GetCameraYaw())
#else
            const AZ::Quaternion targetYaw = AZ::Quaternion::CreateRotationZ(m_predictedAimAngles.GetZ());
            const AZ::Quaternion targetRotation = targetYaw * AZ::Quaternion::CreateRotationX(m_predictedAimAngles.GetX());
            AZ::Quaternion aimRotation = targetRotation;
#endif

#ifndef AZ_RELEASE_BUILD
            int aimDebugMode = 0;
            AZ::Interface<AZ::IConsole>::Get()->GetCvarValue("cl_AimDebugMode", aimDebugMode);
            if (aimDebugMode > 0)
            {
                // Track rate of change based on actual render time
                // Track previous aimRotation
                float currentBlendedYaw = m_predictedAimAngles.GetZ();
                float blendedYawRate = (currentBlendedYaw - m_prevBlendedYaw) / deltaTime;
                AZLOG_INFO("        Camera OnTick absYaw: %f, rate: %f", currentBlendedYaw, blendedYawRate)
                m_prevBlendedYaw = currentBlendedYaw;
            }

#endif

            const AZ::Vector3 targetTranslation = GetEntity()->GetTransform()->GetWorldTM().GetTranslation();
            const AZ::Vector3 cameraPivotOffset = targetYaw.TransformVector(cl_cameraOffset);

            if(GetEnableCollision())
            { 
                AZ::Transform cameraTransform = AZ::Transform::CreateFromQuaternionAndTranslation(aimRotation, targetTranslation + cameraPivotOffset);
                ApplySpringArm(cameraTransform);
                m_activeCameraEntity->GetTransform()->SetWorldTM(cameraTransform);
            }
            else
            { 
                const AZ::Vector3 cameraOffset = cameraPivotOffset + aimRotation.TransformVector(AZ::Vector3(0.f, GetMaxFollowDistance(), 0.f));
                const AZ::Transform cameraTransform = AZ::Transform::CreateFromQuaternionAndTranslation(aimRotation, targetTranslation + cameraOffset);
                m_activeCameraEntity->GetTransform()->SetWorldTM(cameraTransform);
            }
#ifndef AZ_RELEASE_BUILD
            int moveDebugMode = 0;
            AZ::Interface<AZ::IConsole>::Get()->GetCvarValue("cl_MoveDebugMode", moveDebugMode);
            if (moveDebugMode > 0)
            {
                const AZ::Vector3 currentPosition = m_activeCameraEntity->GetTransform()->GetWorldTranslation();
                float speed = currentPosition.GetDistance(m_prevPosition) / deltaTime;
                AZLOG_INFO("$7  Camera OnTick speed: %f", speed)
                m_prevPosition = currentPosition;
            }
#endif
        }

        if (GetSyncAimImmediate())
        {
            SetSyncAimImmediate(false);

            if (!m_activeCameraEntity)
            {
                AZ::TickBus::Handler::BusDisconnect();
            }
        }
    }

    void NetworkSimplePlayerCameraComponentController::ApplySpringArm(AZ::Transform& inOutTransform)
    {
        AZ::Vector3 boxDimensions{ 0.1, 0.1, 0.1 };

        AzPhysics::SceneQueryHits result;
        const AZ::EntityId ignoreEntityId = GetEntityId();
        const AZ::Vector3 cameraPivot = inOutTransform.GetTranslation();

        const AZ::Vector3 direction = -inOutTransform.GetBasisY();
        const float maxDistance = GetMaxFollowDistance();

        // trace from the target to the camera position
        auto request = AzPhysics::ShapeCastRequestHelpers::CreateBoxCastRequest(
                    boxDimensions, inOutTransform, direction, maxDistance,
                    AzPhysics::SceneQuery::QueryType::StaticAndDynamic,
                    AzPhysics::CollisionGroup::All, 
                    [ignoreEntityId](const AzPhysics::SimulatedBody* body, [[maybe_unused]] const Physics::Shape* shape)
                    {
                        return body->GetEntityId() != ignoreEntityId ? AzPhysics::SceneQuery::QueryHitType::Block
                                                             : AzPhysics::SceneQuery::QueryHitType::None;
                    });
        result = m_physicsSceneInterface->QueryScene(m_physicsSceneHandle, &request);
        if (result && !result.m_hits.empty())
        {
            float collisionDistance =
                AZ::GetClamp(result.m_hits[0].m_distance - GetCollisionOffset(), GetMinFollowDistance(), maxDistance);
            inOutTransform.SetTranslation(cameraPivot + direction * collisionDistance);
        }
        else
        {
            inOutTransform.SetTranslation(cameraPivot + direction * maxDistance);
        }
    }

    int NetworkSimplePlayerCameraComponentController::GetTickOrder()
    {
        return AZ::TICK_PRE_RENDER;
    }
}
