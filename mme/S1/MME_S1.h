/* This application was initially developed as a Final Project by
 *     Vicent Ferrer Guasch (vicent.ferrerguasch@aalto.fi)
 * under the supervision of,
 *     Jukka Manner (jukka.manner@aalto.fi)
 *     Jose Costa-Requena (jose.costa@aalto.fi)
 * in AALTO University and partially funded by EIT ICT labs.
 */

/**
 * @file   MME_S1.h
 * @Author Vicent Ferrer
 * @date   May, 2013
 * @brief  MME S1 interface protocol state machine.
 *
 * This module implements the S1 interface state machine.
 */

#ifndef MME_S1_HFILE
#define MME_S1_HFILE

#include "S1AP.h"
#include "MME.h"
#include "NAS_FSM.h"
#include <netinet/sctp.h>

#define S1AP_NONUESIGNALING_STREAM0 0


/* ======================================================================
 * S1 Type definitions
 * ====================================================================== */


typedef struct S1_EndPoint_Info_c{
    ENBname_t               *eNBname;
    Global_ENB_ID_t         *global_eNB_ID;
    SupportedTAs_t          *suportedTAs;
    CSG_IdList_t            *cGS_IdList;
    struct sctp_sndrcvinfo  sndrcvinfo;
}S1_EndPoint_Info_t;

/**@brief S1_EndPoint_Info_t destructor
 * @param [in] epInfo   Structure to be deallocated
 *
 * This function send the S1 message using the SCTP protocol
 * */
void free_S1_EndPoint_Info(S1_EndPoint_Info_t* epInfo);


/* ======================================================================
 * S1 Tool API
 * ====================================================================== */


/**@brief S1 Send message
 * @param [in] ep_S1    Destination EndPoint information
 * @param [in] streamId Strem to send the message
 * @param [in] s1msg    Message to be sent
 *
 * This function send the S1 message using the SCTP protocol
 * */
void s1_sendmsg(struct EndpointStruct_t* ep_S1, uint32_t streamId, S1AP_Message_t *s1msg);


/* ======================================================================
 * S1 MME State Machine API
 * ====================================================================== */


/**@brief S1 Setup Procedure function
 * @param [in] engine   Engine reference
 * @param [in] owner    parent process
 * @param [in] ep       Endpoint information of the eNB to be setup
 *
 * This function Allocates a session and a process for the S1 Setup Procedure
 * */
struct t_process *S1_Setup(struct t_engine_data *self, struct t_process *owner, struct EndpointStruct_t *ep_S1);

/**@brief new user processing
 * @param [in] engine   Engine reference
 * @param [in] ep_S1    Peer Endpoint
 * @param [in] s1msg    S1AP message received
 *
 * Used to create a session structure on S1 State machine due to new user info message.
 */
void S1_newUserSession(struct t_engine_data *engine, struct EndpointStruct_t* ep_S1, S1AP_Message_t *s1msg);

/**@brief Trigger UE context Release
 * @param [in] session Session struct of the current user
 */
void S1_UEContextRelease(struct SessionStruct_t *session);

#endif /* MME_S1_HFILE */
