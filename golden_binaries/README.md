# Golden Binaries Archive

This directory contains reference binaries that represent successful implementations at various stages. These binaries are preserved to prevent loss of working implementations and to serve as comparison points when debugging regressions.

## Stage 14: Quiescence Search

### seajay-stage14-sprt-candidate1-GOLDEN
- **Date:** August 14, 2025
- **Size:** 411,336 bytes
- **MD5:** 0b0ea4c7f8f0aa60079a2ccc2997bf88
- **Performance:** +300 ELO over Stage 13
- **Significance:** This binary saved us from losing the quiescence implementation

**Story:** This binary was created during initial Stage 14 testing with manually added ENABLE_QUIESCENCE flag. When attempts to "fix" minor time losses caused catastrophic regression, this binary proved that the +300 ELO gain was real. Investigation revealed that all subsequent builds had quiescence disabled due to the missing compiler flag.

**Lessons:**
1. Always preserve working binaries
2. Check binary sizes when debugging
3. Never use compile-time flags for core features
4. Trust SPRT results over minor issues

## Usage

These binaries should NEVER be deleted. They serve as:
- Reference implementations for comparison
- Fallback options if current development fails
- Proof that certain performance levels are achievable
- Historical record of the development journey

## Verification

To verify a golden binary:
```bash
# Check size
ls -la seajay-stage14-sprt-candidate1-GOLDEN

# Check MD5
md5sum seajay-stage14-sprt-candidate1-GOLDEN

# Check version
echo -e "uci\nquit" | ./seajay-stage14-sprt-candidate1-GOLDEN | grep "id name"
```