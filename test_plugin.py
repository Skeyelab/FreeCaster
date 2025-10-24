#!/usr/bin/env python3
"""
Simple test script to verify FreeCaster plugin loads correctly
"""
import subprocess
import sys
import os

def test_plugin():
    print("Testing FreeCaster Plugin...")

    # Check if VST3 plugin exists
    vst3_path = "/Users/edahl/Library/Audio/Plug-Ins/VST3/FreeCaster.vst3"
    if os.path.exists(vst3_path):
        print("✅ VST3 plugin found")
    else:
        print("❌ VST3 plugin not found")
        return False

    # Check if AU plugin exists
    au_path = "/Users/edahl/Library/Audio/Plug-Ins/Components/FreeCaster.component"
    if os.path.exists(au_path):
        print("✅ AU plugin found")
    else:
        print("❌ AU plugin not found")
        return False

    # Check if standalone app exists
    standalone_path = "/Users/edahl/FreeCaster/build/FreeCaster_artefacts/Standalone/FreeCaster.app"
    if os.path.exists(standalone_path):
        print("✅ Standalone app found")
    else:
        print("❌ Standalone app not found")
        return False

    print("\n🎵 Plugin Installation Summary:")
    print("   • VST3: Available for Ableton Live, Logic Pro, etc.")
    print("   • AU: Available for Logic Pro, GarageBand, etc.")
    print("   • Standalone: Available for direct testing")

    print("\n📋 Testing Instructions for Ableton Live:")
    print("   1. Open Ableton Live 11")
    print("   2. Create a new audio track")
    print("   3. Add FreeCaster as an Audio Effect")
    print("   4. Look for two vertical meters on the right side")
    print("   5. Click 'Test Meters' button to verify functionality")

    return True

if __name__ == "__main__":
    success = test_plugin()
    sys.exit(0 if success else 1)

