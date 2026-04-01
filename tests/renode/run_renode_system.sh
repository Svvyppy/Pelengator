#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
workspace_dir="$(cd "${script_dir}/../.." && pwd)"
renode_arg="${1:-}"
firmware_elf="${2:-}"
platform_repl="${3:-}"
template_resc="${script_dir}/peleng_system_test.resc"
adc_source="${PELENG_ADC_SOURCE:-${script_dir}/data}"
rcc_script="${script_dir}/rcc_g4.py"
adc_script="${script_dir}/adc_cluster_g4.py"
runtime_script="${workspace_dir}/renode/adc_runtime.py"
prepare_script="${workspace_dir}/renode/prepare_adc_data.py"

expected_d12="${PELENG_EXPECT_D12:--4}"
expected_d13="${PELENG_EXPECT_D13:--4}"
expected_d14="${PELENG_EXPECT_D14:--3}"
expected_valid="${PELENG_EXPECT_VALID:-1}"

skip() {
    printf 'SKIP: %s\n' "$1" >&2
    exit 77
}

if [[ -z "${renode_arg}" ]]; then
    skip "Renode executable was not provided"
fi

renode_bin="$(command -v "${renode_arg}" 2>/dev/null || true)"
if [[ -z "${renode_bin}" ]]; then
    skip "Renode executable not found: ${renode_arg}"
fi

if ! command -v python3 >/dev/null 2>&1; then
    skip "python3 is required for ADC sample conversion and log parsing"
fi

if [[ ! -f "${firmware_elf}" ]]; then
    skip "Firmware ELF not found: ${firmware_elf}"
fi

if [[ ! -f "${platform_repl}" ]]; then
    skip "Renode platform description not found: ${platform_repl}"
fi

if [[ ! -f "${template_resc}" ]]; then
    skip "Renode system test script not found: ${template_resc}"
fi

if [[ ! -f "${rcc_script}" ]]; then
    skip "Renode RCC helper script not found: ${rcc_script}"
fi

if [[ ! -f "${adc_script}" ]]; then
    skip "Renode ADC helper script not found: ${adc_script}"
fi

if [[ ! -f "${runtime_script}" ]]; then
    skip "Renode ADC runtime helper not found: ${runtime_script}"
fi

if [[ ! -f "${prepare_script}" ]]; then
    skip "Renode ADC data preparation helper not found: ${prepare_script}"
fi

if [[ ! -d "${adc_source}" && ! -f "${adc_source}" ]]; then
    skip "ADC source path not found: ${adc_source}"
fi

tmp_dir="$(mktemp -d)"
cleanup() {
    rm -rf "${tmp_dir}"
}
trap cleanup EXIT

rendered_resc="${tmp_dir}/peleng_system_test.rendered.resc"
rendered_platform="${tmp_dir}/peleng_system_platform.rendered.repl"
renode_log="${tmp_dir}/renode.log"
manifest_path=""
run_time_s=""

escape_sed_replacement() {
    printf '%s' "$1" | sed -e 's/[\/&|]/\\&/g'
}

manifest_path="$(
    python3 "${prepare_script}" \
        --source "${adc_source}" \
        --output-dir "${tmp_dir}/adc"
)"

run_time_s="$(
    python3 - "${manifest_path}" <<'PY'
import json
import sys
from pathlib import Path

manifest = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
print(manifest["run_time_s"])
PY
)"

sed \
    -e "s|@RENODE_RCC_SCRIPT@|$(escape_sed_replacement "${rcc_script}")|g" \
    -e "s|@RENODE_ADC_SCRIPT@|$(escape_sed_replacement "${adc_script}")|g" \
    "${platform_repl}" > "${rendered_platform}"

sed \
    -e "s|@FIRMWARE_ELF@|@$(escape_sed_replacement "${firmware_elf}")|g" \
    -e "s|@RENODE_PLATFORM@|@$(escape_sed_replacement "${rendered_platform}")|g" \
    -e "s|@ADC_MANIFEST@|@$(escape_sed_replacement "${manifest_path}")|g" \
    -e "s|@RENODE_RUNTIME_SCRIPT@|@$(escape_sed_replacement "${runtime_script}")|g" \
    -e "s|@RENODE_RUN_TIME_S@|${run_time_s}|g" \
    "${template_resc}" > "${rendered_resc}"

if ! "${renode_bin}" --console --disable-gui -e "include @${rendered_resc}" >"${renode_log}" 2>&1; then
    cat "${renode_log}" >&2
    exit 1
fi

parsed_results="$(python3 - "${renode_log}" <<'PY'
import re
import sys
from pathlib import Path

log_path = Path(sys.argv[1])
lines = log_path.read_text(encoding="utf-8", errors="ignore").splitlines()

capture = False
result_lines = []
for line in lines:
    if "PELENG_RESULT_BEGIN" in line:
        capture = True
        continue
    if "PELENG_RESULT_END" in line:
        capture = False
        break
    if capture:
        result_lines.append(line)

values = []
for line in result_lines:
    tokens = re.findall(r'0x[0-9a-fA-F]+|-?\d+', line)
    if not tokens:
        continue
    token = tokens[-1]
    value = int(token, 16) if token.lower().startswith("0x") else int(token)
    values.append(value)

if len(values) < 4:
    print("PARSE_ERROR")
    sys.exit(0)

def to_int32(value: int) -> int:
    return value - (1 << 32) if value >= (1 << 31) else value

d12 = to_int32(values[0] & 0xFFFFFFFF)
d13 = to_int32(values[1] & 0xFFFFFFFF)
d14 = to_int32(values[2] & 0xFFFFFFFF)
valid = values[3] & 0xFF

print(f"{d12} {d13} {d14} {valid}")
PY
)"

if [[ "${parsed_results}" == "PARSE_ERROR" ]]; then
    printf 'Failed to parse Renode output block from %s\n' "${renode_log}" >&2
    cat "${renode_log}" >&2
    exit 1
fi

read -r got_d12 got_d13 got_d14 got_valid <<<"${parsed_results}"

printf 'Renode delays: d12=%s d13=%s d14=%s valid=%s\n' \
    "${got_d12}" "${got_d13}" "${got_d14}" "${got_valid}"

if [[ "${got_d12}" != "${expected_d12}" \
   || "${got_d13}" != "${expected_d13}" \
   || "${got_d14}" != "${expected_d14}" \
   || "${got_valid}" != "${expected_valid}" ]]; then
    printf 'Unexpected delay result. Expected d12=%s d13=%s d14=%s valid=%s\n' \
        "${expected_d12}" "${expected_d13}" "${expected_d14}" "${expected_valid}" >&2
    cat "${renode_log}" >&2
    exit 1
fi
