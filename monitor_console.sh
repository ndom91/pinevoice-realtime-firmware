#!/usr/bin/env bash

# Script to continuously monitor PineVoice serial console
DEVICE="/dev/ttyACM1"
BAUD="2000000"
LOGFILE="console_output.txt"

echo "Monitoring $DEVICE at $BAUD baud. Press Ctrl+C to stop."
echo "Output will be logged to $LOGFILE"
echo ""

# Ensure log file exists
touch "$LOGFILE"

while true; do
    if [ -e "$DEVICE" ]; then
        echo "=== Device connected at $(date) ==="
        echo "=== Device connected at $(date) ===" >> "$LOGFILE"

        # Configure serial port
        stty -F "$DEVICE" "$BAUD" raw -echo 2>/dev/null

        # Read from device and log
        cat "$DEVICE" 2>/dev/null | while IFS= read -r line; do
            echo "$line"
            echo "$line" >> "$LOGFILE"
        done

        echo "=== Device disconnected at $(date) ==="
        echo "=== Device disconnected at $(date) ===" >> "$LOGFILE"
        echo ""
    else
        # Wait for device to appear
        sleep 0.1
    fi
done
