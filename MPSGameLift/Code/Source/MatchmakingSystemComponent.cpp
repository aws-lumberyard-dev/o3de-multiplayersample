/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MatchmakingSystemComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <AWSCoreBus.h>
#include <ResourceMapping/AWSResourceMappingBus.h>

#include <Framework/ServiceRequestJob.h>


namespace MPSGameLift
{
    namespace ServiceAPI
    {
        struct PlayerSkill
        {
            bool OnJsonKey(const char* key, AWSCore::JsonReader& reader)
            {
                if (strcmp(key, "N") == 0)
                {
                    return reader.Accept(skill);
                }
                return reader.Ignore();
            }

            int skill;
        };

        struct PlayerAttributes
        {
            bool OnJsonKey(const char* key, AWSCore::JsonReader& reader)
            {
                if (strcmp(key, "skill") == 0)
                {
                    return reader.Accept(skill);
                }
                return reader.Ignore();
            }

            PlayerSkill skill;
        };

        struct Latencies
        {
            bool OnJsonKey(const char* key, AWSCore::JsonReader& reader)
            {
                return reader.Accept(latencies[key]);
            }

            AZStd::unordered_map<AZStd::string, int> latencies;
        };

        struct Player
        {
            bool OnJsonKey(const char* key, AWSCore::JsonReader& reader)
            {
                if (strcmp(key, "LatencyInMs") == 0)
                {
                    return reader.Accept(latencies);
                }
                if (strcmp(key, "PlayerId") == 0)
                {
                    return reader.Accept(playerId);
                }
                if (strcmp(key, "Team") == 0)
                {
                    return reader.Accept(team);
                }
                if (strcmp(key, "PlayerAttributes") == 0)
                {
                    return reader.Accept(playerAttributes);
                }
                return reader.Ignore();
            }

            AZStd::string playerId;
            AZStd::string team;
            Latencies latencies;
            PlayerAttributes playerAttributes;
        };


        //! Struct for storing the success response.
        struct RequestMatchmakingResponse
        {
            bool OnJsonKey(const char* key, AWSCore::JsonReader& reader)
            {
                if (strcmp(key, "TicketId") == 0)
                {
                    return reader.Accept(ticketId);
                }
                if (strcmp(key, "Players") == 0)
                {
                    return reader.Accept(players);
                }

                return reader.Ignore();
            }

            AZStd::string ticketId;
            AZStd::vector<Player> players;
        };

        //! Struct for storing the error.
        struct RequestMatchmakingError
        {
            bool OnJsonKey(const char* key, AWSCore::JsonReader& reader)
            {
                if (strcmp(key, "message") == 0)
                {
                    return reader.Accept(message);
                }

                if (strcmp(key, "type") == 0)
                {
                    return reader.Accept(type);
                }

                return reader.Ignore();
            }

            //! Do not rename the following members since they are expected by the AWSCore dependency.
            AZStd::string message; //!< Error message.
            AZStd::string type; //!< Error type.
        };

        // Service RequestJobs
        AWS_FEATURE_GEM_SERVICE(MPSGameLift);

        //! GET request to place a matchmaking request "/requestmatchmaking".
        class RequestMatchmaking
            : public AWSCore::ServiceRequest
        {
        public:
            SERVICE_REQUEST(MPSGameLift, HttpMethod::HTTP_GET, "");

            struct Parameters
            {
                bool BuildRequest(AWSCore::RequestBuilder& request)
                {
                    return request.WriteJsonBodyParameter(*this);
                }

                bool WriteJson([[maybe_unused]]AWSCore::JsonWriter& writer) const
                {
                    return true;
                }
            };

            RequestMatchmakingResponse result;
            RequestMatchmakingError error;
            Parameters parameters; //! Request parameter.
        };

        using MPSRequestMatchmakingRequestJob = AWSCore::ServiceRequestJob<RequestMatchmaking>;
    }  // ServiceAPI

    void MatchmakingSystemComponent::Activate()
    {
        AZ::Interface<IMatchmaking>::Register(this);
    }

    void MatchmakingSystemComponent::Deactivate()
    {
        AZ::Interface<IMatchmaking>::Unregister(this);
    }

    void MatchmakingSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MatchmakingSystemComponent, AZ::Component>()
                ->Version(1)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<MatchmakingSystemComponent>("MatchmakingSystemComponent", "[Description of functionality provided by this component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "ComponentCategory")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                 ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<MatchmakingSystemComponent>("MatchmakingSystemComponent")
                ->Attribute(AZ::Script::Attributes::Category, "MPSGameLift Gem")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Method("RequestMatch", [](const AZStd::string& latencies)
                    {
                        if (const auto matchmaking = AZ::Interface<IMatchmaking>::Get())
                        {
                            return matchmaking->RequestMatch(latencies);
                        }
                        return false;
                    })
                ->Method("HasMatch", [](const AZStd::string& ticketId)
                    {
                        if (const auto matchmaking = AZ::Interface<IMatchmaking>::Get())
                        {
                            return matchmaking->HasMatch(ticketId);
                        }
                        return false;
                    })
                ;
        }
    }

    void MatchmakingSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("MPSGameLiftMatchmaking"));
    }

    void SetApiEndpointAndRegion(ServiceAPI::MPSRequestMatchmakingRequestJob::Config* config, const AZStd::string& latencies)
    {
        AZStd::string actualRegion;
        AWSCore::AWSResourceMappingRequestBus::BroadcastResult(actualRegion, &AWSCore::AWSResourceMappingRequests::GetDefaultRegion);

        AZStd::string restApi;
        AWSCore::AWSResourceMappingRequestBus::BroadcastResult(restApi, &AWSCore::AWSResourceMappingRequests::GetResourceNameId, "MPSMatchmaking");
        config->region = actualRegion.c_str();
        config->endpointOverride = AZStd::string::format("https://%s.execute-api.%s.amazonaws.com/%s?latencies=%s",
            restApi.c_str(), actualRegion.c_str(), "Prod/requestmatchmaking", latencies.c_str()).c_str();
    }

   bool MatchmakingSystemComponent::RequestMatch([[maybe_unused]]const AZStd::string& latencies)
    {
        if (!m_ticketId.empty())
        {
            AZ_Warning("MatchmakingSystemComponent", false, "Ticket already exists %s", m_ticketId.c_str())
            return true;
        }
        ServiceAPI::MPSRequestMatchmakingRequestJob::Config* config = ServiceAPI::MPSRequestMatchmakingRequestJob::GetDefaultConfig();

        // Setup example game service endpoint
        SetApiEndpointAndRegion(config, "us-east-1_150");

        ServiceAPI::MPSRequestMatchmakingRequestJob* requestJob = ServiceAPI::MPSRequestMatchmakingRequestJob::Create(
            [this](ServiceAPI::MPSRequestMatchmakingRequestJob* successJob)
            {
                m_ticketId = successJob->result.ticketId;
            },
            []([[maybe_unused]] ServiceAPI::MPSRequestMatchmakingRequestJob* failJob)
            {
                AZ_Error("MatchmakingSystemComponent", false, "Unable to request match error: %s", failJob->error.message.c_str());
            },
            config);

        requestJob->Start();
        return true;
    }

    bool MatchmakingSystemComponent::HasMatch(const AZStd::string& ticketId)
    {
        if (ticketId.empty())
        {
            return false;
        }
        return false;
    }
}
