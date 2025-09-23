#!/bin/bash

echo "Xóa 256 IP ảo khỏi interface loopback (lo)..."

for i in $(seq 2 251); do
    ip="127.0.0.$i"
    sudo ip addr del $ip/8 dev lo 2>/dev/null
    echo "Đã xóa IP: $ip"
done

echo "Hoàn tất."
