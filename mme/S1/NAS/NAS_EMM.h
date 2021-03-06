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
 * @file   NAS_EMM.h
 * @Author Vicent Ferrer
 * @date   August, 2015
 * @brief  NAS FSM header
 *
 * This module implements the NAS EMM interface state machine on the MME EndPoint.
 */

#ifndef NAS_EMM_H
#define NAS_EMM_H

#include "MME.h"
#include "S1AP.h"
#include "ECMSession_priv.h"
#include "timermgr.h"
#include "EMMCtx_iface.h"
#include <glib.h>

/**
 * @brief NAS EMM constructor
 * @param [in]  emm EMM stack handler
 * @param [in]  tm  Timer manager
 * @return emm stack handler
 *
 *  Allocates the EMM stack. Use emm_free to delete the structure.
 */
gpointer emm_init(gpointer ecm, TimerMgr tm);

/**
 * @brief Dealocates the EMM stack handler
 * @param [in]  emm_h EMM stack handler to be removed.
 */
void emm_free(gpointer emm_h);


void emm_registerECM(EMMCtx emm_h, gpointer ecm);

void emm_deregister(EMMCtx emm_h);


/**
 * @brief Stops EMM
 * @param [in]  emm_h EMM stack handler.
 *
 * This function is used by the lower layers to change the EMM to Deregitered.
 * Used for resets or lower layer errors, it detaches the user without signaling and
 * forwards the error to ESM layer
 */
void emm_stop(EMMCtx emm_h);

/**
 * @brief NAS processing function
 * @param [in]  emm_h EMM Stack handler
 * @param [in]  msg pointer to the message to be processed
 * @param [in]  len message lenght
 *
 *  Function to process the EMM message. Used by the lower layer EMM.
 */
void emm_processMsg(gpointer emm_h, gpointer msg, gsize len);

void emm_getGUTIfromMsg(gpointer buffer, gsize len, guti_t* guti);

void emm_getIMSIfromAttach(gpointer buffer, gsize len, guint64* imsi);

/**
 * @brief get KeNB
 * @param [in]   emm_h EMM Stack handler
 * @param [out]  kasme    derived key - 256 bits
 */
void emm_getKeNB(const EMMCtx emm, uint8_t *keNB);

/**
 * @brief get Next Hop
 * @param [in]   emm_h EMM Stack handler
 * @param [out]  nh    Next Hop derived key - 256 bits
 * @param [out]  ncc   Next Hop Chaining Count - 3 bits
 */
void emm_getNH(const EMMCtx emm, guint8 *nh, guint8 *ncc);

void emm_getUESecurityCapabilities(const EMMCtx emm, UESecurityCapabilities_t *cap);

void emm_getUEAMBR(const EMMCtx emm, UEAggregateMaximumBitrate_t *ambr);

void emm_modifyE_RABList(EMMCtx emm,  E_RABsToBeModified_t* l,
                         void (*cb)(gpointer), gpointer args);

/* void emm_setE_RABSetupuListCtxtSURes(EMMCtx emm, E_RABSetupListCtxtSURes_t* l); */

/* void emm_setE_RABToBeSwitchedDLList(EMMCtx emm, E_RABToBeSwitchedDLList_t* l); */

void emm_UEContextReleaseReq(EMMCtx emm, void (*cb)(gpointer), gpointer args);

guint32 *emm_getM_TMSI_p(EMMCtx emm);

void emm_triggerAKAprocedure(EMMCtx emm_h);

void emm_getEPSSessions(EMMCtx emm_h, GList **sessions);

void emm_getBearers(EMMCtx emm_h, GList **bearers);

const guint64 emm_getIMSI(const EMMCtx emm_h);

#endif /* NAS_EMM_H */
