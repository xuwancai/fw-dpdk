/*
 * init_dispatcher_conf: init the global var dc via xml file
 * file: the xml file with full path
 * return: 0:successed, -1: failed
 */
int init_dispatcher_conf(char *file)
{
	int i = 0;
	xmlDocPtr doc;
	xmlNodePtr node = NULL, tmpnode;
	xmlChar *value = NULL;
	xmlChar *value1 = NULL;
	int core_num = sysconf(_SC_NPROCESSORS_ONLN);
	int app_num = 0;
	int dis_num = 0;
	u_int8_t bind[32] = {0};

	dc = (struct dispatcher_conf *)malloc(sizeof(struct dispatcher_conf));
	if (dc == NULL) {
		fprintf(stderr, "init dispacther_conf failed! not enough memory!\n");
		return -1;
	}
	memset(dc, 0, sizeof(struct dispatcher_conf));

	if (-1 == open_xml_file(&doc, file)) {
		return -1;
	}

	if (-1 == move_to_dst_node(&doc, &node, XML_NODE_DISPATCHER_CONF)){
		xmlFreeDoc(doc);
		return -1;
	}

	value = get_content_by_name(&node, "MainThreadOnCpu");
	if (NULL != value)
		dc->process_affinity = atoi((char *)value);

	value = get_content_by_name(&node, "ThreadNum");
	if (NULL != value)
		dc->thread_num = atoi((char *)value);

	value = get_content_by_name(&node, "MultipleTask");
	if (NULL != value)
		dc->multiple_task = atoi((char *)value);

	if (!get_child_node_by_name(&node, &tmpnode, "WorkThreadToCpu")) {
		while (NULL != tmpnode) {
			value = get_child_node_value_by_name(&tmpnode, "threadid");
			value1 = get_child_node_value_by_name(&tmpnode, "coreid");
			if (NULL != value && NULL != value1)
				dc->thread_to_cpucore[atoi((char *)value)] = atoi((char *)value1);
			tmpnode = tmpnode->next;
		}
	}

	if (!get_child_node_by_name(&node, &tmpnode, "NetIfToThread")) {
		while (NULL != tmpnode) {
			value = get_child_node_value_by_name(&tmpnode, "threadid");
			value1 = get_child_node_value_by_name(&tmpnode, "nics");
			if (NULL != value && NULL != value1) {
				strncpy(dc->netif_to_thread[atoi((char *)value)], (char *)value1,
						sizeof(dc->netif_to_thread[atoi((char *)value)]));
			}
			tmpnode = tmpnode->next;
		}
	}

	value = get_content_by_name(&node, "skbheadernum");
	if (NULL != value)
		dc->netmap_skb_header_num = atoi((char *)value);

	value = get_content_by_name(&node, "polltimeout");
	if (NULL != value)
		dc->netmap_poll_timeout = atoi((char *)value);

	value = get_content_by_name(&node, "policy");
	if (NULL != value)
		dc->policy = atoi((char *)value);

	value = get_content_by_name(&node, "ntuple");
	if (NULL != value)
		dc->n_tuple = atoi((char *)value);

	dc->phy_cpu_num = nm_get_phy_cpu_num();
	if (0 == nm_get_core_mask(&app_num, &dis_num, bind, 0)) {
		dc->multiple_task = app_num;
		dc->thread_num = dis_num;
		for (i = 0; i < dc->thread_num; i++) {
			dc->thread_to_cpucore[i] = bind[i];
			(dc->thread_num_cpu[bind[i] / (core_num / dc->phy_cpu_num)])++;
		}

		bzero(bind, sizeof(bind));
		app_num = 0;
		dis_num = 0;
		nm_get_core_mask(&app_num, &dis_num, bind, 1);
		for (i = 1; i <= app_num; i++) {
			(dc->multiple_task_cpu[bind[i] / (core_num / dc->phy_cpu_num)])++;
		}
	}

	free_xml_item(2, value, value1);
	xmlFreeDoc(doc);
	xmlCleanupParser();
	return 0;
}
