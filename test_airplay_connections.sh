#!/bin/bash

# Automated AirPlay Connection Testing Script
# This script will test connections to all discovered AirPlay devices

echo "Starting automated AirPlay connection testing..."
echo "=============================================="

# Function to test connection to a device
test_device() {
    local device_name="$1"
    local device_host="$2"
    local device_port="$3"

    echo "Testing connection to: $device_name at $device_host:$device_port"
    echo "------------------------------------------------------------"

    # Connect to the device (this will be logged in test_log.txt)
    # We'll use the FreeCaster app's connection functionality
    # For now, we'll simulate this by checking if the device is reachable
    if nc -z "$device_host" "$device_port" 2>/dev/null; then
        echo "✅ Device $device_name is reachable at $device_host:$device_port"
        return 0
    else
        echo "❌ Device $device_name is not reachable at $device_host:$device_port"
        return 1
    fi
}

# Function to parse discovered devices from logs
parse_devices_from_logs() {
    echo "Parsing discovered devices from logs..."

    # Extract device information from the logs
    grep "Device found -" test_log.txt | while read line; do
        # Parse the device information
        # Format: "Device found - DEVICE_NAME at HOST:PORT"
        device_info=$(echo "$line" | sed 's/.*Device found - //' | sed 's/ at /|/' | sed 's/:/|/')

        if [ -n "$device_info" ]; then
            device_name=$(echo "$device_info" | cut -d'|' -f1)
            device_host=$(echo "$device_info" | cut -d'|' -f2)
            device_port=$(echo "$device_info" | cut -d'|' -f3)

            echo "Found device: $device_name at $device_host:$device_port"
            test_device "$device_name" "$device_host" "$device_port"
            echo ""
        fi
    done
}

# Function to analyze connection results
analyze_results() {
    echo "Analyzing connection results..."
    echo "=============================="

    # Count successful connections
    successful=$(grep -c "RTSP request successful with status 200" test_log.txt)
    echo "Successful RTSP requests: $successful"

    # Count failed connections
    failed=$(grep -c "RTSP request failed" test_log.txt)
    echo "Failed RTSP requests: $failed"

    # Show specific error types
    echo ""
    echo "Error breakdown:"
    grep "RTSP request failed" test_log.txt | sort | uniq -c

    echo ""
    echo "Authentication status:"
    grep -E "(Apple-Response|rsaaeskey|aesiv)" test_log.txt | tail -10

    echo ""
    echo "Device discovery results:"
    grep "Device found -" test_log.txt | wc -l | xargs echo "Total devices discovered:"
}

# Main execution
main() {
    echo "Starting automated testing at $(date)"
    echo ""

    # Parse devices from logs
    parse_devices_from_logs

    echo ""
    echo "Connection testing completed at $(date)"
    echo ""

    # Analyze results
    analyze_results

    echo ""
    echo "Full log available in test_log.txt"
    echo "Use 'tail -f test_log.txt' to monitor real-time connections"
}

# Run the main function
main
