#pragma once

// Wire protocol string constants, kept in sync with the Python protocol.py.
// Messages are single-frame JSON objects sent over ZeroMQ.

namespace proto {

// Message types.
constexpr auto kAction = "action";            // Qt  -> Python
constexpr auto kCommand = "command";          // Qt  -> Python
constexpr auto kObservation = "observation";  // Python -> Qt
constexpr auto kFeatures = "features";        // Python -> Qt
constexpr auto kStatus = "status";            // Python -> Qt

// Control-plane command verbs.
constexpr auto kEnable = "enable";
constexpr auto kDisable = "disable";
constexpr auto kHome = "home";
constexpr auto kEstop = "estop";

constexpr int kVersion = 1;

}  // namespace proto
