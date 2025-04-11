/*
 *    Copyright (c) 2025, The OpenThread Authors.
 *    All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *    POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file includes implementation of the MUD Forwarder.
 *   Basically, it advertises as a thread service so other devices can send it their MUD URLs
 */
#ifndef OTBR_MUD_FORWARDER_HPP_
#define OTBR_MUD_FORWARDER_HPP_

#include "ncp/rcp_host.hpp"
#include "common/types.hpp"

#include <openthread/message.h>
#include <openthread/ip6.h>
#include <openthread/udp.h>
#include <openthread/error.h>

namespace otbr {
namespace MUD {
    class MudForwarder
    {
    public:
        
        /**
         * Constructor
         * 
         * @param[in]   aInstance   The OpenThread Instance.
         */
        MudForwarder(Ncp::RcpHost &aHost);

        otError Init();
        otError Deinit();
    
    private:
        otError InitSocket();
        otError RegisterService();
        bool IsMudServiceRegistered();
        static void HandleMUDNewDeviceMessage(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
        void HandleMUDNewDeviceMessage(otMessage *aMessage, const otMessageInfo *aMessageInfo);
        void HandleThreadStateChanged(otChangedFlags aFlags);

        otUdpSocket   mSocket;
        Ncp::RcpHost &mHost;
        Ip6Address mMudManagerIp;
    };

} // Namespace MUD

} // Namespace otbr

#endif // OTBR_MUD_FORWARDER_HPP_