// oneAPI Level Zero compatibility shim for Lux9 kernel
// Provides oneAPI API emulation for GPU drivers

#ifndef ONEAPI_COMPAT_H
#define ONEAPI_COMPAT_H

#include <stdint.h>
#include <stddef.h>

// Basic oneAPI types
typedef struct ze_device_handle_t* ze_device_handle_t;
typedef struct ze_context_handle_t* ze_context_handle_t;
typedef struct ze_command_queue_handle_t* ze_command_queue_handle_t;
typedef struct ze_kernel_handle_t* ze_kernel_handle_t;
typedef struct ze_buffer_handle_t* ze_buffer_handle_t;

// Error codes
typedef enum {
    ZE_RESULT_SUCCESS = 0,
    ZE_RESULT_ERROR_DEVICE_LOST = 1,
    ZE_RESULT_ERROR_OUT_OF_HOST_MEMORY = 2,
    ZE_RESULT_ERROR_OUT_OF_DEVICE_MEMORY = 3,
    ZE_RESULT_ERROR_MODULE_BUILD_FAILURE = 4,
    ZE_RESULT_ERROR_MODULE_LINK_FAILURE = 5,
} ze_result_t;

// Device properties
typedef struct {
    uint32_t deviceId;
    char name[256];
    size_t maxMemAllocSize;
    uint32_t maxComputeUnits;
} ze_device_properties_t;

// Context descriptor
typedef struct {
    uint32_t flags;
} ze_context_desc_t;

// Command queue descriptor
typedef struct {
    uint32_t flags;
    uint32_t mode;
    uint32_t priority;
} ze_command_queue_desc_t;

// Kernel descriptor
typedef struct {
    uint32_t flags;
    const char* name;
} ze_kernel_desc_t;

// Buffer descriptor
typedef struct {
    size_t size;
    uint32_t flags;
} ze_buffer_desc_t;

// Function declarations
ze_result_t zeInit(uint32_t flags);
ze_result_t zeDeviceGet(ze_context_handle_t hContext, uint32_t* pCount, ze_device_handle_t* phDevices);
ze_result_t zeDeviceGetProperties(ze_device_handle_t hDevice, ze_device_properties_t* pDeviceProperties);
ze_result_t zeContextCreate(ze_context_handle_t hContext, const ze_context_desc_t* desc, ze_context_handle_t* phContext);
ze_result_t zeContextDestroy(ze_context_handle_t hContext);
ze_result_t zeCommandQueueCreate(ze_context_handle_t hContext, ze_device_handle_t hDevice, const ze_command_queue_desc_t* desc, ze_command_queue_handle_t* phCommandQueue);
ze_result_t zeCommandQueueDestroy(ze_command_queue_handle_t hCommandQueue);
ze_result_t zeKernelCreate(ze_context_handle_t hContext, ze_device_handle_t hDevice, const ze_kernel_desc_t* desc, ze_kernel_handle_t* phKernel);
ze_result_t zeKernelDestroy(ze_kernel_handle_t hKernel);
ze_result_t zeKernelSetArgumentValue(ze_kernel_handle_t hKernel, uint32_t argIndex, size_t argSize, const void* pArgValue);
ze_result_t zeCommandQueueExecuteCommandLists(ze_command_queue_handle_t hCommandQueue, uint32_t numCommandLists, void* phCommandLists, void* hFence);
ze_result_t zeBufferCreate(ze_context_handle_t hContext, const ze_buffer_desc_t* desc, ze_buffer_handle_t* phBuffer);
ze_result_t zeBufferDestroy(ze_buffer_handle_t hBuffer);
ze_result_t zeBufferWrite(ze_buffer_handle_t hBuffer, size_t offset, size_t size, const void* pData);
ze_result_t zeBufferRead(ze_buffer_handle_t hBuffer, size_t offset, size_t size, void* pData);

#endif // ONEAPI_COMPAT_H