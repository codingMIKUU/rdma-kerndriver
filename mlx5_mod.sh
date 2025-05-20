echo "rmmod start" && \
sudo rmmod mlx5_ib && \
echo "modprobe start" && \
sudo modprobe mlx5_ib && \
echo "modprobe end"