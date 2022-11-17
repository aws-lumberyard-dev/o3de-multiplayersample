/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/Components/NetworkPlayerMovementComponent.h>

#include <Source/Components/NetworkAiComponent.h>
#include <Multiplayer/Components/NetworkCharacterComponent.h>
#include <Source/Components/NetworkAnimationComponent.h>
#include <Source/Components/NetworkSimplePlayerCameraComponent.h>
#include <Multiplayer/Components/NetworkTransformComponent.h>
#include <Multiplayer/Components/LocalPredictionPlayerInputComponent.h>
#include <AzCore/Time/ITime.h>
#include <AzFramework/Components/CameraBus.h>
#include <AzFramework/Input/Buses/Requests/InputSystemCursorRequestBus.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/CharacterBus.h>
#include <AzFramework/Physics/RigidBodyBus.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>
#include <AzFramework/Physics/Components/SimulatedBodyComponentBus.h>
#include <PhysX/CharacterGameplayBus.h>
#include <PhysX/CharacterControllerBus.h>


namespace MultiplayerSample
{
#pragma optimize("",off)
    AZ_CVAR(float, cl_WasdStickAccel, 5.0f, nullptr, AZ::ConsoleFunctorFlags::Null, "The linear acceleration to apply to WASD inputs to simulate analog stick controls");
    AZ_CVAR(float, cl_AimStickScaleZ, 0.1f, nullptr, AZ::ConsoleFunctorFlags::Null, "The scaling to apply to aim and view adjustments");
    AZ_CVAR(float, cl_AimStickScaleX, 0.05f, nullptr, AZ::ConsoleFunctorFlags::Null, "The scaling to apply to aim and view adjustments");
    AZ_CVAR(float, cl_MaxMouseDelta, 1024.f, nullptr, AZ::ConsoleFunctorFlags::Null, "Mouse deltas will be clamped to this maximum");

#ifndef AZ_RELEASE_BUILD
    AZ_CVAR(int, cl_AimDebugMode, 0, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Aim debug mode");
    AZ_CVAR(int, cl_MoveDebugMode, 0, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Move debug mode");
    AZ_CVAR(bool, cl_MoveBlendingEnabled, true, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "True if client should blend previous and current network tick movement");
#endif

    // Input Event Ids for Player Controls
    const StartingPointInput::InputEventNotificationId MoveFwdEventId("move_fwd");
    const StartingPointInput::InputEventNotificationId MoveBackEventId("move_back");
    const StartingPointInput::InputEventNotificationId MoveLeftEventId("move_left");
    const StartingPointInput::InputEventNotificationId MoveRightEventId("move_right");

    const StartingPointInput::InputEventNotificationId SprintEventId("sprint");
    const StartingPointInput::InputEventNotificationId JumpEventId("jump");
    const StartingPointInput::InputEventNotificationId CrouchEventId("crouch");

    const StartingPointInput::InputEventNotificationId LookLeftRightEventId("lookLeftRight");
    const StartingPointInput::InputEventNotificationId LookUpDownEventId("lookUpDown");

    const StartingPointInput::InputEventNotificationId ZoomInEventId("zoomIn");
    const StartingPointInput::InputEventNotificationId ZoomOutEventId("zoomOut");


    NetworkPlayerMovementComponentController::NetworkPlayerMovementComponentController(NetworkPlayerMovementComponent& parent)
        : NetworkPlayerMovementComponentControllerBase(parent)
        , m_updateAI{ [this] { UpdateAI(); }, AZ::Name{ "MovementControllerAi" } }
    {
        ;
    }

    void NetworkPlayerMovementComponentController::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        NetworkAiComponent* networkAiComponent = GetParent().GetNetworkAiComponent();
        m_aiEnabled = (networkAiComponent != nullptr) ? networkAiComponent->GetEnabled() : false;
        if (m_aiEnabled)
        {
            m_updateAI.Enqueue(AZ::TimeMs{ 0 }, true);
            m_networkAiComponentController = GetNetworkAiComponentController();
        }
        else if (IsNetEntityRoleAutonomous())
        {
            AZ::TickBus::Handler::BusConnect();
            StartingPointInput::InputEventNotificationBus::MultiHandler::BusConnect(MoveFwdEventId);
            StartingPointInput::InputEventNotificationBus::MultiHandler::BusConnect(MoveBackEventId);
            StartingPointInput::InputEventNotificationBus::MultiHandler::BusConnect(MoveLeftEventId);
            StartingPointInput::InputEventNotificationBus::MultiHandler::BusConnect(MoveRightEventId);
            StartingPointInput::InputEventNotificationBus::MultiHandler::BusConnect(SprintEventId);
            StartingPointInput::InputEventNotificationBus::MultiHandler::BusConnect(JumpEventId);
            StartingPointInput::InputEventNotificationBus::MultiHandler::BusConnect(CrouchEventId);
            StartingPointInput::InputEventNotificationBus::MultiHandler::BusConnect(LookLeftRightEventId);
            StartingPointInput::InputEventNotificationBus::MultiHandler::BusConnect(LookUpDownEventId);
            StartingPointInput::InputEventNotificationBus::MultiHandler::BusConnect(ZoomInEventId);
            StartingPointInput::InputEventNotificationBus::MultiHandler::BusConnect(ZoomOutEventId);
        }

        AzPhysics::SimulatedBody* worldBody = nullptr;
        AzPhysics::SimulatedBodyComponentRequestsBus::EventResult(worldBody, GetEntityId(), &AzPhysics::SimulatedBodyComponentRequests::GetSimulatedBody);
        if (worldBody)
        {
            if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
            {
                m_gravity = sceneInterface->GetGravity(worldBody->m_sceneOwner).GetZ();
            }
        }

        m_previousPosition = GetEntity()->GetTransform()->GetWorldTranslation();

        Physics::CharacterRequestBus::EventResult(m_stepHeight, GetEntityId(), &Physics::CharacterRequestBus::Events::GetStepHeight);
        PhysX::CharacterControllerRequestBus::EventResult(m_radius, GetEntityId(), &PhysX::CharacterControllerRequestBus::Events::GetRadius);
    }

    void NetworkPlayerMovementComponentController::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        if (IsNetEntityRoleAutonomous() && !m_aiEnabled)
        {
            AZ::TickBus::Handler::BusDisconnect();
            StartingPointInput::InputEventNotificationBus::MultiHandler::BusDisconnect(MoveFwdEventId);
            StartingPointInput::InputEventNotificationBus::MultiHandler::BusDisconnect(MoveBackEventId);
            StartingPointInput::InputEventNotificationBus::MultiHandler::BusDisconnect(MoveLeftEventId);
            StartingPointInput::InputEventNotificationBus::MultiHandler::BusDisconnect(MoveRightEventId);
            StartingPointInput::InputEventNotificationBus::MultiHandler::BusDisconnect(SprintEventId);
            StartingPointInput::InputEventNotificationBus::MultiHandler::BusDisconnect(JumpEventId);
            StartingPointInput::InputEventNotificationBus::MultiHandler::BusDisconnect(CrouchEventId);
            StartingPointInput::InputEventNotificationBus::MultiHandler::BusDisconnect(LookLeftRightEventId);
            StartingPointInput::InputEventNotificationBus::MultiHandler::BusDisconnect(LookUpDownEventId);
            StartingPointInput::InputEventNotificationBus::MultiHandler::BusDisconnect(ZoomInEventId);
            StartingPointInput::InputEventNotificationBus::MultiHandler::BusDisconnect(ZoomOutEventId);
        }
    }

    bool NetworkPlayerMovementComponentController::ShouldProcessInput() const
    {
        AzFramework::SystemCursorState systemCursorState{AzFramework::SystemCursorState::Unknown};
        AzFramework::InputSystemCursorRequestBus::EventResult( systemCursorState, AzFramework::InputDeviceMouse::Id,
            &AzFramework::InputSystemCursorRequests::GetSystemCursorState);

        // only process input when the system cursor isn't unconstrainted and visible a.k.a console mode
        return systemCursorState != AzFramework::SystemCursorState::UnconstrainedAndVisible;
    }

    void NetworkPlayerMovementComponentController::CreateInput(Multiplayer::NetworkInput& input, float deltaTime)
    {
        CreateInputImpl(input, deltaTime, /*predicted=*/false);
    }

    bool NetworkPlayerMovementComponentController::CreateInputImpl(Multiplayer::NetworkInput& input, float deltaTime, bool predicted)
    {
        // Inputs for your own component always exist
        NetworkPlayerMovementComponentNetworkInput* playerInput = input.FindComponentInput<NetworkPlayerMovementComponentNetworkInput>();

        if (!ShouldProcessInput())
        {
            // clear our input  
            m_forwardWeight = 0.f;
            m_leftWeight = 0.f;
            m_backwardWeight = 0.f;
            m_rightWeight = 0.f;
            m_viewYaw = 0.f;
            m_viewPitch = 0.f;
            m_sprinting = false;
            m_jumping = false;
            m_crouching = false;

            m_forwardWasDown = false;
            m_backwardDown = false;
            m_leftWasDown = false;
            m_rightDown = false;

            playerInput->m_forwardAxis = StickAxis(0.f);
            playerInput->m_strafeAxis = StickAxis(0.f);
            playerInput->m_viewYaw = MouseAxis(m_viewYaw);
            playerInput->m_viewPitch = MouseAxis(m_viewPitch);
            playerInput->m_sprint = m_sprinting;
            playerInput->m_jump = m_jumping;
            playerInput->m_crouch = m_crouching;

            return false;
        }

        // Movement axis
        // Since we're on a keyboard, this adds a touch of an acceleration curve to the keyboard inputs
        // This is so that tapping the keyboard moves the virtual stick less than just holding it down
        m_forwardWeight = std::min<float>(m_forwardWasDown ? m_forwardWeight + cl_WasdStickAccel * deltaTime : 0.0f, 1.0f);
        m_leftWeight = std::min<float>(m_leftWasDown ? m_leftWeight + cl_WasdStickAccel * deltaTime : 0.0f, 1.0f);
        m_backwardWeight = std::min<float>(m_backwardWasDown ? m_backwardWeight + cl_WasdStickAccel * deltaTime : 0.0f, 1.0f);
        m_rightWeight = std::min<float>(m_rightWasDown ? m_rightWeight + cl_WasdStickAccel * deltaTime : 0.0f, 1.0f);

        playerInput->m_forwardAxis = StickAxis(m_forwardWeight - m_backwardWeight);
        playerInput->m_strafeAxis = StickAxis(m_leftWeight - m_rightWeight);

#ifndef AZ_RELEASE_BUILD
        [[maybe_unused]] float elapsedTime = 0.f;
        if (cl_AimDebugMode > 0)
        {
            AZ::ScriptTimePoint time;
            AZ::TickRequestBus::BroadcastResult(time, &AZ::TickRequestBus::Events::GetTimeAtCurrentTick);
            Multiplayer::LocalPredictionPlayerInputComponentController* playerInputController = GetLocalPredictionPlayerInputComponentController();
            AZ_Assert(playerInputController, "Missing required LocalPredictionPlayerInputComponentController");

            // the accumulator tells us how many ms we are ahead of the last network tick
            // at this point the accumulator has updated so it should only contain the left over time
            elapsedTime = aznumeric_cast<float>(playerInputController->GetAccumulator());

            if (cl_AimDebugMode == 2)
            {
                constexpr float speed = 10.f;
                // do not use elapsedTime, input is supposed to be for deltaTime which is constant
                // do not use deltaTime, that gets applied in ProcessInput
                m_viewYaw = speed * deltaTime;
            }
        }
#endif

        // View Axis are clamped and brought into the -1,1 range for transport across the network 
        // TODO only use the amount of view yaw and pitch used in deltaTime
        playerInput->m_viewYaw = MouseAxis(AZStd::clamp<float>(m_viewYaw, -cl_MaxMouseDelta, cl_MaxMouseDelta) / cl_MaxMouseDelta);
        playerInput->m_viewPitch = MouseAxis(AZStd::clamp<float>(m_viewPitch, -cl_MaxMouseDelta, cl_MaxMouseDelta) / cl_MaxMouseDelta);


#ifndef AZ_RELEASE_BUILD
        if (cl_AimDebugMode > 0)
        {
            AZLOG_INFO("$5 ClientInput deltaYaw: %f, playerInput->m_viewYaw: %f  time: %f, elapsed: %f", m_viewYaw, aznumeric_cast<float>(playerInput->m_viewYaw), elapsedTime)
        }
#endif

        // Strafe input
        playerInput->m_sprint = m_sprinting;
        playerInput->m_jump = m_jumping;
        playerInput->m_crouch = m_crouching;

        if (!predicted)
        {
            // clear "was" state for each key that was released
            m_forwardWasDown = m_forwardWasDown && m_forwardDown;
            m_backwardWasDown = m_backwardDown;
            m_leftWasDown = m_leftDown;
            m_rightWasDown = m_rightDown;

            // reset accumulated yaw and pitch
            m_viewYaw = 0.f;
            m_viewPitch = 0.f;

            // reset jumping until next press
            m_jumping = false;

            // reset accumulated amounts
            m_viewYaw = 0.f;
            m_viewPitch = 0.f;
        }


#ifndef AZ_RELEASE_BUILD
        int moveDebugMode = 0;
        AZ::Interface<AZ::IConsole>::Get()->GetCvarValue("cl_MoveDebugMode", moveDebugMode);
        if (moveDebugMode > 0)
        {
            AZLOG_INFO("$7 ClientInput f: %f, b: %f  l: %f, r: %f - sprint: %s", m_forwardWeight, m_backwardWeight, m_leftWeight, m_rightWeight, m_sprinting ? "true" : "false")
        }
#endif

        // Just a note for anyone who is super confused by this, ResetCount is a predictable network property, it gets set on the client
        // through correction packets
        playerInput->m_resetCount = GetNetworkTransformComponentController()->GetResetCount();

        return true;
    }

    void NetworkPlayerMovementComponentController::OnTick(float deltaTime,[[maybe_unused]] AZ::ScriptTimePoint time)
    {
        Multiplayer::NetworkInputArray inputArray(GetEntityHandle());
        Multiplayer::NetworkInput& input = inputArray[0];
        if (!CreateInputImpl(input, deltaTime, /*predicted=*/true))
        {
            // if we didn't create any input don't attempt to process it
            // this can happen if we don't have focus, the console is open etc.
            return;
        }

        if (!m_previousPositionSet)
        {
            m_currentPosition = GetEntity()->GetTransform()->GetWorldTranslation();
            m_previousPosition = m_currentPosition;

            m_currentAimAngles = GetNetworkSimplePlayerCameraComponentController()->GetAimAngles();
            m_previousAimAngles = m_currentAimAngles;

            m_previousPositionSet = true;
        }

        // the accumulator tells us how many ms we are ahead of the last network tick
        Multiplayer::LocalPredictionPlayerInputComponentController* playerInputController = GetLocalPredictionPlayerInputComponentController();
        const auto accumulator = playerInputController->GetAccumulator();
        const float elapsedTime = aznumeric_cast<float>(accumulator) + deltaTime;

#ifndef AZ_RELEASE_BUILD
        if (cl_MoveDebugMode == 2)
        { 
            m_rightDown = true;
            m_rightWasDown = true;
        }

        if (cl_AimDebugMode == 2)
        {
            constexpr float speed = 10.f;
            m_viewYaw = speed * elapsedTime;
            m_viewPitch = 0.f;
        }
#endif

        if (cl_MoveBlendingEnabled)
        {
            // blend between the most recent and most recent -1 network tick
            AZ::TimeMs inputRateMs;
            AZ::Interface<AZ::IConsole>::Get()->GetCvarValue("cl_InputRateMS", inputRateMs);
            const float clientInputRateSec = aznumeric_cast<float>(AZ::TimeMsToSecondsDouble(inputRateMs));
            const float percent = AZStd::clamp<float>(elapsedTime / clientInputRateSec, 0.f, 1.f);

            // these are euler angles so lerp is OK and we want to maintain direction but we'll need to normalize and clamp
            AZ::Vector3 aimAngles = m_previousAimAngles.Lerp(m_previousAimAngles + m_aimAnglesDelta, percent);
            AZ::Vector3 previousAimAngles = GetNetworkSimplePlayerCameraComponentController()->GetAimAnglesPrevious();
            if (m_previousAimAngles != previousAimAngles)
            {
                previousAimAngles = m_previousAimAngles;
            }
            aimAngles.SetX(AZ::GetClamp(NormalizeHeading(aimAngles.GetX()), -AZ::Constants::QuarterPi * 1.5f, AZ::Constants::QuarterPi * 1.5f));
            aimAngles.SetZ(NormalizeHeading(aimAngles.GetZ()));

            Physics::RigidBodyRequestBus::Event(GetEntityId(), &Physics::RigidBodyRequestBus::Events::DisablePhysics);
            GetEntity()->GetTransform()->SetLocalRotationQuaternion(AZ::Quaternion::CreateRotationZ(aimAngles.GetZ()));

            const AZ::Vector3 translation = m_previousPosition.Lerp(m_currentPosition, percent);
            GetEntity()->GetTransform()->SetWorldTranslation(translation);

            GetNetworkSimplePlayerCameraComponentController()->SetAimAngles(aimAngles);
            Physics::RigidBodyRequestBus::Event(GetEntityId(), &Physics::RigidBodyRequestBus::Events::EnablePhysics);
        }
        else
        { 
            // process input based on the new elapsedTime since last network tick
            ProcessInputImpl(input, elapsedTime, /*predicted=*/true);
        }

#ifndef AZ_RELEASE_BUILD
        if (cl_AimDebugMode > 0)
        {
            const AZ::Vector3 aimAngles = GetNetworkSimplePlayerCameraComponentController()->GetAimAngles();
            AZLOG_INFO("$6    Movement OnTick deltaYaw: %f  normalizedYaw: %f elapsed: %f  delta: %f acc: %f", m_viewYaw, aimAngles.GetZ(), elapsedTime, deltaTime, accumulator)
        }

        if (cl_MoveDebugMode > 0)
        {
            AZLOG_INFO("$8 Movement OnTick f: %d, b: %d  l: %d, r: %d - sprint: %d", m_forwardDown ? 1 : 0, m_backwardDown ? 1 : 0, m_leftDown ? 1 : 0, m_rightDown ? 1 : 0, m_sprinting ? 1 : 0)
        }
#endif


        /*
        //! increment input time
        m_inputTimeMs += deltaTime;
        if (m_leftDown)
        {
            m_leftInputMs += deltaTime;
            m_leftAcc += 1.0; // TODO will need actual stick values for gamepads 
        }
        if (m_rightDown)
        {
            m_rightInputMs += deltaTime;
            m_rightAcc += 1.0;
        }
        if (m_forwardDown)
        {
            m_forwardInputMs += deltaTime;
            m_forwardAcc += 1.0;
        }
        if (m_backwardDown)
        {
            m_backwardInputMs += deltaTime;
            m_backwardAcc += 1.0;
        }

        NetworkPlayerMovementComponentNetworkInput playerInput;
        // run values through quantization (MouseAxis) to get the same quantized values on client
        playerInput.m_viewYaw = MouseAxis(m_viewYaw * cl_AimStickScaleZ);
        playerInput.m_viewPitch = MouseAxis(m_viewPitch * cl_AimStickScaleX);
        playerInput.m_sprint = m_sprinting;
        playerInput.m_jump = m_jumping;
        playerInput.m_crouch = m_crouching;

        AZ::Vector3 aimAngles = GetNetworkSimplePlayerCameraComponentController()->GetAimAngles();
        if (cl_MoveBlendingEnabled)
        {
            // blend most recent movement inputs - will always lag  [cl_InputRateMS .. 2 * cl_InputRateMS]
            const float percent = AZStd::clamp<float>(elapsedTime / clientInputRateSec, 0.f, 1.f);
            const AZ::Quaternion orientation = m_previousOrientation.Slerp(m_currentOrientation, percent);
        }
        else
        {
            // predict movement
            aimAngles.SetZ(NormalizeHeading(aimAngles.GetZ() - playerInput.m_viewYaw * elapsedTime));
            aimAngles.SetX(NormalizeHeading(aimAngles.GetX() - playerInput.m_viewPitch * elapsedTime));
            aimAngles.SetX(
                NormalizeHeading(AZ::GetClamp(aimAngles.GetX(), -AZ::Constants::QuarterPi * 1.5f, AZ::Constants::QuarterPi * 1.5f)));
            GetNetworkSimplePlayerCameraComponentController()->SetPredictedAimAngles(aimAngles);

        }

        const AZ::Quaternion newOrientation = AZ::Quaternion::CreateRotationZ(aimAngles.GetZ());
        GetEntity()->GetTransform()->SetLocalRotationQuaternion(newOrientation);

        // Update velocity
        UpdateVelocity(playerInput);

        GetNetworkCharacterComponentController()->TryMoveWithVelocity(GetVelocity(), deltaTime);
        */
    }

    int NetworkPlayerMovementComponentController::GetTickOrder()
    {
        // tick right after the ClientInput() and ProcessInput() 
        // (ClientInput/ProcessInput only tick when the accumulator exceeds the cl_InputRateMS value)
        return AZ::TICK_ATTACHMENT + 1;
    }

    void NetworkPlayerMovementComponentController::ProcessInput(Multiplayer::NetworkInput& input, float deltaTime)
    {
        ProcessInputImpl(input, deltaTime, /*predicted=*/false);
    }

    void NetworkPlayerMovementComponentController::ProcessInputImpl(Multiplayer::NetworkInput& input, float deltaTime, bool predicted)
    {
        //if (!ShouldProcessInput())
        //{
        //    return;
        //}

        // If the input reset count doesn't match the state's reset count it can mean two things:
        //  1) On the server: we were reset and we are now receiving inputs from the client for an old reset count
        //  2) On the client: we were reset and we are replaying old inputs after being corrected
        // In both cases we don't want to process these inputs
        NetworkPlayerMovementComponentNetworkInput* playerInput = input.FindComponentInput<NetworkPlayerMovementComponentNetworkInput>();
        if (playerInput->m_resetCount != GetNetworkTransformComponentController()->GetResetCount())
        {
            return;
        }

        if (cl_MoveBlendingEnabled && IsNetEntityRoleAutonomous())
        {
            // reset back to state of last network tick, not needed when processing multiple inputs in sequence
            Physics::RigidBodyRequestBus::Event(GetEntityId(), &Physics::RigidBodyRequestBus::Events::DisablePhysics);

            GetEntity()->GetTransform()->SetWorldTranslation(m_currentPosition);
            GetEntity()->GetTransform()->SetLocalRotationQuaternion(AZ::Quaternion::CreateRotationZ(m_currentAimAngles.GetZ()));
            GetNetworkSimplePlayerCameraComponentController()->SetAimAngles(m_currentAimAngles);

            //SetSelfGeneratedVelocity(m_previousGeneratedVelocity);
            //SetVelocityFromExternalSources(m_previousExternalVelocity);

            // TODO might need to trigger physics update to get ground state to reset 
            Physics::RigidBodyRequestBus::Event(GetEntityId(), &Physics::RigidBodyRequestBus::Events::EnablePhysics);
        }

        GetNetworkAnimationComponentController()->ModifyActiveAnimStates().SetBit(
            aznumeric_cast<uint32_t>(CharacterAnimState::Sprinting), playerInput->m_sprint);
        GetNetworkAnimationComponentController()->ModifyActiveAnimStates().SetBit(
            aznumeric_cast<uint32_t>(CharacterAnimState::Crouching), playerInput->m_crouch);

        bool onGround = true;
        PhysX::CharacterGameplayRequestBus::EventResult(onGround, GetEntityId(), &PhysX::CharacterGameplayRequestBus::Events::IsOnGround);
        if (onGround)
        {
            if (m_wasOnGround)
            {
                // the Landing anim state will automatically turn off
                GetNetworkAnimationComponentController()->ModifyActiveAnimStates().SetBit( 
                    aznumeric_cast<uint32_t>(CharacterAnimState::Landing), true);
            }

            GetNetworkAnimationComponentController()->ModifyActiveAnimStates().SetBit(
                aznumeric_cast<uint32_t>(CharacterAnimState::Jumping), playerInput->m_jump);
        }


        // Update orientation
        //AZ::Vector3 aimAngles = m_previousAimAngles; // GetNetworkSimplePlayerCameraComponentController()->GetAimAngles();
        AZ::Vector3 aimAngles = m_currentAimAngles;
        const float yawDelta = aimAngles.GetZ() - playerInput->m_viewYaw * deltaTime * cl_AimStickScaleZ * cl_MaxMouseDelta;
        const float pitchDelta = aimAngles.GetX() - playerInput->m_viewPitch * deltaTime * cl_AimStickScaleX * cl_MaxMouseDelta;
        if (IsNetEntityRoleAutonomous() && predicted)
        {
            // set the delta angles before normalizing and clamping to make them easier to blend
            m_aimAnglesDelta.SetZ(yawDelta);
            m_aimAnglesDelta.SetX(pitchDelta);
        }

        aimAngles.SetZ(NormalizeHeading(yawDelta));
        aimAngles.SetX(NormalizeHeading(pitchDelta));

        //aimAngles.SetZ(NormalizeHeading(aimAngles.GetZ() - playerInput->m_viewYaw * deltaTime * (1.f/cl_AimStickScaleZ)));
        //aimAngles.SetX(NormalizeHeading(aimAngles.GetX() - playerInput->m_viewPitch * deltaTime * (1.f/cl_AimStickScaleX)));
        aimAngles.SetX(
            NormalizeHeading(AZ::GetClamp(aimAngles.GetX(), -AZ::Constants::QuarterPi * 1.5f, AZ::Constants::QuarterPi * 1.5f)));
        GetNetworkSimplePlayerCameraComponentController()->SetAimAngles(aimAngles);

        //GetNetworkSimplePlayerCameraComponentController()->SetPredictedAimAngles(aimAngles);

#ifndef AZ_RELEASE_BUILD
        if (cl_AimDebugMode > 0)
        {
            AZLOG_INFO("ProcessInput deltaYaw: %f, absYaw: %f, deltaTime: %f", aznumeric_cast<float>(playerInput->m_viewYaw), aimAngles.GetZ(), deltaTime)
        }
        if (cl_MoveDebugMode > 0)
        {
            AZLOG_INFO("ProcessInput f: %d, b: %d  l: %d, r: %d - sprint: %d", m_forwardDown ? 1 : 0, m_backwardDown ? 1 : 0, m_leftDown ? 1 :0, m_rightDown ? 1: 0, m_sprinting ? 1 : 0)
        }
#endif

        //if (IsNetEntityRoleAutonomous())
        //{ 
        //    m_previousVelocity = m_currentVelocity;
        //}
        //else
        //{
            const AZ::Quaternion newOrientation = AZ::Quaternion::CreateRotationZ(aimAngles.GetZ());
            GetEntity()->GetTransform()->SetLocalRotationQuaternion(newOrientation);
        //}


        // Update velocity
        UpdateVelocity(*playerInput, deltaTime);

        // absolute velocity is based on velocity generated by the player and other sources
        const AZ::Vector3 absoluteVelocity = GetVelocityFromExternalSources() + GetSelfGeneratedVelocity();

        // if we're not moving down on a platform and have a negative velocity we're falling
        GetNetworkAnimationComponentController()->ModifyActiveAnimStates().SetBit(
            aznumeric_cast<uint32_t>(CharacterAnimState::Falling), !onGround && absoluteVelocity.GetZ() < 0.f);

        //if (IsNetEntityRoleAutonomous())
        //{
        //    m_currentVelocity = absoluteVelocity;
        //}
        //else
        //{
            GetNetworkCharacterComponentController()->TryMoveWithVelocity(absoluteVelocity, deltaTime);
        //}

        if (!predicted)
        {
            m_previousAimAngles = m_currentAimAngles;
            m_currentAimAngles = aimAngles;

            m_previousPosition = m_currentPosition;
            m_currentPosition = GetEntity()->GetTransform()->GetWorldTranslation();

            m_aimAnglesDelta = AZ::Vector3::CreateZero();
            m_wasOnGround = onGround;
            m_previousExternalVelocity = GetVelocityFromExternalSources();
            m_previousGeneratedVelocity = GetSelfGeneratedVelocity();
        }
    }

    void NetworkPlayerMovementComponentController::UpdateVelocity(const NetworkPlayerMovementComponentNetworkInput& playerInput, float deltaTime)
    {
        AZ::Vector3 velocityFromExternalSources = GetVelocityFromExternalSources(); // non-player generated (jump pads, explosions etc.)
        AZ::Vector3 selfGeneratedVelocity = GetSelfGeneratedVelocity(); // player generated

        bool onGround = true;
        PhysX::CharacterGameplayRequestBus::EventResult(onGround, GetEntityId(), &PhysX::CharacterGameplayRequestBus::Events::IsOnGround);
        if (onGround)
        {
            if (playerInput.m_jump)
            {
                selfGeneratedVelocity.SetZ(GetJumpVelocity());
            }
            else
            {
                selfGeneratedVelocity.SetZ(0);
            }

            // prevent downward base velocity when on the ground
            if (velocityFromExternalSources.GetZ() < 0.f)
            {
                velocityFromExternalSources.SetZ(0.f);
            }
        }
        else
        {
            // apply gravity
            velocityFromExternalSources.SetZ(velocityFromExternalSources.GetZ() + m_gravity * deltaTime);
            selfGeneratedVelocity.SetZ(selfGeneratedVelocity.GetZ() + m_gravity * deltaTime);
        }

        const float fwdBack = playerInput.m_forwardAxis;
        const float leftRight = playerInput.m_strafeAxis;

        // TODO break out into air/water/ground speeds
        float speed = 0.0f;
        if (playerInput.m_crouch)
        {
            speed = GetCrouchSpeed();
        }
        else if (fwdBack < 0.0f)
        {
            speed = GetReverseSpeed();
        }
        else
        {
            if (playerInput.m_sprint)
            {
                speed = GetSprintSpeed();
            }
            else
            {
                speed = GetWalkSpeed();
            }
        }

        if (fwdBack != 0.0f || leftRight != 0.0f)
        {
            const float stickInputAngle = AZ::Atan2(leftRight, fwdBack);
            const float currentHeading = GetNetworkTransformComponentController()->GetRotation().GetEulerRadians().GetZ();
            const float targetHeading = NormalizeHeading(currentHeading + stickInputAngle);
            
            // instant acceleration for now
            const AZ::Vector3 moveVelocity = GetSlopeHeading(targetHeading, onGround) * speed;
            selfGeneratedVelocity.SetX(moveVelocity.GetX());
            selfGeneratedVelocity.SetY(moveVelocity.GetY());
            selfGeneratedVelocity.SetZ(selfGeneratedVelocity.GetZ() + moveVelocity.GetZ());
        }
        else
        {
            // instant deceleration for now
            selfGeneratedVelocity.SetX(0.f);
            selfGeneratedVelocity.SetY(0.f);
        }

        SetVelocityFromExternalSources(velocityFromExternalSources);
        SetSelfGeneratedVelocity(selfGeneratedVelocity);
    }

    AZ::Vector3 NetworkPlayerMovementComponentController::GetSlopeHeading(float targetHeading, bool onGround) const
    {
        const AZ::Vector3 fwd = AZ::Quaternion::CreateRotationZ(targetHeading).TransformVector(AZ::Vector3::CreateAxisY());
        if (!onGround)
        {
            return fwd;
        }

        const AZ::Vector3 origin = GetEntity()->GetTransform()->GetWorldTranslation();
        constexpr float epsilon = 0.01f;

        // start the trace in front of the player at the step height plus some epsilon
        const AZ::Vector3 start = origin + fwd * (m_radius + epsilon) + AZ::Vector3(0.f, 0.f, m_stepHeight + epsilon);

        AzPhysics::RayCastRequest request;
        request.m_start = start;
        request.m_direction = AZ::Vector3::CreateAxisZ(-1.f);
        request.m_distance = (m_stepHeight + epsilon) * 2.f;
        AZ::EntityId ignore = GetEntityId();
        request.m_queryType = AzPhysics::SceneQuery::QueryType::Static;
        request.m_filterCallback = [ignore](const AzPhysics::SimulatedBody* body, [[maybe_unused]] const Physics::Shape* shape)
        {
            return body->GetEntityId() != ignore ? AzPhysics::SceneQuery::QueryHitType::Block
                                                 : AzPhysics::SceneQuery::QueryHitType::None;
        };

        AzPhysics::SceneQueryHits result;
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            if (AzPhysics::SceneHandle sceneHandle = sceneInterface->GetSceneHandle(AzPhysics::DefaultPhysicsSceneName);
                sceneHandle != AzPhysics::InvalidSceneHandle)
            {
                result = sceneInterface->QueryScene(sceneHandle, &request);
            }
        }
        if (result)
        {
            // we use epsilon here to avoid the case where we are pushing up against an object and become slightly
            // elevated
            if (result.m_hits[0].IsValid() && result.m_hits[0].m_position.GetZ() < origin.GetZ() - epsilon)
            {
                const AZ::Vector3 delta = result.m_hits[0].m_position - origin;
                return delta.GetNormalized();
            }
        }
        return fwd;
    }

    float NetworkPlayerMovementComponentController::NormalizeHeading(float heading) const
    {
        // Ensure heading in range [-pi, +pi]
        if (heading > AZ::Constants::Pi)
        {
            return static_cast<float>(heading - AZ::Constants::TwoPi);
        }
        else if (heading < -AZ::Constants::Pi)
        {
            return static_cast<float>(heading + AZ::Constants::TwoPi);
        }
        return heading;
    }

    void NetworkPlayerMovementComponentController::OnPressed(float value)
    {
        const StartingPointInput::InputEventNotificationId* inputId = StartingPointInput::InputEventNotificationBus::GetCurrentBusId();

        if (inputId == nullptr)
        {
            return;
        }
        else if (*inputId == MoveFwdEventId)
        {
            m_forwardDown = true;
            m_forwardWasDown = true;
        }
        else if (*inputId == MoveBackEventId)
        {
            m_backwardDown = true;
            m_backwardWasDown = true;
        }
        else if (*inputId == MoveLeftEventId)
        {
            m_leftDown = true;
            m_leftWasDown = true;
        }
        else if (*inputId == MoveRightEventId)
        {
            m_rightDown = true;
            m_rightWasDown = true;
        }
        else if (*inputId == SprintEventId)
        {
            m_sprinting = true;
            m_wasSprinting = true;
        }
        else if (*inputId == JumpEventId)
        {
            m_jumping = true;
            m_wasJumping = true;
        }
        else if (*inputId == CrouchEventId)
        {
            m_crouching = true;
        }
        else if (*inputId == LookLeftRightEventId)
        {
            m_viewYaw += value;
        }
        else if (*inputId == LookUpDownEventId)
        {
            m_viewPitch += value;
        }
    }

    void NetworkPlayerMovementComponentController::OnReleased([[maybe_unused]]float value)
    {
        const StartingPointInput::InputEventNotificationId* inputId = StartingPointInput::InputEventNotificationBus::GetCurrentBusId();

        if (inputId == nullptr)
        {
            return;
        }
        else if (*inputId == MoveFwdEventId)
        {
            m_forwardDown = false;
        }
        else if (*inputId == MoveBackEventId)
        {
            m_backwardDown = false;
        }
        else if (*inputId == MoveLeftEventId)
        {
            m_leftDown = false;
        }
        else if (*inputId == MoveRightEventId)
        {
            m_rightDown = false;
        }
        else if (*inputId == SprintEventId)
        {
            m_sprinting = false;
        }
        else if (*inputId == JumpEventId)
        {
            m_jumping = false;
        }
        else if (*inputId == CrouchEventId)
        {
            m_crouching = false;
        }
    }

    void NetworkPlayerMovementComponentController::OnHeld(float value)
    {
        const StartingPointInput::InputEventNotificationId* inputId = StartingPointInput::InputEventNotificationBus::GetCurrentBusId();

        if (inputId == nullptr)
        {
            return;
        }
        else if (*inputId == LookLeftRightEventId)
        {
            // accumulate input to be processed in CreateInput()
            m_viewYaw += value;
        }
        else if (*inputId == LookUpDownEventId)
        {
            // accumulate input to be processed in CreateInput()
            m_viewPitch += value;
        }
    }

    void NetworkPlayerMovementComponentController::UpdateAI()
    {
        float deltaTime = static_cast<float>(m_updateAI.TimeInQueueMs()) / 1000.f;
        if (m_networkAiComponentController != nullptr)
        {
            m_networkAiComponentController->TickMovement(*this, deltaTime);
        }
    }

#pragma optimize("",on)
} // namespace MultiplayerSample
