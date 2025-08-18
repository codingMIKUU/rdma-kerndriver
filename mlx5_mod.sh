echo "bridge modules rmmod start" && \
sudo rmmod mlx5_table_bridge && \
echo "rmmod start" && \
sudo rmmod mlx5_ib && \
echo "modprobe start" && \
sudo modprobe mlx5_ib && \
echo "modprobe end" && \
echo "bridge modules insmod start" && \
sudo insmod /root/zxm/mlx5_table_bridge/mlx5_table_bridge.ko