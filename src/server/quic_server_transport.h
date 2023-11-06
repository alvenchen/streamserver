
#pragma once

#include "server_state_machine.h"

namespace quic {

class QuicServerTransport{


private:
    QuicServerConnectionState* serverConn_;
};

}