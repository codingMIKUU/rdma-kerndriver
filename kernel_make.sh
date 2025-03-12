echo "make start"
sudo make -j32 CFLAGS="-w ${CFLAGS}" #32个核编译生成.ko文件
echo "install start"
sudo make install #生成的 .ko 文件复制到系统的模块目录中，通常是 /lib/modules/$(uname -r)/kernel/
echo "rmmod start"
sudo rmmod mlx5_ib
echo "modprobe start"
sudo modprobe mlx5_ib #也可以用insmod，但insmod不处理mod依赖
echo "modprobe end"