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

#ifndef DEVICEPRESENCE_H_
#define DEVICEPRESENCE_H_

#include <list>
#include <string>

#include "BrokerTypes.h"
#include "ResourcePresence.h"
#include "PresenceSubscriber.h"

namespace OIC
{
    namespace Service
    {
        class DevicePresence
        {
        public:
            DevicePresence();
            ~DevicePresence();

            void initializeDevicePresence(PrimitiveResourcePtr pResource);

            void addPresenceResource(ResourcePresence * rPresence);
            void removePresenceResource(ResourcePresence * rPresence);

            bool isEmptyResourcePresence() const;
            const std::string getAddress() const;

        private:
            void requestAllResourcePresence();
            void subscribeCB(OCStackResult ret,const unsigned int seq, const std::string& Hostaddress);
            void * timeOutCB(unsigned int msg);

            std::list<ResourcePresence * > resourcePresenceList;

            std::string address;
            DEVICE_STATE state;
            bool isWithinTime;

            SubscribeCB pSubscribeRequestCB;
            TimeoutCB pTimeoutCB;
            PresenceSubscriber presenceSubscriber;
        };
    } // namespace Service
} // namespace OIC

#endif /* DEVICEPRESENCE_H_ */