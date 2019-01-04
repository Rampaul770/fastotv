/*  Copyright (C) 2014-2018 FastoGT. All right reserved.

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

#include <vector>

#include <common/libev/io_loop_observer.h>  // for IoLoopObserver
#include <common/net/types.h>               // for HostAndPort

#include "commands_info/auth_info.h"  // for AuthInfo

#include "client/types.h"         // for BandwidthHostType
#include "client_server_types.h"  // for bandwidth_t
#include "commands_info/chat_message.h"

#include "inner/inner_server_command_seq_parser.h"  // for InnerServerComman...

namespace fastotv {
namespace client {
namespace bandwidth {
class TcpBandwidthClient;
}
namespace inner {
class InnerClient;

struct StartConfig {
  common::net::HostAndPort inner_host;
  AuthInfo ainf;
};

class InnerTcpHandler : public fastotv::inner::InnerServerCommandSeqParser, public common::libev::IoLoopObserver {
 public:
  enum {
    ping_timeout_server = 30  // sec
  };

  explicit InnerTcpHandler(const StartConfig& config);
  virtual ~InnerTcpHandler();

  void RequestServerInfo();                        // should be execute in network thread
  void RequestChannels();                          // should be execute in network thread
  void RequesRuntimeChannelInfo(stream_id sid);    // should be execute in network thread
  void PostMessageToChat(const ChatMessage& msg);  // should be execute in network thread
  void Connect(common::libev::IoLoop* server);     // should be execute in network thread
  void DisConnect(common::Error err);              // should be execute in network thread

  void PreLooped(common::libev::IoLoop* server) override;
  void Accepted(common::libev::IoClient* client) override;
  void Moved(common::libev::IoLoop* server, common::libev::IoClient* client) override;
  void Closed(common::libev::IoClient* client) override;
  void DataReceived(common::libev::IoClient* client) override;
  void DataReadyToWrite(common::libev::IoClient* client) override;
  void PostLooped(common::libev::IoLoop* server) override;
  void TimerEmited(common::libev::IoLoop* server, common::libev::timer_id_t id) override;
#if LIBEV_CHILD_ENABLE
  void Accepted(common::libev::IoChild* child) override;
  void Moved(common::libev::IoLoop* server, common::libev::IoChild* child) override;
  void ChildStatusChanged(common::libev::IoChild* child, int status) override;
#endif

 private:
  common::ErrnoError CreateAndConnectTcpBandwidthClient(common::libev::IoLoop* server,
                                                        const common::net::HostAndPort& host,
                                                        BandwidthHostType hs,
                                                        bandwidth::TcpBandwidthClient** out_band) WARN_UNUSED_RESULT;

  void HandleInnerRequestCommand(fastotv::inner::InnerClient* connection,
                                 common::protocols::three_way_handshake::cmd_seq_t id,
                                 int argc,
                                 char* argv[]) override;
  void HandleInnerResponceCommand(fastotv::inner::InnerClient* connection,
                                  common::protocols::three_way_handshake::cmd_seq_t id,
                                  int argc,
                                  char* argv[]) override;
  void HandleInnerApproveCommand(fastotv::inner::InnerClient* connection,
                                 common::protocols::three_way_handshake::cmd_seq_t id,
                                 int argc,
                                 char* argv[]) override;

  // inner handlers
  common::ErrnoError HandleInnerSuccsessResponceCommand(fastotv::inner::InnerClient* connection,
                                                        common::protocols::three_way_handshake::cmd_seq_t id,
                                                        int argc,
                                                        char* argv[]) WARN_UNUSED_RESULT;
  common::ErrnoError HandleInnerFailedResponceCommand(fastotv::inner::InnerClient* connection,
                                                      common::protocols::three_way_handshake::cmd_seq_t id,
                                                      int argc,
                                                      char* argv[]) WARN_UNUSED_RESULT;

  common::Error ParserResponceResponceCommand(int argc, char* argv[], json_object** out) WARN_UNUSED_RESULT;

  fastotv::inner::InnerClient* inner_connection_;
  std::vector<bandwidth::TcpBandwidthClient*> bandwidth_requests_;
  common::libev::timer_id_t ping_server_id_timer_;

  const StartConfig config_;

  bandwidth_t current_bandwidth_;
};

}  // namespace inner
}  // namespace client
}  // namespace fastotv
