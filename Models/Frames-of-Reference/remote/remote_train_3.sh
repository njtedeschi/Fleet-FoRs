#!/bin/bash
# TRAINING 2024-12-17

# Define the experiments directory variable
EXPERIMENTS_DIR="experiments/2024-12-17"

# Define parallel arrays for servers and their corresponding arguments
# servers=("pitts" "wittgenstein" "montague")
servers=("pitts" "wittgenstein" "montague")
# args=("1 4" "2 5" "3 6")
args=("1 2 3 4 5 6" "7 8 9 10 11 12" "13 14 15 16")

# Get the length of the servers array
length=${#servers[@]}

# Loop through the arrays
for ((i=0; i<$length; i++)); do
    server=${servers[$i]}
    arg=${args[$i]}
    echo "Running train_3 on $server with args $arg"

#     # Use SSH to run commands on the server
    ssh "$server" << EOF
cd Fleet-FoRs/Models/Frames-of-Reference
# nohup sh -c './train_3.sh $server $EXPERIMENTS_DIR $arg ; echo "Training completed on $server for conditions $arg" | mail -s "Training completed on $server" nicotedeschi@gmail.com' > /dev/null 2>&1 &
nohup sh -c './train_3.sh $server $EXPERIMENTS_DIR $arg ; echo "Training completed on $server for conditions $arg" | mail -s "Training completed on $server" nicotedeschi@gmail.com' > ~/train_log_$server.txt 2>&1 &
exit
EOF
#     ssh "$server" <<EOF
# nohup sh -c 'echo "Running background job on $server at \$(date)" >> ~/test_nohup_output_$server.txt; sleep 60; echo "Finished background job on $server at \$(date)" >> ~/test_nohup_output_$server.txt && echo "Test job completed on $server" | mail -s "Test Job Completed on $server" nicotedeschi@gmail.com' > /dev/null 2>&1 &
# exit
# EOF

done
