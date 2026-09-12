#pragma once

#include <string>

namespace Mc3ds {
    inline const std::string rebuildTemplate = R"MC3DS(BasicInfo:
  Title: Proteus
  ProductCode: @PRODUCT_CODE@
  Logo: Nintendo

TitleInfo:
  Category: Application
  UniqueId: @UNIQUE_ID@

Option:
  UseOnSD: true
  FreeProductCode: true
  MediaFootPadding: false
  EnableCrypt: false
  EnableCompress: true

AccessControlInfo:
  CoreVersion: 2
  Priority: 16
  MaxCpu: 158
  IdealProcessor: 0
  AffinityMask: 1
  SystemMode: 80MB
  SystemModeExt: Legacy
  CpuSpeed: 268mhz
  EnableL2Cache: false
  CanAccessCore2: false
  MemoryType: application
  ResourceLimitCategory: application
  UseExtSaveData: true
  ExtSaveDataId: @UNIQUE_ID@
  HandleTableSize: 0x200
  ReleaseKernelMajor: 2
  ReleaseKernelMinor: 51
  DescVersion: 3
  ServiceAccessControl:
    - "$hioFIO"
    - "$hostio0"
    - "$hostio1"
    - "cfg:u"
    - "fs:USER"
    - "gsp::Gpu"
    - "hid:USER"
    - "ndm:u"
    - "pxi:dev"
    - "APT:A"
    - "ac:u"
    - "act:u"
    - "am:app"
    - "boss:U"
    - "cam:u"
    - "cecd:u"
    - "dlp:FKCL"
    - "dlp:SRVR"
    - "dsp::DSP"
    - "frd:u"
    - "http:C"
    - "ir:USER"
    - "ldr:ro"
    - "mic:u"
    - "news:u"
    - "nfc:u"
    - "nim:aoc"
    - "nwm::UDS"
    - "ptm:u"
    - "qtm:u"
    - "soc:U"
    - "ssl:C"
    - "y2r:u"
  IORegisterMapping:
    - 1FF50000-1FF57FFF
    - 1FF70000-1FF77FFF
  MemoryMapping:
    - 1F000000-1F5FFFFF:r
  SystemCallAccess:
    ControlMemory: 0x01
    QueryMemory: 0x02
    ExitProcess: 0x03
    GetProcessIdealProcessor: 0x06
    CreateThread: 0x08
    ExitThread: 0x09
    SleepThread: 0x0A
    GetThreadPriority: 0x0B
    SetThreadPriority: 0x0C
    GetThreadIdealProcessor: 0x0F
    GetCurrentProcessorNumber: 0x11
    CreateMutex: 0x13
    ReleaseMutex: 0x14
    CreateSemaphore: 0x15
    ReleaseSemaphore: 0x16
    CreateEvent: 0x17
    SignalEvent: 0x18
    ClearEvent: 0x19
    CreateTimer: 0x1A
    SetTimer: 0x1B
    CancelTimer: 0x1C
    ClearTimer: 0x1D
    CreateMemoryBlock: 0x1E
    MapMemoryBlock: 0x1F
    UnmapMemoryBlock: 0x20
    CreateAddressArbiter: 0x21
    ArbitrateAddress: 0x22
    CloseHandle: 0x23
    WaitSynchronization1: 0x24
    WaitSynchronizationN: 0x25
    DuplicateHandle: 0x27
    GetSystemTick: 0x28
    GetHandleInfo: 0x29
    GetSystemInfo: 0x2A
    GetProcessInfo: 0x2B
    GetThreadInfo: 0x2C
    ConnectToPort: 0x2D
    SendSyncRequest1: 0x2E
    SendSyncRequest2: 0x2F
    SendSyncRequest3: 0x30
    SendSyncRequest4: 0x31
    SendSyncRequest: 0x32
    GetProcessId: 0x35
    GetProcessIdOfThread: 0x36
    GetThreadId: 0x37
    GetResourceLimit: 0x38
    GetResourceLimitLimitValues: 0x39
    GetResourceLimitCurrentValues: 0x3A
    GetThreadContext: 0x3B
    Break: 0x3C
    OutputDebugString: 0x3D

SystemControlInfo:
  SaveDataSize: 0K
  RemasterVersion: 0
  StackSize: 0x40000
  Dependency:
    ac: 0x0004013000002402
    act: 0x0004013000003802
    am: 0x0004013000001502
    boss: 0x0004013000003402
    camera: 0x0004013000001602
    cecd: 0x0004013000002602
    cfg: 0x0004013000001702
    codec: 0x0004013000001802
    csnd: 0x0004013000002702
    dlp: 0x0004013000002802
    dsp: 0x0004013000001a02
    friends: 0x0004013000003202
    gpio: 0x0004013000001b02
    gsp: 0x0004013000001c02
    hid: 0x0004013000001d02
    http: 0x0004013000002902
    i2c: 0x0004013000001e02
    ir: 0x0004013000003302
    mcu: 0x0004013000001f02
    mic: 0x0004013000002002
    ndm: 0x0004013000002b02
    news: 0x0004013000003502
    nfc: 0x0004013000004002
    nim: 0x0004013000002c02
    nwm: 0x0004013000002d02
    pdn: 0x0004013000002102
    ps: 0x0004013000003102
    ptm: 0x0004013000002202
    qtm: 0x0004013020004202
    ro: 0x0004013000003702
    socket: 0x0004013000002e02
    spi: 0x0004013000002302
    ssl: 0x0004013000002f02
)MC3DS";
}
