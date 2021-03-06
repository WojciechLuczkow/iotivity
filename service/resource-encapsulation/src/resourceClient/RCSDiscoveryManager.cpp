//******************************************************************
//
// Copyright 2015 Samsung Electronics All Rights Reserved.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "RCSDiscoveryManager.h"

#include "RCSRemoteResourceObject.h"
#include "PrimitiveResource.h"

#include "ScopeLogger.h"

#define TAG "RCSDiscoveryManager"

using namespace OIC::Service;

namespace
{
    void findCallback(std::shared_ptr< PrimitiveResource > primitiveResource,
            RCSDiscoveryManager::ResourceDiscoveredCallback cb)
    {
        SCOPE_LOG_F(DEBUG, TAG);

        if (!primitiveResource)
        {
            OC_LOG(ERROR, TAG, "findCallback : primitiveResource is null.");
            return;
        }

        cb(std::make_shared< RCSRemoteResourceObject >(primitiveResource));
    }
}

namespace OIC
{
    namespace Service
    {
        RCSDiscoveryManager* RCSDiscoveryManager::getInstance()
        {
            static RCSDiscoveryManager instance;
            return &instance;
        }

        void RCSDiscoveryManager::discoverResource(const RCSAddress& address,
                ResourceDiscoveredCallback cb)
        {
            discoverResourceByType(address, OC_RSRVD_WELL_KNOWN_URI, "", std::move(cb));
        }

        void RCSDiscoveryManager::discoverResource(const RCSAddress& address,
                const std::string& relativeURI, ResourceDiscoveredCallback cb)
        {
            discoverResourceByType(address, relativeURI, "", std::move(cb));
        }

        void RCSDiscoveryManager::discoverResourceByType(const RCSAddress& address,
                const std::string& resourceType,
                ResourceDiscoveredCallback cb)
        {
            discoverResourceByType(address, OC_RSRVD_WELL_KNOWN_URI, resourceType, std::move(cb));
        }

        void RCSDiscoveryManager::discoverResourceByType(const RCSAddress& address,
                const std::string& relativeURI, const std::string& resourceType,
                ResourceDiscoveredCallback cb)
        {
            if (!cb)
            {
                OC_LOG(ERROR, TAG, "discoverResourceByType NULL Callback");
                throw InvalidParameterException { "discoverResourceByType NULL Callback'" };
            }
            else
            {
                std::string resourceURI = relativeURI + "?rt=" + resourceType;
                OIC::Service::discoverResource(address, resourceURI,
                    std::bind(findCallback, std::placeholders::_1, std::move(cb)));
            }
        }
    }
}
