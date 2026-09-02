$ErrorActionPreference = 'Stop'

$module = Split-Path -Parent $PSScriptRoot
$output = Join-Path $env:TEMP 'harmonycar_bluetooth_protocol_test.exe'

gcc -std=c11 -Wall -Wextra -Werror `
    (Join-Path $module 'bluetooth_protocol.c') `
    (Join-Path $PSScriptRoot 'bluetooth_protocol_test.c') `
    -o $output
if ($LASTEXITCODE -ne 0) {
    throw 'bluetooth protocol test build failed'
}

& $output
if ($LASTEXITCODE -ne 0) {
    throw 'bluetooth protocol test failed'
}
