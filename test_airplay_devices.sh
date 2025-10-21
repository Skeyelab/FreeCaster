#!/bin/bash
# Quick AirPlay Device Discovery Test

echo "================================================"
echo "AirPlay Device Discovery Test"
echo "================================================"
echo ""
echo "Scanning for AirPlay devices on your network..."
echo "(This will run for 10 seconds)"
echo ""

# Scan for AirPlay devices
timeout 10 dns-sd -B _raop._tcp local. 2>/dev/null || {
    dns-sd -B _raop._tcp local. &
    DNS_PID=$!
    sleep 10
    kill $DNS_PID 2>/dev/null
}

echo ""
echo "================================================"
echo ""
echo "If you saw devices listed above, FreeCaster"
echo "should be able to discover them too!"
echo ""
echo "If nothing appeared:"
echo "  1. Enable macOS AirPlay Receiver in System Settings"
echo "  2. Install shairport-sync: brew install shairport-sync"
echo "  3. Check firewall settings"
echo ""
