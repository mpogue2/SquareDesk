#!/usr/bin/env swift
// Simulates a BT remote Stop command by sending it through macOS MediaRemote.framework.
// SquareDesk must be running. Run with:  swift send_stop_command.swift

import Foundation

let bundleURL = NSURL(fileURLWithPath: "/System/Library/PrivateFrameworks/MediaRemote.framework")
guard let bundle = CFBundleCreate(kCFAllocatorDefault, bundleURL) else {
    print("ERROR: Could not load MediaRemote.framework")
    exit(1)
}

typealias MRMediaRemoteSendCommandType = @convention(c) (UInt32, AnyObject?) -> Bool
guard let ptr = CFBundleGetFunctionPointerForName(bundle, "MRMediaRemoteSendCommand" as CFString) else {
    print("ERROR: Could not find MRMediaRemoteSendCommand")
    exit(1)
}
let MRMediaRemoteSendCommand = unsafeBitCast(ptr, to: MRMediaRemoteSendCommandType.self)

// MediaRemote command IDs (from MRMediaRemoteCommand enum in private headers)
// kMRPlay = 0, kMRPause = 1, kMRTogglePlayPause = 2, kMRStop = 3
let kMRStop: UInt32 = 3

let result = MRMediaRemoteSendCommand(kMRStop, nil)
print("Stop command sent (result: \(result))")
