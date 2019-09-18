/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#ifndef _APL_CONTENT_H
#define _APL_CONTENT_H

#include <set>
#include <map>

#include "apl/common.h"
#include "apl/content/package.h"
#include "apl/engine/properties.h"
#include "apl/utils/session.h"

namespace apl {

class JsonData;
class ImportRequest;
class ImportRef;
class RootConfig;
class Metrics;

/**
 * Hold all of the documents and data necessary to inflate an APL component hierarchy.
 * The an approximate use of Content is (without error-checking):
 *
 *      // Initial creation of Content from an APL document
 *      auto content = Content::create( document );
 *      if (!content)
 *         return;  // Failed to create the document
 *      if (checkRequests(content))
 *         return READY_TO_GO;
 *
 *      // When a package comes in, call:
 *      content->addPackage(request, data);
 *      if (checkRequests(content))
 *          return READY_TO_GO
 *
 *      // Helper method to check for new packages that are needed
 *      bool checkRequests(ContentPtr& content) {
 *        for (ImportRequest request : content->getRequestedPackages())
 *          // Request package "request"
 *
 *        return content->isReady();
 *      }
 *
 * The other aspect of content is connecting the named APL document parameters
 * with actual data sets.  Use the addData() method to wire up parameter names
 * with JSON data.
 */
class Content {
public:
    /**
     * Construct the working Content object from a document.
     * @param document The JSON document.
     * @return A pointer to the content or nullptr if invalid.
     */
    static ContentPtr create(JsonData&& document);

    /**
     * Construct the working Content object from a document, including a session for
     * reporting errors.
     * @param document The JSON document
     * @param session A logging session
     * @return A pointer to the content or nullptr if invalid
     */
    static ContentPtr create(JsonData&& document, const SessionPtr& session);

    /**
     * @return The main document package
     */
    const PackagePtr& getDocument() const { return mMainPackage; }

    /**
     * Return a package by name
     * @param name The package name
     * @return The PackagePtr or nullptr if it does not exist.
     */
    PackagePtr getPackage(const std::string& name) const;

    /**
     * Retrieve a set of packages that have been requested.  This method only returns an
     * individual package a single time.  Once it has been called, the "requested" packages
     * are moved internally into a "pending" list of packages.
     *
     * @return The set of packages that should be loaded.
     */
    std::set<ImportRequest> getRequestedPackages();

    /**
     * @return true if this document is waiting for a number of packages to be loaded.
     */
    bool isWaiting() const { return mRequested.size() > 0 || mPending.size() > 0; }

    /**
     * @return True if this content is complete and ready to be inflated.
     */
    bool isReady() const { return mState == State::READY; }

    /**
     * @return True if this content is in an error state and can't be inflated.
     */
    bool isError() const { return mState == State::ERROR; }

    /**
     * Add a requested package to the document.
     * @param info The requested package import structure.
     * @param raw Parsed data for the package.
     */
    void addPackage(const ImportRequest& request, JsonData&& raw);

    /**
     * Add data
     * @param name The name of the data source
     * @param data The raw data source
     */
    void addData(const std::string& name, JsonData&& data);

    /**
     * @return The number of parameters
     */
    size_t getParameterCount() const { return mMainParameters.size(); }

    /**
     * Retrieve the name of a parameter
     * @param index Index of the parameter
     * @return Name
     */
    const std::string& getParameterAt(size_t index) const {
        return mMainParameters.at(index);
    }

    /**
     * @return Main document APL version.
     */
    const std::string getAPLVersion() const { return mMainPackage->version(); }

    /**
     * @return The background object (color or gradient) for this document.  Returns
     *         the transparent color if no background is defined.
     */
    Object getBackground(const Metrics& metrics, const RootConfig& config) const;

    /**
     * @return The active session
     */
    const SessionPtr& getSession() const { return mSession; }

private:  // Non-public methods used by other classes
    friend class RootContext;

    const std::map<ImportRef, PackagePtr>& loaded() const { return mLoaded; }
    const rapidjson::Value& getMainTemplate() const { return mMainTemplate; }
    bool getMainProperties(Properties& out) const;

public:
    /**
     * Internal constructor. Do not call this directly.
     * @param session
     * @param mainPackagePtr
     * @param mainTemplate
     * @param parameterNames
     */
    Content(const SessionPtr& session,
            const PackagePtr& mainPackagePtr,
            const rapidjson::Value& mainTemplate,
            std::vector<std::string>&& parameterNames);

private:  // Private internal methods
    void addImportList(Package& package);
    void addImport(Package& package, const rapidjson::Value& value);
    void updateStatus();

private:
    enum State {
        LOADING,
        READY,
        ERROR
    };

    SessionPtr mSession;
    PackagePtr mMainPackage;
    std::vector<std::string> mMainParameters;

    State mState;

    std::set<ImportRequest> mRequested;
    std::set<ImportRequest> mPending;
    std::map<ImportRef, PackagePtr> mLoaded;

    std::map<std::string, JsonData> mParameterValues;
    const rapidjson::Value& mMainTemplate;
};


} // namespace apl

#endif //_APL_CONTENT_H
