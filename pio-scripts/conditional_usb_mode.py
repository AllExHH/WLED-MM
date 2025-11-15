#!/usr/bin/env python3
"""
Conditional USB Mode Script for WLED-MM

This script automatically sets ARDUINO_USB_MODE based on the build context:
- For development builds: ARDUINO_USB_MODE=1 (allows USB debugging)  
- For release builds (CI): ARDUINO_USB_MODE=0 (allows normal boot without USB debugger)

The script detects release builds by checking for the WLED_RELEASE environment variable
which is set to True in the GitHub Actions CI workflow.

CRITICAL: This change only applies to boards with USB-OTG (ARDUINO_USB_CDC_ON_BOOT=1).
For boards with classical UART-to-USB chips (ARDUINO_USB_CDC_ON_BOOT=0), 
ARDUINO_USB_MODE=1 is harmless and left unchanged.
"""

Import('env')
import os

def has_cdc_on_boot_enabled(env):
    """
    Check if ARDUINO_USB_CDC_ON_BOOT is set to 1 in the build configuration.
    
    Returns True if CDC_ON_BOOT=1, False otherwise.
    This is used to identify boards with USB-OTG (native USB) vs UART-to-USB chips.
    """
    cpp_defines = env.get('CPPDEFINES', [])
    build_flags = env.get('BUILD_FLAGS', [])
    
    # Check in CPPDEFINES
    for define in cpp_defines:
        if isinstance(define, (list, tuple)) and len(define) == 2:
            if define[0] == 'ARDUINO_USB_CDC_ON_BOOT' and define[1] == '1':
                return True
    
    # Check in raw build flags
    for flag in build_flags:
        if isinstance(flag, str) and 'ARDUINO_USB_CDC_ON_BOOT=1' in flag:
            return True
    
    return False

def has_usb_mode_enabled(env):
    """
    Check if ARDUINO_USB_MODE is set to 1 in the build configuration.
    
    Returns True if USB_MODE=1, False otherwise.
    """
    cpp_defines = env.get('CPPDEFINES', [])
    build_flags = env.get('BUILD_FLAGS', [])
    
    # Check in CPPDEFINES
    for define in cpp_defines:
        if isinstance(define, (list, tuple)) and len(define) == 2:
            if define[0] == 'ARDUINO_USB_MODE' and define[1] == '1':
                return True
    
    # Check in raw build flags
    for flag in build_flags:
        if isinstance(flag, str) and 'ARDUINO_USB_MODE=1' in flag:
            return True
    
    return False

def conditional_usb_mode(env):
    """
    Conditionally set ARDUINO_USB_MODE based on build context.
    
    For ESP32-C3, ESP32-S2, and ESP32-S3 variants with USB-OTG (CDC_ON_BOOT=1):
    - Development builds: ARDUINO_USB_MODE=1 (default, good for debugging)
    - Release builds: ARDUINO_USB_MODE=0 (prevents hanging without USB debugger)
    
    For boards with classical UART-to-USB chip (CDC_ON_BOOT=0):
    - ARDUINO_USB_MODE=1 is harmless and left unchanged
    """
    
    # Check if this is a release build (CI sets WLED_RELEASE=True)
    is_release_build = os.environ.get('WLED_RELEASE', '').lower() in ('true', '1', 'yes')
    
    if is_release_build:
        # Check if this board uses USB-OTG (CDC_ON_BOOT=1)
        if not has_cdc_on_boot_enabled(env):
            print("WLED Release build detected - board has UART-to-USB chip (CDC_ON_BOOT=0)")
            print("  Keeping ARDUINO_USB_MODE=1 (harmless for UART-to-USB boards)")
            return
        
        print("WLED Release build detected - board has USB-OTG (CDC_ON_BOOT=1)")
        print("  Setting ARDUINO_USB_MODE=0 for production")
        
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
        # Development build
        has_usb_mode = has_usb_mode_enabled(env)
        has_cdc_boot = has_cdc_on_boot_enabled(env)
        
        if has_usb_mode and has_cdc_boot:
            print("Development build detected - keeping ARDUINO_USB_MODE=1 for USB-OTG debugging")
            # Warning in orange/yellow using ANSI color codes
            print("\033[93m  WARNING: This build is NOT suitable for production devices!\033[0m")
            print("\033[93m  Production builds require WLED_RELEASE=True environment variable.\033[0m")
        elif has_cdc_boot:
            # CDC_ON_BOOT=1 present but not USB_MODE=1 
            print("Development build detected -  USB-OTG enabled, but ARDUINO_USB_MODE=1 missing for debugging.")
        elif has_usb_mode:
            # USB_MODE=1 present but not CDC_ON_BOOT=1 (UART-to-USB board)
            print("Development build detected - board has UART-to-USB chip")
        # If neither flag is present, don't print anything

# Apply the conditional USB mode logic
conditional_usb_mode(env)
