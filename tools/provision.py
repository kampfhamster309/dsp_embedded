#!/usr/bin/env python3
"""
provision.py – Flash device certificate and PSK credentials to the ESP32-S3.

Writes credentials into the 'dsp_certs' NVS partition (offset 0x190000,
64 KB) using nvs_partition_gen.py from ESP-IDF, then flashes the generated
binary with esptool.py.

NVS layout written by this tool:
  Namespace 'dsp_identity':
    cert_der   – DER-encoded device X.509 certificate   (binary blob)
    key_der    – DER-encoded device private key          (binary blob)

  Namespace 'dsp_psk' (optional, only when --psk-* flags are supplied):
    identity   – PSK identity bytes (1–64 bytes)         (binary blob)
    psk_key    – PSK key bytes (16–32 bytes)             (binary blob)

Usage:
    python3 tools/provision.py \\
        --cert  components/dsp_identity/certs/device_cert.der \\
        --key   components/dsp_identity/certs/device_key.der  \\
        [--psk-identity <hex-string>]  \\
        [--psk-key      <hex-string>]  \\
        [--port /dev/ttyACM1]          \\
        [--baud 115200]                \\
        [--idf-path ~/esp/esp-idf]     \\
        [--dry-run]

Arguments:
    --cert          Path to DER-encoded device certificate (required)
    --key           Path to DER-encoded private key (required)
    --psk-identity  PSK identity as a hex string, e.g. 64737031 (optional)
    --psk-key       PSK key as a hex string, 32–64 hex chars (optional,
                    required when --psk-identity is given)
    --port          Serial port connected to the ESP32-S3
                    (default: /dev/ttyACM1)
    --baud          Serial baud rate (default: 115200)
    --idf-path      Path to ESP-IDF installation
                    (default: $IDF_PATH env var or ~/esp/esp-idf)
    --dry-run       Generate the NVS binary but skip the flash step
    --output        Path to write the generated NVS binary
                    (default: <tmpdir>/dsp_certs.bin)

Examples:
    # Provision cert+key only:
    python3 tools/provision.py \\
        --cert components/dsp_identity/certs/device_cert.der \\
        --key  components/dsp_identity/certs/device_key.der

    # Provision cert+key + PSK:
    python3 tools/provision.py \\
        --cert         components/dsp_identity/certs/device_cert.der \\
        --key          components/dsp_identity/certs/device_key.der  \\
        --psk-identity 6473705f656d6265 \\
        --psk-key      deadbeefdeadbeefdeadbeefdeadbeef

    # Dry run (generate binary, do not flash):
    python3 tools/provision.py \\
        --cert  components/dsp_identity/certs/device_cert.der \\
        --key   components/dsp_identity/certs/device_key.der  \\
        --dry-run --output /tmp/dsp_certs.bin
"""

import argparse
import os
import subprocess
import sys
import tempfile
import textwrap
from pathlib import Path

# ---------------------------------------------------------------------------
# Constants matching partitions.csv
# ---------------------------------------------------------------------------

PARTITION_OFFSET = 0x190000   # dsp_certs partition offset
PARTITION_SIZE   = 0x10000    # 64 KB

# NVS key name limits (ESP-IDF: max 15 chars for namespace + key names)
_NVS_KEY_MAX = 15

# PSK constraints (must match dsp_psk.h)
PSK_MIN_KEY_BYTES = 16
PSK_MAX_KEY_BYTES = 32
PSK_MAX_ID_BYTES  = 64


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _die(msg: str) -> None:
    sys.stdout.flush()
    print(f"[provision] ERROR: {msg}", file=sys.stderr, flush=True)
    sys.exit(1)


def _info(msg: str) -> None:
    print(f"[provision] {msg}", flush=True)


def _validate_hex(hex_str: str, name: str) -> bytes:
    """Parse a hex string and return bytes; die on invalid input."""
    s = hex_str.strip().replace(" ", "").replace(":", "")
    if len(s) % 2 != 0:
        _die(f"{name}: odd-length hex string")
    try:
        return bytes.fromhex(s)
    except ValueError as exc:
        _die(f"{name}: invalid hex string – {exc}")


def _find_nvs_gen(idf_path: Path) -> Path:
    """Locate nvs_partition_gen.py inside the ESP-IDF tree."""
    candidate = (
        idf_path
        / "components"
        / "nvs_flash"
        / "nvs_partition_generator"
        / "nvs_partition_gen.py"
    )
    if candidate.is_file():
        return candidate
    _die(
        f"nvs_partition_gen.py not found at {candidate}\n"
        "  Make sure --idf-path points to a valid ESP-IDF installation."
    )


def _find_idf_path(explicit: str | None) -> Path:
    """Return the ESP-IDF root directory."""
    if explicit:
        p = Path(explicit).expanduser().resolve()
        if not p.is_dir():
            _die(f"--idf-path does not exist: {p}")
        return p
    env = os.environ.get("IDF_PATH")
    if env:
        return Path(env).resolve()
    default = Path.home() / "esp" / "esp-idf"
    if default.is_dir():
        return default
    _die(
        "ESP-IDF not found.  Set $IDF_PATH, pass --idf-path, or install "
        "ESP-IDF to ~/esp/esp-idf."
    )


def _find_esptool(idf_path: Path) -> str:
    """Return the esptool.py command to use."""
    # Prefer the one in ESP-IDF's Python virtual environment
    candidates = [
        idf_path / "tools" / "idf_tools.py",
        Path(sys.executable).parent / "esptool.py",
    ]
    # Try the ESP-IDF venv esptool first
    idf_venv_esptool = (
        idf_path / ".espressif" / "python_env" / "idf5.5_py3.12_env"
        / "bin" / "esptool.py"
    )
    if idf_venv_esptool.is_file():
        return str(idf_venv_esptool)
    # Fall back to module invocation via the current Python
    return f"{sys.executable} -m esptool"


# ---------------------------------------------------------------------------
# NVS CSV generation
# ---------------------------------------------------------------------------

def _write_csv(
    csv_path: Path,
    cert_path: Path,
    key_path: Path,
    psk_id: bytes | None,
    psk_key: bytes | None,
    tmp_dir: Path,
) -> None:
    """Write the NVS partition CSV file."""
    lines = [
        "key,type,encoding,value",
        # Namespace: dsp_identity
        "dsp_identity,namespace,,",
        f"cert_der,file,binary,{cert_path}",
        f"key_der,file,binary,{key_path}",
    ]

    if psk_id is not None and psk_key is not None:
        # Write PSK identity and key as temporary binary files so the
        # nvs_partition_gen 'file,binary' encoding works correctly for
        # arbitrary byte sequences (hex2bin is limited to inline hex).
        id_bin = tmp_dir / "psk_identity.bin"
        key_bin = tmp_dir / "psk_key.bin"
        id_bin.write_bytes(psk_id)
        key_bin.write_bytes(psk_key)

        lines += [
            # Namespace: dsp_psk
            "dsp_psk,namespace,,",
            f"identity,file,binary,{id_bin}",
            f"psk_key,file,binary,{key_bin}",
        ]

    csv_path.write_text("\n".join(lines) + "\n")
    _info(f"NVS CSV written to {csv_path}")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Flash DSP Embedded device credentials to the ESP32-S3.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=textwrap.dedent("""\
            Examples:
              # Cert + key only
              python3 tools/provision.py \\
                  --cert components/dsp_identity/certs/device_cert.der \\
                  --key  components/dsp_identity/certs/device_key.der

              # Cert + key + PSK
              python3 tools/provision.py \\
                  --cert         components/dsp_identity/certs/device_cert.der \\
                  --key          components/dsp_identity/certs/device_key.der  \\
                  --psk-identity 6473705f656d62656464656400000000 \\
                  --psk-key      deadbeefdeadbeefdeadbeefdeadbeef

              # Dry run – generate binary, skip flash
              python3 tools/provision.py \\
                  --cert    components/dsp_identity/certs/device_cert.der \\
                  --key     components/dsp_identity/certs/device_key.der  \\
                  --dry-run --output /tmp/dsp_certs.bin
        """),
    )

    parser.add_argument(
        "--cert", required=True,
        help="Path to DER-encoded device certificate.",
    )
    parser.add_argument(
        "--key", required=True,
        help="Path to DER-encoded device private key.",
    )
    parser.add_argument(
        "--psk-identity", default=None,
        help="PSK identity as a hex string (1–64 bytes).  "
             "Example: 6473705f6964 (= 'dsp_id' in ASCII hex).",
    )
    parser.add_argument(
        "--psk-key", default=None,
        help="PSK key as a hex string (16–32 bytes = 32–64 hex chars).  "
             "Example: deadbeefdeadbeefdeadbeefdeadbeef  (16 bytes).",
    )
    parser.add_argument(
        "--port", default="/dev/ttyACM1",
        help="Serial port connected to the device (default: /dev/ttyACM1).",
    )
    parser.add_argument(
        "--baud", type=int, default=115200,
        help="Serial baud rate (default: 115200).",
    )
    parser.add_argument(
        "--idf-path", default=None,
        help="Path to ESP-IDF installation "
             "(default: $IDF_PATH or ~/esp/esp-idf).",
    )
    parser.add_argument(
        "--dry-run", action="store_true",
        help="Generate NVS binary but do not flash the device.",
    )
    parser.add_argument(
        "--output", default=None,
        help="Path to write the generated NVS binary "
             "(default: <tmpdir>/dsp_certs.bin).",
    )

    args = parser.parse_args()

    # ------------------------------------------------------------------
    # Validate inputs
    # ------------------------------------------------------------------

    cert_path = Path(args.cert).resolve()
    key_path  = Path(args.key).resolve()

    if not cert_path.is_file():
        _die(f"Certificate file not found: {cert_path}")
    if not key_path.is_file():
        _die(f"Key file not found: {key_path}")

    cert_size = cert_path.stat().st_size
    key_size  = key_path.stat().st_size
    if cert_size == 0:
        _die(f"Certificate file is empty: {cert_path}")
    if key_size == 0:
        _die(f"Key file is empty: {key_path}")

    _info(f"Certificate : {cert_path}  ({cert_size} bytes)")
    _info(f"Private key : {key_path}  ({key_size} bytes)")

    # PSK validation
    psk_id_bytes:  bytes | None = None
    psk_key_bytes: bytes | None = None

    if args.psk_identity or args.psk_key:
        if not args.psk_identity:
            _die("--psk-key requires --psk-identity to also be specified.")
        if not args.psk_key:
            _die("--psk-identity requires --psk-key to also be specified.")

        psk_id_bytes  = _validate_hex(args.psk_identity, "--psk-identity")
        psk_key_bytes = _validate_hex(args.psk_key, "--psk-key")

        if not (1 <= len(psk_id_bytes) <= PSK_MAX_ID_BYTES):
            _die(
                f"--psk-identity must be 1–{PSK_MAX_ID_BYTES} bytes "
                f"(got {len(psk_id_bytes)})."
            )
        if not (PSK_MIN_KEY_BYTES <= len(psk_key_bytes) <= PSK_MAX_KEY_BYTES):
            _die(
                f"--psk-key must be {PSK_MIN_KEY_BYTES}–{PSK_MAX_KEY_BYTES} "
                f"bytes (got {len(psk_key_bytes)}).  "
                f"Provide {PSK_MIN_KEY_BYTES*2}–{PSK_MAX_KEY_BYTES*2} hex chars."
            )
        _info(f"PSK identity: {len(psk_id_bytes)} bytes")
        _info(f"PSK key     : {len(psk_key_bytes)} bytes")

    # ------------------------------------------------------------------
    # Locate ESP-IDF tooling
    # ------------------------------------------------------------------

    idf_path = _find_idf_path(args.idf_path)
    nvs_gen  = _find_nvs_gen(idf_path)
    _info(f"ESP-IDF     : {idf_path}")
    _info(f"nvs_gen     : {nvs_gen}")

    # ------------------------------------------------------------------
    # Generate NVS binary in a temp directory
    # ------------------------------------------------------------------

    with tempfile.TemporaryDirectory(prefix="dsp_provision_") as tmp_str:
        tmp_dir  = Path(tmp_str)
        csv_path = tmp_dir / "dsp_certs.csv"

        output_bin: Path
        if args.output:
            output_bin = Path(args.output).resolve()
            output_bin.parent.mkdir(parents=True, exist_ok=True)
        else:
            output_bin = tmp_dir / "dsp_certs.bin"

        _write_csv(csv_path, cert_path, key_path,
                   psk_id_bytes, psk_key_bytes, tmp_dir)

        # Run nvs_partition_gen.py generate
        nvs_cmd = [
            sys.executable,
            str(nvs_gen),
            "generate",
            str(csv_path),
            str(output_bin),
            str(PARTITION_SIZE),
        ]

        _info(f"Generating NVS image: {output_bin}")
        _info(f"  Command: {' '.join(nvs_cmd)}")

        result = subprocess.run(nvs_cmd, capture_output=True, text=True)
        if result.stdout:
            for line in result.stdout.strip().splitlines():
                _info(f"  nvs_gen: {line}")
        if result.returncode != 0:
            _die(
                f"nvs_partition_gen.py failed (exit {result.returncode}):\n"
                + (result.stderr or result.stdout)
            )

        bin_size = output_bin.stat().st_size
        _info(
            f"NVS image generated: {output_bin}  "
            f"({bin_size} bytes, "
            f"partition size {PARTITION_SIZE} bytes)"
        )

        # ------------------------------------------------------------------
        # Flash (unless --dry-run)
        # ------------------------------------------------------------------

        if args.dry_run:
            _info("--dry-run: skipping flash step.")
            _info(f"To flash manually:")
            _info(
                f"  python3 -m esptool --chip esp32s3 "
                f"--port {args.port} --baud {args.baud} "
                f"write_flash {hex(PARTITION_OFFSET)} {output_bin}"
            )
            return

        # Copy binary to a stable path for error reporting if tmp_dir is gone
        if not args.output:
            # The file is in tmp_dir which will be cleaned up; copy it out
            import shutil
            stable = Path(tempfile.mktemp(suffix="_dsp_certs.bin"))
            shutil.copy2(output_bin, stable)
            output_bin = stable

        esptool_cmd = [
            sys.executable, "-m", "esptool",
            "--chip",  "esp32s3",
            "--port",  args.port,
            "--baud",  str(args.baud),
            "write_flash",
            hex(PARTITION_OFFSET),
            str(output_bin),
        ]

        _info(f"Flashing to {args.port} at offset {hex(PARTITION_OFFSET)} ...")
        _info(f"  Command: {' '.join(esptool_cmd)}")

        flash_result = subprocess.run(esptool_cmd)
        if flash_result.returncode != 0:
            _die(
                f"esptool.py write_flash failed (exit {flash_result.returncode}).\n"
                f"  NVS binary is preserved at: {output_bin}"
            )

        _info("Provisioning complete.")
        _info(
            "Reset the device to load the new credentials "
            "(press EN button or power-cycle)."
        )


if __name__ == "__main__":
    main()
