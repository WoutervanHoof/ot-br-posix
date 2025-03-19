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
 *   This file includes implementation of the MUD Manager.
 *   Basically, it advertises as a thread service so other devices can send it their MUD URLs
 */

#define OTBR_LOG_TAG "MUDForwarder"

#include "mud_forwarder/mud_forwarder.hpp"

#include <common/logging.hpp>
#include <common/code_utils.hpp>
#include <string.h>

namespace otbr {
namespace MUD {

MudForwarder::MudForwarder(otbr::Ncp::RcpHost &aHost)
    : mHost(aHost)
{
    // Initialize a socket
    otbrLogDebug("Starting MUD Forwarder");

    memset(&mSocket, 0, sizeof(mSocket));
}

otError MudForwarder::Init()
{
    otError error;
    otSockAddr  sockaddr;
    otNetifIdentifier netif = OT_NETIF_THREAD;
    const char *address_string = "::";

    VerifyOrExit(!otUdpIsOpen(mHost.GetInstance(), &mSocket), error = OT_ERROR_ALREADY);
    SuccessOrExit(error = otUdpOpen(mHost.GetInstance(), &mSocket, HandleMUDNewDeviceMessage, this));
    SuccessOrExit(error = otIp6AddressFromString(address_string, &sockaddr.mAddress));

    sockaddr.mPort = 1234;

    error = otUdpBind(mHost.GetInstance(), &mSocket, &sockaddr, netif);

exit:
    return error;
}



otError MudForwarder::Deinit()
{
    otbrLogDebug("Deinitializig MUD Forwarder..")
    return otUdpClose(mHost.GetInstance(), &mSocket);
}

void MudForwarder::HandleMUDNewDeviceMessage(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<MudForwarder *>(aContext)->HandleMUDNewDeviceMessage(aMessage, aMessageInfo);
}

void MudForwarder::HandleMUDNewDeviceMessage(otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    // TODO: not just log, actually do smt with the message
    char buf[1500];
    int  length;

    otbrLogInfo("%d bytes from ", otMessageGetLength(aMessage) - otMessageGetOffset(aMessage));

    char string[OT_IP6_ADDRESS_STRING_SIZE];

    otIp6AddressToString(&(aMessageInfo->mPeerAddr), string, sizeof(string));
    otbrLogInfo("%s", string);
    otbrLogInfo(" %d ", aMessageInfo->mPeerPort);

    length      = otMessageRead(aMessage, otMessageGetOffset(aMessage), buf, sizeof(buf) - 1);
    buf[length] = '\0';

    otbrLogInfo("%s", buf);
}

} // Namespace MUD

} // Namespace otbr