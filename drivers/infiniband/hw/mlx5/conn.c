#include "conn.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/net.h>
#include <linux/in.h>
#include <linux/inet.h>
#include <linux/socket.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <rdma/ib_verbs.h>
#include <linux/delay.h>


int conn_server_kernel(char *addr, int port, struct ibv_qp_info *local_qp_info,
                       struct ibv_qp_info *remote_qp_info)
{
    struct socket *sock;
    struct sockaddr_in server_addr;
    sock = (struct socket *)kmalloc(sizeof(struct socket), GFP_KERNEL);
    int ret;
    char send_buf[] = "good";
    char recv_buf[100]; // 假设接收的字符串不会超过 100 字节
    
    
    // 创建套接字
    ret = sock_create_kern(&init_net, AF_INET, SOCK_STREAM, IPPROTO_TCP, &sock);
    if (ret < 0) {
        pr_err("Socket creation failed: %d\n", ret);
        return ret;
    }

    // // 设置服务器地址
    // memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = in_aton(addr);

    // ret=sock->ops->connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr), 0);
    // if (ret < 0) {
    //     pr_err("Connection failed: %d\n", ret);
    //     sock_release(sock);
    //     return ret;
    // }
    // 连接到服务器
    ret = kernel_connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr), 0);
    if (ret < 0) {
        pr_err("Connection failed: %d\n", ret);
        sock_release(sock);
        return ret;
    }

    // // 发送字符串
    // char *send_buf2 = NULL;
    // send_buf2 = kmalloc(5, GFP_KERNEL);
    // memset(send_buf2, 'a', 5);
    // // prin_fo("Sending before size: %d,*send)buf2%d\n", sizeof(send_buf2),sizeof(*send_buf2));
    // ret = kernel_sendmsg(sock, &(struct msghdr){0}, 
    //                      &(struct kvec){ send_buf2, sizeof(send_buf2) }, 1, 
    //                      sizeof(send_buf2));
    // pr_info("Sending after size: %d\n", sizeof(send_buf2));
    // if (ret < 0) {
    //     pr_err("Send failed: %d\n", ret);
    //     sock_release(sock);
    //     return ret;
    // }
    // pr_info("Sent string: %s\n", send_buf2);

    // // 准备接收字符串
    // pr_info("Ready to receive string:\n");
    // pr_info("Receiving before size: %d\n", sizeof(recv_buf));
    // ret = kernel_recvmsg(sock, &(struct msghdr){0}, 
    //                      &(struct kvec){ recv_buf, sizeof(recv_buf) }, 1, 
    //                      sizeof(recv_buf), 0);
    
    // if (ret < 0) {
    //     pr_err("Receive failed: %d\n", ret);
    //     sock_release(sock);
    //     return ret;
    // }

    // // 打印接收到的字符串
    // pr_info("Received string: %s\n", recv_buf);
    // pr_info("Receiving after size: %d\n", sizeof(recv_buf));

    //发送本端QP信息
    pr_info("Sending local QP info1111:qpn:%u,interface id:0x%llx,subnet:%llx,size:%d, ret:%d\n",
        local_qp_info->qpn,
        local_qp_info->gid.global.interface_id,
        local_qp_info->gid.global.subnet_prefix,
        sizeof(*local_qp_info),ret);
    // //转换为大端
    // local_qp_info->qpn = cpu_to_be32(local_qp_info->qpn);
    // local_qp_info->gid.global.interface_id = cpu_to_be64(local_qp_info->gid.global.interface_id);  
    // local_qp_info->gid.global.subnet_prefix = cpu_to_be64(local_qp_info->gid.global.subnet_prefix);

    ret=kernel_sendmsg(sock, &(struct msghdr){0}, 
    &(struct kvec){local_qp_info, sizeof(*local_qp_info) }, 1, 
    sizeof(*local_qp_info));
    if (ret < 0) {
        pr_err("Send failed: %d\n", ret);
        sock_release(sock);
        return ret;
    }

    pr_info("Ready to receive remote QP info:,ret:%d\n",ret);
    ret=kernel_recvmsg(sock, &(struct msghdr){0}, 
                  &(struct kvec){remote_qp_info, sizeof(*remote_qp_info) }, 1, 
                  sizeof(*remote_qp_info), 0);
    if (ret < 0) {
        pr_err("Receive failed: %d\n", ret);
        sock_release(sock);
        return ret;
    }
    pr_info("Complete to receive remote QP info:\n");
    // local_qp_info->qpn = cpu_to_be32(local_qp_info->qpn);
    // local_qp_info->gid.global.interface_id = cpu_to_be64(local_qp_info->gid.global.interface_id);  
    // local_qp_info->gid.global.subnet_prefix = cpu_to_be64(local_qp_info->gid.global.subnet_prefix);
    //打印对端QP信息
    pr_info("Receiving remote QP info:qpn:%u,interface id:%llx,subnet:%llx,size:%d, ret:%d\n",
        remote_qp_info->qpn,
        remote_qp_info->gid.global.interface_id,
        remote_qp_info->gid.global.subnet_prefix,
        sizeof(*remote_qp_info),ret);
    pr_info("Sending before size: %d\n", sizeof(send_buf));
    

    // 关闭连接
    sock_release(sock);
    return 0;
}
