#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
workspace_dir="$(cd "${script_dir}/.." && pwd)"
renode_bin="${RENODE_BIN:-renode}"
firmware_elf="${1:-${workspace_dir}/build/Debug/Peleng.elf}"
adc_source="${2:-${PELENG_ADC_SOURCE:-${workspace_dir}/tests/renode/data}}"
pid_file="${script_dir}/.renode-debug.pid"

if ! command -v "${renode_bin}" >/dev/null 2>&1; then
    printf 'Renode executable not found: %s\n' "${renode_bin}" >&2
    exit 1
fi

if ! command -v python3 >/dev/null 2>&1; then
    printf 'python3 is required for ADC data preparation\n' >&2
    exit 1
fi

if [[ ! -f "${firmware_elf}" ]]; then
    printf 'Firmware ELF not found: %s\n' "${firmware_elf}" >&2
    exit 1
fi

tmp_dir="$(mktemp -d)"
cleanup() {
    rm -f "${pid_file}"
    rm -rf "${tmp_dir}"
}
trap cleanup EXIT

manifest_path="$(
    python3 "${script_dir}/prepare_adc_data.py" \
        --source "${adc_source}" \
        --output-dir "${tmp_dir}/adc" \
        --loop
)"

"${renode_bin}" \
    -e "set bin_path @${firmware_elf}" \
    -e "set adc_manifest_path @${manifest_path}" \
    -e "set adc_runtime_path @${script_dir}/adc_runtime.py" \
    -e "include @${script_dir}/Peleng.resc" &
renode_pid=$!
printf '%s\n' "${renode_pid}" > "${pid_file}"
wait "${renode_pid}"
