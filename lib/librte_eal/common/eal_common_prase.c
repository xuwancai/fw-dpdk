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
#include <rte_prase.h>

#include "eal_internal_cfg.h"
#include "eal_options.h"
#include "eal_filesystem.h"


struct rte_prase_config rte_prase_config;

#define CONFIG_FILE           "config.db"
#define CONFIG_SPLIT          ':'

#define CONFIG_COUNT_SPLIT    "COUNT"

#define CONFIG_DISPATCHER     "Dispatcher"
#define CONFIG_PHY_PORT       "PhysicalPort"
#define CONFIG_APP_MAIN       "AppMain"

/* Return a pointer to the prase configuration structure */
struct rte_prase_config *
rte_eal_get_prase_config(void)
{
	return &rte_prase_config;
}


static uint8_t
rte_prase_strsplit(char* buf, char split, uint8_t* store)
{
	uint8_t num = 0;
	uint8_t data = 0;

	while (*buf != '\0') {
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
    #define DISPATCHER_ID 			"DispatcherID"
    #define DISPATCHER_AFF_CORE 	"DispatcherAffinityCore"
    #define DISPATCHER_PHY_PORT 	"DispatcherPhysicalPort"
    
    char *data;
    char *end;
	char *buf_ptr;
	char buf[BUFSIZ];
	int index;
	struct rte_dispatcher_config *dispatcher_conf = NULL;
	
	if ((data = strchr(root, CONFIG_SPLIT)) == NULL) {
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
		buf_ptr = buf;
	    while(isspace(buf_ptr))
			++buf_ptr;

		/* 必须保证每个配置项都有换行 */
		if (*buf == '\r' || *buf == '\n')
			break;
		

        /* 忽略注释 */  
		if (*buf_ptr == '#')
			continue;
			
		if ((data = strchr(buf_ptr, CONFIG_SPLIT)) == NULL) {
			RTE_LOG(ERR, EAL, "%s(): cannot prase dispatch item: %s\n",
				__func__, buf_ptr);
			return -1;
		}
		while(isspace(++data));

	    if (strstr(buf_ptr, DISPATCHER_ID)) {
		    index = strtoul(data, NULL, 0);
		    if (index > rte_prase_config.rte_dispatcher_count) {
		    	RTE_LOG(ERR, EAL, "%s(): parse dispatch index %d\n",
					__func__, index);
				return -1;
		    }
		    dispatcher_conf = rte_prase_config.dispatcher + index;
		    continue;
	    }

	    if (strstr(buf_ptr, DISPATCHER_AFF_CORE)) {
		    dispatcher_conf->affinity_core = strtoul(data, NULL, 0);
		    continue;
	    }

	    if (strstr(buf_ptr, DISPATCHER_PHY_PORT)) {
		    dispatcher_conf->phy_port_count = rte_prase_strsplit(data, ',', dispatcher_conf->phy_port);
		    continue;
	    }
	}	

    return 0;
}

static int
rte_eal_prase_physical_port(FILE *f, char* root) 
{
    #define PHY_PORT_ID 		"PhysicalPortID"
    #define RX_QUEUE_NUM 		"RxQueueNum"
    #define TX_QUEUE_NUM 		"TxQueueNum"
    
    char *data;
    char *end;
	char *buf_ptr;
	char buf[BUFSIZ];
	int index;
	struct rte_phy_port_config *phy_port_conf = NULL;
	
	if ((data = strchr(root, CONFIG_SPLIT)) == NULL) {
		RTE_LOG(ERR, EAL, "%s(): cannot prase dispatch count: %s\n",
			__func__, root);
		return -1;
	}
	
	rte_prase_config.rte_phy_port_count = strtoul(++data, &end, 0);
	if ((data[0] == '\0') || (end == NULL) || (*end != '\n') || 
		rte_prase_config.rte_phy_port_count > RTE_MAX_ETHPORTS) {
		RTE_LOG(ERR, EAL, "%s(): cannot parse phy nic count value %s\n",
				__func__, data);
		return -1;
	}
	
	while (fgets(buf, sizeof(buf), f) != NULL) {
		buf_ptr = buf;
	    while(isspace(buf_ptr))
		    ++buf_ptr;

		/* 必须保证每个配置项都有换行 */
		if (*buf == '\r' || *buf == '\n')
			break;

        /* 忽略注释 */  
		if (*buf_ptr == '#')
			continue;
			
		if ((data = strchr(buf_ptr, CONFIG_SPLIT)) == NULL) {
			RTE_LOG(ERR, EAL, "%s(): cannot prase dispatch item: %s\n",
				__func__, buf_ptr);
			return -1;
		}
		while(isspace(++data));
		
	    if (strstr(buf_ptr, PHY_PORT_ID)) {
		    index = strtoul(data, NULL, 0);
		    if (index > rte_prase_config.rte_phy_port_count) {
		    	RTE_LOG(ERR, EAL, "%s(): parse dispatch index %d\n",
					__func__, index);
				return -1;
		    }
		    phy_port_conf = rte_prase_config.phy_port + index;
		    continue;
	    }

	    if (strstr(buf_ptr, RX_QUEUE_NUM)) {
		    phy_port_conf->rx_queue_num = strtoul(data, NULL, 0);
		    continue;
	    }

	    if (strstr(buf_ptr, TX_QUEUE_NUM)) {
		    phy_port_conf->tx_queue_num = strtoul(data, NULL, 0);
		    continue;
	    }
	}	

    return 0;
}

static int
rte_eal_prase_app_main(__attribute__((unused)) FILE *f, char* root) 
{ 
    char *data;
    char *end;
	
	if ((data = strchr(root, CONFIG_SPLIT)) == NULL) {
		RTE_LOG(ERR, EAL, "%s(): cannot prase dispatch count: %s\n",
			__func__, root);
		return -1;
	}
	
	rte_prase_config.rte_appmain_count = strtoul(++data, &end, 0);
	if ((data[0] == '\0') || (end == NULL) || (*end != '\n')) {
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
	char *buf_ptr;
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
		buf_ptr = buf;
	    while(isspace(buf_ptr))
		    ++buf_ptr;

        /* 忽略注释 */  
		if (*buf_ptr == '#')
			continue;

        /* 错误的配置 */ 
        if (strstr(buf_ptr, CONFIG_COUNT_SPLIT) == NULL)
	        RTE_LOG(ERR, EAL, "%s(): cannot prase config: %s\n",
				__func__, buf_ptr);
	        continue;

	    if (strstr(buf_ptr, CONFIG_DISPATCHER) &&
		    rte_eal_prase_dispatch(f, buf_ptr)) {
			RTE_LOG(ERR, EAL, "%s(): cannot prase dispatch config: %s\n",
				__func__, buf_ptr);
			fclose(f);
		    return -1;
	    } else if (strstr(buf_ptr, CONFIG_PHY_PORT) &&
		    rte_eal_prase_physical_port(f, buf_ptr)) {
		    RTE_LOG(ERR, EAL, "%s(): cannot prase pyhsical port config: %s\n",
				__func__, buf_ptr);
			fclose(f);
		    return -1;
	    } else if (strstr(buf_ptr, CONFIG_APP_MAIN) &&
	        rte_eal_prase_app_main(f, buf_ptr)) {
		    RTE_LOG(ERR, EAL, "%s(): cannot prase app main config: %s\n",
				__func__, buf_ptr);
			fclose(f);
		    return -1;
	    } else {
			RTE_LOG(ERR, EAL, "%s(): cannot prase config: %s\n", __func__, buf_ptr);
			return -1;
		}
    }

	return 0;	
}

