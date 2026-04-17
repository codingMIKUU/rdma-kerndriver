echo "make start"
sudo make -j32 CFLAGS="-w ${CFLAGS}" && \
echo "install start" && \
sudo make install && \
echo "bridge modules rmmod start" && \
sudo rmmod mlx5_table_bridge && \
echo "rmmod start" && \
sudo rmmod mlx5_ib && \
echo "modprobe start" && \
sudo modprobe mlx5_ib && \
echo "modprobe end" && \
echo "bridge modules insmod start" && \
sudo insmod /home/dell/zxm/mlx5_table_bridge/mlx5_table_bridge.ko