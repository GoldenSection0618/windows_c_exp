param(
    [string]$ExePath = "..\x64\Debug\C_PROGRAM_EXPERIMENT.exe"
)

$ErrorActionPreference = "Stop"

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectRoot = Resolve-Path (Join-Path $scriptRoot "..\..")
$exeFullPath = Resolve-Path -Path (Join-Path $projectRoot $ExePath) -ErrorAction SilentlyContinue

if (-not $exeFullPath) {
    throw "未找到可执行文件: $ExePath"
}

Push-Location $projectRoot
try {
    & powershell -NoProfile -ExecutionPolicy Bypass `
        -File "scripts\run_test_batch.ps1" `
        $exeFullPath.ProviderPath `
        "tests\exp7" `
        "tests\exp7\suites.acceptance" `
        "build\windows_exp7_output" `
        "scripts\run_smoke_test.ps1"

    if ($LASTEXITCODE -ne 0) {
        throw "exp7 acceptance failed"
    }
} finally {
    Pop-Location
}
