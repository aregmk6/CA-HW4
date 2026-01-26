#!/bin/bash

for i in {1..20}; do
    diff "segel-tests/example${i}.img" "tests (1)/tests/example${i}.img"
done

