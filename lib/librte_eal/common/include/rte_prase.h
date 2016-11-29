/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2010-2014 Intel Corporation. All rights reserved.
 *   All rights reserved.
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

/**
 * @file
 * Holds the structures for the eal prase configuration
 */

#ifndef RTE_PRASE_H
#define RTE_PRASE_H

struct rte_dispatcher_config {
	uint8_t affinity_core;
	uint8_t phy_port_count;
	uint8_t phy_port[RTE_MAX_ETHPORTS];
};

struct rte_phy_port_config {
	//struct rte_pci_addr pci_addr;
	uint8_t rx_queue_num;
	uint8_t tx_queue_num;
};

#define KIN_NAME_MAX (32)
struct rte_kni_config {
	char name[KIN_NAME_MAX];
	uint8_t rx_queue;
	uint8_t tx_queue;
};

/**
 * prase custom global configuration
 */
struct rte_prase_config {
    /* 根据appmain count确定dispatcher与appmain创建的队列数 */
	uint8_t rte_appmain_count;

	/* dispatcher配置信息，主要用于启动进程数量和接收端口数量 */
	uint8_t rte_dispatcher_count;
    struct rte_dispatcher_config dispatcher[RTE_MAX_DISPATCHERS];

	/* 物理端口配置信息，初始化端口属性 */
	uint8_t rte_phy_port_count;
	struct rte_phy_port_config phy_port[RTE_MAX_ETHPORTS];

    /* 确定kni是否需要在此处配置，确认虚拟网卡是否有用 */
	//uint8_t rte_kni_count;
	//struct rte_kni_config kni_port[RTE_MAX_KNIPORTS];
};

/**
 * 解析配置文件，存储到rte_global_config中
 */
int rte_eal_prase_config_file(void);

struct rte_prase_config *rte_eal_get_prase_config(void);


#endif /* RTE_PRASE_H */
