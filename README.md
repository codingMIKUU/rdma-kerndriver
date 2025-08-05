# FCScale内核实现

* 内核多qp随机发送
* cqe polling采用队列形式，每次遍历进行polling
* 单wqe单cqe，在内核8192个qp下性能差，原因是因为poll次数过多，暂未优化
