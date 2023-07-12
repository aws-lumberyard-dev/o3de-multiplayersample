
#include <Utils/RegionalLatencySystemComponent.h>

#include <aws/core/http/HttpResponse.h>
#include <HttpRequestor/HttpRequestorBus.h>
#include <HttpRequestor/HttpTypes.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#pragma optimize("",off)
namespace MPSGameLift
{
    RegionalLatencySystemComponent::RegionalLatencySystemComponent()
    {
        AZ::Interface<IRegionalLatencyFinder>::Register(this);

    }

    RegionalLatencySystemComponent::~RegionalLatencySystemComponent()
    {
        AZ::Interface<IRegionalLatencyFinder>::Unregister(this);
    }

    void RegionalLatencySystemComponent::Activate()
    {
    }

    void RegionalLatencySystemComponent::Deactivate()
    {
    }

    void RegionalLatencySystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (const auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<RegionalLatencySystemComponent, AZ::Component>()
                ->Version(1)
            ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<RegionalLatencySystemComponent>("RegionalLatencySystemComponent")
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
                ->Method("GetLatencyForRegionMs", [](const AZStd::string& region) -> uint32_t
                    {
                        if (const auto latencyFinder = AZ::Interface<IRegionalLatencyFinder>::Get())
                        {
                            return aznumeric_cast<uint32_t>(latencyFinder->GetLatencyForRegion(region).count());
                        }

                        return 0;
                    })
            ;
        }
    }

    void RegionalLatencySystemComponent::RequestLatencies()
    {
        for(auto region : Regions)
        {
            AZStd::string regionEndpoint = AZStd::string::format(RegionalEndpointUrlFormat, region);
            AZ_Info("RegionalLatencySystemComponent", "regionEndpoint %s", regionEndpoint.c_str());

            HttpRequestor::HttpRequestorRequestBus::Broadcast(&HttpRequestor::HttpRequestorRequests::AddTextRequest, regionEndpoint, Aws::Http::HttpMethod::HTTP_GET,
                [this, region](const AZStd::string& response, Aws::Http::HttpResponseCode responseCode)
                {
                    AZStd::chrono::milliseconds roundTripTime;
                    HttpRequestor::HttpRequestorRequestBus::BroadcastResult(roundTripTime, &HttpRequestor::HttpRequestorRequests::GetLastRoundTripTime);

                    m_latencyMap.insert({ region, roundTripTime });

                    if (responseCode == Aws::Http::HttpResponseCode::OK) 
                    {
                        AZ_Info("RegionalLatencySystemComponent", "%s call succeed in %lld ms. Response: %s", 
                            region, roundTripTime.count(), response.c_str());
                    }
                    else
                    {
                        AZ_Info("RegionalLatencySystemComponent", "Failed");
                    }
                });
        }
    }

    bool RegionalLatencySystemComponent::HasLatencies() const
    {
        for (const auto& iter : m_latencyMap)
        {
            if (iter.second == AZStd::chrono::milliseconds(0))
            {
                // latency not set for region so fail check
                return false;
            }
        }
        return true;
    }

    AZStd::chrono::milliseconds RegionalLatencySystemComponent::GetLatencyForRegion(const AZStd::string& region) const
    {
        if (const auto latencyKeyValue = m_latencyMap.find(region); latencyKeyValue != m_latencyMap.end())
        {
            return latencyKeyValue->second;
        }

        return {};
    }
} // namespace MPSGameLift
#pragma optimize("",on)