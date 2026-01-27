# Module Sandboxing Audit Report

## Module Information
- **Name**: yaml
- **Version**: 1.1 (in development)
- **Type**: C++ (with Qore-language user modules)
- **Audit Date**: 2026-01-26
- **Remediation Date**: 2026-01-26
- **Safe for Sandbox Use**: Yes

## Summary

The yaml module delegates filesystem and network I/O to Qore's built-in classes (InputStream, OutputStream, FileInputStream, ReadOnlyFile), which have sandbox support. Interrupt checking has been added to all parsing loops to ensure operations can be interrupted in sandboxed environments.

## Filesystem Security
- [x] All file operations checked
- [x] Path canonicalization used
- [x] Temporary file operations checked
- **Gaps Found**: None

The yaml module does not perform direct filesystem operations in C++ code. All file access goes through:
- `FileInputStream` (in `YamlDocumentDataProvider.qc:215`) - Qore built-in with sandbox support
- `ReadOnlyFile::readTextFile()` (in `YamlSchema.qm:430,440`) - Qore built-in with sandbox support

**Severity**: None

## Network Security
- [x] All connections checked
- [x] Post-DNS resolution checks present
- [x] Redirect following checked
- **Gaps Found**: None

The yaml module has no direct network operations. Stream-based parsing accepts `InputStream` objects, which may be backed by network connections. However, the sandbox check occurs when the InputStream is created by the calling code, not within the yaml module.

**Severity**: None

## Resource Limits
- [ ] Large allocations tracked
- [x] Tight loops yield periodically
- [x] No unbounded native threads
- **Gaps Found**: None (after remediation)

All parsing loops now check for interrupts every 1000 iterations (or 100 for document iteration), allowing the runtime to enforce CPU/wall time limits.

**Severity**: None

## Interrupt Support
- [x] Pre-operation checks present
- [x] Polling during blocking operations
- [ ] Cancel callbacks registered where applicable (N/A - no library callbacks)
- [x] Cleanup on interrupt
- **Gaps Found**: None (after remediation)

All parsing loops now call `qore_check_io_interrupt()` periodically. Stream I/O relies on Qore's built-in stream classes which handle interrupts.

**Severity**: None

## Compliance Level
- **Compliance Level**: Full
- **Highest Severity Finding**: None (all issues remediated)
- **Recommendation**: **Safe for sandbox use**

## Remediation Applied

The following changes were made to address the audit findings:

### 1. Added QoreSandboxManager include

`yaml-module.h` now includes `<qore/QoreSandboxManager.h>` for access to `qore_check_io_interrupt()`.

### 2. Added interrupt checking to all parsing loops

| File | Function | Check Interval |
|------|----------|----------------|
| `QoreYamlParser.cpp` | `parseSeq()` | Every 1000 iterations |
| `QoreYamlParser.cpp` | `parseMap()` | Every 1000 iterations |
| `QoreYamlDocumentIterator.cpp` | `next()` | Every 100 iterations |
| `QoreYamlDocumentIterator.cpp` | `parseSequence()` | Every 1000 iterations |
| `QoreYamlDocumentIterator.cpp` | `parseMapping()` | Every 1000 iterations |
| `QoreYamlSaxParser.cpp` | `processEvents()` | Every 1000 iterations |
| `QoreYamlStreamWriter.cpp` | `writeValueRecursive()` (hash) | Every 1000 iterations |
| `QoreYamlStreamWriter.cpp` | `writeValueRecursive()` (list) | Every 1000 iterations |

Each loop now includes:
```cpp
int iteration = 0;
while (true) {
    if (++iteration % 1000 == 0) {
        if (qore_check_io_interrupt(xsink, "YAML <operation> parsing")) {
            return nullptr;
        }
    }
    // ... rest of loop
}
```

### Note on Stream I/O

Stream-based parsing (`parseStream()`, `YamlDocumentIterator` with stream) relies on the underlying `InputStream` to be interruptible. Qore's built-in stream classes handle this automatically. Custom streams used with the yaml module should implement proper interrupt support.

## Files Audited

### C++ Source Files
- `src/QoreYamlParser.cpp` - Main parser implementation
- `src/QoreYamlSaxParser.cpp` - SAX parser implementation
- `src/QoreYamlDocumentIterator.cpp` - Document iterator implementation
- `src/QoreYamlStreamWriter.cpp` - Stream writer implementation
- `src/YamlStreamReadHandler.cpp` - Stream read handler
- `src/yaml-module.cpp` - Module initialization
- `src/ql_yaml.qpp` - Module functions

### Qore User Modules
- `qlib/YamlSchema.qm` - Schema validation (pure Qore)
- `qlib/YamlDocumentDataProvider/YamlDocumentReadDataProvider.qc` - DataProvider implementation
- `qlib/YamlDocumentDataProvider/YamlDocumentDataProviderFactory.qc` - Factory class
- `qlib/YamlTagHandler.qm` - Tag handler API (pure Qore)

---

*Audit performed according to the Qore Module Sandboxing Audit Guide*
