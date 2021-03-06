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
 * @file   S1AP test.c
 * @Author Vicent Ferrer
 * @date   April, 2013
 * @brief
 *
 *
 *  */

#include <stdio.h>
#include <string.h>
#include "S1AP.h"

#define MAX_FILE_SIZE 10000

int main (int argc, char **argv){
    S1AP_Message_t *msg, *msg1;
    /*S1AP_PROTOCOL_IES_t *newIe;
    CSG_IdList_t *csglist;
    CSG_IdList_Item_t *csgItem;*/
    FILE *f;
    unsigned char buffer[MAX_FILE_SIZE], res[MAX_FILE_SIZE];
    uint32_t n, size=0;

    if(argc!=2){
        printf("Number of parameters incorrect %d\n\tS1APtest filename\n", argc);
        return 1;
    }
    info();
    f = fopen(argv[1], "rb");
    if (f){
        n = fread(buffer, 1, MAX_FILE_SIZE, f);
    }
    else{
        printf("error opening file\n");
        return 1;
    }
    fclose(f);

    /*
    printf("The buffer looks like: n %u\n", 16);
    for( i = 0; i < 16; i++) {
        printf("%#x ", buffer[i]);
        if(i%16==15){
            printf("\n");
        }
    }*/
    printf("n = %u\n", n);
    msg = s1ap_decode((void *)buffer, n);

    /*test = ((Global_ENB_ID_t*)msg->pdu->value->elem[0]->value)->pLMNidentity->tbc.s;*/

    /*printf("\ntest pLMNidentity = %#x%x%x\n", test[0], test[1], test[2]);*/


    /*newIe = newProtocolIE();
    msg->pdu->value->addIe(msg->pdu->value, newIe);

    csglist = new_CSG_IdList();
    newIe->id=id_CSG_IdList;
    newIe->value = csglist;
    newIe->freeValue= csglist->freeIE;
    newIe->showValue=csglist->showIE;

    csgItem = new_CSG_IdList_Item();
    csglist->additem(csglist, csgItem);

    csgItem->cSG_id = new_CSG_id();
    csgItem->cSG_id->id=0x7ffffff;  *//*Only 27 bits*/

    /*Show message*/
    msg->showmsg(msg);
    /*Encode Message*/
    memset(res, 0, MAX_FILE_SIZE);
    s1ap_encode(res, &size, msg);

    f=fopen("test.s1","wb");
    if (!f)
    {
        printf("Unable to open file!");
        return 1;
    }
    fwrite(res, size, 1, f);
    fclose(f);

    msg1 = s1ap_decode((void *)res, size);
    /*Show message*/
    msg1->showmsg(msg1);

    msg->freemsg(msg);
    msg1->freemsg(msg1);
    return 0;
}
