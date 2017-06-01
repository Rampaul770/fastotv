/*  Copyright (C) 2014-2017 FastoGT. All right reserved.

    This file is part of FastoTV.

    FastoTV is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FastoTV is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FastoTV. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "serializer/json_serializer.h"

#define RESPONCE_INFO_REQUEST_ID_FIELD "request_id"
#define RESPONCE_INFO_STATE_FIELD "state"
#define RESPONCE_INFO_COMMAND_FIELD "command"
#define RESPONCE_INFO_RESPONCE_FIELD "responce_json"

namespace fasto {
namespace fastotv {
namespace server {

class ResponceInfo : public JsonSerializer<ResponceInfo> {
 public:
  ResponceInfo();
  ResponceInfo(const std::string& request_id,
               const std::string& state_command,
               const std::string& command,
               const std::string& responce);

  common::Error Serialize(serialize_type* deserialized) const WARN_UNUSED_RESULT;
  static common::Error DeSerialize(const serialize_type& serialized,
                                   value_type* obj) WARN_UNUSED_RESULT;

  std::string GetRequestId() const;
  std::string GetState() const;
  std::string GetCommand() const;
  std::string GetResponceJson() const;

  bool Equals(const ResponceInfo& inf) const;

 private:
  std::string request_id_;
  std::string state_;
  std::string command_;
  std::string responce_json_;
};

inline bool operator==(const ResponceInfo& lhs, const ResponceInfo& rhs) {
  return lhs.Equals(rhs);
}

inline bool operator!=(const ResponceInfo& x, const ResponceInfo& y) {
  return !(x == y);
}
}  // namespace server
}  // namespace fastotv
}  // namespace fasto
