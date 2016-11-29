/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2010-2014 Intel Corporation. All rights reserved.
 *   Copyright(c) 2014 6WIND S.A.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <getopt.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <rte_eal.h>
#include <rte_log.h>
#include <rte_lcore.h>
#include <rte_version.h>
#include <rte_devargs.h>
#include <rte_memcpy.h>

#include "eal_internal_cfg.h"
#include "eal_options.h"
#include "eal_filesystem.h"
#include "eal_prase.h"

struct rte_prase_config rte_prase_config;

#define CONFIG_FILE           "config.db"
#define CONFIG_SPLIT          ':'

#define CONFIG_COUNT_SPLIT    "COUNT"

#define CONFIG_DISPATCHER     "Dispatcher"
#define CONFIG_PHYSICAL_NIC   "Physical nic"
#define CONFIG_APP_MAIN       "App_Main"

static int
rte_prase_strsplit(char* buf, char split, uint8* store)
{
	uint8 num = 0;
	uint8 data = 0;

	while (*buf != NULL) {
       if (*buf == split) {
          *(store + num++) = data;
          data = 0;
       }
       data = data * 10 + *buf - '0';
       buf++;
	}
	
	*(store + num++) = data;
	
	return num;
}

static int
rte_eal_prase_dispatch(FILE *f, char* root) 
{
    #define DISPATCHER_INDEX "Dispatcher index"
    #define DISPATCHER_CORE "Dispatcher affinity core"
    #define DISPATCHER_PORT "Dispatcher physical port"
    
    char *data;
    char *end;
	char buf[BUFSIZ];
	int index;
	struct rte_dispatcher_config *dispatcher_conf = NULL;
	
	if (data = strchr(root, CONFIG_SPLIT)) {
		RTE_LOG(ERR, EAL, "%s(): cannot prase dispatch count: %s\n",
			__func__, root);
		return -1;
	}
	
	rte_prase_config.rte_dispatcher_count = strtoul(++data, &end, 0);
	if ((data[0] == '\0') || (end == NULL) || (*end != '\n') || 
		rte_prase_config.rte_dispatcher_count > RTE_MAX_DISPATCHERS) {
		RTE_LOG(ERR, EAL, "%s(): cannot parse dispatch count value %s\n",
				__func__, data);
		return -1;
	}
	
	while (fgets(buf, sizeof(buf), f) != NULL) {
	    while(isspace(buf))
		    buf++;

        /* ºöÂÔ×¢ÊÍ */  
		if (*buf == '#')
			continue;
			
		if (data = strchr(buf, CONFIG_SPLIT)) {
			RTE_LOG(ERR, EAL, "%s(): cannot prase dispatch item: %s\n",
				__func__, buf);
			return -1;
		}
		while(isspace(++data));

	    if (strstr(buf, DISPATCHER_INDEX)) {
		    index = strtoul(data, NULL, 0);
		    if (index > rte_prase_config.rte_dispatcher_count) {
		    	RTE_LOG(ERR, EAL, "%s(): parse dispatch index %d\n",
					__func__, index);
				return -1;
		    }
		    dispatcher_conf = rte_prase_config.dispatcher + index;
		    continue;
	    }

	    if (strstr(buf, DISPATCHER_CORE)) {
		    dispatcher_conf->affinity_core = strtoul(data, NULL, 0);
		    continue;
	    }

	    if (strstr(buf, DISPATCHER_PORT)) {
		    dispatcher_conf->phy_port_count = rte_prase_strsplit(data, ',', dispatcher_conf->phy_port);
		    continue;
	    }
	}	

    return 0;
}

static int
rte_eal_prase_physical_nic(FILE *f, char* root) 
{
    #define PYHSICAL_NIC_INDEX "Physical nic index"
    #define RX_QUEUE_NUM "Rx queue num"
    #define TX_QUEUE_NUM "Tx queue num"
    
    char *data;
    char *end;
	char buf[BUFSIZ];
	int index;
	struct rte_phy_nic_config *phy_nic_conf = NULL;
	
	if (data = strchr(root, CONFIG_SPLIT)) {
		RTE_LOG(ERR, EAL, "%s(): cannot prase dispatch count: %s\n",
			__func__, root);
		return -1;
	}
	
	rte_prase_config.rte_phy_nic_count = strtoul(++data, &end, 0);
	if ((data[0] == '\0') || (end == NULL) || (*end != '\n') || 
		rte_prase_config.rte_phy_nic_count > RTE_MAX_ETHPORTS) {
		RTE_LOG(ERR, EAL, "%s(): cannot parse phy nic count value %s\n",
				__func__, data);
		return -1;
	}
	
	while (fgets(buf, sizeof(buf), f) != NULL) {
	    while(isspace(buf))
		    buf++;

        /* ºöÂÔ×¢ÊÍ */  
		if (*buf == '#')
			continue;
			
		if (data = strchr(buf, CONFIG_SPLIT)) {
			RTE_LOG(ERR, EAL, "%s(): cannot prase dispatch item: %s\n",
				__func__, buf);
			return -1;
		}
		while(isspace(++data));
		
	    if (strstr(buf, PYHSICAL_NIC_INDEX)) {
		    index = strtoul(data, NULL, 0);
		    if (index > rte_prase_config.rte_phy_nic_count) {
		    	RTE_LOG(ERR, EAL, "%s(): parse dispatch index %d\n",
					__func__, index);
				return -1;
		    }
		    phy_nic_conf = rte_prase_config.phy_nic + index;
		    continue;
	    }

	    if (strstr(buf, RX_QUEUE_NUM)) {
		    phy_nic_conf->rx_queue_num = strtoul(data, NULL, 0);
		    continue;
	    }

	    if (strstr(buf, TX_QUEUE_NUM)) {
		    phy_nic_conf->tx_queue_num = strtoul(data, NULL, 0);
		    continue;
	    }
	}	

    return 0;
}

static int
rte_eal_prase_app_main(FILE *f, char* root) 
{ 
    char *data;
    char *end;
	int index;
	
	if (data = strchr(root, CONFIG_SPLIT)) {
		RTE_LOG(ERR, EAL, "%s(): cannot prase dispatch count: %s\n",
			__func__, root);
		return -1;
	}
	
	rte_prase_config.rte_appmain_count = strtoul(++data, &end, 0);
	if ((data[0] == '\0') || (end == NULL) || (*end != '\n') || 
		rte_prase_config.rte_phy_nic_count > RTE_MAX_ETHPORTS) {
		RTE_LOG(ERR, EAL, "%s(): cannot parse appmain count value %s\n",
				__func__, data);
		return -1;
	}	

    return 0;
}

int
rte_eal_prase_config_file(void)
{
	FILE *f;
	char buf[BUFSIZ];
	
	if (access(CONFIG_FILE, F_OK)) {
		RTE_LOG(ERR, EAL, "%s(): cannot access prase config file: %s\n",
			__func__, CONFIG_FILE);
        return -1;
	}

	if ((f = fopen(CONFIG_FILE, "r")) == NULL) {
		RTE_LOG(ERR, EAL, "%s(): cannot open prase config file: %s\n",
			__func__, CONFIG_FILE);
		return -1;
	}

    while (fgets(buf, sizeof(buf), f) != NULL) {
	    while(isspace(buf))
		    buf++;

        /* ºöÂÔ×¢ÊÍ */  
		if (*buf == '#')
			continue;

        /* ´íÎóµÄÅäÖÃ */ 
        if (strstr(buf, CONFIG_COUNT_SPLIT) == NULL)
	        RTE_LOG(ERR, EAL, "%s(): cannot prase config: %s\n",
				__func__, buf);
	        continue;

	    if (strstr(buf, CONFIG_DISPATCHER) &&
		    rte_eal_prase_dispatch(f, buf)) {
			RTE_LOG(ERR, EAL, "%s(): cannot prase dispatch config: %s\n",
				__func__, buf);
			fclose(f);
		    return -1;
	    }

	    if (strstr(buf, CONFIG_PHYSICAL_NIC) &&
		    rte_eal_prase_physical_nic(f, buf)) {
		    RTE_LOG(ERR, EAL, "%s(): cannot prase pyhsical nic config: %s\n",
				__func__, buf);
			fclose(f);
		    return -1;
	    }

	    if (strstr(buf, CONFIG_APP_MAIN) &&
	        rte_eal_prase_app_main(f, buf)) {
		    RTE_LOG(ERR, EAL, "%s(): cannot prase app main config: %s\n",
				__func__, buf);
			fclose(f);
		    return -1;
	    }
    }

	return 0;	
}



