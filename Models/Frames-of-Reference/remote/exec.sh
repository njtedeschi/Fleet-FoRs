#!/bin/bash

# Unified Remote Script
CONFIG_FILE="remote/config.txt"
LOG_FILE="remote/calls.log"

echo "Reading configuration from $CONFIG_FILE..."

# Check if the configuration file exists
if [[ ! -f $CONFIG_FILE ]]; then
    echo "Configuration file not found: $CONFIG_FILE"
    exit 1
fi

# Log the configuration and execution time
{
    echo "---"
    echo "Execution Date: $(date)"
    echo "Configuration Contents:"
    cat "$CONFIG_FILE"
    echo "---"
} >> "$LOG_FILE"

# Source the configuration file
source $CONFIG_FILE

# Validate required variables
if [[ -z "$EXPERIMENTS_DIR" || -z "$REMOTE_SCRIPT" || -z "${server_args[@]}" || -z "$MAKE_COMMAND" ]]; then
    echo "Missing required configuration variables. Please check $CONFIG_FILE."
    exit 1
fi

# Loop through the server-argument pairs
for pair in "${server_args[@]}"; do
    server=$(echo "$pair" | cut -d: -f1)
    condition_list=$(echo "$pair" | cut -d: -f2)

    echo "Running $REMOTE_SCRIPT on $server for conditions $condition_list..."

    # Use SSH to execute the remote script
    ssh "$server" << EOF
cd Fleet-FoRs/Models/Frames-of-Reference
nohup sh -c './$REMOTE_SCRIPT $server "$MAKE_COMMAND" $EXPERIMENTS_DIR $condition_list ; echo "Execution completed on $server for conditions $condition_list" | mail -s "Execution completed on $server" nicotedeschi@gmail.com' > ~/remote_log_$server.txt 2>&1 &
exit
EOF

    {
        echo "Executed $REMOTE_SCRIPT on $server with make command '$MAKE_COMMAND' for conditions $condition_list"
        echo "Timestamp: $(date)"
        echo "---"
    } >> "$LOG_FILE"

done

echo "All tasks dispatched."

###########
# # TESTING
###########
# for pair in "${server_args[@]}"; do
#     server=$(echo "$pair" | cut -d: -f1)
#     arg_list=$(echo "$pair" | cut -d: -f2)

#     # Split the argument list into individual arguments
#     IFS=' ' read -r -a arg_array <<< "$arg_list"

#     for arg in "${arg_array[@]}"; do
#         echo "Simulating execution of $REMOTE_SCRIPT on $server with args $arg..."

#         # Simulate the SSH command instead of actually executing it
#         echo "ssh "$server" << EOF" >> /dev/null
#         echo "cd $EXPERIMENTS_DIR" >> /dev/null
#         echo "nohup sh -c './$REMOTE_SCRIPT $server $EXPERIMENTS_DIR $arg ; echo "Execution completed on $server for conditions $arg" | mail -s "Execution completed on $server" user@example.com' > ~/remote_log_$server.txt 2>&1 &" >> /dev/null
#         echo "exit" >> /dev/null
#         echo "EOF" >> /dev/null

#         {
#             echo "Simulated execution of $REMOTE_SCRIPT on $server with args $arg"
#             echo "Timestamp: $(date)"
#             echo "---"
#         } >> "$LOG_FILE"
#     done

# done

# echo "All tasks simulated."