#!/usr/bin/env python3
"""
Conditional USB Mode Script for WLED-MM

This script automatically sets ARDUINO_USB_MODE based on the build context:
- For development builds: ARDUINO_USB_MODE=1 (allows USB debugging)  
- For release builds (CI): ARDUINO_USB_MODE=0 (allows normal boot without USB debugger)

The script detects release builds by checking for the WLED_RELEASE environment variable
which is set to True in the GitHub Actions CI workflow.
"""

Import('env')
import os

def conditional_usb_mode(env):
    """
    Conditionally set ARDUINO_USB_MODE based on build context.
    
    For ESP32-C3, ESP32-S2, and ESP32-S3 variants:
    - Development builds: ARDUINO_USB_MODE=1 (default, good for debugging)
    - Release builds: ARDUINO_USB_MODE=0 (prevents hanging without USB debugger)
    """
    
    # Check if this is a release build (CI sets WLED_RELEASE=True)
    is_release_build = os.environ.get('WLED_RELEASE', '').lower() in ('true', '1', 'yes')
    
    if is_release_build:
        print("WLED Release build detected - setting ARDUINO_USB_MODE=0 for production")
        
        # Find and modify ARDUINO_USB_MODE in build flags
        build_flags = env.get('BUILD_FLAGS', [])
        cpp_defines = env.get('CPPDEFINES', [])
        
        # Look through CPPDEFINES and modify ARDUINO_USB_MODE if found
        modified = False
        new_defines = []
        
        for define in cpp_defines:
            if isinstance(define, (list, tuple)) and len(define) == 2:
                if define[0] == 'ARDUINO_USB_MODE' and define[1] == '1':
                    # Change ARDUINO_USB_MODE from 1 to 0 for release builds
                    new_defines.append(('ARDUINO_USB_MODE', '0'))
                    modified = True
                    print(f"  Changed ARDUINO_USB_MODE from 1 to 0")
                else:
                    new_defines.append(define)
            else:
                new_defines.append(define)
        
        if modified:
            env.Replace(CPPDEFINES=new_defines)
        
        # Also check raw build flags for -DARDUINO_USB_MODE=1
        new_build_flags = []
        for flag in build_flags:
            if isinstance(flag, str) and 'ARDUINO_USB_MODE=1' in flag:
                # Replace ARDUINO_USB_MODE=1 with ARDUINO_USB_MODE=0
                new_flag = flag.replace('ARDUINO_USB_MODE=1', 'ARDUINO_USB_MODE=0')
                new_build_flags.append(new_flag)
                modified = True
                print(f"  Modified build flag: {flag} -> {new_flag}")
            else:
                new_build_flags.append(flag)
        
        if modified:
            env.Replace(BUILD_FLAGS=new_build_flags)
            
    else:
        print("Development build detected - keeping ARDUINO_USB_MODE=1 for debugging")

# Apply the conditional USB mode logic
conditional_usb_mode(env)