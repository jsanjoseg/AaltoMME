/* This application was initially developed as a Final Project by
 *     Vicent Ferrer Guasch (vicent.ferrerguasch@aalto.fi)
 * under the supervision of,
 *     Jukka Manner (jukka.manner@aalto.fi)
 *     Jose Costa-Requena (jose.costa@aalto.fi)
 * in AALTO University and partially funded by EIT ICT labs.
 */

/**
 * @file   ECMSession_Idle.c
 * @Author Vicent Ferrer
 * @date   August, 2015
 * @brief  ECM Session Idle State
 *
 */

#include "ECMSession_Idle.h"
#include "MME.h"
#include "logmgr.h"
#include "MME_S1_priv.h"
#include "ECMSession_FSMConfig.h"
#include "ECMSession.h"
#include "ECMSession_priv.h"
#include "NAS_EMM.h"
#include "S1Assoc_priv.h"
#include "NAS_Definitions.h"

static void ecm_processMsg(gpointer _ecm, S1AP_Message_t *s1msg, int r_sid){
    ECMSession_t *ecm = (ECMSession_t *)_ecm;

    ENB_UE_S1AP_ID_t *eNB_ID;
    Unconstrained_Octed_String_t *nASPDU;
    TAI_t *tAI;
    EUTRAN_CGI_t *eCGI;
    RRC_Establishment_Cause_t *cause;
    guti_t guti;
    struct mme_t * mme = s1_getMME(s1Assoc_getS1(ecm->assoc));
    memset(&guti, 0, sizeof(guti_t));

    if(s1msg->pdu->procedureCode == id_initialUEMessage &&
       s1msg->choice == initiating_message){

        eNB_ID = (ENB_UE_S1AP_ID_t*)s1ap_findIe(s1msg, id_eNB_UE_S1AP_ID);
        ecm->eNBUEId = eNB_ID->eNB_id;

        nASPDU = (Unconstrained_Octed_String_t*)s1ap_findIe(s1msg, id_NAS_PDU);

        tAI = (TAI_t*)s1ap_findIe(s1msg, id_TAI);
        memcpy(ecm->tAI.sn, tAI->pLMNidentity->tbc.s, 3);
        memcpy(&(ecm->tAI.tAC), tAI->tAC->s, 2);
        eCGI = (EUTRAN_CGI_t*)s1ap_findIe(s1msg, id_EUTRAN_CGI);
        cause = (RRC_Establishment_Cause_t*)s1ap_findIe(s1msg,
                                                        id_RRC_Establishment_Cause);

        ecm->l_sid = 1;
        if(r_sid != 0){
            ecm->r_sid_valid = TRUE;
            ecm->r_sid = r_sid;
        }

        if(cause->cause.noext == RRC_mo_Signalling){
            /* Attach, Detach, TAU*/
            emm_getGUTIfromMsg(nASPDU->str, nASPDU->len, &guti);
            if(guti.mtmsi != 0 && mme_GUMMEI_IsLocal(mme, guti.tbcd_plmn, guti.mmegi, guti.mmec)){
                mme_lookupEMMCtxt(mme, guti.mtmsi, &(ecm->emm));
            }
        }else if(cause->cause.noext == RRC_mo_Data){
            /* Service Request: User Plane Radio, Uplink signaling
             * Extended Service Request: CS fallback*/
            //sTMSI = (S_TMSI_t*)s1ap_findIe(s1msg, id_S_TMSI);

        }

        if(ecm->emm == NULL){
            ecm->emm = emm_init(ecm);
        }else{
            emm_registerECM(ecm->emm, ecm);
        }

        emm_processMsg(ecm->emm, nASPDU->str, nASPDU->len);
        ecm_ChangeState(ecm, ECM_Connected);
    }
}

static void release(gpointer _ecm, cause_choice_t choice, uint32_t cause){

}

void linkECMSessionIdle(ECMSession_State* s){
    s->processMsg = ecm_processMsg;
    s->release = release;
}
