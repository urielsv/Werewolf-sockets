#!/bin/bash

tmux kill-session -t werewolf 2>/dev/null

tmux new-session -d -s werewolf

# 2x3 grid
tmux split-window -h
tmux split-window -v
tmux select-pane -t 0
tmux split-window -v
tmux select-pane -t 0
tmux split-window -v
tmux select-pane -t 2
tmux split-window -v

for i in {0..5}; do
    tmux send-keys -t werewolf:0.$i "cd build/client && ./werewolf_client 127.0.0.1 8080" C-m
done

tmux attach-session -t werewolf
