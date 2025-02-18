#include "conn.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/net.h>
#include <linux/in.h>
#include <linux/socket.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <rdma/ib_verbs.h>



int conn_server_kernel(char *addr, int port, struct ibv_qp_info *local_qp_info,
                       struct ibv_qp_info *remote_qp_info)
{
    struct socket *sock;
    struct sockaddr_in server_addr;
    int ret;

    // 创建套接字
    ret = sock_create_kern(&init_net, AF_INET, SOCK_STREAM, IPPROTO_TCP, &sock);
    if (ret < 0) {
        pr_err("Socket creation failed: %d\n", ret);
        return ret;
    }

    // 设置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = in_aton(addr);

    // 连接到服务器
    ret = kernel_connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr), 0);
    if (ret < 0) {
        pr_err("Connection failed: %d\n", ret);
        sock_release(sock);
        return ret;
    }
    // 发送本端QP信息
    pr_info("Sending local QP info:qpn:%d,interface id:%d,subnet:%d,size:%d\n",
        local_qp_info->qpn,
        local_qp_info->gid.global.interface_id,
        local_qp_info->gid.global.subnet_prefix,
        sizeof(*local_qp_info));
    ret = kernel_sendmsg(sock, &(struct msghdr) {
        .msg_flags = MSG_NOSIGNAL,
    }, &(struct kvec) {
        .iov_base = local_qp_info,
        .iov_len = sizeof(*local_qp_info),
    }, 1, sizeof(*local_qp_info));
    if (ret < 0) {
        pr_err("Send failed: %d\n", ret);
        sock_release(sock);
        return ret;
    }

    // ret = kernel_sendmsg(sock, &(struct msghdr) {
    //     .msg_flags = MSG_NOSIGNAL,
    // }, &(struct kvec) {
    //     .iov_base = str,
    //     .iov_len = 5,
    // }, 1, 5);
    // if (ret < 0) {
    //     pr_err("Send failed: %d\n", ret);
    //     sock_release(sock);
    //     return ret;
    // }

    // 接收对端QP信息
    ret = kernel_recvmsg(sock, &(struct msghdr) {
        .msg_flags = MSG_NOSIGNAL,
    }, &(struct kvec) {
        .iov_base = remote_qp_info,
        .iov_len = sizeof(*remote_qp_info),
    }, 1, sizeof(*remote_qp_info), MSG_WAITALL);
    if (ret < 0) {
        pr_err("Receive failed: %d\n", ret);
        sock_release(sock);
        return ret;
    }

    // 打印对端QP信息
    // pr_info("Received remote QP info:\n");
    // print_qp_info(remote_qp_info);

    // 关闭连接
    sock_release(sock);
    return 0;
}