#!/bin/bash
# TESTING 2024-11-04

# Define the experiments directory variable
EXPERIMENTS_DIR="experiments/2024-11-04"

# Define parallel arrays for servers and their corresponding arguments
# servers=("pitts" "montague" "wittgenstein")
servers=("montague")
# args=("3 5" "1 2" "4 6")
args=("3") # need to call 3 with montague later

# Get the length of the servers array
length=${#servers[@]}

# Loop through the arrays
for ((i=0; i<$length; i++)); do
    server=${servers[$i]}
    arg=${args[$i]}
    echo "Running test on $server with args $arg"

#     # Use SSH to run commands on the server
    ssh "$server" << EOF
cd Fleet-FoRs/Models/Frames-of-Reference
nohup sh -c './test_3.sh $server $EXPERIMENTS_DIR $arg ; echo "Testing completed on $server for conditions $arg" | mail -s "Testing completed on $server" nicotedeschi@gmail.com' > /dev/null 2>&1 &
exit
EOF
#     ssh "$server" <<EOF
# nohup sh -c 'echo "Running background job on $server at \$(date)" >> ~/test_nohup_output_$server.txt; sleep 60; echo "Finished background job on $server at \$(date)" >> ~/test_nohup_output_$server.txt && echo "Test job completed on $server" | mail -s "Test Job Completed on $server" nicotedeschi@gmail.com' > /dev/null 2>&1 &
# exit
# EOF

done
