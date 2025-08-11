#!/bin/bash

# YOLO - Quick Claude CLI launcher
# Launches Claude CLI with dangerous permissions flag and passes through any additional parameters

claude --dangerously-skip-permissions "$@"