# Frama-C Plan 9 Plugin

A Frama-C plugin that enables seamless formal verification of Plan 9 C code.

## ⚠️ Status: Alternative Approach Available

This OCaml plugin was designed for older versions of Frama-C (≤28.0) that used `Makefile.dynamic` for plugin builds.

**For Frama-C 31.0+ (Gallium and newer), use the wrapper script instead:**

```bash
# Recommended for all versions
./scripts/frama-c-plan9 -wp kernel/9front-port/devram.c
```

The wrapper script provides the same functionality without requiring OCaml compilation:
- ✅ Works with any Frama-C version
- ✅ No build dependencies
- ✅ Simpler maintenance
- ✅ Same user experience

See `../../README_FRAMAC.md` for complete documentation.

## Features (Plugin Approach)

- ✅ Automatic Plan 9 preprocessing
- ✅ Provides missing type definitions (Fmt, Qid, Waitmsg, uuid_t)
- ✅ Filters incompatible pragmas
- ✅ Configures correct preprocessor options
- ✅ Works with any Plan 9 codebase

## Building (For Frama-C ≤28.0 only)

### Prerequisites

- Frama-C version ≤28.0 (versions with Makefile.dynamic support)
- OCaml 4.08+
- GNU Make

### Build Steps

```bash
cd frama-c-plugin/plan9
make all
```

**Note**: This will NOT work with Frama-C 31.0+ (Gallium) due to build system changes. For modern Frama-C, use the wrapper script approach.

## Installation (Legacy)

### System-wide Installation

```bash
make install
```

This installs the plugin to your Frama-C installation directory.

### Local Use

You can also load the plugin directly without installing:

```bash
frama-c -load-module ./Plan9.cmxs -plan9 yourfile.c
```

## Usage

### Basic Usage

```bash
frama-c -plan9 kernel/9front-port/devram.c
```

### With Verification

```bash
frama-c -plan9 -wp -wp-rte kernel/9front-port/devram.c
```

### With Verbose Output

```bash
frama-c -plan9 -plan9-verbose kernel/9front-port/devram.c
```

### With Value Analysis

```bash
frama-c -plan9 -eva kernel/msgord.c
```

## Plugin Options

- **`-plan9`**: Enable Plan 9 compatibility mode (required)
- **`-plan9-verbose`**: Print debug information about plugin actions

## How It Works

1. **Type Injection**: Adds definitions for types excluded by `#ifndef __FRAMAC__` in Plan 9 headers
2. **Pragma Filtering**: Removes Plan 9-specific pragmas that Frama-C doesn't understand
3. **Preprocessor Configuration**: Sets up correct include paths and defines

## Example

### Verifying secure_wipe()

```bash
# Without plugin (old way - requires manual preprocessing)
./scripts/framac_plan9_v2.sh kernel/9front-port/devram.c /tmp/devram_fc.c
frama-c -wp /tmp/devram_fc.c

# With plugin (new way - automatic!)
frama-c -plan9 -wp kernel/9front-port/devram.c
```

## Development

### Plugin Structure

```
plan9/
├── Makefile           # Build system
├── plan9_plugin.ml    # Main implementation
├── plan9_plugin.mli   # Interface
└── README.md          # This file
```

### Rebuilding

```bash
make clean
make all
```

### Testing

```bash
make test
```

## Compatibility

Works with:
- 9front kernel
- Plan 9 from Bell Labs
- Inferno OS
- Harvey OS
- Any Plan 9 codebase

## License

Same as lux9-kernel project.

## Technical Details

### Type Definitions Provided

The plugin automatically includes:

```c
typedef unsigned char uuid_t[16];

struct Fmt {
    unsigned char runes;
    void *start;
    void *to;
    void *stop;
    int (*flush)(Fmt *);
    void *farg;
    int nfmt;
    void *args;
    int r;
    int width;
    int prec;
    unsigned long flags;
};

struct Qid {
    unsigned long long path;
    unsigned long vers;
    unsigned char type;
};

struct Waitmsg {
    int pid;
    unsigned long time[3];
    char msg[128];
};
```

### Pragmas Filtered

- `#pragma varargck`
- `#pragma lib`
- `#pragma src`
- `#pragma incomplete`
- `#pragma pack`
- `#pragma textflag`
- `#pragma profile`

## Troubleshooting

### Plugin Not Found

```bash
# Check Frama-C can find the plugin
frama-c -print-plugin-path

# Verify plugin exists
ls $(frama-c -print-libpath)/plugins/Plan9.cmxs
```

### Build Errors

```bash
# Check Frama-C version
frama-c -version

# Ensure OCaml compiler matches Frama-C's
ocamlc -version
```

### Include Path Issues

The plugin automatically adds these paths if they exist:
- `kernel/include`
- `kernel/9front-pc64`
- `kernel/9front-port`

Run from the lux9-kernel root directory, or configure paths manually.

## Support

For issues, see the main lux9-kernel repository documentation.
