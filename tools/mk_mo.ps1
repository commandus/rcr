$GETTEXT_MSGFMT_EXECUTABLE = $($args[0])
$_moPath = $($args[1])
$_moFile = $($args[2])
$_poFile = $($args[3])
New-Item -ItemType Directory -Force -Path $_moPath | Out-Null
$C="$GETTEXT_MSGFMT_EXECUTABLE -o $_moFile $_poFile"
Write-Output($C)
Invoke-expression $C
