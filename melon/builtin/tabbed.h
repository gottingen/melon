//
// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//


#pragma once

#include <string>
#include <vector>

namespace melon {

    // Contain the information for showing a tab.
    struct TabInfo {
        std::string tab_name;
        std::string path;

        [[nodiscard]] bool valid() const { return !tab_name.empty() && !path.empty(); }
    };

    // For appending TabInfo
    class TabInfoList {
    public:
        TabInfoList() = default;

        TabInfoList(const TabInfoList &) = delete;

        void operator=(const TabInfoList &) = delete;

        TabInfo *add() {
            _list.push_back(TabInfo());
            return &_list[_list.size() - 1];
        }

        size_t size() const { return _list.size(); }

        const TabInfo &operator[](size_t i) const { return _list[i]; }

        void resize(size_t newsize) { _list.resize(newsize); }

    private:
        std::vector<TabInfo> _list;
    };

    // Inherit this class to show the service with one or more tabs.
    // NOTE: tabbed services are not shown in /status.
    // Example:
    //   #include <melon/builtin/common.h>
    //   ...
    //   void MySerivce::GetTabInfo(melon::TabInfoList* info_list) const {
    //     melon::TabInfo* info = info_list->add();
    //     info->tab_name = "my_tab";
    //     info->path = "/MyService/MyMethod";
    //   }
    //   void MyService::MyMethod(::google::protobuf::RpcController* controller,
    //                            const XXXRequest* request,
    //                            XXXResponse* response,
    //                            ::google::protobuf::Closure* done) {
    //     ...
    //     if (use_html) {
    //       os << "<!DOCTYPE html><html><head>\n"
    //          << "<script language=\"javascript\" type=\"text/javascript\" src=\"/js/jquery_min\"></script>\n"
    //          << melon::TabsHead() << "</head><body>";
    //       cntl->server()->PrintTabsBody(os, "my_tab");
    //     }
    //     ...
    //     if (use_html) {
    //       os << "</body></html>";
    //     }
    //   }
    // Note: don't forget the jquery.
    class Tabbed {
    public:
        virtual ~Tabbed() = default;

        virtual void GetTabInfo(TabInfoList *info_list) const = 0;
    };

}  // namespace melon

