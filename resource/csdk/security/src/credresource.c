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

#define __STDC_LIMIT_MACROS
#include "ocstack.h"
#include "logger.h"
#include "oic_malloc.h"
#include "cJSON.h"
#include "resourcemanager.h"
#include "psinterface.h"
#include "utlist.h"
#include "srmresourcestrings.h"
#include "credresource.h"
#include "ocrandom.h"
#include "doxmresource.h"
#include "base64.h"
#include "srmutility.h"
#include "cainterface.h"
#include "pbkdf2.h"
#include <stdlib.h>
#ifdef WITH_ARDUINO
#include <string.h>
#else
#include <strings.h>
#endif
#include <stdint.h>

#define TAG  "SRM-CREDL"


static OicSecCred_t        *gCred = NULL;
static OCResourceHandle    gCredHandle = NULL;

/**
 * This function frees OicSecCred_t object's fields and object itself.
 */
static void FreeCred(OicSecCred_t *cred)
{
    if(NULL == cred)
    {
        OC_LOG (INFO, TAG, "Invalid Parameter");
        return;
    }
    //Note: Need further clarification on roleID data type
#if 0
    //Clean roleIds
    OICFree(cred->roleIds);
#endif

    //Clean PublicData
    OICFree(cred->publicData.data);

    //Clean PrivateData
    OICFree(cred->privateData.data);

    //Clean Period
    OICFree(cred->period);

    //Clean Owners
    OICFree(cred->owners);

    //Clean Cred node itself
    OICFree(cred);
}

void DeleteCredList(OicSecCred_t* cred)
{
    if (cred)
    {
        OicSecCred_t *credTmp1 = NULL, *credTmp2 = NULL;
        LL_FOREACH_SAFE(cred, credTmp1, credTmp2)
        {
            LL_DELETE(cred, credTmp1);
            FreeCred(credTmp1);
        }
    }
}

/**
 * This function converts credential data into JSON format.
 * Caller needs to invoke 'free' when done using
 * returned string.
 * @param cred  pointer to instance of OicSecCred_t structure.
 *
 * @retval
 *      pointer to JSON credential representation - if credential for subjectId found
 *      NULL                                      - if credential for subjectId not found
 */
char * BinToCredJSON(const OicSecCred_t * cred)
{
    cJSON *jsonRoot = NULL;
    char *jsonStr = NULL;

    if (cred)
    {
        char base64Buff[B64ENCODE_OUT_SAFESIZE(sizeof(((OicUuid_t*)0)->id)) + 1] = {};
        uint32_t outLen = 0;
        B64Result b64Ret = B64_OK;

        jsonRoot = cJSON_CreateObject();
        VERIFY_NON_NULL(TAG, jsonRoot, ERROR);

        cJSON *jsonCredArray = NULL;
        cJSON_AddItemToObject(jsonRoot, OIC_JSON_CRED_NAME,
                jsonCredArray = cJSON_CreateArray());
        VERIFY_NON_NULL(TAG, jsonCredArray, ERROR);

        while(cred)
        {
            cJSON *jsonCred = cJSON_CreateObject();
            VERIFY_NON_NULL(TAG, jsonCred, ERROR);

            //CredID -- Mandatory
            cJSON_AddNumberToObject(jsonCred, OIC_JSON_CREDID_NAME, (int)cred->credId);

            //Subject -- Mandatory
            outLen = 0;
            memset(base64Buff, 0, sizeof(base64Buff));
            b64Ret = b64Encode(cred->subject.id, sizeof(cred->subject.id), base64Buff,
                   sizeof(base64Buff), &outLen);
            VERIFY_SUCCESS(TAG, b64Ret == B64_OK, ERROR);
            cJSON_AddStringToObject(jsonCred, OIC_JSON_SUBJECT_NAME, base64Buff);

            //Note: Need further clarification on roleID data type
#if 0
            //RoleId -- Not Mandatory
            if(cred->roleIdsLen > 0)
            {
                cJSON *jsonRoleIdsArray = NULL;
                cJSON_AddItemToObject (jsonCred, OIC_JSON_ROLEIDS_NAME,
                                         jsonRoleIdsArray = cJSON_CreateArray());
                VERIFY_NON_NULL(TAG, jsonRoleIdsArray, ERROR);
                for (size_t i = 0; i < cred->roleIdsLen; i++)
                {
                    cJSON_AddItemToArray (jsonRoleIdsArray,
                            cJSON_CreateString((char *)cred->roleIds[i].id));
                }
            }
#endif

            //CredType -- Mandatory
            cJSON_AddNumberToObject(jsonCred, OIC_JSON_CREDTYPE_NAME,(int)cred->credType);

#if 0
            //PublicData -- Not Mandatory
            if(cred->publicData.data)
            {
                cJSON_AddStringToObject(jsonCred, OIC_JSON_PUBLICDATA_NAME, cred->publicData.data);
            }
#endif
            //PrivateData -- Not Mandatory
            if(cred->privateData.data)
            {
                cJSON_AddStringToObject(jsonCred, OIC_JSON_PRIVATEDATA_NAME, cred->privateData.data);
            }

            //Period -- Not Mandatory
            if(cred->period)
            {
                cJSON_AddStringToObject(jsonCred, OIC_JSON_PERIOD_NAME,
                                        cred->period);
            }

            //Owners -- Mandatory
            cJSON *jsonOwnrArray = NULL;
            cJSON_AddItemToObject (jsonCred, OIC_JSON_OWNERS_NAME,
                                             jsonOwnrArray = cJSON_CreateArray());
            VERIFY_NON_NULL(TAG, jsonOwnrArray, ERROR);
            for (size_t i = 0; i < cred->ownersLen; i++)
            {
                outLen = 0;
                memset(base64Buff, 0, sizeof(base64Buff));
                b64Ret = b64Encode(cred->owners[i].id, sizeof(cred->owners[i].id),
                        base64Buff, sizeof(base64Buff), &outLen);
                VERIFY_SUCCESS(TAG, b64Ret == B64_OK, ERROR);
                cJSON_AddItemToArray (jsonOwnrArray,
                                       cJSON_CreateString((char *)(base64Buff)));
            }

            /* Attach current cred node to cred Array */
            cJSON_AddItemToArray(jsonCredArray, jsonCred);
            cred = cred->next;
        }

        jsonStr = cJSON_PrintUnformatted(jsonRoot);
    }

exit:
    if (jsonRoot)
    {
        cJSON_Delete(jsonRoot);
    }
    return jsonStr;
}

/*
 * This internal method converts JSON cred into binary cred.
 */
OicSecCred_t * JSONToCredBin(const char * jsonStr)
{
    OCStackResult ret = OC_STACK_ERROR;
    OicSecCred_t * headCred = NULL;
    OicSecCred_t * prevCred = NULL;
    cJSON *jsonCredArray = NULL;

    cJSON *jsonRoot = cJSON_Parse(jsonStr);
    VERIFY_NON_NULL(TAG, jsonRoot, ERROR);

    jsonCredArray = cJSON_GetObjectItem(jsonRoot, OIC_JSON_CRED_NAME);
    VERIFY_NON_NULL(TAG, jsonCredArray, ERROR);
    if (cJSON_Array == jsonCredArray->type)
    {
        int numCred = cJSON_GetArraySize(jsonCredArray);
        int idx = 0;

        unsigned char base64Buff[sizeof(((OicUuid_t*)0)->id)] = {};
        uint32_t outLen = 0;
        B64Result b64Ret = B64_OK;

        VERIFY_SUCCESS(TAG, numCred > 0, ERROR);
        do
        {
            cJSON *jsonCred = cJSON_GetArrayItem(jsonCredArray, idx);
            VERIFY_NON_NULL(TAG, jsonCred, ERROR);

            OicSecCred_t *cred = (OicSecCred_t*)OICCalloc(1, sizeof(OicSecCred_t));
            VERIFY_NON_NULL(TAG, cred, ERROR);

            headCred = (headCred) ? headCred : cred;
            if (prevCred)
            {
                prevCred->next = cred;
            }
            size_t jsonObjLen = 0;
            cJSON *jsonObj = NULL;

            //CredId -- Mandatory
            jsonObj = cJSON_GetObjectItem(jsonCred, OIC_JSON_CREDID_NAME);
            if(jsonObj)
            {
                VERIFY_SUCCESS(TAG, cJSON_Number == jsonObj->type, ERROR);
                cred->credId = jsonObj->valueint;
            }

            //subject -- Mandatory
            jsonObj = cJSON_GetObjectItem(jsonCred, OIC_JSON_SUBJECT_NAME);
            VERIFY_NON_NULL(TAG, jsonObj, ERROR);
            VERIFY_SUCCESS(TAG, cJSON_String == jsonObj->type, ERROR);
            outLen = 0;
            memset(base64Buff, 0, sizeof(base64Buff));
            b64Ret = b64Decode(jsonObj->valuestring, strlen(jsonObj->valuestring),
                    base64Buff, sizeof(base64Buff), &outLen);
            VERIFY_SUCCESS(TAG, (b64Ret == B64_OK && outLen <= sizeof(cred->subject.id)),
                           ERROR);
            memcpy(cred->subject.id, base64Buff, outLen);

            //CredType -- Mandatory
            jsonObj = cJSON_GetObjectItem(jsonCred, OIC_JSON_CREDTYPE_NAME);
            VERIFY_NON_NULL(TAG, jsonObj, ERROR);
            VERIFY_SUCCESS(TAG, cJSON_Number == jsonObj->type, ERROR);
            cred->credType = jsonObj->valueint;

            //PrivateData is mandatory for some of the credential types listed below.
            jsonObj = cJSON_GetObjectItem(jsonCred, OIC_JSON_PRIVATEDATA_NAME);
            if ((cred->credType & SYMMETRIC_PAIR_WISE_KEY) ||
                (cred->credType & SYMMETRIC_GROUP_KEY) ||
                (cred->credType & PIN_PASSWORD))
            {
                VERIFY_NON_NULL(TAG, jsonObj, ERROR);
                VERIFY_SUCCESS(TAG, cJSON_String == jsonObj->type, ERROR);
            }
            if(jsonObj && cJSON_String == jsonObj->type)
            {
                jsonObjLen = strlen(jsonObj->valuestring) + 1;
                cred->privateData.data = (char *)OICMalloc(jsonObjLen);
                VERIFY_NON_NULL(TAG, (cred->privateData.data), ERROR);
                strncpy((char *)cred->privateData.data, (char *)jsonObj->valuestring, jsonObjLen);
            }

            //Period -- Not Mandatory
            jsonObj = cJSON_GetObjectItem(jsonCred, OIC_JSON_PERIOD_NAME);
            if(jsonObj && cJSON_String == jsonObj->type)
            {
                jsonObjLen = strlen(jsonObj->valuestring) + 1;
                cred->period = (char *)OICMalloc(jsonObjLen);
                VERIFY_NON_NULL(TAG, cred->period, ERROR);
                strncpy(cred->period, jsonObj->valuestring, jsonObjLen);
            }

            //Owners -- Mandatory
            jsonObj = cJSON_GetObjectItem(jsonCred, OIC_JSON_OWNERS_NAME);
            VERIFY_NON_NULL(TAG, jsonObj, ERROR);
            VERIFY_SUCCESS(TAG, cJSON_Array == jsonObj->type, ERROR);
            cred->ownersLen = cJSON_GetArraySize(jsonObj);
            VERIFY_SUCCESS(TAG, cred->ownersLen > 0, ERROR);
            cred->owners = (OicUuid_t*)OICCalloc(cred->ownersLen, sizeof(OicUuid_t));
            VERIFY_NON_NULL(TAG, (cred->owners), ERROR);
            for(size_t i = 0; i < cred->ownersLen; i++)
            {
                cJSON *jsonOwnr = cJSON_GetArrayItem(jsonObj, i);
                VERIFY_NON_NULL(TAG, jsonOwnr, ERROR);
                VERIFY_SUCCESS(TAG, cJSON_String == jsonOwnr->type, ERROR);
                outLen = 0;
                memset(base64Buff, 0, sizeof(base64Buff));
                b64Ret = b64Decode(jsonOwnr->valuestring, strlen(jsonOwnr->valuestring),
                         base64Buff, sizeof(base64Buff), &outLen);
                VERIFY_SUCCESS(TAG, (b64Ret == B64_OK &&
                               outLen <= sizeof(cred->owners[i].id)), ERROR);
                memcpy(cred->owners[i].id, base64Buff, outLen);
            }
            prevCred = cred;
        } while( ++idx < numCred);
    }

    ret = OC_STACK_OK;

exit:
    cJSON_Delete(jsonRoot);
    if (OC_STACK_OK != ret)
    {
        DeleteCredList(headCred);
        headCred = NULL;
    }
    return headCred;
}

/**
 * This function generates the bin credential data.
 *
 * @param subject pointer to subject of this credential.
 * @param credType credential type.
 * @param publicData public data such as public key.
 * @param privateData private data such as private key.
 *        The privateData is expected in base64 encoded format.
 * @param ownersLen length of owners array
 * @param owners array of owners.
 *
 * @retval
 *      pointer to instance of OicSecCred_t  - success
 *      NULL                                 - error
 */
OicSecCred_t * GenerateCredential(const OicUuid_t * subject, OicSecCredType_t credType,
                                 const char * publicData, const char * privateData,
                                 size_t ownersLen, const OicUuid_t * owners)
{
    (void)publicData;
    OCStackResult ret = OC_STACK_ERROR;

    OicSecCred_t *cred = (OicSecCred_t*)OICCalloc(1, sizeof(OicSecCred_t));
    VERIFY_NON_NULL(TAG, cred, ERROR);

    //CredId is assigned before appending new cred to the existing
    //credential list and updating svr database in AddCredential().
    cred->credId = 0;

    VERIFY_NON_NULL(TAG, subject, ERROR);
    memcpy(cred->subject.id, subject->id , sizeof(cred->subject.id));

    VERIFY_SUCCESS(TAG, credType < (NO_SECURITY_MODE | SYMMETRIC_PAIR_WISE_KEY |
            SYMMETRIC_GROUP_KEY | ASYMMETRIC_KEY | SIGNED_ASYMMETRIC_KEY | PIN_PASSWORD), ERROR);
    cred->credType = credType;

#if 0
    if(publicData)
    {
        cred->publicData.data = (char *)OICMalloc(strlen(publicData)+1);
        VERIFY_NON_NULL(TAG, cred->publicData.data, ERROR);
        strncpy((char *)cred->publicData.data, publicData, strlen(publicData)+1);
    }
#endif

    if(privateData)
    {
        cred->privateData.data = (char *)OICMalloc(strlen(privateData)+1);
        VERIFY_NON_NULL(TAG, cred->privateData.data, ERROR);
        strncpy((char *)cred->privateData.data, privateData, strlen(privateData)+1);
    }

    VERIFY_SUCCESS(TAG, ownersLen > 0, ERROR);
    cred->ownersLen = ownersLen;

    cred->owners = (OicUuid_t*)OICCalloc(cred->ownersLen, sizeof(OicUuid_t));
    VERIFY_NON_NULL(TAG, cred->owners, ERROR);
    for(size_t i = 0; i < cred->ownersLen; i++)
    {
        memcpy(cred->owners[i].id, owners[i].id, sizeof(cred->owners[i].id));
    }

    ret = OC_STACK_OK;
exit:
    if (OC_STACK_OK != ret)
    {
        DeleteCredList(cred);
        cred = NULL;
    }
    return cred;
}

static bool UpdatePersistentStorage(const OicSecCred_t *cred)
{
    bool ret = false;

    // Convert Cred data into JSON for update to persistent storage
    char *jsonStr = BinToCredJSON(cred);
    if (jsonStr)
    {
        cJSON *jsonCred = cJSON_Parse(jsonStr);
        OICFree(jsonStr);

        if ((jsonCred) &&
          (OC_STACK_OK == UpdateSVRDatabase(OIC_JSON_CRED_NAME, jsonCred)))
        {
            ret = true;
        }
        cJSON_Delete(jsonCred );
    }
    else //Empty cred list
    {
        if (OC_STACK_OK == UpdateSVRDatabase(OIC_JSON_CRED_NAME, NULL))
        {
            ret = true;
        }
    }
    return ret;
}

/**
 * Compare function used LL_SORT for sorting credentials
 *
 * @param first   pointer to OicSecCred_t struct
 * @param second  pointer to OicSecCred_t struct
 *
 *@retval
 *  -1    if credId of first is less than credId of second
 *   0    if credId of first is equal to credId of second
 *   1    if credId of first is greater than credId of second
 */
static int CmpCredId(const OicSecCred_t * first, const OicSecCred_t *second)
{
    if(first->credId < second->credId)
    {
        return -1;
    }
    else if(first->credId > second->credId)
    {
        return 1;
    }
    else
        return 0;
}

/**
 * GetCredId goes through the cred list and returns the next
 * available credId. The next credId could be the credId that is
 * available due deletion of OicSecCred_t object or one more than
 * credId of last credential in the list.
 *
 * @retval
 *      next available credId  - success
 *      0                      - error
 */

static uint16_t GetCredId()
{
    //Sorts credential list in incremental order of credId
    LL_SORT(gCred, CmpCredId);


    OicSecCred_t *currentCred = NULL, *credTmp = NULL;
    uint16_t nextCredId = 1;

    LL_FOREACH_SAFE(gCred, currentCred, credTmp)
    {
        if(currentCred->credId == nextCredId)
        {
            nextCredId += 1;
        }
        else
        {
            break;
        }
    }

    VERIFY_SUCCESS(TAG, nextCredId < UINT16_MAX, ERROR);
    return nextCredId;

exit:
    return 0;
}


/**
 * This function adds the new cred to the credential list.
 *
 * @param cred pointer to new credential.
 *
 * @retval
 *      OC_STACK_OK     - cred not NULL and persistent storage gets updated
 *      OC_STACK_ERROR  - cred is NULL or fails to update persistent storage
 */
OCStackResult AddCredential(OicSecCred_t * newCred)
{
    OCStackResult ret = OC_STACK_ERROR;

    VERIFY_SUCCESS(TAG, NULL != newCred, ERROR);

    //Assigning credId to the newCred
    newCred->credId = GetCredId();

    VERIFY_SUCCESS(TAG, newCred->credId != 0, ERROR);

    //Append the new Cred to existing list
    LL_APPEND(gCred, newCred);

    if(UpdatePersistentStorage(gCred))
    {
        ret = OC_STACK_OK;
    }

exit:
    return ret;
}

OCStackResult RemoveCredential(const OicUuid_t *subject)
{
    OCStackResult ret = OC_STACK_ERROR;
    OicSecCred_t *cred = NULL;
    OicSecCred_t *tempCred = NULL;
    bool deleteFlag = false;

    LL_FOREACH_SAFE(gCred, cred, tempCred)
    {
        if(memcmp(cred->subject.id, subject->id, sizeof(subject->id)) == 0)
        {
            LL_DELETE(gCred, cred);
            FreeCred(cred);
            deleteFlag = 1;
        }
    }

    if(deleteFlag)
    {
        if(UpdatePersistentStorage(gCred))
        {
            ret = OC_STACK_RESOURCE_DELETED;
        }
    }
    return ret;

}

static OCEntityHandlerResult HandlePostRequest(const OCEntityHandlerRequest * ehRequest)
{
    OCEntityHandlerResult ret = OC_EH_ERROR;

    //Get binary representation of json
    OicSecCred_t * cred  = JSONToCredBin(((OCSecurityPayload*)ehRequest->payload)->securityData);

    if(cred)
    {
        //If the Post request credential has credId, it will be
        //discarded and the next available credId will be assigned
        //to it before getting appended to the existing credential
        //list and updating svr database.
        ret = (OC_STACK_OK == AddCredential(cred))? OC_EH_RESOURCE_CREATED : OC_EH_ERROR;
    }

    return ret;
}

static OCEntityHandlerResult HandleDeleteRequest(const OCEntityHandlerRequest *ehRequest)
{
    OC_LOG(INFO, TAG, "Processing CredDeleteRequest");

    OCEntityHandlerResult ehRet = OC_EH_ERROR;

    if(NULL == ehRequest->query)
   {
       return ehRet;
   }

   OicParseQueryIter_t parseIter = {.attrPos=NULL};
   OicUuid_t subject = {.id={0}};

   //Parsing REST query to get the subject
   ParseQueryIterInit((unsigned char *)ehRequest->query, &parseIter);
   while(GetNextQuery(&parseIter))
   {
       if(strncasecmp((char *)parseIter.attrPos, OIC_JSON_SUBJECT_NAME,
               parseIter.attrLen) == 0)
       {
           unsigned char base64Buff[sizeof(((OicUuid_t*)0)->id)] = {};
           uint32_t outLen = 0;
           B64Result b64Ret = B64_OK;

           b64Ret = b64Decode((char *)parseIter.valPos, parseIter.valLen,
                   base64Buff, sizeof(base64Buff), &outLen);

           VERIFY_SUCCESS(TAG, (b64Ret == B64_OK && outLen <= sizeof(subject.id)), ERROR);
           memcpy(subject.id, base64Buff, outLen);
       }
   }

   if(OC_STACK_RESOURCE_DELETED == RemoveCredential(&subject))
   {
       ehRet = OC_EH_RESOURCE_DELETED;
   }

exit:
    return ehRet;
}


/*
 * This internal method is the entity handler for Cred resources
 * to handle REST request (PUT/POST/DEL)
 */
OCEntityHandlerResult CredEntityHandler (OCEntityHandlerFlag flag,
                                        OCEntityHandlerRequest * ehRequest,
                                        void* callbackParameter)
{
    (void)callbackParameter;
    OCEntityHandlerResult ret = OC_EH_ERROR;

    if(!ehRequest)
    {
        return OC_EH_ERROR;
    }
    if (flag & OC_REQUEST_FLAG)
    {
        OC_LOG (INFO, TAG, "Flag includes OC_REQUEST_FLAG");
        //TODO :  Handle PUT/DEL methods
        switch(ehRequest->method)
        {
            case OC_REST_GET:
                ret = OC_EH_FORBIDDEN;
                break;
            case OC_REST_POST:
                ret = HandlePostRequest(ehRequest);
                break;
            case OC_REST_DELETE:
                ret = HandleDeleteRequest(ehRequest);
                break;
            default:
                ret = OC_EH_ERROR;
                break;
        }
    }

    //Send payload to request originator
    ret = (SendSRMResponse(ehRequest, ret, NULL) == OC_STACK_OK ?
                       ret : OC_EH_ERROR);

    return ret;
}

/*
 * This internal method is used to create '/oic/sec/Cred' resource.
 */
OCStackResult CreateCredResource()
{
    OCStackResult ret;

    ret = OCCreateResource(&gCredHandle,
                           OIC_RSRC_TYPE_SEC_CRED,
                           OIC_MI_DEF,
                           OIC_RSRC_CRED_URI,
                           CredEntityHandler,
                           NULL,
                           OC_RES_PROP_NONE);

    if (OC_STACK_OK != ret)
    {
        OC_LOG (FATAL, TAG, "Unable to instantiate Cred resource");
        DeInitCredResource();
    }
    return ret;
}

/**
 * Get the default value
 * @retval  NULL for now. Update it when we finalize the default info.
 */
static OicSecCred_t* GetCredDefault()
{
    return NULL;
}

/**
 * Initialize Cred resource by loading data from persistent storage.
 *
 * @retval
 *     OC_STACK_OK    - no errors
 *     OC_STACK_ERROR - stack process error
 */
OCStackResult InitCredResource()
{
    OCStackResult ret = OC_STACK_ERROR;

    //Read Cred resource from PS
    char* jsonSVRDatabase = GetSVRDatabase();

    if (jsonSVRDatabase)
    {
        //Convert JSON Cred into binary format
        gCred = JSONToCredBin(jsonSVRDatabase);
    }
    /*
     * If SVR database in persistent storage got corrupted or
     * is not available for some reason, a default Cred is created
     * which allows user to initiate Cred provisioning again.
     */
    if (!jsonSVRDatabase || !gCred)
    {
        gCred = GetCredDefault();
    }
    //Instantiate 'oic.sec.cred'
    ret = CreateCredResource();
    OICFree(jsonSVRDatabase);
    return ret;
}

/**
 * Perform cleanup for Cred resources.
 *
 * @return
 *     OC_STACK_OK              - no errors
 *     OC_STACK_ERROR           - stack process error
 *     OC_STACK_NO_RESOURCE     - resource not found
 *     OC_STACK_INVALID_PARAM   - invalid param
 */
OCStackResult DeInitCredResource()
{
    OCStackResult result = OCDeleteResource(gCredHandle);
    DeleteCredList(gCred);
    gCred = NULL;
    return result;
}

/**
 * This method is used by tinydtls/SRM to retrieve credential for given Subject.
 *
 * @param subject - subject for which credential is required.
 *
 * @retval
 *     reference to OicSecCred_t - if credential is found
 *     NULL                      - if credential not found
 */
const OicSecCred_t* GetCredResourceData(const OicUuid_t* subject)
{
    OicSecCred_t *cred = NULL;

   if ( NULL == subject)
    {
       return NULL;
    }

    LL_FOREACH(gCred, cred)
    {
        if(memcmp(cred->subject.id, subject->id, sizeof(subject->id)) == 0)
        {
            return cred;
        }
    }
    return NULL;
}


#if defined(__WITH_DTLS__)
/**
 * This internal callback is used by lower stack (i.e. CA layer) to
 * retrieve PSK credentials from RI security layer.
 *
 * Note: When finished, caller should initialize memory to zeros and
 * invoke OICFree to delete @p credInfo.
 *
 * @param credInfo
 *     binary blob containing PSK credentials
 *
 * @retval none
 */
void GetDtlsPskCredentials(CADtlsPskCredsBlob_t **credInfo)
{
    CADtlsPskCredsBlob_t * caBlob = NULL;
    if(credInfo)
    {
        caBlob = (CADtlsPskCredsBlob_t *)OICCalloc(sizeof(CADtlsPskCredsBlob_t), 1);
        if (caBlob)
        {
            OicUuid_t deviceID = {.id={}};

            // Retrieve Device ID from doxm resource and copy in PSK creds blob
            VERIFY_SUCCESS(TAG, GetDoxmDeviceID(&deviceID) == OC_STACK_OK, ERROR);
            memcpy(caBlob->identity, deviceID.id, sizeof(caBlob->identity));

            OicSecCred_t *cred = NULL;
            size_t count = 0;
            LL_FOREACH(gCred, cred)
            {
                // Currently, Iotivity supports only symmetric pair wise key credentials
                if (cred->credType == SYMMETRIC_PAIR_WISE_KEY)
                {
                    ++count;
                }
            }
            caBlob->num = count;
            if (caBlob->num)
            {
                caBlob->creds =
                    (OCDtlsPskCreds*) OICMalloc(caBlob->num * sizeof(OCDtlsPskCreds));
                VERIFY_NON_NULL(TAG, caBlob->creds, ERROR);

                unsigned int i = 0;
                LL_FOREACH(gCred, cred)
                {
                    if ((cred->credType == SYMMETRIC_PAIR_WISE_KEY) &&
                            (i < count))

                    {
                        // Copy subject ID
                        memcpy(caBlob->creds[i].id, cred->subject.id,
                                sizeof(caBlob->creds[i].id));

                        // Convert PSK from JSON to binary before copying
                        uint32_t outLen = 0;
                        B64Result b64Ret = b64Decode(cred->privateData.data,
                                strlen(cred->privateData.data), caBlob->creds[i].psk,
                                sizeof(caBlob->creds[i].psk), &outLen);
                        VERIFY_SUCCESS(TAG, b64Ret == B64_OK, ERROR);
                        i++;
                    }
                }
            }
        }
        *credInfo = caBlob;
        // Return from here after making the credential list
        return;
    }

exit:
    if (caBlob)
    {
        memset(caBlob->creds, 0, caBlob->num * sizeof(OCDtlsPskCreds));
        OICFree(caBlob->creds);
    }
    OICFree(caBlob);
}

/**
 * Add temporal PSK to PIN based OxM
 *
 * @param[in] tmpSubject UUID of target device
 * @param[in] credType Type of credential to be added
 * @param[in] pin numeric characters
 * @param[in] pinSize length of 'pin'
 * @param[in] ownersLen Number of owners
 * @param[in] owners Array of owners
 * @param[out] tmpCredSubject Generated credential's subject.
 *
 * @return OC_STACK_OK for success and errorcode otherwise.
 */
OCStackResult AddTmpPskWithPIN(const OicUuid_t* tmpSubject, OicSecCredType_t credType,
                            const char * pin, size_t pinSize,
                            size_t ownersLen, const OicUuid_t * owners, OicUuid_t* tmpCredSubject)
{
    OCStackResult ret = OC_STACK_ERROR;

    if(tmpSubject == NULL || pin == NULL || pinSize == 0 || tmpCredSubject == NULL)
    {
        return OC_STACK_INVALID_PARAM;
    }

    uint8_t privData[OWNER_PSK_LENGTH_128] = {0,};
    int dtlsRes = DeriveCryptoKeyFromPassword((const unsigned char *)pin, pinSize, owners->id,
                                              UUID_LENGTH, PBKDF_ITERATIONS,
                                              OWNER_PSK_LENGTH_128, privData);
    VERIFY_SUCCESS(TAG, (dtlsRes == 0) , ERROR);

    uint32_t outLen = 0;
    char base64Buff[B64ENCODE_OUT_SAFESIZE(OWNER_PSK_LENGTH_128) + 1] = {};
    B64Result b64Ret = b64Encode(privData, OWNER_PSK_LENGTH_128, base64Buff,
                                sizeof(base64Buff), &outLen);
    VERIFY_SUCCESS(TAG, (B64_OK == b64Ret), ERROR);

    OicSecCred_t* cred = GenerateCredential(tmpSubject, credType, NULL,
                                            base64Buff, ownersLen, owners);
    if(NULL == cred)
    {
        OC_LOG(ERROR, TAG, "GeneratePskWithPIN() : Failed to generate credential");
        return OC_STACK_ERROR;
    }

    memcpy(tmpCredSubject->id, cred->subject.id, UUID_LENGTH);

    ret = AddCredential(cred);
    if( OC_STACK_OK != ret)
    {
        OC_LOG(ERROR, TAG, "GeneratePskWithPIN() : Failed to add credential");
    }

exit:
    return ret;
}

#endif /* __WITH_DTLS__ */
