//******************************************************************
//
// Copyright 2015 Intel Mobile Communications GmbH All Rights Reserved.
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

#include "resourcemanager.h"
#include "securevirtualresourcetypes.h"
#include "aclresource.h"
#include "ocmalloc.h"
#include "logger.h"
#include "utlist.h"
#include <string.h>

#define TAG PCF("SRM-RM")

/**
 * This method is used by all secure resource modules to send responses to REST queries.
 *
 * @param ehRequest - pointer to entity handler request data structure.
 * @param rspPayload - response payload in JSON.
 *
 * @retval  OC_STACK_OK for Success, otherwise some error value
 */
OCStackResult SendSRMResponse(OCEntityHandlerRequest *ehRequest, char *rspPayload)
{
    OCEntityHandlerResponse response;
    if (ehRequest)
    {
        response.requestHandle = ehRequest->requestHandle;
        response.resourceHandle = ehRequest->resource;
        response.ehResult = OC_EH_OK;
        response.payload = (unsigned char *)rspPayload;
        response.payloadSize = (rspPayload ? strlen(rspPayload) : 0);
        response.persistentBufferFlag = 0;

        return OCDoResponse(&response);
    }
    return OC_STACK_ERROR;
}

/**
 * Initialize all secure resources ( /oic/sec/cred, /oic/sec/acl, /oic/sec/pstat etc).
 *
 * @retval  OC_STACK_OK for Success, otherwise some error value
 */
OCStackResult InitSecureResources( )
{
    OCStackResult ret;

    ret = InitACLResource();
    /*
     * TODO : Update this method for all other resources
     * InitPstatResource();
     * InitCredentialResource();
     */

    return ret;
}

/**
 * Perform cleanup for secure resources ( /oic/sec/cred, /oic/sec/acl, /oic/sec/pstat etc).
 *
 * @retval  OC_STACK_OK for Success, otherwise some error value
 */
OCStackResult DestroySecureResources( )
{
    DeInitACLResource();
    /*
     * TODO : Update this method for all other resources
     * DeInitPstatResource();
     * DeInitCredentialResource();
     */
    return OC_STACK_OK;
}

