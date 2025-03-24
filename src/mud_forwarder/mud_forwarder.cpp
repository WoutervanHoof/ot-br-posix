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

#include <openthread/server.h>
#include <common/logging.hpp>
#include <common/code_utils.hpp>
#include <string.h>


namespace otbr {
namespace MUD {

MudForwarder::MudForwarder(otbr::Ncp::RcpHost &aHost)
    : mHost(aHost)
{
    // Initialize a socket
    otbrLogInfo("Starting MUD Forwarder");

    memset(&mSocket, 0, sizeof(mSocket));
}

otError MudForwarder::Init()
{
    otError error;

    SuccessOrExit(error = MudForwarder::InitSocket());

    error = MudForwarder::RegisterService();

exit:
    return error;
}

otError MudForwarder::InitSocket()
{
    otError                 error = OT_ERROR_NONE;
    otSockAddr              sockaddr;
    const otNetifIdentifier netif = OT_NETIF_THREAD;
    std::string             address_string("::");

    // Start listening on udp port 1234
    otbrLogInfo("Initializing MUD Forwarder");

    sockaddr.mPort = 1234;
    VerifyOrExit(!otUdpIsOpen(mHost.GetInstance(), &mSocket), error = OT_ERROR_INVALID_STATE);
    SuccessOrExit(error = otUdpOpen(mHost.GetInstance(), &mSocket, HandleMUDNewDeviceMessage, this));
    SuccessOrExit(error = otIp6AddressFromString(address_string.c_str(), &sockaddr.mAddress));
    error = otUdpBind(mHost.GetInstance(), &mSocket, &sockaddr, netif);

exit:
    return error;
}

otError MudForwarder::RegisterService()
{
    otError                 error = OT_ERROR_NONE;
    const otNetifAddress   *addresses;
    const size_t            ip6StringSize = OT_IP6_ADDRESS_STRING_SIZE;
    otServiceConfig         config;
    std::string             serviceName("MUD_Forwarder");
    otbr::Ip6Address        address;

    char tempbuffer[OT_IP6_ADDRESS_SIZE];

    // Advertize service in network
    otbrLogInfo("Advertizing service...");

    // Set config ServiceData
    config.mEnterpriseNumber = 44970; // OpenThread IANA enterprise number
    config.mServerConfig.mStable = true;
    sprintf(reinterpret_cast<char*>(config.mServiceData), serviceName.c_str());
    config.mServiceDataLength = serviceName.length() + 1;

    addresses = otIp6GetUnicastAddresses(mHost.GetInstance());
    for (const otNetifAddress *addr = addresses; addr; addr = addr->mNext)
    {
        // A meshlocal, non RLOC address should be reachable by all devices, regardless of topology changes
        otIp6AddressToString(&(addr->mAddress), tempbuffer, OT_IP6_ADDRESS_SIZE);
        otbrLogInfo("checking address %s", tempbuffer);
        if (addr->mMeshLocal && !addr->mRloc) {
            otbrLogInfo("valid address found!");
            assert(sizeof(config.mServerConfig.mServerData) > OT_IP6_ADDRESS_STRING_SIZE);
            otIp6AddressToString(&(addr->mAddress), reinterpret_cast<char*>(config.mServerConfig.mServerData), ip6StringSize);
            // TODO: check if this is changed to the actual number of bytes written
            config.mServerConfig.mServerDataLength = ip6StringSize;
        }
    }

    SuccessOrExit(error = otServerAddService(mHost.GetInstance(), &config));
    SuccessOrExit(error = otServerRegister(mHost.GetInstance()));
    otbrLogInfo("Sucessfully registered service");

exit:
    return error;
}

otError MudForwarder::Deinit()
{
    otbrLogInfo("Deinitializig MUD Forwarder..");
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