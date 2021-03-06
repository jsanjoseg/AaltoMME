/* AaltoMME - Mobility Management Entity for LTE networks
 * Copyright (C) 2013 Vicent Ferrer Guash & Jesus Llorente Santos
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file   NAS.c
 * @Author Vicent Ferrer
 * @date   May, 2013
 * @brief  NAS
 */

#include "NAS.h"
#include "NASlog.h"
#include "NASHandler.h"
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>     /*htonl*/
#include <openssl/hmac.h>

/* ************************************************** */
/*                  Internal Functions                */
/* ************************************************** */

/**
 * @brief Increment the NAS Counter
 * @param [in]  h           NAS handler
 * @param [in]  direction   0 for uplink, 1 for downlink
 * @return 1 on success, 0 if overflow
 *
 * It increments the NAS counter depending on the direction provided,
 * if an overflow is about to occur (5 counts before), returns 0
 */
int nas_incrementNASCount(const NAS h, const NAS_Direction direction){
    NASHandler *n = (NASHandler*)h;

    /* NAS COUNT structure:
     * 1 Byte    NAS Sequence (8 least significant bits)
     * 2 Bytes   NAS overflow counter
     * 1 Byte    Zeros
     */
    nas_msg(NAS_DEBUG, 0, "Increment counter for direction %u from %u",
            direction,
            n->nas_count[direction]);

    n->nas_count[direction]++;

    if(n->nas_count[direction] > 0xffffff-5){
        return 0;
    }
    return 1;
}

/**
 * @brief Set the NAS Counter
 * @param [in]  h           NAS handler
 * @param [in]  direction   0 for uplink, 1 for downlink
 * @return 1 on success, 0 if overflow
 *
 * It increments the NAS counter depending on the direction provided,
 * if an overflow is about to occur (5 counts before), returns 0
 */
int nas_setCOUNT(const NAS h, const NAS_Direction direction, const uint8_t recv){
    NASHandler *n = (NASHandler*)h;

    /* NAS COUNT structure:
     * 1 Byte    NAS Sequence (8 least significant bits)
     * 2 Bytes   NAS overflow counter
     * 1 Byte    Zeros
     */
    nas_msg(NAS_DEBUG, 0, "Increment counter for direction %u from %u",
            direction,
            n->nas_count[direction]);

    /* if((n->nas_count[direction]&0xFF)>(uint8_t)(recv+1)){ */
    /*     n->nas_count[direction] += 0x100; */
    /* } */
    n->nas_count[direction] = ((n->nas_count[direction]&0xFFFF00) | (uint8_t)recv)+1;

    if(n->nas_count[direction] > 0xFFFFFF-5){
        return 0;
    }
    return 1;
}

/**
 * @brief Set the NAS Counter short
 * @param [in]  h           NAS handler
 * @param [in]  direction   0 for uplink, 1 for downlink
 * @return 1 on success, 0 if overflow
 *
 * It increments the NAS counter depending on the direction provided,
 * if an overflow is about to occur (5 counts before), returns 0
 */
int nas_setCOUNTshort(const NAS h, const NAS_Direction direction, const uint8_t recv){
    NASHandler *n = (NASHandler*)h;

    /* NAS COUNT structure:
     * 1 Byte    NAS Sequence (8 least significant bits)
     * 2 Bytes   NAS overflow counter
     * 1 Byte    Zeros
     */
    nas_msg(NAS_DEBUG, 0, "Increment counter for direction %u from %u",
            direction,
            n->nas_count[direction]);

    /* if((n->nas_count[direction]&0x1F)> (uint8_t)(recv+1) && */
    /*    (n->nas_count[direction]&0xE0)==0xE0){ */
    /*     n->nas_count[direction] += 0x100; */
    /* } */
    n->nas_count[direction] = ((n->nas_count[direction]&0xFFFFE0) | (uint8_t)recv)+1;

    if(n->nas_count[direction] > 0xFFFFFF-5){
        return 0;
    }
    return 1;
}

/**
 * @brief Check received NAS Sequence number (SN) with NAS COUNT
 * @param [in] last   Last accepted COUNT
 * @param [in] recv   Received NAS SN
 * @return 1 if ok, 0 retransmission
 */
static int nas_checkCOUNT(const uint32_t last, const uint8_t recv){
    if((recv-(last&0xFF)) <  NAS_COUNT_THRESHOLD){
        return 1;
    }
    return 0;
}

/**
 * @brief Check received NAS Sequence number (SN) short with NAS COUNT
 * @param [in] last   Last accepted COUNT
 * @param [in] recv   Received NAS SN
 * @return 1 if ok, 0 retransmission
 */
static int nas_checkCOUNTshort(const uint32_t last, const uint8_t recv){
    if((recv-(last&0x1F)) <  NAS_COUNT_THRESHOLD){
        return 1;
    }
    return 0;
}


static void hmac_sha256(const unsigned char *key, unsigned int key_size,
                        const unsigned char *message, unsigned int message_len,
                        unsigned char *mac, unsigned mac_size){
    uint8_t res[EVP_MAX_MD_SIZE];
    uint32_t len;
    HMAC_CTX ctx;

    HMAC_CTX_init(&ctx);
    HMAC_Init_ex(&ctx, key, key_size, EVP_sha256(), NULL);
    HMAC_Update(&ctx, message, message_len);
    HMAC_Final(&ctx, res, &len);
    HMAC_CTX_cleanup(&ctx);
    /* Use least significant bits */
    memcpy(mac, res + len - mac_size, mac_size);
}

/**
 * @brief Key derivation function
 * @param [in]  kasme          derived key - 256 bits
 * @param [in]  distinguisher  Algorithm distinguisher
 * @param [in]  algId          Algorithm identity
 * @param [out] k              Derived Key - 128 bits
 */
static void kdf(const uint8_t *kasme,
                const uint8_t distinguisher, const uint8_t algId,
                uint8_t *k){

    /*
    FC = 0x15,
    P0 = algorithm type distinguisher,
    L0 = length of algorithm type distinguisher (i.e. 0x00 0x01)
    P1 = algorithm identity
    L1 = length of algorithm identity (i.e. 0x00 0x01)
     */
    uint8_t s[7];
    s[0]=0x15;
    s[1]=distinguisher;
    s[2]=0x00;
    s[3]=0x01;
    s[4]=algId;
    s[5]=0x00;
    s[6]=0x01;

    hmac_sha256(kasme, 32, s, 7, k, 16);
}


/* ***** Decoding functions ***** */


void dec_EMM(EMM_Message_t *msg, const uint8_t *buf, const size_t s){

    msg->messageType = *(buf++);
    size_t size = s - 1;
    nas_msg(NAS_DEBUG, 0, "DEC : messageType = %#x", msg->messageType);

    switch((NASMessageType_t)msg->messageType){
    case AttachRequest:
        dec_AttachRequest((AttachRequest_t*)msg, buf, size);
        break;
    case AttachAccept:
        dec_AttachAccept((AttachAccept_t*)msg, buf, size);
        break;
    case AttachComplete:
        dec_AttachComplete((AttachComplete_t*)msg, buf, size);
        break;
    case AttachReject:
        dec_AttachReject((AttachReject_t*)msg, buf, size);
        break;
    case DetachRequest:
        dec_DetachRequestUEOrig((DetachRequestUEOrig_t*)msg, buf, size);
        break;
    case DetachAccept:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case TrackingAreaUpdateRequest:
        dec_TrackingAreaUpdateRequest((TrackingAreaUpdateRequest_t*)msg, buf, size);
        break;
    case TrackingAreaUpdateAccept:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case TrackingAreaUpdateComplete:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case TrackingAreaUpdateReject:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case ExtendedServiceRequest:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case ServiceReject:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case GUTIReallocationCommand:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case GUTIReallocationComplete:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case AuthenticationRequest:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case AuthenticationResponse:
        dec_AuthenticationResponse((AuthenticationResponse_t*)msg, buf, size);
        break;
    case AuthenticationReject:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case AuthenticationFailure:
        dec_AuthenticationFailure((AuthenticationFailure_t*)msg, buf, size);
        break;
    case IdentityRequest:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case IdentityResponse:
        dec_IdentityResponse((IdentityResponse_t*)msg, buf, size);
        break;
    case SecurityModeCommand:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case SecurityModeComplete:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case SecurityModeReject:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case EMMStatus:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case EMMInformation:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case DownlinkNASTransport:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case UplinkNASTransport:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case CSServiceNotification:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case DownlinkGenericNASTransport:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case UplinkGenericNASTransport:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    default:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    }
}

void dec_ESM(ESM_Message_t *msg, const uint8_t *buf, const size_t s){

    msg->procedureTransactionIdentity = *(buf++);
    msg->messageType = *(buf++);
    size_t size = s - 2;

    nas_msg(NAS_DEBUG, 0, "DEC : procedureTransactionIdentity %#x, messageType %#x",
            msg->procedureTransactionIdentity, msg->messageType);

    switch((NASMessageType_t)msg->messageType){
    case ActivateDefaultEPSBearerContextRequest:
        dec_ActivateDefaultEPSBearerContextAccept((ActivateDefaultEPSBearerContextAccept_t*)msg, buf, size);
        break;
    case ActivateDefaultEPSBearerContextAccept:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case ActivateDefaultEPSBearerContextReject:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case ActivateDedicatedEPSBearerContextRequest:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case ActivateDedicatedEPSBearerContextAccept:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case ActivateDedicatedEPSBearerContextReject:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case ModifyEPSBearerContextRequest:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case ModifyEPSBearerContextAccept:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case ModifyEPSBearerContextReject:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case DeactivateEPSBearerContextRequest:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case DeactivateEPSBearerContextAccept:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case PDNConnectivityRequest:
        dec_PDNConnectivityRequest((PDNConnectivityRequest_t*)msg, buf, size);
        break;
    case PDNConnectivityReject:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case PDNDisconnectRequest:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case PDNDisconnectReject:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case BearerResourceAllocationRequest:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case BearerResourceAllocationReject:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case BearerResourceModificationRequest:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case BearerResourceModificationReject:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case ESMInformationRequest:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case ESMInformationResponse:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case Notification:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    case ESMStatus:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    default:
        nas_msg(NAS_DEBUG, 0, "DEC : messageType decoder Not available: messageType %#x", msg->messageType);
        break;
    }
}

int dec_NAS(GenericNASMsg_t *msg, const uint8_t *buf, const size_t size){
    uint8_t const *pointer;
    ProtocolDiscriminator_t p;
    SecurityHeaderType_t s;

    memset(msg, 0, sizeof(GenericNASMsg_t));

    if(!nas_getHeader(buf, size, &s, &p))
        return 0;

    pointer = buf;
    msg->header.protocolDiscriminator.v = p;
    msg->header.protocolDiscriminator.s = 0;
    msg->header.securityHeaderType.v = s;
    msg->header.securityHeaderType.s = 0;
    pointer++;

    nas_msg(NAS_DEBUG, 0,"DEC : protocolDiscriminator = %x,"
            " securityHeaderType = %x", p, s);

    if(s != PlainNAS ||
       !(p != EPSSessionManagementMessages || p != EPSMobilityManagementMessages)){
        return 0;
    }

    if(p == EPSMobilityManagementMessages){
        dec_EMM(&(msg->plain.eMM), pointer, size-1);
        return 1;
    }

    dec_ESM(&(msg->plain.eSM), pointer, size-1);
    return 1;
}


/* ***** Encoding functions ***** */


void newNASMsg_EMM(uint8_t **curpos,
                   ProtocolDiscriminator_t protocolDiscriminator,
                   SecurityHeaderType_t securityHeaderType){
    if(curpos == NULL){
        nas_msg(NAS_ERROR, 0,"ENC : buffer parameter not allocated");
        return;
    }
    nasIe_v_t1_l(curpos, protocolDiscriminator);
    nasIe_v_t1_h(curpos, securityHeaderType);
}

void newNASMsg_ESM(uint8_t **curpos,
                   ProtocolDiscriminator_t protocolDiscriminator,
                   uint8_t ePSBearerId){
    if(curpos == NULL){
        nas_msg(NAS_ERROR, 0,"ENC : buffer parameter not allocated");
        return;
    }
    nasIe_v_t1_l(curpos, protocolDiscriminator);
    nasIe_v_t1_h(curpos, ePSBearerId);
}

int newNASMsg_sec(const NAS h,
                  uint8_t *out, size_t *len,
                  const ProtocolDiscriminator_t p,
                  const SecurityHeaderType_t s,
                  const NAS_Direction direction,
                  const uint8_t *plain, const size_t pLen){

    uint8_t buf[pLen+1], count[4], mac[4], *pointer;
    uint32_t ncount;
    size_t cLen = 0;
    NASHandler *n = (NASHandler*)h;

    if(!n->isValid)
        return 0;

    if(!(s == IntegrityProtected ||
         s == IntegrityProtectedAndCiphered ||
         s == IntegrityProtectedWithNewEPSSecurityContext ||
         s == IntegrityProtectedAndCipheredWithNewEPSSecurityContext)){
        /*We shouldn't be here*/
        return 0;
    }

    ncount = htonl(n->nas_count[direction]);
    memcpy(count, &ncount, 4);
    memset(mac, 0, 4);
    *len = 0;
    pointer = out;

    /* Cypher Message*/
    if(s == IntegrityProtectedAndCiphered ||
       s == IntegrityProtectedAndCipheredWithNewEPSSecurityContext){
        /* out+6 instead of buf+1 ?*/
        eea_cyph_cb[n->e](n->ekey, count, 0, direction, buf+1, &cLen, plain, pLen);
        if(pLen != cLen){
            return 0;
        }
    }else{
        memcpy(buf+1, plain, pLen);
    }

    /* NAS SQN*/
    buf[0] = count[3];

    /* Calculate MAC*/
    eia_cb[n->i](n->ikey, count, 0, direction, buf, (pLen +1)*8, mac);

    /* Encode new Message with Security header*/
    newNASMsg_EMM(&pointer, p, s);
    /* Add message authentication code*/
    nasIe_v_t3(&pointer, mac, 4);
    /* Add sequence number and cyphered message */
    nasIe_v_t3(&pointer, buf, pLen + 1);
    *len = pointer - out;
    /* Increment downlink counter*/
    nas_incrementNASCount(n, NAS_DownLink);
    return 1;
}


void encaps_ESM(uint8_t **curpos,
                ProcedureTransactionId_t procedureTransactionIdentity,
                NASMessageType_t messageType){
    nasIe_v_t3(curpos, (uint8_t*)&procedureTransactionIdentity, 1);
    nasIe_v_t3(curpos, (uint8_t*)&messageType, 1);
}

void encaps_EMM(uint8_t **curpos, NASMessageType_t messageType){
    nasIe_v_t3(curpos, (uint8_t*)&messageType, 1);
}

/* Tool functions*/

NAS nas_newHandler(){
    NASHandler *n = (NASHandler*)malloc(sizeof(NASHandler));
    memset(n, 0, sizeof(NASHandler));
    return n;
}

void nas_freeHandler(NAS h){
    NASHandler *n = (NASHandler*)h;
    free(n);
    return;
}


void nas_setSecurity(NAS h, const NAS_EIA i, const NAS_EEA e,
                     const uint8_t *kasme){

    NASHandler *n = (NASHandler*)h;

    n->i = i;
    n->e = e;

    kdf(kasme, 0x02, i, n->ikey);
    kdf(kasme, 0x01, e, n->ekey);

    n->nas_count[0] = 0;
    n->nas_count[1] = 0;
    n->isValid = 1;
    return;
}

int nas_getHeader(const uint8_t *buf, const uint32_t size,
                   SecurityHeaderType_t *s, ProtocolDiscriminator_t *p){
    if(!buf || size<1){
        return 0;
    }
    if(s)
        *s = (buf[0]&0xf0)>>4;
    if(p)
        *p = buf[0]&0x0f;
    return 1;
}

const uint32_t nas_getLastCount(const NAS h, const NAS_Direction direction){
    NASHandler *n = (NASHandler*)h;
    /* -1 because we store the expected count message (UpLink), or the next count
     * to be used in Downlink*/
    return n->nas_count[direction]-1;
}

int nas_authenticateMsg(const NAS h,
                        const uint8_t *buf, const uint32_t size,
                        const NAS_Direction direction, uint8_t *isAuth){
    SecurityHeaderType_t s;
    NASHandler *n = (NASHandler*)h;
    uint8_t count[4];
    uint32_t ncount;
    uint8_t mac[4], mac_x[4], nas_sqn, short_mac[2];

    nas_getHeader(buf, size, &s, NULL);

    *isAuth = 0;
    if(!n->isValid)
        return 0;

    if( s == PlainNAS ){
        return 0;
    }

    switch(s){
    case IntegrityProtected:
    case IntegrityProtectedAndCiphered:
    case IntegrityProtectedWithNewEPSSecurityContext:
    case IntegrityProtectedAndCipheredWithNewEPSSecurityContext:
        /*Check NAS SQN*/
        nas_sqn = buf[5];

        if(!nas_checkCOUNT(n->nas_count[direction], nas_sqn)){
            return 2;
        }
        /*Calculate and validate MAC*/
        memcpy(mac, buf+1, 4);
        ncount = htonl((n->nas_count[direction]&0xFFFF00) | nas_sqn);
        memcpy(count, &ncount, 4);
        eia_cb[n->i](n->ikey, count, 0, direction, buf+5, (size-5)*8, mac_x);

        if(memcmp(mac, mac_x, 4) == 0 /* Integrity Verification OK*/
           || n->i == NAS_EIA0){      /* Integrity verification is not
                                       * applicable when EIA0 is used */
            *isAuth = 1;
            nas_setCOUNT(n, direction, nas_sqn);
        }
        return 1;
    case SecurityHeaderForServiceRequestMessage:
        /*Non-standard L3 message*/

        /*Check NAS SQN*/
        nas_sqn = buf[1]&0x1F;
        if(!nas_checkCOUNTshort(n->nas_count[direction], nas_sqn)){
            return 2;
        }
        /*Calculate and validate MAC*/
        memcpy(short_mac, buf+2, 2);
        ncount = htonl((n->nas_count[direction]&0xFFFFE0) | nas_sqn);
        memcpy(count, &ncount, 4);
        eia_cb[n->i](n->ikey, count, 0, direction, buf, 2*8, mac_x);

        if(memcmp(short_mac, mac_x+2, 2) == 0 /* Integrity Verification OK*/
           || n->i == NAS_EIA0){              /* Integrity verification is not
                                               * applicable when EIA0 is used */
            *isAuth = 1;
            nas_setCOUNTshort(n, direction, nas_sqn);
        }
        return 1;
    default:
        return 0;
    }
}

int dec_secNAS(const NAS h,
               GenericNASMsg_t *msg, const NAS_Direction direction,
               const uint8_t *buf, const size_t size){

    SecurityHeaderType_t s;
    NASHandler *n = (NASHandler*)h;
    uint8_t plain[size], count[4];
    size_t len;
    uint32_t ncount;

    memset(msg, 0, sizeof(GenericNASMsg_t));

    nas_getHeader(buf, size, &s, NULL);

    if(s ==  IntegrityProtected){
        dec_NAS(msg, buf+6, size-6);
        return 1;
    }else if( ! (s == IntegrityProtectedAndCiphered ||
                 s == IntegrityProtectedAndCipheredWithNewEPSSecurityContext ||
                 s == SecurityHeaderForServiceRequestMessage)){
        return 0;
    }

    /* Keep it here, so it can decode IntegrityProtected Messages
     * without a valid context. The context is only checked when decyphering
     * is required. */
    if(!n->isValid)
        return 0;

    ncount = htonl(n->nas_count[direction]);
    memcpy(count, &ncount, 4);
    eea_dec_cb[n->e](n->ekey, count, 0, direction,
                     buf + 6, size - 6,
                     plain, &len);

    return dec_NAS(msg, plain, len);
}

uint8_t nas_isAuthRequired(const NASMessageType_t messageType){
    uint8_t res;

    switch (messageType) {
    case AttachRequest:
    case IdentityResponse:
    case AuthenticationResponse:
    case AuthenticationFailure:
    case SecurityModeReject:
    case DetachRequest:
    case DetachAccept:
    case TrackingAreaUpdateRequest:
    /* Service Request not considered as it doesn't have message type
     * Check TS 24.301*/
    case ExtendedServiceRequest:
        res = 0;
        break;
    default:
        res = 1;
        break;
    }
    return res;
}

void nas_NASOpt_lookup(const union nAS_ie_member *optionals,
                       const uint8_t maxOpts,
                       const uint8_t iei,
                       union nAS_ie_member const **ie){
    uint8_t i;
    *ie = NULL;
    for(i=0; i<maxOpts; i++){
        if(optionals[i].iei==0){
            return;
        }
        if((iei&0xF8) == 0x08){
            /*IEI half byte*/
            if(optionals[i].v_t1_h.v == iei){
                *ie = optionals + i;
                return;
            }
        }else{
            /*IEI full byte*/
            if(optionals[i].iei == iei){
                *ie = optionals + i;
                return;
            }
        }
    }
}
