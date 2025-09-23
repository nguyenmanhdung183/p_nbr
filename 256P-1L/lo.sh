#!/bin/bash

# Tạo 256 IP ảo trên interface loopback (lo)
echo "Thêm 256 IP ảo vào interface loopback (lo)..."

for i in $(seq 2 251); do
    ip="127.0.0.$i"
    sudo ip addr add $ip/8 dev lo
    echo "Đã thêm IP: $ip"
done

echo "Hoàn tất."
