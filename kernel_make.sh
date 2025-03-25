echo "make start"
sudo make -j32 CFLAGS="-w ${CFLAGS}" && \
echo "install start" && \
sudo make install && \
echo "rmmod start" && \
sudo rmmod mlx5_ib && \
echo "modprobe start" && \
sudo modprobe mlx5_ib && \
echo "modprobe end"