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

/**
 * @file
 *
 * This file contains the resource object APIs provided to the developers.
 * RCSResourceObject is a part of the server builder module.
 */
#ifndef SERVER_RCSRESOURCEOBJECT_H
#define SERVER_RCSRESOURCEOBJECT_H

#include <string>
#include <mutex>
#include <thread>

#include <RCSResourceAttributes.h>
#include <RCSResponse.h>
#include <RCSRequest.h>

namespace OC
{
    class OCResourceRequest;
}

namespace OIC
{
    namespace Service
    {

        /**
         * @brief Thrown when lock has not been acquired.
         *
         * @see RCSResourceObject::LockGuard
         * @see RCSResourceObject::getAttributes
         */
        class NoLockException: public RCSException
        {
            public:
                NoLockException(std::string &&what) : RCSException { std::move(what) } {}
        };

        //! @cond
        template < typename T >
        class AtomicWrapper;
        //! @endcond

        /**
         * @brief  RCSResourceObject represents a resource. It handles any requests from
         *        clients automatically with attributes.
         *        It also provides an auto notification mechanism that notifies to the observers.
         *        <br/>
         *         Requests are handled automatically by defaultAction of RCSGetResponse and
         *        RCSSetResponse. You can override them and send your own response.
         *        <br/>
         *         For simple resources, you may want to know whenever attributes are changed
         *        by a set request. In this case, add an AttributeUpdatedListener
         *        with a key interested in instead of overriding SetRequestHandler.
         */
        class RCSResourceObject
        {
            private:
                class WeakGuard;

                typedef AtomicWrapper< std::thread::id > AtomicThreadId;

            public:
                /**
                 * @brief represents the policy of AutoNotify function.
                 *        In accord with this policy, observers are notified of attributes that
                 *        are changed or updated.
                 * @note Attributes are changed or updated according to execution of some functions
                 *       or receipt of 'set-request'.
                 *       (functions - RCSResourceObject::setAttribute,
                 *       RCSResourceObject::removeAttribute, RCSResourceObject::getAttributes)
                 */
                enum class AutoNotifyPolicy
                {
                    NEVER,  /**< Never notify.*/
                    ALWAYS, /**< Always notify.*/
                    UPDATED /**< When attributes are changed, notify.*/
                };

                /**
                 * @brief represents the policy of Set-Request Handler.
                 *        In accord with this policy, attributes of 'set-request' are created or
                 *        ignored.
                 */
                enum class SetRequestHandlerPolicy
                {
                    NEVER,     /**< Server ignore when server is received set-request of attributes
                                    of the new key. */
                    ACCEPTANCE /**< Server creates attributes of the new key When server is received
                                    set-request of attributes of the new key. */
                };

                typedef std::shared_ptr< RCSResourceObject > Ptr;
                typedef std::shared_ptr< const RCSResourceObject > ConstPtr;

                /**
                 * @class   Builder
                 * @brief   This class provides APIs for resource creation, setting properties &
                 *          attributes for the constructed resource.
                 *          It provides the build() API
                 *          which builds a resource and return pointer to RCSResourceObject class.
                 *
                 * @see build()
                 */
                class Builder
                {
                    public:
                        /**
                         * @brief Constructor.
                         *           Sets the resource property values using initializers list.
                         *
                         * @param uri Resource URI value to be set
                         * @param type Resource type value to be set
                         * @param interface Interface value to be set
                         *
                         *NOTE : m_properties value is by default set to
                         *       OC_DISCOVERABLE | OC_OBSERVABLE.
                         *       OC_DISCOVERABLE and OC_OBSERVABLE are defined in octypes.h.
                         */
                        Builder(const std::string &uri, const std::string &type,
                                const std::string &interface);

                        /**
                         * Sets the discoverable(OC_DISCOVERABLE) property for the resource.
                         *
                         * @param discoverable Whether to be discovered.
                         *
                         * @return reference of this Builder
                         *
                         * @see OC_DISCOVERABLE
                         */
                        Builder &setDiscoverable(bool discoverable);

                        /**
                         * Sets the observable(OC_OBSERVABLE) property of the resource.
                         *
                         * @param observable Whether to be observed.
                         *
                         * @return reference of this Builder
                         *
                         * @see  OC_OBSERVABLE
                         */
                        Builder &setObservable(bool observable);

                        /**
                         * Sets attribute of the resource.
                         *
                         * @param attributes Resource attributes to set
                         *
                         * @return reference of this Builder
                         */
                        Builder &setAttributes(const RCSResourceAttributes &attributes);

                        /**
                         * API for setting attributes of the resource.
                         *
                         * @param attributes Resource Attributes to set
                         *
                         * @return reference of this Builder
                         */
                        Builder &setAttributes(RCSResourceAttributes &&attributes);

                        /**
                         * API for constructing a new RCSResourceObject.
                         *
                         * @return Pointer to RCSResourceObject instance created.
                         *
                         * @throw PlatformException
                         *       It catches exception from registerResource API of OCPlatform and
                         *       throws it to developer.
                         *
                         */
                        RCSResourceObject::Ptr build();

                    private:
                        std::string m_uri;
                        std::string m_type;
                        std::string m_interface;
                        uint8_t m_properties;
                        RCSResourceAttributes m_resourceAttributes;
                };

                class LockGuard;

                typedef std::function < RCSGetResponse(const RCSRequest&,
                                                       RCSResourceAttributes&) > GetRequestHandler;
                typedef std::function < RCSSetResponse(const RCSRequest&,
                                                       RCSResourceAttributes&) > SetRequestHandler;

                /**
                 * typedef for characterizing AttributeUpdatedListener with 2 parameters.
                 *
                 * The first Value represents the old Value before being changed.
                 * The second Value means the new Value right after when it is used
                 *
                 * @see addAttributeUpdatedListener
                 */
                typedef std::function < void(const RCSResourceAttributes::Value&,
                                     const RCSResourceAttributes::Value &) > AttributeUpdatedListener;

            public:
                RCSResourceObject(RCSResourceObject&&) = delete;
                RCSResourceObject(const RCSResourceObject&) = delete;

                RCSResourceObject& operator=(RCSResourceObject&&) = delete;
                RCSResourceObject& operator=(const RCSResourceObject&) = delete;

                virtual ~RCSResourceObject();

                /**
                 * API for setting a particular attribute value.
                 *
                 * @param key name of attribute(used to map the attribute value).
                 * @param value attribute value to be mapped against the key.
                 *
                 * @note It is guaranteed thread-safety about attributes.
                 */
                void setAttribute(const std::string& key, const RCSResourceAttributes::Value& value);

                /**
                 * @overload
                 */
                void setAttribute(const std::string& key, RCSResourceAttributes::Value&& value);

                /**
                 * @overload
                 */
                void setAttribute(std::string&& key, const RCSResourceAttributes::Value& value);

                /**
                 * @overload
                 */
                void setAttribute(std::string&& key, RCSResourceAttributes::Value&& value);

                /**
                 * API for getting attribute value corresponding to a key(name of that attribute).
                 *
                 * @param key name of the attribute value to look for.
                 *
                 * @return value of the resource attribute.
                 *
                 * @note It is guaranteed thread-safety about attributes.
                 *
                 * @throw InvalidKeyException
                 *              Throw an exception when key is empty.
                 */
                RCSResourceAttributes::Value getAttributeValue(const std::string& key) const;

                /**
                 * API for retrieving the attribute value associated with the supplied name.
                 *
                 * @param key Name of the attribute
                 *
                 * @return resource attributes value which can support various types.
                 *
                 * @see ValueVariant
                 *
                 * It is guaranteed thread-safety about attributes.
                 */
                template< typename T >
                T getAttribute(const std::string& key) const
                {
                    WeakGuard lock(*this);
                    return m_resourceAttributes.at(key).get< T >();
                }

                /**
                 * API for removing a particular attribute of the resource.
                 *
                 * @param key Name of the attribute.
                 *
                 * @return If the key exists and matched attribute is deleted, return true.
                 *
                 * It is guaranteed thread-safety about attributes.
                 */
                bool removeAttribute(const std::string& key);

                /**
                 * API for checking whether a particular attribute is there for a resource or not.
                 *
                 * @param key Name of the attribute.
                 *
                 * @return If the key exists, return true.
                 *
                 * It is guaranteed thread-safety about attributes.
                 */
                bool containsAttribute(const std::string& key) const;

                /**
                 * API for getting all the attributes of the RCSResourceObject.
                 * It invokes the expectOwnLock() API to check the owner of the lock using the
                 * thread id.
                 * If it is not the owner then it throws an exception.
                 *
                 * @return reference of the attributes of this RCSResourceObject.
                 *
                 * @see expectOwnLock()
                 *
                 * @throw NoLockException
                 *              If you don't do lock with LockGuard, throw exception.
                 */
                RCSResourceAttributes& getAttributes();

                /**
                 * @overload
                 */
                const RCSResourceAttributes& getAttributes() const;

                /**
                 * API for checking whether the particular resource is observable or not
                 */
                virtual bool isObservable() const;

                /**
                 * API for checking whether the particular resource is discoverable or not
                 */
                virtual bool isDiscoverable() const;

                /**
                 * API for setting the resource's get request handler by the developer/application.
                 * If developer set this handler then all get request will come to the application &
                 * developer can send the response to the client using APIs of RCSGetResponse class.
                 *
                 * @param handler Request handler for get requests
                 *
                 * @see RCSGetResponse
                 *
                 */
                virtual void setGetRequestHandler(GetRequestHandler handler);

                /**
                 * API for setting the resource's set request handler by the developer/application.
                 * If developer set this handler then all set request for the resource
                 * will come to the application & developer can send the response to the client
                 * using APIs of RCSSetResponse class.
                 *
                 * @param handler Request handler for set requests
                 *
                 * @see RCSSetResponse
                 *
                 */
                virtual void setSetRequestHandler(SetRequestHandler handler);

                /**
                 * API for setting the Listener for a particular attribute update.
                 *
                 * @param key The interested attribute's key
                 * @param listener Listener for updation of the interested attribute
                 *
                 * @see AttributeUpdatedListener
                 */
                virtual void addAttributeUpdatedListener(const std::string& key,
                        AttributeUpdatedListener listener);

                /**
                 * API for setting the Listener for a particular attribute update.
                 *
                 * @param key The interested attribute's key
                 * @param listener Listener for updation of the interested attribute
                 *
                 * @see AttributeUpdatedListener
                 */
                virtual void addAttributeUpdatedListener(std::string&& key,
                        AttributeUpdatedListener listener);

                /**
                 * API for removing the Listener for a particular attribute update.
                 *
                 * @param key The name of interested attribute's key  
                 *
                 */
                virtual bool removeAttributeUpdatedListener(const std::string& key);

                /**
                 * API for notifying all observers of the RCSResourceObject
                 * with the updated attributes value
                 */
                virtual void notify() const;

                /**
                 * API for setting Auto notify policy
                 *
                 * @param policy policy to be set
                 *
                 * @see AutoNotifyPolicy
                 *
                 */
                void setAutoNotifyPolicy(AutoNotifyPolicy policy);

                /**
                 * API for getting auto notify policy
                 *
                 * @returns AntoNotify policy
                 *
                 * @see AutoNotifyPolicy
                 *
                 */
                AutoNotifyPolicy getAutoNotifyPolicy() const;

                /**
                 * API for setting the policy for a setRequestHandler.
                 *
                 * @param policy policy to be set
                 *
                 * @see SetRequestHandlerPolicy
                 *
                 */
                void setSetRequestHandlerPolicy(SetRequestHandlerPolicy policy);

                /**
                 * API for getting the SetRequestHandler Policy.
                 *
                 * @returns Property of setRequesthandler
                 *
                 * @see SetRequestHandlerPolicy
                 *
                 */
                SetRequestHandlerPolicy getSetRequestHandlerPolicy() const;

        private:
            RCSResourceObject(uint8_t, RCSResourceAttributes&&);

            OCEntityHandlerResult entityHandler(std::shared_ptr< OC::OCResourceRequest >);

            OCEntityHandlerResult handleRequest(std::shared_ptr< OC::OCResourceRequest >);
            OCEntityHandlerResult handleRequestGet(std::shared_ptr< OC::OCResourceRequest >);
            OCEntityHandlerResult handleRequestSet(std::shared_ptr< OC::OCResourceRequest >);
            OCEntityHandlerResult handleObserve(std::shared_ptr< OC::OCResourceRequest >);

            void expectOwnLock() const;

            std::thread::id getLockOwner() const noexcept;

            void setLockOwner(std::thread::id&&) const noexcept;

            void autoNotify(bool, AutoNotifyPolicy) const;
            void autoNotify(bool) const;

            bool testValueUpdated(const std::string&, const RCSResourceAttributes::Value&) const;

            template< typename K, typename V >
            void setAttributeInternal(K&&, V&&);

        private:
            const uint8_t m_properties;

            OCResourceHandle m_resourceHandle;
            RCSResourceAttributes m_resourceAttributes;

            GetRequestHandler m_getRequestHandler;
            SetRequestHandler m_setRequestHandler;
            AutoNotifyPolicy m_autoNotifyPolicy;
            SetRequestHandlerPolicy m_setRequestHandlerPolicy;

            std::unordered_map< std::string, AttributeUpdatedListener >
                    m_keyAttributesUpdatedListeners;

            mutable std::unique_ptr< AtomicThreadId > m_lockOwner;
            mutable std::mutex m_mutex;

            std::mutex m_mutexKeyAttributeUpdate;

        };

        /**
         * @class   LockGuard
         *
         * The class LockGuard owns a mutex for the duration of a scoped block.
         * When a LockGuard is created, it attempts to take ownership of the mutex it is given.
         * When control leaves the scope in which the LockGuard object was created,
         * the LockGuard is destructed and the mutex is released.
         *
         */
        class RCSResourceObject::LockGuard
        {
        public:
           /**
            * The function locks the objects and ensures that all arguments are locked on return.
            * Working of AutoNotifyPolicy follows the current AutoNotifyPolicy status
            *
            * @param Object to be locked
            *
            */
            LockGuard(const RCSResourceObject&);

           /**
            * @overload
            */
            LockGuard(const RCSResourceObject::Ptr);

           /**
            * The function locks the objects and ensures that all arguments are locked on return.
            *
            * @param Object to be locked
            * @param AutoNotifyPolicy is set for sepcifying AutoNotifyPolicy status
            *
            * @see AutoNotifyPolicy
            */
            LockGuard(const RCSResourceObject&, AutoNotifyPolicy);

           /**
            * @overload
            */
            LockGuard(const RCSResourceObject::Ptr, AutoNotifyPolicy);
            ~LockGuard();

            LockGuard(const LockGuard&) = delete;
            LockGuard(LockGuard&&) = delete;

            LockGuard& operator=(const LockGuard&) = delete;
            LockGuard& operator=(LockGuard&&) = delete;

        private:
            void init();

        private:
            const RCSResourceObject& m_resourceObject;

            AutoNotifyPolicy m_autoNotifyPolicy;

            bool m_isOwningLock;

            std::function<void()> m_autoNotifyFunc;
        };

        class RCSResourceObject::WeakGuard
        {
        public:
            WeakGuard(const RCSResourceObject&);
            ~WeakGuard();

            WeakGuard(const WeakGuard&) = delete;
            WeakGuard(WeakGuard&&) = delete;

            WeakGuard& operator=(const WeakGuard&) = delete;
            WeakGuard& operator=(WeakGuard&&) = delete;

            bool hasLocked() const;

        private:
            bool m_isOwningLock;
            const RCSResourceObject& m_resourceObject;
        };
    }
}

#endif // SERVER_RCSRESOURCEOBJECT_H
