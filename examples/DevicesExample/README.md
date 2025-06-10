# Devices Example

This example demonstrates how to use WinKernelLite to manage device information with linked lists and UNICODE_STRING.

## What This Example Shows

1. Creating and managing device information structures
2. Working with linked lists for dynamic collections
3. Memory allocation and tracking
4. String handling with UNICODE_STRING
5. Proper cleanup to prevent memory leaks

## Running the Example

To build and run this example:

```bash
# Navigate to the example directory
cd DevicesExample

# Run the build script
./build_and_run.bat
```

## Expected Output

The example will:

1. Create several device records (keyboard, mouse, printer)
2. Add them to a device list
3. Display the device information
4. Simulate disconnecting a device
5. Display the updated list
6. Remove a device from the list
7. Display the list again
8. Clean up all devices
9. Check for memory leaks

## Key Concepts

This example demonstrates several important concepts:

1. **Ownership Management** - Tracking which devices are owned by a list
2. **Resource Cleanup** - Properly freeing all allocated resources
3. **List Traversal** - Using list navigation to iterate through a collection
4. **Memory Leak Detection** - Using WinKernelLite's tracking to find leaks

For more detailed documentation, see [example_devices_list.md](../../docs/example_devices_list.md) in the docs directory.
