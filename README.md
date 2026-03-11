# DSP Embedded

A lean [Dataspace Protocol (DSP)](https://docs.internationaldataspaces.org/ids-knowledgebase/v/dataspace-protocol) reference implementation in C for resource-constrained embedded systems, targeting the ESP32-S3.

See [target.md](target.md) for full project description and design constraints.
See [development_plan.md](development_plan.md) for the implementation roadmap.

## License

Apache License 2.0 — see [LICENSE](LICENSE).

## Status

⚠️ Under development — see `development_plan.md` for progress.

## Requirements

- [ESP-IDF](https://github.com/espressif/esp-idf) v5.5 or later (firmware build)
- Target hardware: ESP32-S3 (4 MB flash minimum)
- CMake ≥ 3.16, GCC ≥ 13 (host-native build and unit tests)

## Building

```sh
# Set up ESP-IDF environment (once per shell session)
. $IDF_PATH/export.sh

# Configure and build
idf.py set-target esp32s3
idf.py build

# Flash and monitor
idf.py flash monitor
```

## Project Structure

```
dsp_embedded/
├── main/               # Application entry point
├── components/         # DSP Embedded library components (added per milestone)
├── test/               # Unit tests (Unity, host-native build)
├── doc/                # Design documentation
├── tools/              # Helper scripts (provisioning, etc.)
├── partitions.csv      # Custom flash partition table
├── sdkconfig.defaults  # Default Kconfig settings for ESP32-S3
└── CMakeLists.txt      # Top-level build file
```

## Running Tests (host-native)

```sh
cd test
cmake -B build && cmake --build build
./build/dsp_test_runner

# Or via CTest
cmake --build build && ctest --test-dir build --output-on-failure
```
