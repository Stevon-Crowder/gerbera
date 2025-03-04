/*GRB*

    Gerbera - https://gerbera.io/

    upnp_quirks.h - this file is part of Gerbera.

    Copyright (C) 2020-2021 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

    $Id$
*/

/// \file upnp_quirks.h

#ifndef __UPNP_QUIRKS_H__
#define __UPNP_QUIRKS_H__

#include <memory>
#include <pugixml.hpp>
#include <vector>

using QuirkFlags = std::uint32_t;

#define QUIRK_FLAG_NONE 0x00000000
#define QUIRK_FLAG_SAMSUNG 0x00000001
#define QUIRK_FLAG_SAMSUNG_BOOKMARK_SEC 0x00000002
#define QUIRK_FLAG_SAMSUNG_BOOKMARK_MSEC 0x00000004
#define QUIRK_FLAG_IRADIO 0x00000008
#define QUIRK_FLAG_SAMSUNG_FEATURES 0x00000010

// forward declaration
class ActionRequest;
class ContentManager;
class Context;
class CdsItem;
class CdsObject;
class Headers;
struct ClientInfo;

class Quirks {
public:
    Quirks(std::shared_ptr<Context> context, const struct sockaddr_storage* addr, const std::string& userAgent);

    // Look for subtitle file and returns its URL in CaptionInfo.sec response header.
    // To be more compliant with original Samsung server we should check for getCaptionInfo.sec: 1 request header.
    void addCaptionInfo(const std::shared_ptr<CdsItem>& item, const std::unique_ptr<Headers>& headers) const;

    /** \brief Add Samsung specific bookmark information to the request's result.
     *
     * \param item const std::shared_ptr<CdsItem>& Item which will be played and stores the bookmark information.
     * \param result pugi::xml_node& Answer content.
     * \return void
     *
     */
    void restoreSamsungBookMarkedPosition(const std::shared_ptr<CdsItem>& item, pugi::xml_node& result) const;

    /** \brief Stored bookmark information into the database
     *
     * \param request const std::unique_ptr<ActionRequest>& request sent by Samsung client, which holds the position information which should be stored
     * \return void
     *
     */
    void saveSamsungBookMarkedPosition(const std::unique_ptr<ActionRequest>& request) const;

    /** \brief get Samsung Feature List
     *
     * \param request const std::unique_ptr<ActionRequest>& request sent by Samsung client
     * \return void
     *
     */
    void getSamsungFeatureList(const std::unique_ptr<ActionRequest>& request) const;
    std::vector<std::shared_ptr<CdsObject>> getSamsungFeatureRoot(const std::string& objId);

    /** \brief get Samsung ObjectID from Index
     *
     * \param request const std::unique_ptr<ActionRequest>& request sent by Samsung client
     * \return void
     *
     */
    void getSamsungObjectIDfromIndex(const std::unique_ptr<ActionRequest>& request) const;

    /** \brief get Samsung Index from RID
     *
     * \param request const std::unique_ptr<ActionRequest>& request sent by Samsung client
     * \return void
     *
     */
    void getSamsungIndexfromRID(const std::unique_ptr<ActionRequest>& request) const;

    /** \brief block XML header in response for broken clients
     *
     * \param request const std::unique_ptr<ActionRequest>& request sent by Samsung client
     * \return void
     *
     */
    bool blockXmlDeclaration() const;

    /** \brief Check whether the supplied flags are set
     *
     * \param bitset of the flags to check
     * \return bitset of the flags
     *
     */
    int checkFlags(int flags) const;

private:
    std::shared_ptr<Context> context;
    std::shared_ptr<ContentManager> content;
    const ClientInfo* pClientInfo;
};

#endif // __UPNP_QUIRKS_H__
