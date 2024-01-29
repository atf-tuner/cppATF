#!/bin/bash

g++ ../raytracer/raytracer.cpp -o raytracer "$opt_level" "$align_functions" --param early-inlining-insns=$early_inlining_insns
