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

#pragma optimize("", off)
namespace MultiplayerSample
{
    AZ_CVAR(float, cl_WasdStickAccel, 5.f, nullptr, AZ::ConsoleFunctorFlags::Null, "The linear acceleration to apply to WASD inputs to simulate analog stick controls");
    AZ_CVAR(float, cl_AimStickScaleZ, 0.001f, nullptr, AZ::ConsoleFunctorFlags::Null, "The scaling to apply to yaw pitch adjustments");
    AZ_CVAR(float, cl_AimStickScaleX, 0.001f, nullptr, AZ::ConsoleFunctorFlags::Null, "The scaling to apply to pitch view adjustments");
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
            return;
        }

        // Movement axis
        // Since we're on a keyboard, this adds a touch of an acceleration curve to the keyboard inputs
        // This is so that tapping the keyboard moves the virtual stick less than just holding it down
        m_forwardWeight = std::min<float>(m_forwardWasDown ? m_forwardWeight + cl_WasdStickAccel * deltaTime : 0.0f, 1.0f);
        m_leftWeight = std::min<float>(m_leftWasDown ? m_leftWeight + cl_WasdStickAccel * deltaTime : 0.0f, 1.0f);
        m_backwardWeight = std::min<float>(m_backwardWasDown ? m_backwardWeight + cl_WasdStickAccel * deltaTime : 0.0f, 1.0f);
        m_rightWeight = std::min<float>(m_rightWasDown ? m_rightWeight + cl_WasdStickAccel * deltaTime : 0.0f, 1.0f);

        // clear "was" state for each key that was released
        m_forwardWasDown &= m_forwardDown;
        m_backwardWasDown &= m_backwardDown;
        m_leftWasDown &= m_leftDown;
        m_rightWasDown &= m_rightDown;

        // Inputs for your own component always exist
        NetworkPlayerMovementComponentNetworkInput* playerInput = input.FindComponentInput<NetworkPlayerMovementComponentNetworkInput>();

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
            elapsedTime = aznumeric_cast<float>(playerInputController->GetMoveAccumulator());

            if (cl_AimDebugMode == 2)
            {
                constexpr float speed = 10.f;
                // do not use elapsedTime, input is supposed to be for deltaTime which is constant
                // do not use deltaTime, that gets applied in ProcessInput
                m_viewYaw = speed * deltaTime;
            }
        }
#endif

        // View Axis
        // apply aim stick scale here on client otherwise need to set values on both client and server
        // TODO only use the amount of view yaw and pitch used in deltaTime
        playerInput->m_viewYaw = MouseAxis(m_viewYaw * cl_AimStickScaleZ);
        playerInput->m_viewPitch = MouseAxis(m_viewPitch * cl_AimStickScaleX);

#ifndef AZ_RELEASE_BUILD
        if (cl_AimDebugMode > 0)
        {
            AZLOG_INFO("$5 ClientInput deltaYaw: %f, playerInput->m_viewYaw: %f  time: %f, elapsed: %f", m_viewYaw, aznumeric_cast<float>(playerInput->m_viewYaw), elapsedTime)
        }
#endif

        // reset accumulated yaw and pitch
        m_viewYaw = 0.f;
        m_viewPitch = 0.f;

        // Strafe input
        playerInput->m_sprint = m_sprinting;
        playerInput->m_jump = m_jumping;
        playerInput->m_crouch = m_crouching;

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
    }

    void NetworkPlayerMovementComponentController::OnTick(float deltaTime,[[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (!ShouldProcessInput())
        {
            return;
        }

        // LocalPredictionPlayerInputComponentController is listed as required
        Multiplayer::LocalPredictionPlayerInputComponentController* playerInputController = GetLocalPredictionPlayerInputComponentController();
        AZ_Assert(playerInputController, "Missing required LocalPredictionPlayerInputComponentController");

        // the accumulator tells us how many ms we are ahead of the last network tick
        const auto accumulator = playerInputController->GetMoveAccumulator();

        // our tick order is just before the accumulator updates with deltaTime
        const float elapsedTime = aznumeric_cast<float>(accumulator) + deltaTime;

#ifndef AZ_RELEASE_BUILD
        int moveDebugMode = 0;
        AZ::Interface<AZ::IConsole>::Get()->GetCvarValue("cl_MoveDebugMode", moveDebugMode);
        if (moveDebugMode == 2)
        { 
            m_rightDown = true;
            m_rightWasDown = true;
        }
#endif

        AZ::TimeMs inputRateMs;
        AZ::Interface<AZ::IConsole>::Get()->GetCvarValue("cl_InputRateMS", inputRateMs);
        const double clientInputRateSec = AZ::TimeMsToSecondsDouble(inputRateMs);

        //! increment input time
        m_inputTimeMs += deltaTime;
        if (m_leftDown)
        {
            m_leftInputMs += deltaTime;
            m_leftAcc += 1.0; // TODO will need actual stick values for joysticks
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

#ifndef AZ_RELEASE_BUILD
        if (cl_AimDebugMode == 2)
        {
            constexpr float speed = 10.f;
            m_viewYaw = speed * elapsedTime;
            m_viewPitch = 0.f;
        }
#endif
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
            auto percent = accumulator / clientInputRateSec;
            auto orientation = m_previousOrientation.Slerp(m_currentOrientation, accumulator / clientInputRateSec);
            
        }
        else
        {
            // predict movement
            aimAngles.SetZ(NormalizeHeading(aimAngles.GetZ() - playerInput.m_viewYaw * elapsedTime));
            aimAngles.SetX(NormalizeHeading(aimAngles.GetX() - playerInput.m_viewPitch * elapsedTime));
            aimAngles.SetX(
                NormalizeHeading(AZ::GetClamp(aimAngles.GetX(), -AZ::Constants::QuarterPi * 1.5f, AZ::Constants::QuarterPi * 1.5f)));
            GetNetworkSimplePlayerCameraComponentController()->SetPredictedAimAngles(aimAngles);

#ifndef AZ_RELEASE_BUILD
            if (cl_AimDebugMode > 0)
            {
                AZLOG_INFO("$6    Movement OnTick deltaYaw: %f  normalizedYaw: %f elapsed: %f  delta: %f acc: %f", m_viewYaw, aimAngles.GetZ(), elapsedTime, deltaTime, accumulator)
            }
            if (moveDebugMode > 0)
            {
                AZLOG_INFO("$8 Movement OnTick f: %d, b: %d  l: %d, r: %d - sprint: %d", m_forwardDown ? 1 : 0, m_backwardDown ? 1 : 0, m_leftDown ? 1 : 0, m_rightDown ? 1 : 0, m_sprinting ? 1 : 0)
            }
#endif
        }

        const AZ::Quaternion newOrientation = AZ::Quaternion::CreateRotationZ(aimAngles.GetZ());
        GetEntity()->GetTransform()->SetLocalRotationQuaternion(newOrientation);

        // Update velocity
        UpdateVelocity(playerInput);

        GetNetworkCharacterComponentController()->TryMoveWithVelocity(GetVelocity(), deltaTime);
    }

    int NetworkPlayerMovementComponentController::GetTickOrder()
    {
        // tick right after the ClientInput() and ProcessInput() 
        // (ClientInput/ProcessInput only tick when the accumulator exceeds the cl_InputRateMS value)
        return AZ::TICK_ATTACHMENT + 1;
    }

    void NetworkPlayerMovementComponentController::ProcessInput(Multiplayer::NetworkInput& input, float deltaTime)
    {
        if (!ShouldProcessInput())
        {
            return;
        }

        // If the input reset count doesn't match the state's reset count it can mean two things:
        //  1) On the server: we were reset and we are now receiving inputs from the client for an old reset count
        //  2) On the client: we were reset and we are replaying old inputs after being corrected
        // In both cases we don't want to process these inputs
        NetworkPlayerMovementComponentNetworkInput* playerInput = input.FindComponentInput<NetworkPlayerMovementComponentNetworkInput>();
        if (playerInput->m_resetCount != GetNetworkTransformComponentController()->GetResetCount())
        {
            return;
        }

        GetNetworkAnimationComponentController()->ModifyActiveAnimStates().SetBit(
            aznumeric_cast<uint32_t>(CharacterAnimState::Sprinting), playerInput->m_sprint);
        GetNetworkAnimationComponentController()->ModifyActiveAnimStates().SetBit(
            aznumeric_cast<uint32_t>(CharacterAnimState::Jumping), playerInput->m_jump);
        GetNetworkAnimationComponentController()->ModifyActiveAnimStates().SetBit(
            aznumeric_cast<uint32_t>(CharacterAnimState::Crouching), playerInput->m_crouch);

        // Update orientation
        AZ::Vector3 aimAngles = GetNetworkSimplePlayerCameraComponentController()->GetAimAngles();
        aimAngles.SetZ(NormalizeHeading(aimAngles.GetZ() - playerInput->m_viewYaw * deltaTime * (1.f/cl_AimStickScaleZ)));
        aimAngles.SetX(NormalizeHeading(aimAngles.GetX() - playerInput->m_viewPitch * deltaTime * (1.f/cl_AimStickScaleX)));
        aimAngles.SetX(
            NormalizeHeading(AZ::GetClamp(aimAngles.GetX(), -AZ::Constants::QuarterPi * 1.5f, AZ::Constants::QuarterPi * 1.5f)));
        GetNetworkSimplePlayerCameraComponentController()->SetAimAngles(aimAngles);
        //GetNetworkSimplePlayerCameraComponentController()->SetPredictedAimAngles(aimAngles);

#ifndef AZ_RELEASE_BUILD
        if (cl_AimDebugMode > 0)
        {
            AZLOG_INFO("ProcessInput deltaYaw: %f, absYaw: %f, deltaTime: %f", aznumeric_cast<float>(playerInput->m_viewYaw), aimAngles.GetZ(), deltaTime)
        }
        int moveDebugMode = 0;
        AZ::Interface<AZ::IConsole>::Get()->GetCvarValue("cl_MoveDebugMode", moveDebugMode);
        if (moveDebugMode > 0)
        {
            AZLOG_INFO("ProcessInput f: %d, b: %d  l: %d, r: %d - sprint: %d", m_forwardDown ? 1 : 0, m_backwardDown ? 1 : 0, m_leftDown ? 1 :0, m_rightDown ? 1: 0, m_sprinting ? 1 : 0)
        }
#endif

        if (IsNetEntityRoleAutonomous())
        { 
            m_previousOrientation = GetEntity()->GetTransform()->GetLocalRotationQuaternion();
            m_currentOrientation = AZ::Quaternion::CreateRotationZ(aimAngles.GetZ());

            m_previousVelocity = GetVelocity();
        }
        else
        {
            const AZ::Quaternion newOrientation = AZ::Quaternion::CreateRotationZ(aimAngles.GetZ());
            GetEntity()->GetTransform()->SetLocalRotationQuaternion(newOrientation);
        }


        // Update velocity
        UpdateVelocity(*playerInput);


        if (IsNetEntityRoleAutonomous())
        {
            m_currentVelocity = GetVelocity();
        }
        else
        {
            GetNetworkCharacterComponentController()->TryMoveWithVelocity(GetVelocity(), deltaTime);
        }
    }

    void NetworkPlayerMovementComponentController::UpdateVelocity(const NetworkPlayerMovementComponentNetworkInput& playerInput)
    {
        const float fwdBack = playerInput.m_forwardAxis;
        const float leftRight = playerInput.m_strafeAxis;

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

        // Not moving?
        if (fwdBack == 0.0f && leftRight == 0.0f)
        {
            SetVelocity(AZ::Vector3::CreateZero());
        }
        else
        {
            const float stickInputAngle = AZ::Atan2(leftRight, fwdBack);
            const float currentHeading = GetNetworkTransformComponentController()->GetRotation().GetEulerRadians().GetZ();
            const float targetHeading =
                NormalizeHeading(currentHeading + stickInputAngle); // Update current heading with stick input angles
            const AZ::Vector3 fwd = AZ::Vector3::CreateAxisY();
            SetVelocity(AZ::Quaternion::CreateRotationZ(targetHeading).TransformVector(fwd) * speed);
        }
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
            //AZLOG_INFO("yaw %f", value)
        }
        else if (*inputId == LookUpDownEventId)
        {
            m_viewPitch += value;
        }
    }

    void NetworkPlayerMovementComponentController::OnReleased(float value)
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
        else if (*inputId == LookLeftRightEventId)
        {
            m_viewYaw += value;
        }
        else if (*inputId == LookUpDownEventId)
        {
            m_viewPitch += value;
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
            //AZLOG_INFO("yaw %f", value)
            m_viewYaw += value;
        }
        else if (*inputId == LookUpDownEventId)
        {
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
} // namespace MultiplayerSample
#pragma optimize("", on)
