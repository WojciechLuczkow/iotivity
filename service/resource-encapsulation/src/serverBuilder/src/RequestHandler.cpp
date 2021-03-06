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

#include <RequestHandler.h>

#include <OCResourceResponse.h>
#include <ResourceAttributesConverter.h>
#include <RCSResourceObject.h>

namespace
{
    using namespace OIC::Service;

    typedef std::function< OC::OCRepresentation(RCSResourceObject&) > OCRepresentationGetter;

    OC::OCRepresentation getOCRepresentationFromResource(RCSResourceObject& resource)
    {
        RCSResourceObject::LockGuard lock{ resource, RCSResourceObject::AutoNotifyPolicy::NEVER };
        return ResourceAttributesConverter::toOCRepresentation(resource.getAttributes());
    }

    OC::OCRepresentation getOCRepresentation(const RCSResourceAttributes& attrs)
    {
        return ResourceAttributesConverter::toOCRepresentation(attrs);
    }

    template< typename T >
    OCRepresentationGetter wrapGetOCRepresentation(T&& attrs)
    {
        return std::bind(getOCRepresentation, std::forward<T>(attrs));
    }

    std::shared_ptr< OC::OCResourceResponse > doBuildResponse(RCSResourceObject& resource,
            const OCEntityHandlerResult result, int errorCode, OCRepresentationGetter ocRepGetter)
    {
        auto response = std::make_shared< OC::OCResourceResponse >();

        response->setResponseResult(result);
        response->setErrorCode(errorCode);
        response->setResourceRepresentation(ocRepGetter(resource));

        return response;
    }

    AttrKeyValuePairs applyAcceptMethod(RCSResourceObject& resource,
            const RCSResourceAttributes& requestAttrs)
    {
        RCSResourceObject::LockGuard lock(resource, RCSResourceObject::AutoNotifyPolicy::NEVER);

        return replaceAttributes(resource.getAttributes(), requestAttrs);
    }

    AttrKeyValuePairs applyDefaultMethod(RCSResourceObject& resource,
            const RCSResourceAttributes& requestAttrs)
    {
        RCSResourceObject::LockGuard lock(resource, RCSResourceObject::AutoNotifyPolicy::NEVER);

        if (resource.getSetRequestHandlerPolicy()
            != RCSResourceObject::SetRequestHandlerPolicy::ACCEPTANCE
            && !acceptableAttributes(resource.getAttributes(), requestAttrs))
        {
            return AttrKeyValuePairs{ };
        }

        return replaceAttributes(resource.getAttributes(), requestAttrs);
    }

    AttrKeyValuePairs applyIgnoreMethod(RCSResourceObject&, const RCSResourceAttributes&)
    {
        return AttrKeyValuePairs();
    }

    auto getApplyAcceptanceFunc(RCSSetResponse::AcceptanceMethod method) ->
            std::function<AttrKeyValuePairs(RCSResourceObject&, const RCSResourceAttributes&)>
    {
        switch (method)
        {
            case RCSSetResponse::AcceptanceMethod::DEFAULT:
                return applyDefaultMethod;

            case RCSSetResponse::AcceptanceMethod::ACCEPT:
                return applyAcceptMethod;

            case RCSSetResponse::AcceptanceMethod::IGNORE:
                return applyIgnoreMethod;
        }

        return applyIgnoreMethod;
    }

} // unnamed namespace

namespace OIC
{
    namespace Service
    {
        constexpr int RequestHandler::DEFAULT_ERROR_CODE;
        constexpr OCEntityHandlerResult RequestHandler::DEFAULT_RESULT;

        RequestHandler::RequestHandler() :
                m_holder{ std::bind(doBuildResponse, std::placeholders::_1, DEFAULT_RESULT,
                        DEFAULT_ERROR_CODE, getOCRepresentationFromResource) }
        {
        }

        RequestHandler::RequestHandler(const OCEntityHandlerResult& result, int errorCode) :
                m_holder{ std::bind(doBuildResponse, std::placeholders::_1, result, errorCode,
                        getOCRepresentationFromResource) }
        {
        }

        RequestHandler::RequestHandler(const RCSResourceAttributes& attrs,
                const OCEntityHandlerResult& result, int errorCode) :
                m_holder{ std::bind(doBuildResponse, std::placeholders::_1, result, errorCode,
                        wrapGetOCRepresentation(attrs)) }
        {
        }

        RequestHandler::RequestHandler(RCSResourceAttributes&& attrs,
                const OCEntityHandlerResult& result, int errorCode) :
                m_holder{ std::bind(doBuildResponse, std::placeholders::_1, result, errorCode,
                        wrapGetOCRepresentation(std::move(attrs))) }
        {
        }

        std::shared_ptr< OC::OCResourceResponse > RequestHandler::buildResponse(
                RCSResourceObject& resource)
        {
            return m_holder(resource);
        }


        SetRequestHandler::SetRequestHandler() :
                RequestHandler{ }
        {
        }

        SetRequestHandler::SetRequestHandler(const OCEntityHandlerResult& result, int errorCode) :
                RequestHandler{ result, errorCode }
        {
        }


        SetRequestHandler::SetRequestHandler(const RCSResourceAttributes& attrs,
                const OCEntityHandlerResult& result, int errorCode) :
                RequestHandler{ attrs, result, errorCode }
        {
        }

        SetRequestHandler::SetRequestHandler(RCSResourceAttributes&& attrs,
                const OCEntityHandlerResult& result, int errorCode) :
                RequestHandler{ std::move(attrs), result, errorCode }
        {
        }

        AttrKeyValuePairs SetRequestHandler::applyAcceptanceMethod(
                RCSSetResponse::AcceptanceMethod method, RCSResourceObject& resource,
                const RCSResourceAttributes& requestAttrs) const
        {
            return getApplyAcceptanceFunc(method)(resource, requestAttrs);
        }

    }
}
