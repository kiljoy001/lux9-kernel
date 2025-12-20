// Kernel-Userspace Interface for Hurd-9
// Minimal syscall interface optimized for Go runtime

use alloc::vec::Vec;
use alloc::string::String;
use core::arch::asm;

/// System call numbers
#[repr(u64)]
#[derive(Debug, Clone, Copy)]
pub enum SyscallNumber {
    // Memory management
    Mmap = 1,
    Munmap = 2,
    Mprotect = 3,
    
    // File I/O (via 9P)
    Open = 10,
    Read = 11,
    Write = 12,
    Close = 13,
    Seek = 14,
    
    // Process management
    SipSpawn = 20,
    SipExit = 21,
    SipWait = 22,
    SipKill = 23,
    
    // IPC
    ChannelCreate = 30,
    ChannelSend = 31,
    ChannelRecv = 32,
    ChannelClose = 33,
    
    // Time
    GetTime = 40,
    Sleep = 41,
    
    // Threading (for Go runtime)
    Clone = 50,
    Futex = 51,
    
    // Signals
    SigAction = 60,
    SigReturn = 61,
    Kill = 62,
    
    // 9P operations
    P9Mount = 70,
    P9Unmount = 71,
    P9Attach = 72,
    
    // Capabilities
    CapGet = 80,
    CapSet = 81,
    CapRevoke = 82,
    
    // Debug/profiling
    Trace = 90,
    Profile = 91,
}

/// System call arguments
#[derive(Debug)]
pub struct SyscallArgs {
    pub num: SyscallNumber,
    pub arg0: u64,
    pub arg1: u64,
    pub arg2: u64,
    pub arg3: u64,
    pub arg4: u64,
    pub arg5: u64,
}

/// System call results
#[derive(Debug)]
pub enum SyscallResult {
    Ok(u64),
    Error(SyscallError),
}

#[derive(Debug)]
pub enum SyscallError {
    InvalidSyscall,
    PermissionDenied,
    InvalidArgument,
    OutOfMemory,
    FileNotFound,
    ChannelClosed,
    SipNotFound,
    Interrupted,
    WouldBlock,
}

/// Entry point for all system calls
pub fn syscall_entry(args: SyscallArgs) -> SyscallResult {
    // Validate syscall number
    let syscall = args.num;
    
    // Check if SIP has permission for this syscall
    if !validate_syscall_permission(syscall) {
        return SyscallResult::Error(SyscallError::PermissionDenied);
    }
    
    // Dispatch to appropriate handler
    match syscall {
        SyscallNumber::Mmap => syscall_mmap(args),
        SyscallNumber::Munmap => syscall_munmap(args),
        SyscallNumber::Open => syscall_open(args),
        SyscallNumber::Read => syscall_read(args),
        SyscallNumber::Write => syscall_write(args),
        SyscallNumber::SipSpawn => syscall_sip_spawn(args),
        SyscallNumber::ChannelCreate => syscall_channel_create(args),
        SyscallNumber::ChannelSend => syscall_channel_send(args),
        SyscallNumber::ChannelRecv => syscall_channel_recv(args),
        SyscallNumber::GetTime => syscall_get_time(args),
        SyscallNumber::Sleep => syscall_sleep(args),
        SyscallNumber::Clone => syscall_clone(args),
        SyscallNumber::Futex => syscall_futex(args),
        _ => SyscallResult::Error(SyscallError::InvalidSyscall),
    }
}

/// Memory mapping syscall
fn syscall_mmap(args: SyscallArgs) -> SyscallResult {
    let addr = args.arg0 as usize;
    let len = args.arg1 as usize;
    let prot = args.arg2 as u32;
    let flags = args.arg3 as u32;
    let _fd = args.arg4 as i32;
    let offset = args.arg5 as u64;
    
    // Validate arguments
    if len == 0 || len > MAX_MMAP_SIZE {
        return SyscallResult::Error(SyscallError::InvalidArgument);
    }
    
    // Get current SIP
    let current_sip = get_current_sip();
    
    // Perform mapping
    match current_sip.memory_space.mmap(addr, len, prot, flags, offset) {
        Ok(mapped_addr) => SyscallResult::Ok(mapped_addr as u64),
        Err(_) => SyscallResult::Error(SyscallError::OutOfMemory),
    }
}

/// Memory unmapping syscall
fn syscall_munmap(args: SyscallArgs) -> SyscallResult {
    let addr = args.arg0 as usize;
    let len = args.arg1 as usize;
    
    let current_sip = get_current_sip();
    
    match current_sip.memory_space.munmap(addr, len) {
        Ok(_) => SyscallResult::Ok(0),
        Err(_) => SyscallResult::Error(SyscallError::InvalidArgument),
    }
}

/// File open syscall (via 9P)
fn syscall_open(args: SyscallArgs) -> SyscallResult {
    let path_ptr = args.arg0 as *const u8;
    let path_len = args.arg1 as usize;
    let flags = args.arg2 as u32;
    
    // Safely read path from userspace
    let path = match read_user_string(path_ptr, path_len) {
        Ok(p) => p,
        Err(_) => return SyscallResult::Error(SyscallError::InvalidArgument),
    };
    
    let current_sip = get_current_sip();
    
    // Use 9P to open file
    match current_sip.namespace.open_file(path, flags) {
        Ok(fd) => SyscallResult::Ok(fd.0 as u64),
        Err(_) => SyscallResult::Error(SyscallError::FileNotFound),
    }
}

/// File read syscall
fn syscall_read(args: SyscallArgs) -> SyscallResult {
    let fd = FileDescriptor(args.arg0 as u32);
    let buf_ptr = args.arg1 as *mut u8;
    let count = args.arg2 as usize;
    
    // Validate buffer
    if !is_valid_user_buffer(buf_ptr, count) {
        return SyscallResult::Error(SyscallError::InvalidArgument);
    }
    
    let current_sip = get_current_sip();
    
    // Read via 9P
    match current_sip.namespace.read_file(fd, count) {
        Ok(data) => {
            // Copy data to user buffer
            unsafe {
                let copy_len = data.len().min(count);
                core::ptr::copy_nonoverlapping(data.as_ptr(), buf_ptr, copy_len);
            }
            SyscallResult::Ok(data.len() as u64)
        }
        Err(_) => SyscallResult::Error(SyscallError::InvalidArgument),
    }
}

/// File write syscall
fn syscall_write(args: SyscallArgs) -> SyscallResult {
    let fd = FileDescriptor(args.arg0 as u32);
    let buf_ptr = args.arg1 as *const u8;
    let count = args.arg2 as usize;
    
    // Read data from user buffer
    let data = match read_user_buffer(buf_ptr, count) {
        Ok(d) => d,
        Err(_) => return SyscallResult::Error(SyscallError::InvalidArgument),
    };
    
    let current_sip = get_current_sip();
    
    // Write via 9P
    match current_sip.namespace.write_file(fd, data) {
        Ok(written) => SyscallResult::Ok(written as u64),
        Err(_) => SyscallResult::Error(SyscallError::InvalidArgument),
    }
}

/// Spawn new SIP
fn syscall_sip_spawn(args: SyscallArgs) -> SyscallResult {
    let binary_ptr = args.arg0 as *const u8;
    let binary_len = args.arg1 as usize;
    let args_ptr = args.arg2 as *const *const u8;
    let args_count = args.arg3 as usize;
    
    // Read binary from userspace
    let binary = match read_user_buffer(binary_ptr, binary_len) {
        Ok(b) => b,
        Err(_) => return SyscallResult::Error(SyscallError::InvalidArgument),
    };
    
    // Read arguments
    let args_vec = match read_user_string_array(args_ptr, args_count) {
        Ok(a) => a,
        Err(_) => return SyscallResult::Error(SyscallError::InvalidArgument),
    };
    
    let current_sip = get_current_sip();
    
    // Inherit capabilities (restricted)
    let child_caps = current_sip.capabilities.derive_child_capabilities();
    
    // Spawn new SIP
    match get_sip_manager().spawn_sip(&binary, args_vec, child_caps) {
        Ok(sip_id) => SyscallResult::Ok(sip_id.0),
        Err(_) => SyscallResult::Error(SyscallError::OutOfMemory),
    }
}

/// Create IPC channel
fn syscall_channel_create(args: SyscallArgs) -> SyscallResult {
    let name_ptr = args.arg0 as *const u8;
    let name_len = args.arg1 as usize;
    let channel_type = args.arg2 as u32;
    
    let name = match read_user_string(name_ptr, name_len) {
        Ok(n) => n,
        Err(_) => return SyscallResult::Error(SyscallError::InvalidArgument),
    };
    
    let current_sip = get_current_sip();
    
    let channel_type = match channel_type {
        0 => ChannelType::Synchronous,
        1 => ChannelType::Asynchronous,
        2 => ChannelType::Holographic,
        _ => return SyscallResult::Error(SyscallError::InvalidArgument),
    };
    
    match create_ipc_channel(current_sip.id, name, channel_type) {
        Ok(channel_id) => SyscallResult::Ok(channel_id.0),
        Err(_) => SyscallResult::Error(SyscallError::OutOfMemory),
    }
}

/// Send message on IPC channel
fn syscall_channel_send(args: SyscallArgs) -> SyscallResult {
    let channel_id = ChannelId(args.arg0);
    let data_ptr = args.arg1 as *const u8;
    let data_len = args.arg2 as usize;
    let flags = args.arg3 as u32;
    
    let data = match read_user_buffer(data_ptr, data_len) {
        Ok(d) => d,
        Err(_) => return SyscallResult::Error(SyscallError::InvalidArgument),
    };
    
    let blocking = (flags & 1) == 0;
    
    match send_ipc_message(channel_id, data, blocking) {
        Ok(_) => SyscallResult::Ok(0),
        Err(IPCError::WouldBlock) => SyscallResult::Error(SyscallError::WouldBlock),
        Err(_) => SyscallResult::Error(SyscallError::ChannelClosed),
    }
}

/// Receive message from IPC channel
fn syscall_channel_recv(args: SyscallArgs) -> SyscallResult {
    let channel_id = ChannelId(args.arg0);
    let buf_ptr = args.arg1 as *mut u8;
    let buf_len = args.arg2 as usize;
    let flags = args.arg3 as u32;
    
    if !is_valid_user_buffer(buf_ptr, buf_len) {
        return SyscallResult::Error(SyscallError::InvalidArgument);
    }
    
    let blocking = (flags & 1) == 0;
    
    match receive_ipc_message(channel_id, blocking) {
        Ok(data) => {
            let copy_len = data.len().min(buf_len);
            unsafe {
                core::ptr::copy_nonoverlapping(data.as_ptr(), buf_ptr, copy_len);
            }
            SyscallResult::Ok(copy_len as u64)
        }
        Err(IPCError::WouldBlock) => SyscallResult::Error(SyscallError::WouldBlock),
        Err(_) => SyscallResult::Error(SyscallError::ChannelClosed),
    }
}

/// Get current time
fn syscall_get_time(_args: SyscallArgs) -> SyscallResult {
    let timestamp = get_system_time();
    SyscallResult::Ok(timestamp)
}

/// Sleep for specified duration
fn syscall_sleep(args: SyscallArgs) -> SyscallResult {
    let duration_ms = args.arg0;
    
    let current_sip = get_current_sip();
    
    // Block current SIP for specified duration
    current_sip.state = SipState::Blocked(BlockReason::Sleep(Duration(duration_ms)));
    
    // Scheduler will wake us up
    SyscallResult::Ok(0)
}

/// Clone operation for Go runtime (creating threads)
fn syscall_clone(args: SyscallArgs) -> SyscallResult {
    let flags = args.arg0 as u32;
    let stack_ptr = args.arg1 as usize;
    let _parent_tid = args.arg2 as *mut u32;
    let _child_tid = args.arg3 as *mut u32;
    let _tls = args.arg4 as usize;
    
    // For Go runtime, we just create a new goroutine, not a real thread
    let current_sip = get_current_sip();
    
    // Validate stack
    if !current_sip.memory_space.is_valid_address(stack_ptr) {
        return SyscallResult::Error(SyscallError::InvalidArgument);
    }
    
    // Create new goroutine
    let goroutine = Goroutine::new(stack_ptr, flags);
    let goroutine_id = current_sip.go_runtime.add_goroutine(goroutine);
    
    SyscallResult::Ok(goroutine_id.0)
}

/// Futex operation for Go runtime synchronization
fn syscall_futex(args: SyscallArgs) -> SyscallResult {
    let futex_ptr = args.arg0 as *mut u32;
    let op = args.arg1 as u32;
    let val = args.arg2 as u32;
    let timeout_ptr = args.arg3 as *const TimeSpec;
    
    // Validate futex address
    if !is_valid_user_buffer(futex_ptr as *mut u8, 4) {
        return SyscallResult::Error(SyscallError::InvalidArgument);
    }
    
    match op {
        FUTEX_WAIT => {
            // Block until futex value changes
            block_on_futex(futex_ptr, val, timeout_ptr)
        }
        FUTEX_WAKE => {
            // Wake up waiters
            let woken = wake_futex_waiters(futex_ptr, val);
            SyscallResult::Ok(woken as u64)
        }
        _ => SyscallResult::Error(SyscallError::InvalidArgument),
    }
}

/// Userspace helper functions for Go runtime
pub mod go_runtime_helpers {
    use super::*;

    /// Fast path for Go allocator
    #[no_mangle]
    pub extern "C" fn hurd9_go_alloc(size: usize) -> *mut u8 {
        let current_sip = get_current_sip();
        
        match current_sip.go_runtime.heap.allocate(size) {
            Ok(ptr) => ptr,
            Err(_) => core::ptr::null_mut(),
        }
    }

    /// Fast path for Go garbage collection trigger
    #[no_mangle]
    pub extern "C" fn hurd9_go_gc_trigger() {
        let current_sip = get_current_sip();
        current_sip.go_runtime.gc.trigger_collection();
    }

    /// Fast path for goroutine scheduling
    #[no_mangle]
    pub extern "C" fn hurd9_go_yield() {
        let current_sip = get_current_sip();
        current_sip.go_runtime.scheduler.yield_current();
    }
}

// System call assembly stubs for userspace
#[cfg(target_arch = "x86_64")]
pub mod syscall_asm {
    use super::*;

    /// Raw system call assembly
    #[inline(always)]
    pub unsafe fn raw_syscall(
        num: u64,
        arg0: u64,
        arg1: u64,
        arg2: u64,
        arg3: u64,
        arg4: u64,
        arg5: u64,
    ) -> u64 {
        let result: u64;
        asm!(
            "syscall",
            inlateout("rax") num => result,
            in("rdi") arg0,
            in("rsi") arg1,
            in("rdx") arg2,
            in("r10") arg3,
            in("r8") arg4,
            in("r9") arg5,
            lateout("rcx") _,
            lateout("r11") _,
            options(nostack, preserves_flags)
        );
        result
    }

    /// Convenient syscall wrapper
    #[inline]
    pub fn syscall(num: SyscallNumber, args: &[u64]) -> SyscallResult {
        let args_padded = [
            args.get(0).copied().unwrap_or(0),
            args.get(1).copied().unwrap_or(0),
            args.get(2).copied().unwrap_or(0),
            args.get(3).copied().unwrap_or(0),
            args.get(4).copied().unwrap_or(0),
            args.get(5).copied().unwrap_or(0),
        ];

        let result = unsafe {
            raw_syscall(
                num as u64,
                args_padded[0],
                args_padded[1],
                args_padded[2],
                args_padded[3],
                args_padded[4],
                args_padded[5],
            )
        };

        if result > (-4096i64 as u64) {
            // Error case
            SyscallResult::Error(SyscallError::InvalidArgument)
        } else {
            SyscallResult::Ok(result)
        }
    }
}

// Constants and supporting types
const MAX_MMAP_SIZE: usize = 1 << 48; // 256TB max
const FUTEX_WAIT: u32 = 0;
const FUTEX_WAKE: u32 = 1;

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
pub struct FileDescriptor(pub u32);

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
pub struct ChannelId(pub u64);

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
pub struct GoroutineId(pub u64);

#[derive(Debug, Clone, Copy)]
pub struct Duration(pub u64);

#[derive(Debug, Clone, Copy)]
pub struct TimeSpec {
    pub tv_sec: i64,
    pub tv_nsec: i64,
}

#[derive(Debug)]
pub enum ChannelType {
    Synchronous,
    Asynchronous,
    Holographic,
}

#[derive(Debug)]
pub enum IPCError {
    WouldBlock,
    ChannelClosed,
    PermissionDenied,
}

// Placeholder functions - would be implemented by the kernel
fn validate_syscall_permission(_syscall: SyscallNumber) -> bool { true }
fn get_current_sip() -> &'static mut SIP { todo!() }
fn get_sip_manager() -> &'static mut SipManager { todo!() }
fn read_user_string(_ptr: *const u8, _len: usize) -> Result<String, ()> { todo!() }
fn read_user_buffer(_ptr: *const u8, _len: usize) -> Result<Vec<u8>, ()> { todo!() }
fn read_user_string_array(_ptr: *const *const u8, _count: usize) -> Result<Vec<String>, ()> { todo!() }
fn is_valid_user_buffer(_ptr: *mut u8, _len: usize) -> bool { true }
fn create_ipc_channel(_sip: SipId, _name: String, _typ: ChannelType) -> Result<ChannelId, ()> { todo!() }
fn send_ipc_message(_channel: ChannelId, _data: Vec<u8>, _blocking: bool) -> Result<(), IPCError> { todo!() }
fn receive_ipc_message(_channel: ChannelId, _blocking: bool) -> Result<Vec<u8>, IPCError> { todo!() }
fn get_system_time() -> u64 { 0 }
fn block_on_futex(_ptr: *mut u32, _val: u32, _timeout: *const TimeSpec) -> SyscallResult { todo!() }
fn wake_futex_waiters(_ptr: *mut u32, _count: u32) -> u32 { 0 }

use crate::process::sip::{SIP, SipId, SipState, BlockReason, SipManager};
use crate::process::sip::{Goroutine, CapabilitySet};

/// Demo showing syscall usage
pub fn demo_syscalls() {
    println!("Hurd-9 Syscall Interface Demo:");
    println!("1. Go runtime calls hurd9_go_alloc() for fast allocation");
    println!("2. File I/O goes through 9P protocol");
    println!("3. IPC uses exchange heaps and holographic channels");
    println!("4. Minimal syscall surface - most work in userspace");
}