/*MT*

    MediaTomb - http://www.mediatomb.cc/

    js_functions.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

    $Id$
*/

/// \file js_functions.cc

#ifdef HAVE_JS
#include "js_functions.h" // API

#include "content/content_manager.h"
#include "database/database.h"
#include "metadata/metadata_handler.h"
#include "script.h"
#include "util/string_converter.h"

// extern "C" {

duk_ret_t js_print(duk_context* ctx)
{
    duk_push_string(ctx, " ");
    duk_insert(ctx, 0);
    duk_join(ctx, duk_get_top(ctx) - 1);
    log_js("{}", duk_get_string(ctx, 0));
    return 0;
}

duk_ret_t js_copyObject(duk_context* ctx)
{
    auto self = Script::getContextScript(ctx);
    if (!duk_is_object(ctx, 0))
        return duk_error(ctx, DUK_ERR_TYPE_ERROR, "copyObject argument is not an object");
    auto cdsObj = self->dukObject2cdsObject(nullptr);
    self->cdsObject2dukObject(cdsObj);
    return 1;
}

duk_ret_t js_addContainerTree(duk_context* ctx)
{
    auto self = Script::getContextScript(ctx);

    if (!duk_is_array(ctx, 0)) {
        log_js("js_addContainerTree: No Array");
        return 0;
    }

    std::vector<std::shared_ptr<CdsObject>> result;
    auto length = duk_get_length(ctx, -1);

    for (duk_uarridx_t i = 0; i < length; i++) {
        if (duk_get_prop_index(ctx, -1, i)) {
            if (!duk_is_object(ctx, -1)) {
                duk_pop(ctx);
                log_js("js_addContainerTree: no object at {}", i);
                break;
            }
            duk_to_object(ctx, -1);
            auto cdsObj = self->dukObject2cdsObject(nullptr);
            if (cdsObj) {
                result.push_back(std::move(cdsObj));
            } else {
                log_js("js_addContainerTree: no CdsObject at {}", i);
            }
        }
        duk_pop(ctx);
    }

    if (!result.empty()) {
        auto cm = self->getContent();
        auto [containerId, containerStatus] = cm->addContainerTree(result);
        if (containerId != INVALID_OBJECT_ID) {
            /* setting last container ID as return value */
            auto tmp = fmt::to_string(containerId);
            duk_push_string(ctx, tmp.c_str());
            return 1;
        }
    }

    return 0;
}

duk_ret_t js_addCdsObject(duk_context* ctx)
{
    auto* self = Script::getContextScript(ctx);

    if (!duk_is_object(ctx, 0))
        return 0;
    duk_to_object(ctx, 0);
    // stack: js_cds_obj
    const char* containerId = duk_to_string(ctx, 1);
    if (!containerId)
        containerId = "-1";
    // stack: js_cds_obj containerId

    try {
        std::unique_ptr<StringConverter> p2i;
        std::unique_ptr<StringConverter> i2i;

        auto config = self->getConfig();
        if (self->whoami() == S_PLAYLIST) {
            p2i = StringConverter::p2i(config);
        } else {
            i2i = StringConverter::i2i(config);
        }
        duk_get_global_string(ctx, "object_script_path");
        auto rp = duk_get_string(ctx, -1);
        duk_pop(ctx);
        std::string rootPath = rp ? rp : "";

        if (self->whoami() == S_PLAYLIST)
            duk_get_global_string(ctx, "playlist");
        else if (self->whoami() == S_IMPORT)
            duk_get_global_string(ctx, "orig");
        else
            duk_push_undefined(ctx);
        // stack: js_cds_obj containerId js_orig_obj

        if (duk_is_undefined(ctx, -1)) {
            log_debug("Could not retrieve global 'orig'/'playlist' object");
            return 0;
        }

        auto origObject = self->dukObject2cdsObject(self->getProcessedObject());
        if (!origObject)
            return 0;

        std::shared_ptr<CdsObject> cdsObj;
        auto cm = self->getContent();
        int pcdId = INVALID_OBJECT_ID;

        duk_swap_top(ctx, 0);
        // stack: js_orig_obj containerId js_cds_obj
        if (self->whoami() == S_PLAYLIST) {
            int otype = self->getIntProperty("objectType", -1);
            if (otype == -1) {
                log_error("missing objectType property");
                return 0;
            }

            if (!IS_CDS_ITEM_EXTERNAL_URL(otype)) {
                fs::path loc = self->getProperty("location");
                std::error_code ec;
                auto dirEnt = fs::directory_entry(loc, ec);
                if (!ec) {
                    AutoScanSetting asSetting;
                    asSetting.followSymlinks = config->getBoolOption(CFG_IMPORT_FOLLOW_SYMLINKS);
                    asSetting.recursive = false;
                    asSetting.hidden = config->getBoolOption(CFG_IMPORT_HIDDEN_FILES);
                    asSetting.rescanResource = false;
                    asSetting.mergeOptions(config, loc);

                    pcdId = cm->addFile(dirEnt, rootPath, asSetting, false);
                    if (pcdId == INVALID_OBJECT_ID) {
                        log_error("Failed to add object {}", dirEnt.path().string());
                        return 0;
                    }
                    auto mainObj = self->getDatabase()->loadObject(pcdId);
                    cdsObj = self->dukObject2cdsObject(mainObj);
                } else {
                    log_error("Failed to read {}: {}", loc.c_str(), ec.message());
                }
            } else
                cdsObj = self->dukObject2cdsObject(origObject);
        } else
            cdsObj = self->dukObject2cdsObject(origObject);

        if (!cdsObj) {
            return 0;
        }

        auto [parentId, parentStatus] = std::pair(stoiString(containerId), false);

        if (parentId <= 0) {
            log_error("Invalid parent id passed to addCdsObject: {}", parentId);
            return 0;
        }

        cdsObj->setParentID(parentId);
        if (!cdsObj->isExternalItem()) {
            /// \todo get hidden file setting from config manager?
            /// what about same stuff in content manager, why is it not used
            /// there?

            if (self->whoami() == S_PLAYLIST) {
                if (pcdId == INVALID_OBJECT_ID) {
                    return 0;
                }
                cdsObj->setRefID(pcdId);
            } else
                cdsObj->setRefID(origObject->getID());

            cdsObj->setFlag(OBJECT_FLAG_USE_RESOURCE_REF);
        } else if (cdsObj->isExternalItem() && (self->whoami() == S_PLAYLIST) && self->getConfig()->getBoolOption(CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT_LINK_OBJECTS)) {
            cdsObj->setFlag(OBJECT_FLAG_PLAYLIST_REF);
            cdsObj->setRefID(origObject->getID());
        }

        cdsObj->setID(INVALID_OBJECT_ID);
        cm->addObject(cdsObj, parentStatus);

        /* setting object ID as return value */
        auto tmp = fmt::to_string(parentId);
        duk_push_string(ctx, tmp.c_str());
        return 1;
    } catch (const ServerShutdownException& se) {
        log_warning("Aborting script execution due to server shutdown.");
        return duk_error(ctx, DUK_ERR_ERROR, "Aborting script execution due to server shutdown.\n");
    } catch (const std::runtime_error& e) {
        log_error("{}", e.what());
    }
    return 0;
}

static duk_ret_t convertCharsetGeneric(duk_context* ctx, charset_convert_t chr)
{
    auto self = Script::getContextScript(ctx);
    if (duk_get_top(ctx) != 1)
        return DUK_RET_SYNTAX_ERROR;
    if (!duk_is_string(ctx, 0))
        return DUK_RET_TYPE_ERROR;
    const char* ts = duk_to_string(ctx, 0);
    duk_pop(ctx);

    try {
        std::string result = self->convertToCharset(ts, chr);
        duk_push_lstring(ctx, result.c_str(), result.length());
        return 1;
    } catch (const ServerShutdownException& se) {
        log_warning("Aborting script execution due to server shutdown.");
        return DUK_RET_ERROR;
    } catch (const std::runtime_error& e) {
        log_error("{}", e.what());
    }
    return 0;
}

duk_ret_t js_f2i(duk_context* ctx)
{
    return convertCharsetGeneric(ctx, F2I);
}

duk_ret_t js_m2i(duk_context* ctx)
{
    return convertCharsetGeneric(ctx, M2I);
}

duk_ret_t js_p2i(duk_context* ctx)
{
    return convertCharsetGeneric(ctx, P2I);
}

duk_ret_t js_j2i(duk_context* ctx)
{
    return convertCharsetGeneric(ctx, J2I);
}

//} // extern "C"

#endif // HAVE_JS
