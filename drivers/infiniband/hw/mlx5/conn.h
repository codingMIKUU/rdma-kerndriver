#ifndef IB_INFINIBAND_CONN_H
#define IB_INFINIBAND_CONN_H
#include<rdma/ib_verbs.h>
struct ibv_qp_info {
    uint32_t qpn;
    union ib_gid gid;
	char rconn_server[64];
};
int conn_server(char *addr, int port,struct ibv_qp_info *local_qp_info,struct ibv_qp_info *remote_qp_info);


#endif /* INFINIBAND_CONN_H */