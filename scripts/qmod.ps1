# Builds the qmod, named <name>-<version>-bs<packageVersion>-<commit>.qmod
# Arguments are passed on to `qpm qmod zip`, for example --clean or --skip_build
Set-Location "$PSScriptRoot/.."

qpm qmod manifest
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

$mod = Get-Content ./mod.json -Raw | ConvertFrom-Json
$name = "$($mod.name)-$($mod.version)-bs$($mod.packageVersion)"

$commit = git rev-parse --short HEAD 2>$null
if ($LASTEXITCODE -eq 0 -and $commit) {
    $name += "-$commit"
}

qpm qmod zip @args "$name.qmod"
exit $LASTEXITCODE
