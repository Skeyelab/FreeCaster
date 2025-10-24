#!/usr/bin/env python3

"""
Automated AirPlay Connection Testing Script
This script monitors the FreeCaster app logs and tests connections to all discovered devices
"""

import subprocess
import time
import re
import signal
import sys
from datetime import datetime

class AirPlayTester:
    def __init__(self):
        self.discovered_devices = []
        self.test_results = {}
        self.app_process = None

    def start_app(self):
        """Start the FreeCaster app"""
        print("Starting FreeCaster app...")
        self.app_process = subprocess.Popen([
            "./build/FreeCaster_artefacts/Standalone/FreeCaster.app/Contents/MacOS/FreeCaster"
        ], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        time.sleep(3)  # Give app time to start
        print("✅ FreeCaster app started")

    def stop_app(self):
        """Stop the FreeCaster app"""
        if self.app_process:
            print("Stopping FreeCaster app...")
            self.app_process.terminate()
            self.app_process.wait()
            print("✅ FreeCaster app stopped")

    def parse_devices_from_logs(self):
        """Parse discovered devices from the log file"""
        try:
            with open('test_log.txt', 'r') as f:
                content = f.read()

            # Find device discovery entries
            device_pattern = r'Device found - (.+?) at (.+?):(\d+)'
            matches = re.findall(device_pattern, content)

            for match in matches:
                device_name, host, port = match
                self.discovered_devices.append({
                    'name': device_name,
                    'host': host,
                    'port': int(port)
                })

            print(f"✅ Discovered {len(self.discovered_devices)} devices:")
            for device in self.discovered_devices:
                print(f"  - {device['name']} at {device['host']}:{device['port']}")

        except FileNotFoundError:
            print("❌ test_log.txt not found")

    def analyze_connection_results(self):
        """Analyze the connection results from logs"""
        try:
            with open('test_log.txt', 'r') as f:
                content = f.read()

            print("\n📊 Connection Analysis:")
            print("=" * 50)

            # Count successful connections
            successful = len(re.findall(r'RTSP request successful with status 200', content))
            failed = len(re.findall(r'RTSP request failed', content))

            print(f"✅ Successful RTSP requests: {successful}")
            print(f"❌ Failed RTSP requests: {failed}")

            # Analyze specific error types
            print("\n🔍 Error Breakdown:")
            error_patterns = {
                '403 Forbidden': r'403 Forbidden',
                '500 Internal Error': r'500 Internal Error',
                'Connection failed': r'Connection failed'
            }

            for error_type, pattern in error_patterns.items():
                count = len(re.findall(pattern, content))
                if count > 0:
                    print(f"  - {error_type}: {count}")

            # Check authentication status
            print("\n🔐 Authentication Status:")
            apple_response = len(re.findall(r'Apple-Response:', content))
            rsaaeskey = len(re.findall(r'rsaaeskey:', content))
            aesiv = len(re.findall(r'aesiv:', content))

            print(f"  - Apple-Response headers: {apple_response}")
            print(f"  - RSA AES key fields: {rsaaeskey}")
            print(f"  - AES IV fields: {aesiv}")

            # Show recent connection attempts
            print("\n📝 Recent Connection Attempts:")
            recent_connections = re.findall(r'Attempting to connect to device: (.+?)', content)
            if recent_connections:
                for connection in recent_connections[-5:]:  # Last 5 connections
                    print(f"  - {connection}")

        except FileNotFoundError:
            print("❌ test_log.txt not found")

    def monitor_logs(self, duration=60):
        """Monitor logs for a specified duration"""
        print(f"\n🔍 Monitoring logs for {duration} seconds...")
        print("=" * 50)

        start_time = time.time()
        last_size = 0

        while time.time() - start_time < duration:
            try:
                with open('test_log.txt', 'r') as f:
                    content = f.read()

                # Check for new content
                current_size = len(content)
                if current_size > last_size:
                    new_content = content[last_size:]
                    last_size = current_size

                    # Print new log entries
                    for line in new_content.split('\n'):
                        if line.strip():
                            timestamp = datetime.now().strftime("%H:%M:%S")
                            print(f"[{timestamp}] {line}")

                time.sleep(1)  # Check every second

            except FileNotFoundError:
                time.sleep(1)
                continue

        print(f"\n✅ Monitoring completed after {duration} seconds")

    def run_comprehensive_test(self):
        """Run a comprehensive test of all discovered devices"""
        print("🚀 Starting Comprehensive AirPlay Test")
        print("=" * 50)

        try:
            # Start the app
            self.start_app()

            # Wait for device discovery
            print("\n⏳ Waiting for device discovery...")
            time.sleep(10)

            # Parse discovered devices
            self.parse_devices_from_logs()

            # Monitor logs for connection attempts
            self.monitor_logs(30)

            # Analyze results
            self.analyze_connection_results()

        except KeyboardInterrupt:
            print("\n⏹️  Test interrupted by user")
        finally:
            self.stop_app()

    def signal_handler(self, signum, frame):
        """Handle Ctrl+C gracefully"""
        print("\n⏹️  Received interrupt signal, stopping...")
        self.stop_app()
        sys.exit(0)

def main():
    """Main function"""
    tester = AirPlayTester()

    # Set up signal handler for graceful shutdown
    signal.signal(signal.SIGINT, tester.signal_handler)

    print("🎵 FreeCaster Automated AirPlay Tester")
    print("=====================================")
    print("This script will:")
    print("1. Start the FreeCaster app")
    print("2. Monitor device discovery")
    print("3. Test connections to all discovered devices")
    print("4. Analyze results and provide detailed reporting")
    print()

    # Run the comprehensive test
    tester.run_comprehensive_test()

    print("\n✅ Testing completed!")
    print("Check test_log.txt for detailed logs")

if __name__ == "__main__":
    main()
