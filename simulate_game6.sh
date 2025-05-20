#!/bin/bash

tmux kill-session -t werewolf 2>/dev/null

tmux new-session -d -s werewolf

# 2x3 grid + server
# Pane 0: server
# Panes 1-6: clients
tmux split-window -h
tmux split-window -v -t 0
tmux split-window -v -t 1
tmux select-pane -t 2

tmux split-window -v -t 0

tmux select-pane -t 1

tmux split-window -v -t 1

tmux select-pane -t 3

tmux split-window -v -t 3

tmux send-keys -t werewolf:0.0 "cd build/server && ./werewolf_server 8080 6" C-m

# Wait for server to start
sleep 2

for i in {1..6}; do
    tmux send-keys -t werewolf:0.$i "cd build/client && ./werewolf_client 127.0.0.1 8080" C-m
    sleep 0.5
done

tmux attach-session -t werewolf
