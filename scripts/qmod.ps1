# Builds the qmod, named <name>-<version>-bs<packageVersion>.qmod
# Arguments are passed on to `qpm qmod zip`, for example --clean or --skip_build
Set-Location "$PSScriptRoot/.."

qpm qmod manifest
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

$mod = Get-Content ./mod.json -Raw | ConvertFrom-Json
qpm qmod zip @args "$($mod.name)-$($mod.version)-bs$($mod.packageVersion).qmod"
exit $LASTEXITCODE
