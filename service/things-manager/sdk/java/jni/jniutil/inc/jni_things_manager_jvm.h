/******************************************************************
 *
 * Copyright 2015 Samsung Electronics All Rights Reserved.
 *
 *
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************/

/**
  * @file   jni_things_manager_jvm.h
  *
  * @brief  This file contains the essential declarations and functions required
  *            for JNI implementation
  */

#ifndef __JNI_THINGS_MANAGER_JVM_H_
#define __JNI_THINGS_MANAGER_JVM_H_


#include <jni.h>
#include <thread>
#include <mutex>

#define TM_SERVICE_NATIVE_API_CLASS_PATH    "org/iotivity/service/tm/ThingsManagerNativeInterface"
#define TM_SERVICE_NATIVE_API_CLASS_TYPE    "Lorg/iotivity/service/tm/ThingsManagerNativeInterface;"

#define TM_SERVICE_CALLBACK_CLASS_PATH    "org/iotivity/service/tm/ThingsManagerCallback"
#define TM_SERVICE_CALLBACK_CLASS_TYPE    "Lorg/iotivity/service/tm/ThingsManagerCallback;"

#define TM_SERVICE_PLATFORM_CLASS_PATH    "org/iotivity/base/OcPlatform"
#define TM_SERVICE_PLATFORM_CLASS_TYPE    "Lorg/iotivity/base/OcPlatform;"

#define TM_SERVICE_PLATFORM_CONFIG_CLASS_PATH    "org/iotivity/base/PlatformConfig"
#define TM_SERVICE_PLATFORM_CONFIG_CLASS_TYPE    "Lorg/iotivity/base/PlatformConfig;"

#define TM_SERVICE_CAPABILITY_PATH    "org/iotivity/service/tm/Capability"
#define TM_SERVICE_CAPABILITY_TYPE    "Lorg/iotivity/service/tm/Capability;"

#define TM_SERVICE_ACTION_PATH    "org/iotivity/service/tm/Action"
#define TM_SERVICE_ACTION_TYPE    "Lorg/iotivity/service/tm/Action;"

#define TM_SERVICE_ACTIONSET_PATH    "org/iotivity/service/tm/ActionSet"
#define TM_SERVICE_ACTIONSET_TYPE    "Lorg/iotivity/service/tm/ActionSet;"

#define TM_SERVICE_OCRESOURCE_PATH    "org/iotivity/base/OcResource"
#define TM_SERVICE_OCRESOURCE_TYPE    "Lorg/iotivity/base/OcResource;"

#define TM_SERVICE_HEADER_OPTION_PATH    "org/iotivity/base/OcHeaderOption"
#define TM_SERVICE_HEADER_OPTION_TYPE    "Lorg/iotivity/base/OcHeaderOption;"

#define TM_SERVICE_OCREPRESENTATION_PATH    "org/iotivity/base/OcRepresentation"
#define TM_SERVICE_OCREPRESENTATION_TYPE    "Lorg/iotivity/base/OcRepresentation;"

#define TM_SERVICE_OCRESOURCEHANDLE_PATH    "org/iotivity/base/OcResourceHandle"
#define TM_SERVICE_OCRESOURCEHANDLE_TYPE    "Lorg/iotivity/base/OcResourceHandle;"

#define TM_SERVICE_TIME_PATH    "org/iotivity/service/tm/Time"

#define TM_JAVA_VECTOR_CLASS_PATH "java/util/Vector"
#define TM_JAVA_VECTOR_TYPE "Ljava/util/Vector;"

#define TM_JAVA_STRING_TYPE "Ljava/lang/String;"

/**
 * @class   ThingsManagerJVM
 * @brief   This class provides functions related to JNI Environment.
 *
 */
class ThingsManagerJVM
{
    public:
        /**
         * @brief destructor
         */
        ~ThingsManagerJVM() {};

        /**
         * @brief  Get JVM instance
         */
        static JNIEnv *getEnv();

        /**
         * @brief  Release aquired JVM instance
         */
        static void releaseEnv();

    public:
        /**
         *  Java VM pointer
         */
        static JavaVM *m_jvm;

    private:
        /**
         * @brief constructor
         */
        ThingsManagerJVM();

        /**
         *  Mutex for thread synchronization
         */
        static std::mutex m_currentThreadMutex;
};

#ifdef __cplusplus
extern "C" {
#endif
jclass GetJClass(const char *szClassPath);
jobject GetJObjectInstance(const char *szClassPath);
#ifdef __cplusplus
}
#endif
#endif //__JNI_THINGS_MANAGER_JVM_H_