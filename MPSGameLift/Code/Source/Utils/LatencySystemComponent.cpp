
#include <Utils/LatencySystemComponent.h>

#include <aws/core/http/HttpResponse.h>
#include <HttpRequestor/HttpRequestorBus.h>
#include <HttpRequestor/HttpTypes.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#pragma optimize("",off)
namespace MPSGameLift
{
    LatencySystemComponent::LatencySystemComponent()
    {
        AZ::Interface<IRegionalLatencyFinder>::Register(this);

    }

    LatencySystemComponent::~LatencySystemComponent()
    {
        AZ::Interface<IRegionalLatencyFinder>::Unregister(this);
    }

    void LatencySystemComponent::Activate()
    {
    }

    void LatencySystemComponent::Deactivate()
    {
    }

    void LatencySystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<LatencySystemComponent, AZ::Component>()
                ->Version(1)
                ->Field("Regions", &LatencySystemComponent::m_regions)
            ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<LatencySystemComponent>("LatencySystemComponent", "[Description of functionality provided by this component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "ComponentCategory")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                ->DataElement(AZ::Edit::UIHandlers::Default, &LatencySystemComponent::m_regions, "AWS Regions", "List of AWS Regions")
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<LatencySystemComponent>("LatencySystemComponent")
                ->Attribute(AZ::Script::Attributes::Category, "MPSGameLift Gem")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)

                ->Method("RequestLatencies", []()
                    {
                        if (const auto latencyFinder = AZ::Interface<IRegionalLatencyFinder>::Get())
                        {
                            latencyFinder->RequestLatencies();
                        }
                    })
                ->Method("HasLatencies", []()
                    {
                        if (const auto latencyFinder = AZ::Interface<IRegionalLatencyFinder>::Get())
                        {
                            return latencyFinder->HasLatencies();
                        }

                        return false;
                    })
                ->Method("GetLatencyForRegion", [](const AZStd::string& region)
                    {
                        if (const auto latencyFinder = AZ::Interface<IRegionalLatencyFinder>::Get())
                        {
                            return latencyFinder->GetLatencyForRegion(region);
                        }

                        return uint32_t(0);
                    })
            ;
        }
    }

    void LatencySystemComponent::RequestLatencies()
    {
        constexpr const char* AmazonAwsRegions[] = { "us-east-1",  "us-west-2",  "ru-central-1", "me-south-1"};

        for(const char* region : AmazonAwsRegions)
        {
            AZStd::string regionEndpoint = AZStd::string::format("https://dynamodb.%s.amazonaws.com", region);
            AZ_Info("LatencySystemComponent", "regionEndpoint %s", regionEndpoint.c_str());

            HttpRequestor::HttpRequestorRequestBus::Broadcast(&HttpRequestor::HttpRequestorRequests::AddTextRequest, regionEndpoint, Aws::Http::HttpMethod::HTTP_GET,
                [region](const AZStd::string& response, Aws::Http::HttpResponseCode responseCode)
                {

                    AZStd::chrono::milliseconds roundTripTime;
                    HttpRequestor::HttpRequestorRequestBus::BroadcastResult(roundTripTime, &HttpRequestor::HttpRequestorRequests::GetLastRoundTripTime);


                    if (responseCode == Aws::Http::HttpResponseCode::OK) 
                    {
                        AZ_Info("LatencySystemComponent", "%s call succeed in %lld ms. Response: %s", 
                            region, roundTripTime.count(), response.c_str());
                    }
                    else
                    {
                        AZ_Info("LatencySystemComponent", "Failed");
                    }
                });
        }
    }

    void LatencySystemComponent::SetLatencyForRegion(const AZStd::string& region, uint32_t latency)
    {
        m_latencyMap.insert(AZStd::pair<AZStd::string, uint32_t>(region, latency));
    }

    bool LatencySystemComponent::HasLatencies() const
    {
        for (auto iter = m_latencyMap.begin(); iter != m_latencyMap.end(); ++iter)
        {
            if (iter->second == 0)
            {
                // latency not set for region so fail check
                return false;
            }
        }
        return true;
    }

    AZStd::string LatencySystemComponent::GetLatencyString(const AZStd::string & region) const
    {
        const uint32_t latency = GetLatencyForRegion(region);
        return AZStd::string::format("%s_%d", region.c_str(), latency);
    }

    uint32_t LatencySystemComponent::GetLatencyForRegion(const AZStd::string& region) const
    {
        if (const auto latencyKeyValue = m_latencyMap.find(region); latencyKeyValue != m_latencyMap.end())
        {
            return latencyKeyValue->second;
        }
        else 
        {
            return 150; // TODO: Decide if we want to return default
        }
    }
} // namespace MPSGameLift
#pragma optimize("",on)