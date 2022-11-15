#!/bin/bash

gcc main.c -o main -Wall -lSDL2 -lSDL2_image -lSDL2_ttf -lm && ./main && echo $?

