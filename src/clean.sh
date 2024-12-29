#!/bin/bash

clean_make() {
    if [ -f "Makefile" ]; then
        echo "Cleaning in $PWD"
        make clean
    fi
}

find . -type d | while read dir; do
    (cd "$dir" && clean_make)
done