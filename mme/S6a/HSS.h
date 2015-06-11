/* This application was initially developed as a Final Project by
 *     Vicent Ferrer Guasch (vicent.ferrerguasch@aalto.fi)
 * under the supervision of,
 *     Jukka Manner (jukka.manner@aalto.fi)
 *     Jose Costa-Requena (jose.costa@aalto.fi)
 * in AALTO University and partially funded by EIT ICT labs.
 */

/**
 * @file   HSS.h
 * @Author Vicent Ferrer
 * @date   June, 2013
 * @brief  Functions to access to HSS database
 *
 * The current database is a MariaDB.
 */

#include "MME.h"

/* Functions Called from the MME initialize and destroy methods*/
int init_hss();

void disconnect_hss();

void HSS_getAuthVec(struct user_ctx_t *user);

void HSS_UpdateLocation(struct user_ctx_t *user, ServedGUMMEIs_t * sGUMMEIs);

void HSS_syncAuthVec(struct user_ctx_t *user, uint8_t * auts);
