#!/bin/sh

g++-8 -std=c++17 *.cpp SFNUL/*.cpp -o ../RunDir/ProjectLunarFront -DSFML_STATIC -lsfml-network-s -lsfml-system-s -lpthread -lstdc++fs


