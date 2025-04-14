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
#include <common/tlv.hpp>
#include <openthread/mud.h>

#ifndef MUD_MANAGER_ADDRESS
#define MUD_MANAGER_ADDRESS "fd99:aaaa:bbbb:100::1"
#endif

namespace otbr {
namespace MUD {

MudForwarder::MudForwarder(otbr::Ncp::RcpHost &aHost)
    : mHost(aHost)
{
    const char * ipaddr = MUD_MANAGER_ADDRESS;
    mMudManagerIp = Ip6Address(ipaddr);

    // Initialize a socket
    otbrLogInfo("Starting MUD Forwarder");

    memset(&mSocket, 0, sizeof(mSocket));
}

otError MudForwarder::Init()
{
    otError     error   = OT_ERROR_NONE;
    SuccessOrExit(error = MudForwarder::InitSocket());

    mHost.AddThreadStateChangedCallback([this](otChangedFlags aFlags) { HandleThreadStateChanged(aFlags);});

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
    otError         error = OT_ERROR_NONE;
    otServiceConfig config;
    std::string     serviceName("MUD_Forwarder"); // TODO: should be a service ID specified in OpenThread API

    if (IsMudServiceRegistered()) {
        ExitNow(otbrLogInfo("MUD_Forwarder service is already registered."));
    }

    // Set config ServiceData
    config.mEnterpriseNumber = 44970; // OpenThread IANA enterprise number
    config.mServerConfig.mStable = true;
    sprintf(reinterpret_cast<char*>(config.mServiceData), "%s", serviceName.c_str());
    config.mServiceDataLength = serviceName.length() + 1;
    
    // TODO: this could be better, but it works for now
    for (u_int8_t i = 0; i < sizeof(mMudManagerIp.m8); i++) {
        config.mServerConfig.mServerData[i] = mMudManagerIp.m8[i];
    }
    config.mServerConfig.mServerData[sizeof(mMudManagerIp.m8)] = '\0';
    config.mServerConfig.mServerDataLength = sizeof(mMudManagerIp.m8) + 1;
    
    // Advertize service in network
    SuccessOrExit(error = otServerAddService(mHost.GetInstance(), &config));
    SuccessOrExit(error = otServerRegister(mHost.GetInstance()));
    otbrLogInfo("Registered Mudmanager service: %s", mMudManagerIp.ToString().c_str());

exit:
    return error;
}

void MudForwarder::HandleThreadStateChanged(otChangedFlags aFlags)
{
    otError error;
    otDeviceRole role;

    if (aFlags & (OT_CHANGED_THREAD_ROLE)) {
        role = otThreadGetDeviceRole(mHost.GetInstance());
        if (role == OT_DEVICE_ROLE_LEADER || role == OT_DEVICE_ROLE_ROUTER) {
            error = RegisterService();
            otbrLogInfo("Updated service registration. response: %d", error);
        }
    }
}

bool MudForwarder::IsMudServiceRegistered() {
    // TODO: check if service alread exists
    return false;
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

const Tlv *FindTlv(uint8_t aTlvType, const uint8_t *aTlvs, int aTlvsSize)
{
    const Tlv *result = nullptr;

    for (const Tlv *tlv = reinterpret_cast<const Tlv *>(aTlvs);
         reinterpret_cast<const uint8_t *>(tlv) + sizeof(Tlv) < aTlvs + aTlvsSize; tlv = tlv->GetNext())
    {
        if (tlv->GetType() == aTlvType)
        {
            ExitNow(result = tlv);
        }
    }

exit:
    return result;
}

void MudForwarder::HandleMUDNewDeviceMessage(otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    // CHECK: this should never be triggered again.
    const size_t    buflen      = 1500;
    u_int8_t        buf[buflen];
    uint16_t        length;
    const Tlv      *mudUrl;
    const Tlv      *mudChildIP;

    char            mudUrlString[50];
    char            mudIPString[50];

    memset(buf, '\0', buflen);
    otbrLogInfo("%d bytes from ", otMessageGetLength(aMessage) - otMessageGetOffset(aMessage));

    char string[OT_IP6_ADDRESS_STRING_SIZE];

    otIp6AddressToString(&(aMessageInfo->mPeerAddr), string, sizeof(string));
    otbrLogInfo("peer address: %s", string);
    otbrLogInfo("port: %d ", aMessageInfo->mPeerPort);

    length      = otMessageRead(aMessage, otMessageGetOffset(aMessage), buf, sizeof(buf) - 1);
    buf[length] = '\0';
    otbrLogInfo("read %d bytes from message", length);

    otbrLogInfo("buffer: %s", buf);

    mudUrl = FindTlv(OT_MUD_FORWARD_TLV_MUD_URL, buf, length);
    mudChildIP = FindTlv(OT_MUD_FORWARD_TLV_DEVICE_IP, buf, length);

    if (mudUrl == nullptr) {
        otbrLogInfo("no mud url tlv found");
        ExitNow();
    }

    if (mudChildIP == nullptr) {
        otbrLogInfo("no mud child ip found");
        ExitNow();
    }

    otbrLogInfo("read mud url tlv with length %d", mudUrl->GetLength());
    otbrLogInfo("read mud child ip tlv with length %d", mudChildIP->GetLength());

    memcpy(mudUrlString, mudUrl->GetValue(), mudUrl->GetLength());
    memcpy(mudIPString, mudChildIP->GetValue(), mudChildIP->GetLength());

    mudUrlString[mudUrl->GetLength()] = '\0';
    mudIPString[mudChildIP->GetLength()] = '\0';

    otbrLogInfo("mud url: %s", mudUrlString);
    otbrLogInfo("mud child ip: %s", mudIPString);

exit:
    return;
}

} // Namespace MUD

} // Namespace otbr