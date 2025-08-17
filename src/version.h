#pragma once

// SeaJay Version Information
// Format: Major.Minor.Patch-Tag
// Major: Phase number (0 for pre-release)
// Minor: Stage number
// Patch: Remediation/revision number
// Tag: Additional info (remediated, dev, etc.)

#define SEAJAY_VERSION_MAJOR 0
#define SEAJAY_VERSION_MINOR 10
#define SEAJAY_VERSION_PATCH 1
#define SEAJAY_VERSION_TAG "remediated"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define SEAJAY_VERSION \
    TOSTRING(SEAJAY_VERSION_MAJOR) "." \
    TOSTRING(SEAJAY_VERSION_MINOR) "." \
    TOSTRING(SEAJAY_VERSION_PATCH) "-" \
    SEAJAY_VERSION_TAG

// Example: "0.10.1-remediated" for Stage 10 remediation